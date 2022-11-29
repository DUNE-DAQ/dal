package dal_test;

 /**
   *  Test callback.
   */

class TestCallback implements config.Callback
  {
    private config.Configuration db;

    public TestCallback(config.Configuration d)
      {
        db = d;
      }

    public void process_changes(config.Change[] changes, java.lang.Object parameter)
      {

        try {
          // get callback name as string passed as the parameter of callback

          String cb_id = (String) parameter;

          // print out changes description

          System.out.println("[TestCB " + cb_id + "] got changes:");
          config.Change.print(changes, "  ");

          // iterate changes by classes

          for (int i = 0; i < changes.length; i++)
            {
              config.Change change = changes[i];
              System.out.println("* there are changes in the \'" + change.get_class_name() + "\' class");

              // just as example, look for changed objects of the Application
              // class

              if ((change.get_class_name().equals("Application") == true) && (change.get_changed_objects() != null))
                {
                  System.out.println("* " + change.get_changed_objects().length + " updated objects of the Application class");

                  // iterate by all changed objects and print them out

                  for (int j = 0; j < change.get_changed_objects().length; ++j)
                    {
                      dal.Application a = dal.Application_Helper.get(db, change.get_changed_objects()[j]);

                      // an example of correct down cast

                      if (a.class_name().equals("RunControlApplication"))
                        {
                          dal.RunControlApplication_Helper.get(db, a.config_object()).print("  "); // print as RC application
                        }
                      else
                        {
                          a.print("  "); // print as an application
                        }
                    }
                }
            }
        }
        catch(config.ConfigException ex) {
          ers.Logger.error(ex);
          System.exit(1);
        }
      }
  }


public class TestCB
  {

    private static void add_enabled_segments(dal.Segment s, dal.Partition p, java.util.TreeSet<dal.Segment> segments) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
      {
        if (s.disabled(p) == false)
          {
            segments.add(s);
            for (int i = 0; i < s.get_Segments().length; ++i)
              {
                add_enabled_segments(s.get_Segments()[i], p, segments);
              }
          }
      }

    public static void main(String args[])
      {

        if (args.length == 0)
          {
            ers.Logger.error(new ers.Issue("no arguments given (server-name [subscribe])"));
            System.exit(1);
          }


        try
          {

            // create configuration object

            config.Configuration db = new config.Configuration(args[0]);

            // get a partition object (if exists)
            // and use generated method to print out it's contents

            dal.Partition part_objs[] = dal.Partition_Helper.get(db, new config.Query());

            if (part_objs.length == 0)
              {
                ers.Logger.error(new ers.Issue("can not find a partition object"));
                System.exit(1);
              }


            for (int j = 0; j < part_objs.length; ++j)
              {

                // get all segments of partition

                java.util.TreeSet<dal.Segment> segments = new java.util.TreeSet<dal.Segment>(
                    new java.util.Comparator<dal.Segment>()
                      {
                        public int compare(dal.Segment o1, dal.Segment o2)
                          {
                            return o1.UID().compareTo(o2.UID());
                          }
                      });

                  {
                    for (int i = 0; i < part_objs[j].get_Segments().length; ++i)
                      {
                        System.out.println(" * test segment \'" + part_objs[j].get_Segments()[i].UID() + "\' from partition \'" + part_objs[j].UID() + "\'");
                        add_enabled_segments(part_objs[j].get_Segments()[i], part_objs[j], segments);
                      }
                  }


                  // print found components and test get_parents() algorithm

                  {
                    System.out.println("Partition \'" + part_objs[j].UID() + "\' has " + segments.size() + " user segments:");
                    for (dal.Segment s : segments)
                      {
                        dal.Component[][] parents = s.get_parents(part_objs[j]);
                        StringBuffer ps = new StringBuffer();
                        if (parents.length == 1)
                          {
                            if (parents[0].length > 1)
                              {
                                ps.append(" (has " + (parents[0].length - 1) + " parent segment(s): ");
                                for (int k = 1; k < parents[0].length; ++k)
                                  {
                                    if (k != 1)
                                      ps.append(", ");
                                    ps.append('\'' + parents[0][k].UID() + '@' + parents[0][k].class_name() + '\'');
                                  }
                                ps.append(')');
                              }
                            else if (parents[0].length == 1)
                              {
                                ps.append(" (is referenced by the partition object)");
                              }
                            System.out.println(" * " + s.UID() + '@' + s.class_name() + " => " + ps);
                          }
                        else
                          {
                            ers.Logger.error(new ers.Issue("error in calculation of path(s) to segment \'" + s.UID() + '@' + s.class_name() + "\'"));
                            continue;
                          }
                      }
                  }
              }

            // subscribe on changes

            if (args.length == 2)
              {

                // get application object by it's identity (known in advance)
                // and print out it's name

                dal.Application app = null;

                try
                  {
                    app = dal.Application_Helper.get(db, "RootController");
                    System.out.println("The controller's name = \'" + app.UID() + "\'");
                  }
                catch (final config.NotFoundException ex)
                  {
                    ers.Logger.error(new config.GenericException("can not find application \\'RootController\\'", ex));
                    System.exit(1);
                  }

                config.Subscription s1 = new config.Subscription(new TestCallback(db), "callback 1");

                // subscribe partition object

                s1.add(part_objs[0]);

                // subscribe application object

                s1.add(app);

                // subscribe on changes according above criteria

                db.subscribe(s1);

                config.Subscription s2 = new config.Subscription(new TestCallback(db), "callback 2");

                // subscribe partition object

                s2.add(part_objs[0]);

                // subscribe all application objects

                s2.add("Application");

                // subscribe on changes according above criteria

                db.subscribe(s2);

                // sleep for 1 minute to be able to get the changes

                try
                  {
                    Thread.sleep(Integer.valueOf(args[1]).intValue());
                  }
                catch (java.lang.InterruptedException e)
                  {
                    System.out.println("Interrupted, exiting ... ");
                    db.unsubscribe();
                    System.exit(0);
                  }

                // also stops notification

                db.unload();

                System.out.println("Test completed successfully");
              }
          }
        catch(final config.ConfigException ex) {
          ers.Logger.error(ex);
          System.exit(1);
        }
      }
  }

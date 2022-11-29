package dal;

import java.util.ArrayList;

/**
 * Test callback.
 */

class TestCallback implements config.Callback
  {
    dal.Partition p;
    String[] app_types;
    String[] segments;
    dal.Computer[] hosts;

    public TestCallback(dal.Partition p_, String[] app_types_, String[] segments_, dal.Computer[] hosts_)
      {
        p = p_;
        app_types_ = app_types;
        segments_ = segments;
        hosts_ = hosts;
      }

    public void process_changes(config.Change[] changes, java.lang.Object parameter)
      {

        // get callback name as string passed as the parameter of callback

        String cb_id = (String) parameter;

        // print out changes description

        System.out.println("[NewTestCallback " + cb_id + "] got changes:");
        config.Change.print(changes, "  ");

        try
          {
            //dal.AppConfig[] apps = p.get_all_applications(app_types, segments, hosts);
        	// TODO : replace by p.get_all_applications() when available
            dal.Segment root_seg = p.get_segment(p.get_OnlineInfrastructure().UID());
            dal.BaseApplication[] apps = root_seg.get_all_applications(app_types, segments, hosts);

            if (apps != null)
              {
                System.out.println("Found " + apps.length + " applications:");
                for (int i = 0; i < apps.length; ++i)
                  {
                    dal.BaseApplication a = apps[i];
                    System.out.println(" - " + (i + 1) + '\t' + a.get_base_app().UID() + '@' + a.get_base_app().class_name()
                        + " at " + a.get_segment().UID() + '@' + a.get_segment().class_name() + " on " + a.get_host().UID() + '@'
                        + a.get_host().class_name() + " as \'" + a.UID() + '\'');
                  }
              }
            else
              {
                System.out.println("Found no applications");
              }
          }
        catch (config.ConfigException ex)
          {
            ers.Logger.error(ex);
            System.exit(1);
          }
     }
  }

class TableRow {
	char m_fill;
	char m_separator;
	String m_items[];

	TableRow(char fill, char separator) {
		m_fill = fill;
		m_separator = separator;
		m_items = new String[] { "", "", "", "", "", "" };
	}

	TableRow(char fill, char separator, int num, dal.BaseApplication a) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		m_fill = fill;
		m_separator = separator;
		m_items = new String[] { String.valueOf(num), a.get_base_app().UID() + '@' + a.get_base_app().class_name(),
				a.get_host().UID() + '@' + a.get_host().class_name(),
				a.get_segment().get_base_segment().UID() + '@' + a.get_segment().get_base_segment().class_name(), a.get_segment().UID(), a.UID() };
	}

	TableRow(char fill, char separator, dal.Computer host) {
		m_fill = fill;
		m_separator = separator;
		m_items = new String[] { "", "", host.UID() + '@' + host.class_name(), "", "", "" };
	}

	TableRow(char fill, char separator, String s1, String s2, String s3, String s4, String s5, String s6) {
		m_fill = fill;
		m_separator = separator;
		m_items = new String[] { s1, s2, s3, s4, s5, s6 };
	}
};

public class DumpAppConfig
  {

    private static int read_list(java.util.LinkedList<String> l, String args[], int idx)
      {
        idx++;

        for (int i = idx; i < args.length; i++)
          {
            if (args[i].charAt(0) != '-')
              {
                l.add(args[i]);
              }
            else
              {
                return (i - 1);
              }
          }

        return (args.length - 1);
      }

    static private void usage()
      {
        System.out.println("Usage: java dal.DumpAppConfigNew  [-d | --database db_spec]");
        System.out.println("                               [-p | --partition-name pname]");
        System.out.println("                               [-t | --application-types app-type ...]");
        System.out.println("                               [-c | --hosts computer-id ...]");
        System.out.println("                               [-s | --segments segment-name ...]");
        System.out.println("                               [-b | --show-backup-hosts]");
        System.out.println("                               [-T | --timeout value]");
        System.out.println("                               [-h | --help]");
        System.out.println("");
        System.out.println("Options/Arguments:");
        System.out.println("        -d | --database db_spec              name of the database");
        System.out.println("        -p | --partition-name pname          name of the partition object");
        System.out.println("        -t | --application-types app-type+   filter out all applications except given");
        System.out.println("                                             classes (and their subclasses)");
        System.out.println("        -c | --hosts computer-id+            filter out all applications except those which");
        System.out.println("                                             run on given hosts");
        System.out.println("        -s | --segments segment-name+        filter out all applications except those which");
        System.out.println("                                             belong to given segments");
        System.out.println("        -b | --show-backup-hosts             print backup hosts");
        System.out.println("                                             belong to given segments");
        System.out.println("        -T | --timeout value                 wait given amount of time (in ms) for reload command");
        System.out.println("        -h | --help                          print this message");
        System.out.println("");
        System.out.println("Description:");
        System.out.println("        The program prints out applications and their configuration");
        System.out.println("        parameters using dal.Partition.get_all_applications() algorithm.");
        System.out.println("        Can also be used to debug reload (use option -t).");
      }

    public static void main(String args[])
      {

        java.util.LinkedList<String> app_type_names = null;
        java.util.LinkedList<String> segment_names_ = null;
        java.util.LinkedList<String> host_names = null;
        String db_name = new String();
        String partition_name = new String();
        int timeout = 0;
        boolean backup = false;

        for (int i = 0; i < args.length; i++)
          {
            if (args[i].equals("-d") || args[i].equals("--database"))
              {
                db_name = args[++i];
              }
            else
              if (args[i].equals("-p") || args[i].equals("--partition-name"))
                {
                  partition_name = args[++i];
                }
              else
                if (args[i].equals("-T") || args[i].equals("--timeout"))
                  {
                    timeout = java.lang.Integer.valueOf(args[++i]).intValue();
                  }
                else
                  if (args[i].equals("-t") || args[i].equals("--application-types"))
                    {
                      if (app_type_names == null)
                        {
                          app_type_names = new java.util.LinkedList<String>();
                        }
                      i = read_list(app_type_names, args, i);
                    }
                  else
                    if (args[i].equals("-c") || args[i].equals("--hosts"))
                      {
                        if (host_names == null)
                          {
                            host_names = new java.util.LinkedList<String>();
                          }
                        i = read_list(host_names, args, i);
                      }
                    else
                      if (args[i].equals("-s") || args[i].equals("--segments"))
                        {
                          if (segment_names_ == null)
                            {
                              segment_names_ = new java.util.LinkedList<String>();
                            }
                          i = read_list(segment_names_, args, i);
                        }
                      else
                        if (args[i].equals("-b") || args[i].equals("--show-backup-hosts"))
                          {
                        	backup = true;
                          }
                        else
                          if (args[i].equals("-h") || args[i].equals("--help"))
                            {
                              usage();
                              System.exit(0);
                            }
                          else
                            {
                              ers.Logger.error(new ers.Issue("unexpected command line parameter \'" + args[i] + '\''));
                              System.exit(1);
                            }
          }

        String[] segment_names = null;
        if (segment_names_ != null)
          {
            segment_names = segment_names_.toArray(new String[0]);
          }

        try
          {

            // create configuration object

            config.Configuration db = new config.Configuration(db_name);

            // get a partition object (defined by command line or
            // TDAQ_PARTITION)

            dal.Partition p = dal.Algorithms.get_partition(db, partition_name);

            if (p == null)
              {
                ers.Logger.error(new ers.Issue("can not find partition object \'" + partition_name + '\''));
                System.exit(1);
              }

            // create app list

            String[] app_types = null;

            if (app_type_names != null)
              {
                int idx = 0;
                app_types = new String[app_type_names.size()];
                for (java.util.Iterator<String> i = app_type_names.iterator(); i.hasNext();)
                  {
                    app_types[idx++] = i.next();
                  }
              }

            // find hosts

            dal.Computer[] hosts = null;

            if (host_names != null)
              {
                int idx = 0;
                hosts = new dal.Computer[host_names.size()];
                for (java.util.Iterator<String> i = host_names.iterator(); i.hasNext();)
                  {
                      String name = i.next();
                      hosts[idx] = dal.Computer_Helper.get(db, name);
                      System.out.println(" - add host " + hosts[idx].UID() + '@' + hosts[idx].class_name());
                      idx++;
                  }
              }
            
            dal.Segment root_seg = p.get_segment(p.get_OnlineInfrastructure().UID());
            dal.BaseApplication[] apps = root_seg.get_all_applications(app_types, segment_names, hosts);
            System.out.println("Got " + apps.length + " applications:");

            ArrayList<TableRow> rows = new ArrayList<TableRow>();
            
            rows.add(new TableRow('=', '='));
            rows.add(new TableRow(' ', '|', "num","Application Object","Host","Segment","Segment unique id","Application unique id"));
            rows.add(new TableRow('=', '='));

			if (apps != null) {
				for (int i = 0; i < apps.length; ++i) {
					rows.add(new TableRow(' ', '|', i+1, apps[i]));

					if (backup) {
						dal.Computer[] backup_hosts = apps[i].get_backup_hosts();
						if (backup_hosts != null) {
							for (int j = 0; j < backup_hosts.length; ++j) {
								rows.add(new TableRow(' ', '|', backup_hosts[j]));
							}
						}
					}
				}
			}
			
            rows.add(new TableRow('=', '='));

			int[] cw = new int[] {1,1,1,1,1,1};

			for (TableRow row : rows) {
				for(int i = 0; i< cw.length; i++) {
                    int len = row.m_items[i].length();
		              if(len > cw[i])
			                cw[i]=len;
					}
			}
			
			boolean[] align_left = new boolean[] {false, true, true, true, true, true};

			for (TableRow row : rows) {
				for(int i = 0; i< cw.length; i++) {
					System.out.print(row.m_separator);
					System.out.print(row.m_fill);
					
					if(align_left[i] == false)
						for(int j = 0; j < (cw[i] - row.m_items[i].length()); j++)
							System.out.print(row.m_fill);
					
					System.out.print(row.m_items[i]);
					
					if(align_left[i] == true)
						for(int j = 0; j < (cw[i] - row.m_items[i].length()); j++)
							System.out.print(row.m_fill);

					System.out.print(row.m_fill);
				}
				System.out.println(row.m_separator);
			}

            if (timeout > 0)
              {
                db.subscribe(new config.Subscription(new TestCallback(p, app_types, segment_names, hosts), "callback 1"));

                // sleep to be able to get the changes

                try
                  {
                    Thread.sleep(timeout);
                  }
                catch (java.lang.InterruptedException e)
                  {
                    System.out.println("Interrupted, exiting ... ");
                    db.unsubscribe();
                    System.exit(0);
                  }
              }
          }
        catch (final config.ConfigException ex)
          {
            ers.Logger.error(ex);
            System.exit(1);
          }
      }
  }

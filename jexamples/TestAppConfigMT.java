import java.util.TreeSet;
import java.util.Iterator;
import java.util.LinkedList;

import config.Configuration;

import dal.Partition;
import dal.Partition_Helper;

public class TestAppConfigMT
  {

    static class TestThread
      {
        private final int id;
        private config.Configuration db;
        private String partition_name;
        private java.util.LinkedList<String> app_type_names;
        private String[] segment_names;
        private java.util.LinkedList<String> host_names;

        public TestThread(int id_, config.Configuration db_, String partition_name_, java.util.LinkedList<String> app_type_names_,
            String[] segment_names_, java.util.LinkedList<String> host_names_)
          {
            id = id_;
            db = db_;
            partition_name = partition_name_;
            app_type_names = app_type_names_;
            segment_names = segment_names_;
            host_names = host_names_;
          }

        public void print()
          {

            try
              {
                // get a partition object (defined by command line or TDAQ_PARTITION)
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
                        try
                          {
                            hosts[idx] = dal.Computer_Helper.get(db, name);
                            System.out.println(" - [" + id + "] add host " + hosts[idx].UID() + '@' + hosts[idx].class_name());
                            idx++;
                          }
                        catch (final config.ConfigException e)
                          {
                            ers.Logger.error(new ers.Issue("can not get computer \'" + name + '\'', e));
                            System.exit(1);
                          }
                      }
                  }

                dal.AppConfig[] apps = p.get_all_applications(app_types, segment_names, hosts);

                if (apps != null)
                  {
                    System.out.println("[" + id + "] Found " + apps.length + " applications:");
                    for (int i = 0; i < apps.length; ++i)
                      {
                        dal.AppConfig a = apps[i];
                        System.out.println(" - [" + id + "] " + (i + 1) + '\t' + a.get_base_app().UID() + '@'
                            + a.get_base_app().class_name() + " at " + a.get_segment().UID() + '@' + a.get_segment().class_name()
                            + " on " + a.get_host().UID() + '@' + a.get_host().class_name() + " as \'" + a.get_app_id() + '\'');
                      }
                  }
                else
                  {
                    System.out.println("[" + id + "] Found no applications");
                  }
              }
            catch (final config.ConfigException ex)
              {
                ers.Logger.error(new ers.Issue(ex));
                System.exit(1);
              }
          }
      }

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
        System.out.println("Usage: java ... TestAppConfig  [-d | --database db_spec]");
        System.out.println("                               [-p | --partition-name pname]");
        System.out.println("                               [-t | --application-types app-type ...]");
        System.out.println("                               [-c | --hosts computer-id ...]");
        System.out.println("                               [-s | --segments segment-id ...]");
        System.out.println("                               [-h | --help]");
        System.out.println("");
        System.out.println("Options/Arguments:");
        System.out.println("        -d | --database db_spec              database specification in format \"plugin:name\"");
        System.out.println("                                             (otherwise use TDAQ_DB environment variable)");
        System.out.println("        -p | --partition-name pname          partition object id");
        System.out.println("        -t | --application-types app-type+   filter out all applications except given");
        System.out.println("                                             classes (and their subclasses)");
        System.out.println("        -c | --hosts computer-id+            filter out all applications except those which");
        System.out.println("                                             run on given hosts");
        System.out.println("        -s | --segments segment-id+          filter out all applications except those which");
        System.out.println("                                             belong to given segments");
        System.out.println("        -h | --help                          print this message");
        System.out.println("");
        System.out.println("Description:");
        System.out.println("        The program prints out applications and their configuration");
        System.out.println("        parameters using dal.Partition.get_all_applications() algorithm.");
      }

    public static void main(String args[])
      {

        java.util.LinkedList<String> app_type_names = null;
        java.util.LinkedList<String> segment_names = null;
        java.util.LinkedList<String> host_names = null;
        String db_name = new String();
        String partition_name = new String();

        for (int i = 0; i < args.length; i++)
          {
            if (args[i].equals("-d") || args[i].equals("--database"))
              {
                db_name = args[++i];
              }
            else if (args[i].equals("-p") || args[i].equals("--partition-name"))
              {
                partition_name = args[++i];
              }
            else if (args[i].equals("-t") || args[i].equals("--application-types"))
              {
                if (app_type_names == null)
                  {
                    app_type_names = new java.util.LinkedList<String>();
                  }
                i = read_list(app_type_names, args, i);
              }
            else if (args[i].equals("-c") || args[i].equals("--hosts"))
              {
                if (host_names == null)
                  {
                    host_names = new java.util.LinkedList<String>();
                  }
                i = read_list(host_names, args, i);
              }
            else if (args[i].equals("-s") || args[i].equals("--segments"))
              {
                if (segment_names == null)
                  {
                    segment_names = new java.util.LinkedList<String>();
                  }
                i = read_list(segment_names, args, i);
              }
            else if (args[i].equals("-h") || args[i].equals("--help"))
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

        config.Configuration db = null;

        // create configuration object
        try
          {
            db = new config.Configuration(db_name);
          }
        catch (final config.ConfigException ex)
          {
            ers.Logger.error(ex);
            System.exit(1);
          }

        String[] segments = null;

        if (segment_names != null && segment_names.size() > 0)
            segments = segment_names.toArray(new String[0]);

        final TestThread t1 = new TestThread(1, db, partition_name, app_type_names, segments, host_names);
        final TestThread t2 = new TestThread(2, db, partition_name, app_type_names, segments, host_names);
        final TestThread t3 = new TestThread(3, db, partition_name, app_type_names, segments, host_names);
        final TestThread t4 = new TestThread(4, db, partition_name, app_type_names, segments, host_names);

        new Thread(new Runnable() { public void run() { t1.print(); } }).start();
        new Thread(new Runnable() { public void run() { t2.print(); } }).start();
        new Thread(new Runnable() { public void run() { t3.print(); } }).start();
        new Thread(new Runnable() { public void run() { t4.print(); } }).start();
      }
  }

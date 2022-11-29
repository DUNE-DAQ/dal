package dal;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import config.Configuration;
import dal.AppConfig;
import dal.SegConfig;
import dal.SubstituteVariables;

public class DumpApps
  {
    public static void main(final String[] args) {    
        if(args.length != 2) {
            ers.Logger.error(new ers.Issue("wrong number of command line agruments\n" + "usage: java dal.DumpApps \'db\' \'partition\'"));
            System.exit(1);
            return;
        }

        final String partition = args[1];
        final String db = args[0];

        try {
          final Configuration config = new Configuration(db);

          final dal.Partition dalPartition = dal.Algorithms.get_partition(config, partition);
          config.register_converter(new SubstituteVariables(dalPartition));

          final dal.BaseApplication[] allApps = dalPartition.get_all_applications(null, null, null);

          int count = 0;

          for(final dal.BaseApplication app : allApps) {
            ++count;
                            
            final dal.Application app_ = dal.Application_Helper.cast(app.get_base_app());
            if(app_ != null) {
                final String msg = "### (" + count + ") application \'" + app.get_base_app().UID() + "@" + app.get_base_app().class_name() + "\' ###";
                System.out.println(msg);
            } else {
                final String msg = "### (" + count + ") template application " + app.UID() + " ###";
                System.out.println(msg);
            }

            final Map<String, String> env = new TreeMap<>();
            final List<String> progNames = new ArrayList<>();
            final StringBuilder startArgs = new StringBuilder();
            final StringBuilder restartArgs = new StringBuilder();

            try {
                final dal.Tag tag = app.get_info(env, progNames, startArgs, restartArgs);
            
                System.out.println(" - command line start args:");
                System.out.println("    " + startArgs);
                
                System.out.println(" - command line restart args:");
                System.out.println("    " + restartArgs);
                
                System.out.print(" - possible program file names:");
                if(progNames.isEmpty() == true) {
                  System.out.println(" no");
                } else {
                  System.out.println();
                    for(final String l : progNames) {
                      System.out.println("    * " + l);
                    }
                }
                               
                System.out.print(" - environment variables:");
                if(env.isEmpty() == true) {
                  System.out.println(" no");
                } else {
                  System.out.println();
                    for(final Map.Entry<String, String> e : env.entrySet()) {
                      System.out.println("    * " + e.getKey() + "=\"" + e.getValue() + "\"");
                    }
                }
              }
              catch(final config.ConfigException ex) {
                ers.Logger.error(ex);
              }                                              
          }

        }
        catch(final config.ConfigException ex)
        {
            ers.Logger.error(ex);
            System.exit(1);
        }
        
        
    }
  }
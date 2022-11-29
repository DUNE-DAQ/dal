package dal_test;

import java.util.Arrays;
import java.util.Comparator;


public class Dump {

    static private void usage() {
        System.out.println(
                  "Usage: java dal_test.Dump  -c | --class-name class_name\n"
                + "                          [-q | --query query_statement]\n"
                + "                          [-i | --object-id object_id]\n"
                + "                          [-d | --database db_spec]\n"
                + "                          [-p | --substitute-pp partition_name]\n"
                + "                          [-h | --help]\n"
                + "\n"
                + "Options/Arguments:\n"
                + "        -c | --class-name class_name   mandatory full name of generated class (e.g. \"Module\")\n"
                + "        -q | --query query_statement   oks query statement for given class\n"
                + "        -i | --object-id object_id     object identity\n"
                + "        -d | --database db_spec        database specification in format \"plugin:name\",\n"
                + "                                       if not defined, value of TDAQ_DB environment variable is used\n"
                + "        -p | --substitute-pp pname     partition name, which parameters to be used by the\n"
                + "                                       dal.SubstituteVariables attribute converter\n"
                + "        -h | --help                    print this message" + "\n"
                + "Description:\n"
                + "        The program prints out details of an object of automatically generated DAL.\n"
                + "        It demostrates how to dynamically find a DAL class and use it's print() method.\n"
                + "        It also demostrates usage of the dal.SubstituteVariables attribute converter."
        );
    }

  public static void main(String args[]) {

    String db_name = null;
    String class_name = null;
    String query = new String("");
    String object_id = null;
    String substitution = null;

    for(int i = 0; i < args.length; i++) {
      if(args[i].equals("-c") || args[i].equals("--class-name")) {
        class_name = args[++i];
      }
      else if(args[i].equals("-q") || args[i].equals("--query")) {
        query = args[++i];
      }
      else if(args[i].equals("-i") || args[i].equals("--object-id")) {
        object_id = args[++i];
      }
      else if(args[i].equals("-d") || args[i].equals("--database")) {
        db_name = args[++i];
      }
      else if(args[i].equals("-p") || args[i].equals("--substitute-pp")) {
        substitution = args[++i];
      }
      else if(args[i].equals("-h") || args[i].equals("--help")) {
        usage();
        System.exit(0);
      }
      else {
        ers.Logger.error(new ers.Issue("unexpected command line parameter \'" + args[i] + '\''));
        System.exit( 1 );
      }
    }

    if(class_name == null) {
      ers.Logger.error(new ers.Issue("no class name given"));
      System.exit( 1 );
    }

    try {

      config.Configuration db = new config.Configuration(db_name);

      if(substitution != null) {
        dal.Partition p = dal.Algorithms.get_partition(db, substitution);
        if(p != null) {
          db.register_converter(new dal.SubstituteVariables(p));
        }
        else {
          ers.Logger.error(new ers.Issue("can not find partition object \'" + substitution + '\''));
          System.exit(1);
        }
      }

      if(object_id != null) {
        config.DalObject o = dal.__AnyObject__.get(db, class_name, object_id);
        if(o != null) {
          o.print("");
        }
      }
      else {
        config.DalObject objs[] = dal.__AnyObject__.get(db, class_name, new config.Query(query));
        
        Arrays.sort(objs, (config.DalObject o1, config.DalObject o2) -> {
            return o1.UID().compareTo(o2.UID());
        });

        for (int i = 0; i < objs.length; i++) {
          objs[i].print("");
          System.out.println("");
        }
      }

    }
    catch(final config.ConfigException ex) {
      ers.Logger.error(ex);
      System.exit(2);
    }
  }
}

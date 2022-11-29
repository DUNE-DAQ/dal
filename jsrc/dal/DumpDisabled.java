package dal;

import config.GenericException;

public class DumpDisabled {

    private static int read_list(java.util.LinkedList<String> l, String args[], int idx) {
        idx++;

        for (int i = idx; i < args.length; i++) {
            if (args[i].charAt(0) != '-') {
                l.add(args[i]);
            } else {
                return (i - 1);
            }
        }

        return (args.length - 1);
    }

    static private void usage() {
        System.out.println("Usage: java dal.DumpDisabled  [-d | --database db_spec]");
        System.out.println("                              [-w | -W]");
        System.out.println("                              [-p | --partition-name pname]");
        System.out.println("                              [-h | --help]");
        System.out.println("");
        System.out.println("Options/Arguments:");
        System.out.println("        -d | --database db_spec      database specification in format \"plugin:name\"");
        System.out.println("                                     (otherwise use TDAQ_DB environment variable)");
        System.out.println("        -D | --user-disabled IDs     identities of components disabled by user");
        System.out.println("        -E | --user-enabled IDs      identities of components enabled by user (override DB disabling)");
        System.out.println("        -w | --show-why              briefly explain why given component is disabled");
        System.out.println("        -W | --show-why-detailed     provide detailed explanation why given component is disabled");
        System.out.println("        -p | --partition-name pname  partition object id");
        System.out.println("        -h | --help                  print this message");
        System.out.println("");
        System.out.println("Description:");
        System.out.println("        The program prints out disabled status of each component (i.e. resource or segment)");
        System.out.println("        testing status of it's parents.");
    }

    public static void main(String args[]) {
        String db_name = new String();
        String partition_name = new String();
        int show_why = 0;
        java.util.LinkedList<String> disabled_list = null;
        java.util.LinkedList<String> enabled_list = null;

        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-d") || args[i].equals("--database")) {
                db_name = args[++i];
            } else if (args[i].equals("-p") || args[i].equals("--partition-name")) {
                partition_name = args[++i];
            } else if (args[i].equals("-D") || args[i].equals("--user-disabled")) {
                if (disabled_list == null) {
                    disabled_list = new java.util.LinkedList<String>();
                }
                i = read_list(disabled_list, args, i);
            } else if (args[i].equals("-E") || args[i].equals("--user-enabled")) {
                if (enabled_list == null) {
                    enabled_list = new java.util.LinkedList<String>();
                }
                i = read_list(enabled_list, args, i);
            } else if (args[i].equals("-w") || args[i].equals("--show-why")) {
                show_why = 1;
            } else if (args[i].equals("-W") || args[i].equals("--show-why-detailed")) {
                show_why = 2;
            } else if (args[i].equals("-h") || args[i].equals("--help")) {
                usage();
                System.exit(0);
            } else {
                ers.Logger.error(new ers.Issue("unexpected command line parameter \'" + args[i] + '\''));
                System.exit(1);
            }
        }

        try {

            // create configuration object
            config.Configuration db = new config.Configuration(db_name);

            // get a partition object (defined by command line or TDAQ_PARTITION)
            dal.Partition p = dal.Algorithms.get_partition(db, partition_name);

            if (p == null) {
                ers.Logger.error(new ers.Issue("can not find partition object \'" + partition_name + '\''));
                System.exit(1);
            }

            // find user-disabled and user-enabled components
            if (disabled_list != null) {
                int idx = 0;
                dal.Component[] disabled_objs = new dal.Component[disabled_list.size()];
                for (java.util.Iterator<String> i = disabled_list.iterator(); i.hasNext();) {
                    String name = i.next();

                    disabled_objs[idx] = dal.Component_Helper.get(db, name);
                    System.out.println(" - add user-disabled component " + disabled_objs[idx].UID() + '@' + disabled_objs[idx].class_name());
                    idx++;
                }

                p.set_disabled(disabled_objs);
            }

            if (enabled_list != null) {
                int idx = 0;
                dal.Component[] enabled_objs = new dal.Component[enabled_list.size()];
                for (java.util.Iterator<String> i = enabled_list.iterator(); i.hasNext();) {
                    String name = i.next();

                    enabled_objs[idx] = dal.Component_Helper.get(db, name);
                    System.out.println(" - add user-enabled component " + enabled_objs[idx].UID() + '@' + enabled_objs[idx].class_name());
                    idx++;

                }

                p.set_enabled(enabled_objs);
            }

            // get all segments and resources
            dal.Component objs[] = dal.Component_Helper.get(db, new config.Query());

            System.out.println("Found " + objs.length + " objects of classes derived from Component class.");
            System.out.println("Apply disabled(p=\'" + p.UID() + '@' + p.class_name() + "\') algorithm to each of them.");

            // apply disabled() algorithm to each of them
            for (int j = 0; j < objs.length; ++j) {
                System.out.print(" * object \'" + objs[j].UID() + '@' + objs[j].class_name() + "\' is ");
                try {
                    if (objs[j].disabled(p)) {
                        if (show_why == 0) {
                            System.out.println("disabled");
                        } else {
                            System.out.println("disabled because\n" + objs[j].why_disabled(p, "     ", (show_why == 2 ? true : false)));
                        }
                    } else {
                        System.out.println("enabled");
                    }
                } catch (config.ConfigException ex) {
                    ers.Logger.error(new config.GenericException("cannot get disabled status of \'" + p.UID() + '@' + p.class_name() + '\'', ex));
                }
            }
        } catch (config.ConfigException ex) {
            ers.Logger.error(ex);
            System.exit(1);
        }
    }
}

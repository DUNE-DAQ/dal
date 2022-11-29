package dal;

public class TestDisabled
  {
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

	private static void run_test(dal.Component obj, dal.Partition partition) throws config.GenericException, config.SystemException, config.NotFoundException, config.NotValidException {
		System.out.println(" * disabled(\'" + partition.UID() + "@" + partition.class_name() + "\') on object \'"
				+ obj.UID() + "@" + obj.class_name() + "\' returns " + obj.disabled(partition));
	}

	private static void test_application(dal.BaseApplication a, dal.Partition partition) throws config.GenericException, config.SystemException, config.NotFoundException, config.NotValidException  {
		dal.Component c = dal.Component_Helper.cast(a);
		if (c != null) {
			System.out.println(" * disabled(\'" + partition.UID() + "@" + partition.class_name()
					+ "\') on resource application object \'" + a.UID() + "@" + a.class_name() + "\' returns "
					+ c.disabled(partition));
		}
	}

	static void test_segment(dal.Segment s, dal.Partition partition) throws config.GenericException, config.SystemException, config.NotFoundException, config.NotValidException {
		System.out.println(" * disabled(\'" + partition.UID() + "@" + partition.class_name()
				+ "\') on segment object \'" + s.UID() + "@" + s.class_name() + "\' returns " + s.disabled(partition));

		for (dal.Segment ns : s.get_nested_segments())
			test_segment(ns, partition);
	}
    
	static private void usage() {
		System.out.println("Usage: java dal.TestDisabled   [-d | --data db_spec]");
		System.out.println("                               [-p | --partition-name pname]");
		System.out.println("                               [-D | --disabled resource-id ...]");
		System.out.println("                               [-E | --enabled resource-id ...]");
		System.out.println("                               [-a | --test-applications]");
		System.out.println("                               [-s | --test-segments]");
		System.out.println("                               [-h | --help]");
		System.out.println("");
		System.out.println("Options/Arguments:");
		System.out.println("        -d | --data db_spec             name of the database");
		System.out.println("        -p | --partition-name pname     name of the partition object");
		System.out.println("        -D | --disabled resource-id+    identities of components disabled by user");
		System.out.println("        -E | --enabled resource-id+     identities of components enabled by user (override DB disabling)");
		System.out.println("        -a | --test-applications        test resource applications (including templated)");
		System.out.println("        -s | --test-segments            test segments (including templated)");
		System.out.println("        -h | --help                     print this message");
		System.out.println("");
		System.out.println("Description:");
		System.out.println("        This program prints out results of disabled() algorithm applied to all objects of classes derived from Component class and to the objects of application configuration");
	}

	public static void main(String args[]) {
		java.util.LinkedList<String> disabled_list = null;
		java.util.LinkedList<String> enabled_list = null;
		String db_name = new String();
		String partition_name = new String();
		boolean test_applications = false;
		boolean test_segments = false;

		for (int i = 0; i < args.length; i++) {
			if (args[i].equals("-d") || args[i].equals("--data")) {
				db_name = args[++i];
			} else if (args[i].equals("-p") || args[i].equals("--partition-name")) {
				partition_name = args[++i];
			} else if (args[i].equals("-D") || args[i].equals("--disabled")) {
				disabled_list = new java.util.LinkedList<String>();
				i = read_list(disabled_list, args, i);
			} else if (args[i].equals("-E") || args[i].equals("--enabled")) {
				enabled_list = new java.util.LinkedList<String>();
				i = read_list(enabled_list, args, i);
			} else if (args[i].equals("-a") || args[i].equals("--test-applications")) {
				test_applications = true;
			} else if (args[i].equals("-s") || args[i].equals("--test-segments")) {
				test_segments = true;
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

			// get a partition object (defined by command line or
			// TDAQ_PARTITION)

			dal.Partition p = dal.Algorithms.get_partition(db, partition_name);

			if (p == null) {
                ers.Logger.error(new ers.Issue("can not find partition object \'" + partition_name + '\''));
				System.exit(1);
			}

			db.register_converter(new SubstituteVariables(p));

			dal.Component[] objs = dal.Component_Helper.get(db, new config.Query());

			System.out.println("Apply disabled() algorithm on " + objs.length + " objects of base Component class.");

			for (dal.Component i : objs) {
				run_test(i, p);
			}

			dal.Segment rootSeg = null;
			dal.BaseApplication[] apps = null;

			if (test_segments || test_applications) {
				rootSeg = p.get_segment(p.get_OnlineInfrastructure().UID());
			}

			if (test_segments) {
				System.out.println("Test partition segments");
				test_segment(rootSeg, p);
			}

			if (test_applications) {
				apps = rootSeg.get_all_applications(null, null, null);
				System.out.println("Apply disabled algorithm on all resources applications");

				for (dal.BaseApplication app : apps) {
					test_application(app, p);
				}
			}

			if ((disabled_list != null && disabled_list.isEmpty() == false)
					|| (enabled_list != null && enabled_list.isEmpty() == false)) {
				System.out.println(
						"There are " + disabled_list.size() + " objects disabled and there are " + enabled_list.size()
								+ " objects enabled by user.\nApply user configuration and repeat the test.");

				java.util.LinkedList<dal.Component> disabled_obj_list = new java.util.LinkedList<dal.Component>();
				java.util.LinkedList<dal.Component> enabled_obj_list = new java.util.LinkedList<dal.Component>();

				for (int i = 0; i < disabled_list.size(); ++i) {
					dal.Component c = dal.Component_Helper.get(db, disabled_list.get(i));

					if (c != null) {
						disabled_obj_list.add(c);
					}
				}

				for (int i = 0; i < enabled_list.size(); ++i) {
					dal.Component c = dal.Component_Helper.get(db, enabled_list.get(i));

					if (c != null) {
						enabled_obj_list.add(c);
					}
				}

				if (disabled_obj_list.isEmpty() == false) {
					p.set_disabled(disabled_obj_list.toArray(new dal.Component[0]));
				}

				if (enabled_obj_list.isEmpty() == false) {
					p.set_enabled(enabled_obj_list.toArray(new dal.Component[0]));
				}

				System.out.println(
						"Re-apply disabled() algorithm on " + objs.length + " objects of base Component class.");

				for (dal.Component i : objs) {
					run_test(i, p);
				}

				if (test_segments || test_applications)
					rootSeg = p.get_segment(p.get_OnlineInfrastructure().UID());

				if (test_segments) {
					System.out.println("Re-test partition segments");
					test_segment(rootSeg, p);
				}

				if (test_applications) {
					apps = rootSeg.get_all_applications(null, null, null);
					System.out.println("Re-apply disabled algorithm on all resources applications");

					for (dal.BaseApplication app : apps) {
						test_application(app, p);
					}
				}
			}
		} catch (config.ConfigException ex) {
            ers.Logger.error(ex);
			System.exit(1);
		}
	}
  }

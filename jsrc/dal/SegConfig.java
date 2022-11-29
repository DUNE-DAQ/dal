package dal;

import java.util.LinkedList;

/**
 * The class is used to keep information about parameters for segments:
 *  - application unique name (database object ID or generated name for template applications)
 *  - pointer on the database application object describing application parameters
 *  - pointer on the database computer object, that application runs on
 *  - pointer on the database segment object, that application belongs - segment unique name (database object ID or generated name for template segments)
 * 
 *  @author "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
 */

public class SegConfig {
	
	private dal.Partition m_partition;
	dal.Segment m_base_segment;
	dal.BaseApplication m_controller;
	dal.BaseApplication[] m_infrastructure;
	dal.BaseApplication[] m_applications;
	dal.Segment[] m_nested_segments;
	dal.Computer[] m_hosts;
	boolean m_is_disabled;
    boolean m_is_templated;
    
    void clear(dal.Partition p, dal.Segment b)
    {
    	m_partition = p;
    	m_base_segment = b;
		m_controller = null;
		m_infrastructure = null;
		m_applications = null;
		m_nested_segments = null;
		m_hosts = null;
		m_is_disabled = false;
		m_is_templated = false;
    }

	SegConfig(dal.Partition p, dal.Segment b) {
		clear(p, b);
	}
	

	/**
	 * Get base segment.
	 * @return  the base segment
	 * @throws  config.SystemException if segment is disabled and hence no applications are configured
	 */
	public dal.Segment get_base_segment()  {
		return m_base_segment;
	}

	/**
	 * Get segment's controller.
	 * @return  configuration of controller application
	 * @throws  config.SystemException if segment is disabled and hence no applications are configured
	 */
	public dal.BaseApplication get_controller()  {
		return m_controller;
	}

	/**
	 * Get segment's infrastructure application.
	 * @return  vector of infrastructure applications configurations
	 * @throws  config.SystemException if segment is disabled and hence no applications are configured
	 */
	public dal.BaseApplication[] get_infrastructure() {
		return m_infrastructure;
	}

	/**
	 * Get segment's application.
	 * @return  vector of normal applications configurations
	 * @throws  config.SystemException if segment is disabled and hence no applications are configured
	 */
	public dal.BaseApplication[] get_applications() {
		return m_applications;
	}

	/**
	 * Get nested segments.
	 * @return  vector of nested segments configurations
	 */
	public dal.Segment[] get_nested_segments() {
		return m_nested_segments;
	}

	/**
	 * Get hosts.
	 * @return  vector of segment hosts
	 */
	public dal.Computer[] get_hosts() {
		return m_hosts;
	}

    /**
     * Get disabled status.
     * @return true, if the segment is disabled
     */
	public boolean is_disabled() {
	    return m_is_disabled;
    }

    /**
     * Get templated status.
     * @return true, if the segment is templated
     */
	public boolean is_templated() {
	    return m_is_templated;
    }
	
	/**
	 * Is used internally
	 */
	dal.Partition get_partition() {
		return m_partition;
	}

    /**
     * Method returns true if application's type is listed by the app_types, the
     * segment is listed in the use_segments and the host is listed in use_hosts
     * (if containers are defined)
     */
	private static void check_and_add(dal.BaseApplication app, String[] app_types, String[] use_segments,
			dal.Computer[] use_hosts, LinkedList<dal.BaseApplication> apps) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		// check application types, if any
		if (app_types != null) {
			for (int i = 0; true;) {
				if (app.class_name().compareTo(app_types[i]) == 0)
					break;
				if (++i == app_types.length)
					return;
			}
		}

		AppConfig app_config = app.get_app_config(false);

		// check segments, if any
		if (use_segments != null) {
			for (int i = 0; true;) {
				if (use_segments[i].compareTo(app_config.m_segment.UID()) == 0)
					break;
				if (++i == use_segments.length)
					return;
			}
		}

		// check hosts, if any
		if (use_hosts != null) {
			for (int i = 0; true;) {
				if (use_hosts[i].UID().compareTo(app_config.m_host.UID()) == 0)
					break;
				if (++i == use_hosts.length)
					return;
			}
		}

		apps.add(app);
	}

	
	private static void get_applications(LinkedList<dal.BaseApplication> apps, dal.Segment seg_obj, String[] app_types, String[] segments, dal.Computer[] hosts) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		SegConfig seg = seg_obj.get_seg_config(false, false);

		// return, if segment is disabled or not initialized
		if (seg.m_is_disabled == true || seg.m_controller == null) {
			return;
		}

		// check controller
		check_and_add(seg.m_controller, app_types, segments, hosts, apps);

		// check infrastructure
		for (dal.BaseApplication x : seg.m_infrastructure) {
			check_and_add(x, app_types, segments, hosts, apps);
		}

		// check applications
		for (dal.BaseApplication x : seg.m_applications) {
			check_and_add(x, app_types, segments, hosts, apps);
		}

		// check nested segments
		for (dal.Segment x : seg.m_nested_segments) {
			get_applications(apps, x, app_types, segments, hosts);
		}
	}


    /**
     * Get all application of this and nested segments.
     *
     * The method returns description of applications running in this and nested segments.
     * The description is read from attributes of this Segment and nested Segment objects (i.e. without database queries).
     * In the parameters one can precise types of applications, names of segments and hosts the applications run on.
     *
     * @param seg
     *          the segment object
     * @param app_types
     *          parameter defines application class names (also takes into account their subclasses),
     *          which objects have to be taken into account (use all applications, if the parameter is null)
     * @param use_segments
     *          parameter defines names of segment, which applications have to be taken into account
     *          (use all segments, if the parameter is null)
     * @param use_hosts
     *          parameter defines names of hosts where the applications have to run on
     *          (use all hosts, if the parameter is null)
     *
     * @return the result of the algorithm's work (see dal.BaseApplication class for more information on application description)
     */
	static dal.BaseApplication[] get_all_applications(dal.Segment seg, String[] app_types, String[] use_segments, dal.Computer[] use_hosts) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
	{
		// calculate complete list of application types taking into account subclasses
		if (app_types != null) {
			java.util.TreeSet<String> set = new java.util.TreeSet<String>();

			for (String name : app_types) {
				if (set.contains(name) == false) {
					if (seg.configuration_object().subclasses() != null) {
						java.util.TreeSet<String> sbc = seg.configuration_object().subclasses().get(name);
						if (sbc != null) {
							for (java.util.Iterator<String> j = sbc.iterator(); j.hasNext();) {
								set.add(j.next());
							}
						} else if (seg.configuration_object().superclasses() != null) {
							if (seg.configuration_object().superclasses().get(name) == null) {
								ers.Logger.error(new ers.Issue("class \'" + name + "\' is not defined by the database schema"));
								continue;
							}
						}
					}

					set.add(name);
				}
			}

			if (set.size() > 0) {
				app_types = set.toArray(new String[0]);
			} else {
				app_types = null;
			}
		}

		LinkedList<dal.BaseApplication> apps = new LinkedList<dal.BaseApplication>();

		get_applications(apps, seg, app_types, use_segments, use_hosts);

		return apps.toArray(new dal.BaseApplication[0]);
	}
}

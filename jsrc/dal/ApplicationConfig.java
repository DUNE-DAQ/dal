package dal;

import java.util.Arrays;
import java.util.LinkedList;
import java.util.Map;
import java.util.TreeMap;

/**
   *  The <b>ApplicationConfig</b> stores information about partition's root segment initialized by the applications and segments configuration algorithms.
   *  A calculation of the configuration for a big partition requires a lot of CPU resources.
   *  By this reason such calculation is done once for applications and segments at first request and is stored inside partitions.
   *  Above configuration is cleaned on each database reload or modification (is implemented using config.ConfigAction mechanism).
   *  <p>
   *  By efficiency reasons an object of such class is used as a singleton per partition object.
   *  <p>
   *
   *  @author "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
   */

public class ApplicationConfig implements config.ConfigAction {

	dal.Segment m_root_segment;

	/** 
	 * Create new object to store in cache disabled status of components, to allow
	 * user disabling and enabling and to get explanation "why given component is
	 * disabled".
	 *
	 * @param db
	 *            configuration database object
	 */

	public ApplicationConfig(config.Configuration db) {
		clear_root_segment();
		db.add_config_action(this);
	}
	
	private static void check_non_template_segment(dal.Segment seg, dal.BaseApplication base_app) throws config.GenericException, config.SystemException, config.NotFoundException, config.NotValidException
	{
	  if(seg.get_seg_config(false, false).is_templated() )
	    {
		  throw new config.GenericException("Bad configuration description of template segment \"" + seg.UID() + "\": the segment contains non-template application \"" + base_app.UID() + "\"");
	    }
	}

	private static dal.Computer get_host(dal.Segment seg, dal.BaseApplication base_app, dal.Application app) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		if (app != null) {
			dal.Computer h = app.get_RunsOn();
			if (h != null)
				return h;
		}

		dal.Computer[] hosts = seg.get_hosts();
		if (hosts.length != 0)
			return hosts[0];

        throw new config.GenericException("Failed to find default host for segment \'" + seg.UID() + "\' to run \'" + base_app.UID() + "@" + base_app.class_name()
                + "\' (there is no any defined enabled default host for segment, partition or localhost)");
	}

	private static String appId(dal.TemplateApplication a, String segment_id, dal.Computer host, int num) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		if (a.get_RunsOn().compareTo(dal.TemplateApplication.RunsOn.FirstHost.toString()) == 0
				|| a.get_RunsOn().compareTo(dal.TemplateApplication.RunsOn.FirstHostWithBackup.toString()) == 0) {
			if (num != 0) {
				return new String(a.UID() + ':' + segment_id + ':' + num);
			} else {
				return new String(a.UID() + ':' + segment_id);
			}
		} else {
			int dn_start = host.UID().indexOf('.');
			if (dn_start == -1) {
				dn_start = host.UID().length();
			}

			String host_name = host.UID().substring(0, dn_start);

			if (num != 0) {
				return new String(a.UID() + ':' + segment_id + ':' + host_name + ':' + num);
			} else {
				return new String(a.UID() + ':' + segment_id + ':' + host_name);
			}
		}
	}
    
    private static dal.Computer[] set_backup_hosts(dal.TemplateApplication a, BackupHostFactory2 factory) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
		if (a.get_RunsOn().compareTo(dal.TemplateApplication.RunsOn.FirstHostWithBackup.toString()) == 0) {
			if (factory.get_size() > 1) {
				dal.Computer first_backup_host = factory.get_next();
				if (factory.get_size() > 2) {
					return new dal.Computer[] { first_backup_host, factory.get_next() };
				} else {
					return new dal.Computer[] { first_backup_host };
				}
			}
		}

		return new dal.Computer[] {};
    }
    
	private static AppConfig reset_app_config(dal.BaseApplication app_obj) {
		dal.BaseApplication_Impl app = (dal.BaseApplication_Impl) app_obj;

		if (app.p_app_config == null) {
			app.p_app_config = new AppConfig();
		} else {
			app.p_app_config.clear();
		}

		return app.p_app_config;
	}

	private static SegConfig reset_seg_config(dal.Segment seg_obj, dal.Partition p) {
		dal.Segment_Impl seg = (dal.Segment_Impl) seg_obj;

		if (seg.p_seg_config == null) {
			seg.p_seg_config = new SegConfig(p, seg_obj);
		} else {
			seg.p_seg_config.clear(p, seg_obj);
		}

		return seg.p_seg_config;
	}
	
    public static AppConfig get_app_config(dal.BaseApplication app_obj, boolean no_except)
            throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
        dal.BaseApplication ptr = null;

        try {
            Class<?> c = app_obj.getClass();
            if (c != dal.BaseApplication_Impl.class) {
                java.lang.reflect.Field gen_obj_field = c.getDeclaredField("p_gen_obj");
                gen_obj_field.setAccessible(true);

                dal.BaseApplication p_gen_obj = (dal.BaseApplication) gen_obj_field.get(app_obj);

                if (p_gen_obj != null) {
                    ptr = p_gen_obj;
                } else {
                    ptr = (dal.BaseApplication) app_obj.configuration_object().get("BaseApplication", app_obj.UID());

                    if (ptr != null) {
                        p_gen_obj = ptr;
                        gen_obj_field.set(app_obj, p_gen_obj);
                    } else {
                        if (no_except)
                            return null;
                        else
                            throw new config.GenericException("Application \'" + app_obj.UID() + "@" + app_obj.class_name() + "\' was not initialized by DAL algorithm");
                    }
                }
            } else {
                ptr = app_obj;
            }
        } catch (final NoSuchFieldException e) {
            throw new config.SystemException("NoSuchFieldException exception for application \'" + app_obj.UID() + "@" + app_obj.class_name() + "\'", e);
        } catch (final SecurityException e) {
            throw new config.SystemException("SecurityException for application \'" + app_obj.UID() + "@" + app_obj.class_name() + "\'", e);
        } catch (final IllegalAccessException e) {
            throw new config.SystemException("IllegalAccessException for application \'" + app_obj.UID() + "@" + app_obj.class_name() + "\'", e);
        }

        AppConfig app_config = ((dal.BaseApplication_Impl) ptr).p_app_config;

        if (app_config == null) {
            if (no_except)
                return null;
            else
                throw new config.GenericException(
                        "Application \'" + app_obj.UID() + "@" + app_obj.class_name() + "\' was not initialized by DAL algorithm");
        }

        return app_config;
    }
	
	public static SegConfig get_seg_config(dal.Segment seg_obj, boolean check_disabled, boolean no_except) throws config.NotFoundException, config.NotValidException, config.SystemException {
		dal.Segment ptr = null;

		try {
			Class<?> c = seg_obj.getClass();

			if (c != dal.Segment_Impl.class) {
				java.lang.reflect.Field gen_obj_field = c.getDeclaredField("p_gen_obj");
				gen_obj_field.setAccessible(true);

				dal.Segment p_gen_obj = (dal.Segment) gen_obj_field.get(seg_obj);

				if (p_gen_obj != null) {
					ptr = p_gen_obj;
				} else {
					ptr = (dal.Segment) seg_obj.configuration_object().get("Segment", seg_obj.UID());

					if (ptr != null) {
						p_gen_obj = ptr;
						gen_obj_field.set(seg_obj, p_gen_obj);
					} else {
						if (no_except)
							return null;
						else
                            throw new config.SystemException("Segment \'" + seg_obj.UID() + "@" + seg_obj.class_name() + "\' was not initialized by DAL algorithm");
					}
				}
			} else {
				ptr = seg_obj;
			}
        } catch (final NoSuchFieldException e) {
            throw new config.SystemException("NoSuchFieldException for segment \'" + seg_obj.UID() + "@" + seg_obj.class_name() + "\'", e);
        } catch (final SecurityException e) {
            throw new config.SystemException("SecurityException for segment \'" + seg_obj.UID() + "@" + seg_obj.class_name() + "\'", e);
        } catch (final IllegalAccessException e) {
            throw new config.SystemException("IllegalAccessException for segment \'" + seg_obj.UID() + "@" + seg_obj.class_name() + "\'", e);
		}
		
		SegConfig seg_config = ((dal.Segment_Impl)ptr).p_seg_config;

		if (seg_config == null) {
			if (no_except)
				return null;
			else
                throw new config.SystemException("Segment \'" + seg_obj.UID() + "@" + seg_obj.class_name() + "\' was not initialized by DAL algorithm");
		}

		return seg_config;
	}

	private static void add_normal_application(dal.Application a, dal.Segment seg, LinkedList<dal.BaseApplication> apps) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
	{
		check_non_template_segment(seg, a);
		dal.BaseApplication app_obj = dal.BaseApplication_Helper.get(a.configuration_object(), a.config_object(), a.UID());
		AppConfig app_config = reset_app_config(app_obj);
		app_config.m_host = get_host(seg, a, a);
		app_config.m_segment = seg;
		app_config.m_base_app = a;
		apps.add(app_obj);
	}

	private static void add_template_application(dal.TemplateApplication a, String type, dal.Segment seg,
			LinkedList<dal.BaseApplication> apps, BackupHostFactory2 factory) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		int start_idx = 0;
		dal.Computer[] hosts = seg.get_hosts();
		int end_idx = hosts.length;

		if (hosts.length == 0) {
			throw new config.SystemException(type + " template application \'" + a.UID() + '@' + a.class_name()
					+ "\' may not be run, since segment has no enabled hosts");
		}

		if (a.get_RunsOn().compareTo(dal.TemplateApplication.RunsOn.AllButFirstHost.toString()) == 0) {
			if (hosts.length < 2) {
				throw new config.SystemException(type + " template application \'" + a.UID() + '@' + a.class_name()
						+ "\' may not be run on \"" + dal.TemplateApplication.RunsOn.AllButFirstHost.toString()
						+ "\" since segment has " + hosts.length + " enabled hosts only");
			}

			start_idx = 1;
		} else if (a.get_RunsOn().compareTo(dal.TemplateApplication.RunsOn.FirstHost.toString()) == 0
				|| a.get_RunsOn().compareTo(dal.TemplateApplication.RunsOn.FirstHostWithBackup.toString()) == 0) {
			end_idx = 1;
		}

		int default_num_of_tapps = a.get_Instances();

		for (int i = start_idx; i < end_idx; ++i) {
			dal.Computer h = hosts[i];
			int num_of_tapps = (default_num_of_tapps != 0 ? default_num_of_tapps : (h.get_NumberOfCores()));

			for (int j = 1; j <= num_of_tapps; ++j) {
				dal.BaseApplication app_obj = dal.BaseApplication_Helper.get(a.configuration_object(),
						a.config_object(), appId(a, seg.UID(), h, (num_of_tapps == 1 ? 0 : j)));
				AppConfig app_config = reset_app_config(app_obj);
				app_config.m_is_templated = true;
				app_config.m_host = h;
				app_config.m_template_backup_hosts = set_backup_hosts(a, factory);
				app_config.m_segment = seg;
				app_config.m_base_app = a;
				apps.add(app_obj);
			}
		}
	}
	
	static void add_computers(LinkedList<dal.Computer> v, dal.ComputerBase cb) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		dal.Computer cp = dal.Computer_Helper.cast(cb);

		if (cp != null) {
			v.add(cp);
		} else {
			dal.ComputerSet cs = dal.ComputerSet_Helper.cast(cb);

			if (cs != null) {
				add_computers(v, cs.get_Contains());
			}
		}
	}

	private static void add_computers(LinkedList<dal.Computer> v, dal.ComputerBase[] hosts) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		for (dal.ComputerBase x : hosts) {
			add_computers(v, x);
		}
	}

    private static void add_enabled_hosts(LinkedList<dal.Computer> to, dal.ComputerBase[] from)
            throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
        LinkedList<dal.Computer> hosts = new LinkedList<dal.Computer>();
        add_computers(hosts, from);

        for (dal.Computer x : hosts)
            if (x.get_State() == true)
                to.add(x);
    }

	private static void get_resource_applications(dal.ResourceBase obj, LinkedList<dal.BaseApplication> out,
			dal.Partition p) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		if (p != null)
			if (p == null || p.resources().get_disabled(obj, p, true) == false) {

				// test if the resource base can be casted to the application
				dal.BaseApplication a = dal.BaseApplication_Helper.cast(obj);
				if (a != null) {
					out.add(a);
				}

				// test if the resource base can contain nested resources
				dal.ResourceSet s = dal.ResourceSet_Helper.cast(obj);
				if (s != null) {
					for (dal.ResourceBase o : s.get_Contains()) {
						get_resource_applications(o, out, p);
					}
				}
			}
	}

	private static void add_applications(dal.Segment seg, dal.Rack rack, dal.Partition p, dal.Computer default_host) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		dal.SegConfig seg_config = seg.get_seg_config(false, false);

		LinkedList<dal.Computer> hosts = new LinkedList<dal.Computer>();

		// fill hosts of segment
		if (rack == null) {
			add_enabled_hosts(hosts, seg.get_Hosts());
		} else {
			add_enabled_hosts(hosts, rack.get_Nodes());

			if (hosts.size() < 2) {
				throw new config.SystemException("Failed to create config for segment \'" + seg.UID() + '@'
						+ seg.class_name() + "\': number of enabled computers in \'" + rack.UID() + '@'
						+ rack.class_name() + "\' is " + hosts.size() + " (at least two required)");
			}
		}

		// if there are no hosts, try to use partition and closest parent segment efault host
		if (hosts.isEmpty() == true && default_host != null) {
			hosts.add(default_host);
		}

		// check local-host, if there are no hosts
		if (hosts.isEmpty() == true) {
			try {
				String hostname = java.net.InetAddress.getLocalHost().getCanonicalHostName();
				dal.Computer host = dal.Computer_Helper.get(seg.configuration_object(), hostname);
				if (host.get_State() == true) {
					hosts.add(host);
				}
			} catch (final java.net.UnknownHostException e) {
				throw new config.SystemException("cannot get local host name", e);
			}
		}

		seg_config.m_hosts = hosts.toArray(new dal.Computer[0]);

		// backup ....
		BackupHostFactory2 factory = new BackupHostFactory2(seg);

		// add controller
		{
			dal.RunControlApplicationBase ctrl_obj = seg.get_IsControlledBy();
			dal.BaseApplication app_obj = null;
			AppConfig app_config = null;

			dal.RunControlApplication a = dal.RunControlApplication_Helper.cast(ctrl_obj);
			if (a != null) {
				check_non_template_segment(seg, a);
				app_obj = dal.BaseApplication_Helper.get(a.configuration_object(), a.config_object(), a.UID());
				app_config = reset_app_config(app_obj);
				app_config.m_host = get_host(seg, a, a);
			} else {
				dal.TemplateApplication t = dal.TemplateApplication_Helper.cast(ctrl_obj);

				if (t != null) {
					if (t.get_RunsOn().compareTo(dal.TemplateApplication.RunsOn.FirstHost.toString()) != 0
							&& t.get_RunsOn()
									.compareTo(dal.TemplateApplication.RunsOn.FirstHostWithBackup.toString()) != 0) {
						throw new config.SystemException("Failed to create config for segment \'" + seg.UID() + '@'
								+ seg.class_name() + "\': controller template application \'" + t.UID() + '@'
								+ t.class_name() + "\' may only be run on first host (\"" + t.get_RunsOn()
								+ "\" is set instead)");
					}

					app_obj = dal.BaseApplication_Helper.get(t.configuration_object(), t.config_object(), seg.UID());
					app_config = reset_app_config(app_obj);
					app_config.m_is_templated = true;
					app_config.m_host = get_host(seg, t, null);
					app_config.m_template_backup_hosts = set_backup_hosts(t, factory);
				}
			}

			app_config.m_segment = seg;
			app_config.m_base_app = dal.BaseApplication_Helper.cast(ctrl_obj);
			seg_config.m_controller = app_obj;
		}

		// add infrastructure
		LinkedList<dal.BaseApplication> applications = new LinkedList<dal.BaseApplication>();
		for (dal.InfrastructureBase x : seg.get_Infrastructure()) {
			dal.Resource r = dal.Resource_Helper.cast(x);

			if (r == null) {
				dal.InfrastructureApplication a = dal.InfrastructureApplication_Helper.cast(x);
				if (a != null) {
					add_normal_application(a, seg, applications);
				} else {
					dal.TemplateApplication t = dal.TemplateApplication_Helper.cast(x);
					if (t != null)
						add_template_application(t, "infrastructure", seg, applications, factory);
				}
			}
		}
		seg_config.m_infrastructure = applications.toArray(new dal.BaseApplication[0]);

		// add applications
		applications = new LinkedList<dal.BaseApplication>();

		// resource applications
		for (dal.ResourceBase x : seg.get_Resources()) {
			LinkedList<dal.BaseApplication> res_apps = new LinkedList<dal.BaseApplication>();
			get_resource_applications(x, res_apps, p);

			for (dal.BaseApplication j : res_apps) {
				dal.Application a = dal.Application_Helper.cast(j);
				if (a != null) {
					add_normal_application(a, seg, applications);
				} else {
					dal.TemplateApplication t = dal.TemplateApplication_Helper.cast(j);
					if (t != null)
						add_template_application(t, "resource", seg, applications, factory);
				}
			}
		}

		// normal applications
		for (dal.BaseApplication x : seg.get_Applications()) {
			dal.Resource r = dal.Resource_Helper.cast(x);

			if (r == null) {
				dal.Application a = dal.Application_Helper.cast(x);
				if (a != null) {
					add_normal_application(a, seg, applications);
				} else {
					dal.TemplateApplication t = dal.TemplateApplication_Helper.cast(x);
					if (t != null)
						add_template_application(t, "normal", seg, applications, factory);
				}
			}

		}

		seg_config.m_applications = applications.toArray(new dal.BaseApplication[0]);
	}
	
	// The functions to recursively find first enabled host

	private static dal.Computer find_enabled(dal.ComputerBase[] hosts) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		for (dal.ComputerBase i : hosts) {
			dal.Computer c = find_enabled(i);
			if (c != null)
				return c;
		}
		return null;
	}

	private static dal.Computer find_enabled(dal.ComputerBase cb) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		dal.Computer c = dal.Computer_Helper.cast(cb);
		if (c != null) {
			if (c.get_State() == true)
				return c;
		} else {
			dal.ComputerSet cs = dal.ComputerSet_Helper.cast(cb);
			if (cs != null) {
				for (dal.ComputerBase i : cs.get_Contains()) {
					c = find_enabled(i);
					if (c != null)
						return c;
				}
			}
		}
		return null;
	}
	
	private static String seg_config_to_name(String s) {
		if (s.isEmpty() == true)
			return new String("partition");
		else
			return new String("segment \"" + s + "\"");
	}

	static void
	check_mulpiple_inclusion(Map<String, String> fuse, String id, String parent) throws config.SystemException
	{
    	String old = fuse.put(id, parent);

    	if(old != null)
            throw new config.SystemException("The segment \"" + id + "\" is included by:\n" + 
                    "  1) " + seg_config_to_name(old) + "\n" +
                    "  2) " + seg_config_to_name(parent) );
	}

	private static void add_segments(dal.Segment seg, dal.Partition p, dal.Segment[] objs, dal.Rack rack, dal.Computer default_host, Map<String, String> fuse) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		dal.SegConfig seg_config = seg.get_seg_config(false, false);

		dal.Computer c = find_enabled(seg.get_Hosts());
		if (c != null)
			default_host = c;

		seg_config.m_is_disabled = p.resources().get_disabled(seg, p, true);

		if (seg_config.m_is_disabled == false) {
			add_applications(seg, rack, p, default_host);
		}

		LinkedList<dal.Segment> nested_segments = new LinkedList<dal.Segment>();

		for (dal.Segment x : objs) {
			dal.TemplateSegment ts = dal.TemplateSegment_Helper.cast(x);

			if (ts != null) {
				boolean ts_is_disabled = p.resources().get_disabled(ts, p, true);

				for (dal.Rack y : ts.get_Racks()) {
					boolean is_disabled = (ts_is_disabled || p.resources().get_disabled(y, p, true));

					String id = new String(x.UID() + ':' + y.UID());
					check_mulpiple_inclusion(fuse, id, seg.UID());

					dal.Segment s = dal.Segment_Helper.get(seg.configuration_object(), ts.config_object(), id);
					dal.SegConfig nested_seg_config = reset_seg_config(s, p);
					nested_seg_config.m_is_disabled = is_disabled;
					nested_seg_config.m_is_templated = true;
					nested_seg_config.m_base_segment = x;
					nested_seg_config.m_nested_segments = new dal.Segment[0];

					nested_segments.add(s);

					if (is_disabled == false) {
						add_applications(s, y, p, default_host);
					}
				}
			} else {
				check_mulpiple_inclusion(fuse, x.UID(), seg.UID());

				dal.Segment s = dal.Segment_Helper.get(seg.configuration_object(), x.config_object(), x.UID());
				dal.SegConfig nested_seg_config = reset_seg_config(s, p);
				nested_seg_config.m_is_templated = false;
				nested_seg_config.m_base_segment = x;

				nested_segments.add(s);

				add_segments(s, p, x.get_Segments(), null, default_host, fuse);
			}
		}

		seg_config.m_nested_segments = nested_segments.toArray(new Segment[0]);
	}

	
	public synchronized dal.Segment get_segment(dal.Partition p, String id) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		if (m_root_segment == null) {
			dal.OnlineSegment onlseg = p.get_OnlineInfrastructure();
			dal.Segment root_segment = dal.Segment_Helper.get(p.configuration_object(), onlseg.config_object());

			// reinitialize seg config
			SegConfig root_segment_config = reset_seg_config(root_segment, p);

			dal.Computer default_host = p.get_DefaultHost();

			if (default_host != null && default_host.get_State() == false)
				default_host = null;

			final Map<String, String> fuse = new TreeMap<>();
			fuse.put(root_segment.UID(), "");

			add_segments(root_segment, p, p.get_Segments(), null, default_host, fuse);

			LinkedList<dal.BaseApplication> infrastrcuture_apps = new LinkedList<dal.BaseApplication>(Arrays.asList(root_segment_config.m_infrastructure));
			LinkedList<dal.BaseApplication> normal_apps = new LinkedList<dal.BaseApplication>(Arrays.asList(root_segment_config.m_applications));

			for (dal.Application a : p.get_OnlineInfrastructureApplications()) {
				dal.ResourceBase r = dal.ResourceBase_Helper.cast(a);
				if (r != null && p.resources().get_disabled(r, p, true))
					continue;

				dal.InfrastructureBase ib = dal.InfrastructureBase_Helper.cast(a);
				add_normal_application(a, root_segment, (ib != null) ? infrastrcuture_apps : normal_apps);
			}

			root_segment_config.m_infrastructure = infrastrcuture_apps.toArray(new dal.BaseApplication[0]);
			root_segment_config.m_applications = normal_apps.toArray(new dal.BaseApplication[0]);

			m_root_segment = root_segment;
		}

		dal.Segment seg = (dal.Segment) p.configuration_object().get("Segment", id);

		if (seg == null) {
			int idx = id.indexOf(':');

			if (idx > 0) {
				String seg_id = id.substring(0, idx);

				seg = dal.Segment_Helper.get(p.configuration_object(), seg_id);

				if (seg == null) {
					throw new config.SystemException("cannot find template segment \'" + seg_id + "\' (full name: \'" + id + "\'");
				}

				dal.TemplateSegment ts = dal.TemplateSegment_Helper.cast(seg);

				if (ts != null) {
					throw new config.SystemException("segment \'" + ts.UID() + "@" + ts.class_name() + "\' does not have rack \"" + id.substring(idx + 1) + "\" (full name: \'" + id + "\'");
				} else {
					throw new config.SystemException("object \'" + seg_id + "\' is not template segment (full name: \'" + id + "\'");
				}
			} else {
				throw new config.SystemException("no such non-template segment object (full name: \'" + id + "\'");
			}
		}

		return seg;
	}
	


	/** config.ConfigAction: clean the application configuration on any database update */

	public void notify(config.Change[] changes) {
		clear_root_segment();
	}

	/** config.ConfigAction: clean the application configuration on any database unload */

	public void unload() { 
		clear_root_segment();
	}

	/** config.ConfigAction: clean the application configuration on any database load */

	public void load() {
		clear_root_segment();
	}

	/** config.ConfigAction: clean the application configuration on any database modification */

	public void update(config.ConfigObject obj, String name) {
		clear_root_segment();
	}

	private synchronized void clear_root_segment() {
		m_root_segment = null;
	}
}

package dal;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.TreeMap;

/**
    *  The class is used to keep information about parameters for applications:
    *  - application unique name (database object ID or generated name for template applications)
    *  - pointer on the database application object describing application parameters
    *  - pointer on the database computer object, that application runs on
    *  - pointer on the database segment object, that application belongs
    *  - segment unique name (database object ID or generated name for template segments)
    *
    *  @author "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
    */


public class AppConfig {
	
    dal.BaseApplication m_base_app;
    dal.Computer m_host;
	dal.Segment m_segment;
	boolean m_is_templated;
	dal.Computer[] m_template_backup_hosts;
	
	void clear()
	{
		m_base_app = null;
		m_host = null;
		m_segment = null;
		m_is_templated = false;
		m_template_backup_hosts = null;
	}

	AppConfig() {
		clear();
	}

	/**
	 * Get base application object used to configure the application.
	 * 
	 * @return the application object
	 */

	public dal.BaseApplication get_base_app() {
		return m_base_app;
	}

	/**
	 * Get the host where the application runs on.
	 * 
	 * @return the computer object
	 */

	public dal.Computer get_host() {
		return m_host;
	}

	/**
	 * Get segment that the application belongs.
	 * 
	 * @return the segment object
	 */

	public dal.Segment get_segment() {
		return m_segment;
	}

	/**
	 * Get templated status.
	 * 
	 * @return true, if the application is templated
	 */
	public boolean get_is_templated() {
		return m_is_templated;
	}

	/**
	 * Get backup hosts.
	 * 
	 * @return the backup hosts
	 */
	public dal.Computer[] get_backup_hosts() throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		dal.Application a = dal.Application_Helper.cast(m_base_app);

		if (a != null) {
			if (a.get_BackupHosts().length == 0)
				return null;

			LinkedList<dal.Computer> hosts = new LinkedList<dal.Computer>();

			for (dal.ComputerBase c : a.get_BackupHosts()) {
				Algorithms.add_computers(hosts, c);
			}

			return hosts.toArray(new dal.Computer[0]);
		} else {
			return m_template_backup_hosts;
		}
	}
	
    /**
     * Get partial application information.
     * 
     * The method returns vector of allowed tags and path to this application
     * from partition level.
     * 
     * Parameters:
     * 
     * @param a
     *          the application object
     * @param tags
     *          output vector of supported tags for this application
     * @param s_list
     *          output list of segments containing this application
     * @param partition
     *          reference on partition
     */
  
  static void get_some_info(dal.BaseApplication a,
		                    List<dal.Tag> tags,
                            List<dal.Segment> s_list,
                            dal.Partition partition) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
  {
      s_list.clear();
      
      dal.AppConfig ac = a.get_app_config(false);

      dal.Segment root_segment = dal.Segment_Helper.cast(partition.get_OnlineInfrastructure());
      dal.Computer host = ac.get_host();

      final List<List<dal.Component>> paths = new ArrayList<>();      
      {
          // Very inefficient to work with arrays
                    
          final dal.Component[][] p = ac.m_segment.get_parents(partition);
          for(final dal.Component[] c : p) {
              final List<dal.Component> l = new ArrayList<>(Arrays.asList(c));
              l.add(ac.m_segment);
              
              paths.add(l);
          }          
      }                
      
      if(paths.isEmpty() == true) {
          throw new config.GenericException("The applications " + a.UID() + " is not in the partition control tree");
      } else if(paths.size() > 1) {
          final StringBuilder msg = new StringBuilder();
          msg.append("There are too many paths from the partition object ");
          msg.append(partition.UID());
          msg.append(": ");
          
          for(final List<dal.Component> i : paths) {
              msg.append(" * path including ");
              msg.append(i.size());
              msg.append(" components:\n");
              
              for(final dal.Component j : i) {
                  msg.append("  - ");
                  msg.append(j.UID());
                  msg.append("\n");
              }
          }
          
          throw new config.GenericException(msg.toString());
      }
      
      final List<dal.Component> path = paths.get(0);                  
      if((path.size() == 1) && (path.get(0).UID().equals(root_segment.UID()) == true)) {
          s_list.add(root_segment);
      } else {
          dal.Segment[] segs = root_segment.get_nested_segments();
                    
          for(final dal.Component i : path) {                            
              final dal.TemplateSegment tseg = dal.TemplateSegment_Helper.cast(i);
              final dal.Segment seg = dal.Segment_Helper.cast(i);
              
              if(seg != null) {
                  if(segs != null) {
                      for(final dal.Segment j : segs) {                            
                          if(j.config_object().implementation().equals(seg.config_object().implementation()) == true) {
                              if(tseg != null) {                                  
                                  if(ac.get_segment().UID().equals(j.UID()) == false) {
                                      continue;
                                  }
                              }
                              
                              s_list.add(j);
                              segs = j.get_nested_segments();
                              break;
                          }
                          
                          if(j.equals(segs[segs.length - 1])) {
                              throw new config.NotFoundException("Cannot find segment " + seg.UID() + " as a nested child of partition " + partition.UID());
                          }
                      }
                  }
              } else {
                  break;
              }
          }
          
          s_list.add(0, root_segment);
      }
      
      // Check Application has a Program linked to it and that program is linked to a sw repository
      try {
          ac.get_base_app().get_Program().get_BelongsTo();
      }
      catch(final Exception ex) {
          throw new config.GenericException("Failed to read application's Program object", ex);
      }
          
      // Get the Tags
      //   - get the tag or possible tags (Application ExplicitTag, DefaultTags
      //     from the segment list, Partition DefaultTags)
      //   - eliminate those which are not supported by the hw
      final List<dal.Tag> tempTags = new ArrayList<>();
      
      // Get possible tags
      if(ac.get_base_app().get_ExplicitTag() != null) {
          // Application ExplicitTag
          tempTags.add(ac.get_base_app().get_ExplicitTag());
      } else {
          // DefaultTags from the segment list
          final ListIterator<dal.Segment> revIt = s_list.listIterator(s_list.size());
          while(revIt.hasPrevious() == true) {
              final dal.Segment i = revIt.previous();
              if((s_list.size() > 1) && (i.equals(root_segment) == true)) {
                  continue;
              }
              
              final dal.Tag[] dt = i.get_DefaultTags();
              if(dt != null && dt.length > 0) {
                  Collections.addAll(tempTags, dt);
                  break;
              }
          }
          
          // Partition DefaultTags
          final dal.Tag[] pdt = partition.get_DefaultTags();
          if((pdt != null) && (pdt.length > 0) && (tempTags.isEmpty() == true)) {
              Collections.addAll(tempTags, pdt);
          }
      }
      
      // Report error if there are no possible tags
      if(tempTags.isEmpty() == true) {
          throw new config.GenericException("There are no tags defined for application " + a.UID());
      }
      
      // Remove tags which are not supported by the hardware
      {
          boolean found_a_hw_tag = false;
          
          // Go through all tags and remove tags if not for this hardware
          for(final dal.Tag t : tempTags) {
              if(dal.Algorithms.is_compatible(t, host, partition) == true) {
                  tags.add(t);
                  found_a_hw_tag = true;
              } 
          }
          
          // No Tags found to support this hardware
          if(found_a_hw_tag == false) {
              final StringBuilder msg = new StringBuilder();
              
              msg.append("Application's and/or default tags (");
              for(final dal.Tag i : tempTags) {
                  msg.append(" ");
                  msg.append(i.get_HW_Tag() + "-" + i.get_SW_Tag());
                  msg.append(" ");
              }
              
              msg.append(") are not supported by the host ");
              msg.append(host.UID());
              msg.append(" (HW tag: \"");
              msg.append(host.get_HW_Tag());
              msg.append("\")");
              
              throw new config.GenericException(msg.toString());
          }
      }
  }

  
  static private String get_host_and_backup_list(dal.BaseApplication app) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
	final StringBuilder s = new StringBuilder(app.get_host().UID());

	final dal.InfrastructureApplication ia = dal.InfrastructureApplication_Helper.cast(app.get_base_app());

	if (ia != null) {
		LinkedList<dal.Computer> backup_hosts = new LinkedList<dal.Computer>();
		for (dal.ComputerBase c : ia.get_BackupHosts()) {
			dal.ApplicationConfig.add_computers(backup_hosts, c);
		}

		for (dal.Computer c : backup_hosts) {
			s.append(',');
			s.append(c.UID());
		}
	}

	return s.toString();
  }

  /**
   * Get full information about application.
   * 
   * The method returns vector of allowed tags, process environment, possible
   * program names and command line arguments.
   * 
   * @param a
   *          the application object
   * @param environment
   *          output map of process environment name:value pairs
   * @param program_names
   *          output vector of possible program names
   * @param startArgs
   *          output string with command line arguments to start application
   * @param restartArgs
   *          output string with command line arguments to re-start
   *          application
   *
   * @return suitable tag for this application
   */

static public dal.Tag get_info(
		dal.BaseApplication a,
		Map<String, String> environment,
		List<String> program_names,
		StringBuilder startArgs,
		StringBuilder restartArgs) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException 
  {
    dal.AppConfig ac = a.get_app_config(false);

    dal.Partition partition = ac.m_segment.get_seg_config(false, false).get_partition();
    dal.Computer host = a.get_host();

    dal.Tag tagToReturn = null;
    
    final List<dal.Tag> tags = new ArrayList<>();
    final LinkedList<dal.Segment> s_list = new LinkedList<>();
    get_some_info(a, tags, s_list, partition);
    
    final List<String> tmp_paths_to_shared_libraries = new ArrayList<>();
    final List<String> tmp_search_paths = new ArrayList<>();
    
    for(final dal.Tag t : tags) {
        try {
            dal.Algorithms.get_parameters(ac.m_base_app.get_Program(), program_names, tmp_search_paths, tmp_paths_to_shared_libraries, t, host, partition);
            tagToReturn = t;
            break;
        }
        catch(final config.GenericException ex) {
            if(t.equals(tags.get(tags.size() - 1))) {
                throw ex;
            }
        }
    }

    final LinkedList<String> search_paths = new LinkedList<>();
    final LinkedList<String> paths_to_shared_libraries = new LinkedList<>();
    
    // Insert in front paths to shared libraries and search paths for the Uses relationship of the Application object (recursively)
    {
        // Make binary tag string (HW and SW)
        final StringBuilder binary_tag = new StringBuilder();
        binary_tag.append(tagToReturn.get_HW_Tag());
        binary_tag.append('-');
        binary_tag.append(tagToReturn.get_SW_Tag());
        
        // Check if the partition has RepositoryRoot (in this case copy first item from local copy of paths to libs)
        int idx = (partition.get_RepositoryRoot().isEmpty() == true) ? 0 : 1;
        int idx2 = idx * 2;
        if(idx == 1) {
            paths_to_shared_libraries.addFirst(tmp_paths_to_shared_libraries.get(0));
            search_paths.addFirst(tmp_search_paths.get(1));
            search_paths.addFirst(tmp_search_paths.get(0));
        }
        
        // Append paths to shared libraries from application's repositories
        final dal.SW_Package[] pkgs = ac.m_base_app.get_Uses();
        if(pkgs != null) {
            for(final dal.SW_Package i : pkgs) {
                dal.Algorithms.get_paths(i, search_paths, paths_to_shared_libraries, binary_tag.toString());
            }
        }
        
        // Copy rest of paths to shared libraries from application's repositories
        while(idx < tmp_paths_to_shared_libraries.size()) {
            if(paths_to_shared_libraries.contains(tmp_paths_to_shared_libraries.get(idx)) == false) {
                paths_to_shared_libraries.addLast(tmp_paths_to_shared_libraries.get(idx));
            }
            
            ++idx;
        }
        
        // Copy rest of search paths from application's repositories
        while(idx2 < tmp_search_paths.size()) {
            if(search_paths.contains(tmp_search_paths.get(idx2)) == false) {
                search_paths.addLast(tmp_search_paths.get(idx2));
            }
            
            ++idx2;
        }
    }
    
    // set application's process environment
    
    environment.clear();
    
    try {
        dal.Algorithms.add_front_partition_environment(environment, partition);
        
        // Application needs Environment
        final dal.Parameter[] bap = ac.m_base_app.get_ProcessEnvironment();
        if(bap != null) { 
            dal.Algorithms.add_env_vars(environment, Arrays.asList(bap), tagToReturn);
        }
        
        // Application's ComputerProgram needs Environment
        final dal.Parameter[] cpp = ac.m_base_app.get_Program().get_ProcessEnvironment();
        if(cpp != null) {
            dal.Algorithms.add_env_vars(environment, Arrays.asList(cpp), tagToReturn);
        }
        
        // Segment list NeedsEnvironment
        final Map<String, String> parent_var_names = new TreeMap<>();
        final Iterator<dal.Segment> it = s_list.descendingIterator();
        while(it.hasNext() == true) {
            final dal.Segment s = it.next();
            
            final dal.Parameter[] p = s.get_ProcessEnvironment();
            if(p != null) {
                dal.Algorithms.add_env_vars(environment, Arrays.asList(p), tagToReturn);
            }
            
            final dal.BaseApplication[] infr = s.get_infrastructure();
            if(infr != null) {
                for(final dal.BaseApplication j : infr) {                      
                    final dal.InfrastructureBase ia = dal.InfrastructureBase_Helper.cast(j);
                    final String swv_name = ia.get_SegmentProcEnvVarName();
                    if(swv_name.isEmpty() == false) {
                        try {
                            final String value =
                                (ia.get_SegmentProcEnvVarValue().equals(dal.InfrastructureBase.SegmentProcEnvVarValue.AppId.toString())) ? j.UID() :
                                (ia.get_SegmentProcEnvVarValue().equals(dal.InfrastructureBase.SegmentProcEnvVarValue.RunsOn.toString())) ? j.get_host().UID() :
                                get_host_and_backup_list(j);

                            dal.Algorithms.add_env_var2(environment, swv_name, value);
                            
                            // check if one is looking for parent with this name; add and mark if found
                            final String x = parent_var_names.get(swv_name);
                            if((x != null) && (x.isEmpty() == false)) {
                                dal.Algorithms.add_env_var2(environment, x, value);
                                parent_var_names.put(swv_name, "");
                            }
                            
                            // add to parent search list
                            final String swv_parent_name = ia.get_SegmentProcEnvVarParentName();
                            if(swv_parent_name.isEmpty() == false) {
                                final String xx = parent_var_names.get(swv_name);
                                if(xx == null) {
                                    parent_var_names.put(swv_name, swv_parent_name);
                                }
                            }
                        }
                        catch(final Exception ex) {
                            throw new config.GenericException("Failed to build environment for application " + a.UID(), ex);
                        }
                    }
                }
            }
        }
        
        dal.Algorithms.add_end_partition_environment(environment, partition, ac.m_base_app, ac.m_base_app.get_Program(), tagToReturn);
        
        // Set Application ID and name variables
        dal.Algorithms.add_env_var(environment, "TDAQ_APPLICATION_OBJECT_ID", ac.m_base_app.UID());
        dal.Algorithms.add_env_var(environment, "TDAQ_APPLICATION_NAME", a.UID());
    }
    catch(final Exception ex) {
        throw new config.GenericException("Failed to build environment for application " + a.UID(), ex);

    }
    
    // Add "PATH" and "LD_LIBRARY_PATH" variables
    dal.Algorithms.set_path(environment, "PATH", search_paths);
    dal.Algorithms.set_path(environment, "LD_LIBRARY_PATH", paths_to_shared_libraries);
    
    final String sa = ac.m_base_app.get_Program().get_DefaultParameters() + " " + ac.m_base_app.get_Parameters();
    final String rsa = ac.m_base_app.get_Program().get_DefaultParameters() + " " + ac.m_base_app.get_RestartParameters();
    
    startArgs.delete(0, startArgs.length());
    startArgs.append(dal.Algorithms.substitute_variables(sa, environment, "env(", ")"));
    
    restartArgs.delete(0, restartArgs.length());
    restartArgs.append(dal.Algorithms.substitute_variables(rsa, environment, "env(", ")"));
    
    return tagToReturn;
  }
  
  
}

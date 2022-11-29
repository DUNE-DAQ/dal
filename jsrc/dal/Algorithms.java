package dal;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import is.InfoNotFoundException;

import java.io.File;


// the class SW_Repository_Comparator is required to use TreeSet<dal.SW_Repository>,
// i.e. to define a comparator for two dal.SW_Repository objects

class SW_Repository_Comparator implements Comparator<dal.SW_Repository>
  {
    @Override
    public int compare(dal.SW_Repository a, dal.SW_Repository b)
      {
        return b.UID().compareTo(a.UID());
      }
  }

class SW_Package_Comparator implements Comparator<dal.SW_Package>
  {
    @Override
    public int compare(dal.SW_Package a, dal.SW_Package b)
      {
        return b.UID().compareTo(a.UID());
      }
  }
  
class BackupHostFactory
{
	private SegConfig m_seg;
	private int m_num_of_hosts;
	private int m_count;

	public BackupHostFactory(SegConfig seg) {
		m_seg = seg;
		m_num_of_hosts = seg.m_hosts.length;
		m_count = 0;
	}

	Computer get_next() {
		int idx = m_count++ % m_num_of_hosts;

		if (idx == 0) {
			m_count++;
			return m_seg.m_hosts[1];
		}

		return m_seg.m_hosts[idx];
	}

	int get_size() {
		return m_num_of_hosts;
	}
};

class BackupHostFactory2
{
	private dal.Computer[] m_hosts;
	private int m_num_of_hosts;
	private int m_count;

	public BackupHostFactory2(dal.Segment seg) throws config.GenericException, config.SystemException, config.NotFoundException, config.NotValidException {
		m_hosts = seg.get_hosts();
		m_num_of_hosts = m_hosts.length;
		m_count = 0;
	}

	Computer get_next() {
		int idx = m_count++ % m_num_of_hosts;

		if (idx == 0) {
			m_count++;
			return m_hosts[1];
		}

		return m_hosts[idx];
	}

	int get_size() {
		return m_num_of_hosts;
	}
};


public class Algorithms {
    private final static String s_share_bin_str = "share/bin";
    private final static String s_bin_str = "bin";
    private final static String s_lib_str = "lib";
    
    private final static String s_repository = System.getenv("TDAQ_DB_REPOSITORY");


    /**
     * Search and return partition object with ID equal to the 'pname' parameter,
     * or, if not set, the TDAQ_PARTITION environment variable
     * 
     * @param db    the database object
     * @param pname the partition name
     * @return the dal.Partition object
     */

    public static dal.Partition get_partition(config.Configuration db, String pname) throws config.GenericException, config.SystemException, config.NotFoundException {
        String name = new String(pname);

        if (name.length() == 0)
            try {
                name = System.getenv("TDAQ_PARTITION");
            } catch (final SecurityException ex) {
                throw new config.GenericException("System.getenv() failed: " + ex.getMessage());
            }

        if (name.length() == 0)
            throw new config.GenericException("No partition UID provided. What is the UID of the partition you are looking for?");

        return dal.Partition_Helper.get(db, name);
    }

    // ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Helper method to copy a path 'p' to list of paths 'out'.
	private static void add_path(LinkedList<dal.Component> p, LinkedList<dal.Component[]> out) {
		out.add(p.toArray(new dal.Component[0]));
	}

	private static void make_parents_list(config.DalObject child, dal.ResourceSet resource_set,
			LinkedList<dal.Component> p_list, LinkedList<dal.Component[]> out) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		// add the resource set to the path
		p_list.add(resource_set);

		// check if the application is in the resource relationship, i.e. is a resource
		// or belongs to resource set(s)
		for (dal.ResourceBase s : resource_set.get_Contains()) {
			if (s.config_object().implementation() == child.config_object().implementation()) {
				add_path(p_list, out);
			} else {
				dal.ResourceSet rs = dal.ResourceSet_Helper.cast(s);
				if (rs != null) {
					make_parents_list(child, rs, p_list, out);
				}
			}
		}

		// remove the resource set from the path
		p_list.removeLast();
	}

	private static void make_parents_list(config.DalObject child, dal.Segment segment, LinkedList<dal.Component> p_list,
			LinkedList<dal.Component[]> out, boolean is_segment) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		// add the segment to the path
		p_list.add(segment);

		// check if the application is in the nested segment
		for (dal.Segment s : segment.get_Segments()) {
			if (s.config_object().implementation() == child.config_object().implementation()) {
				add_path(p_list, out);
			} else {
				make_parents_list(child, s, p_list, out, is_segment);
			}
		}

		// check if the application is in the resource relationship, i.e. is a resource
		// or belongs to resource set(s)
		if (!is_segment) {
		  for (dal.ResourceBase s : segment.get_Resources()) {
			  if ((s.UID().compareTo(child.UID()) == 0)) {
			  	add_path(p_list, out);
			  } else {
				  dal.ResourceSet rs = dal.ResourceSet_Helper.cast(s);
			  	if (rs != null) {
				  	make_parents_list(child, rs, p_list, out);
				  }
			  }
		  }
		}

		// remove the segment from the path
		p_list.removeLast();
	}

	private static void check_segment(LinkedList<dal.Component[]> out, dal.Segment segment, config.DalObject child, boolean is_segment) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		LinkedList<dal.Component> s_list = new LinkedList<dal.Component>();

		if (segment.config_object().implementation() == child.config_object().implementation()) {
			add_path(s_list, out);
		}

		make_parents_list(child, segment, s_list, out, is_segment);
	}

	public static Component[][] get_parents(dal.Component obj, dal.Partition p)
			throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		LinkedList<dal.Component[]> parents = new LinkedList<dal.Component[]>();
		
		final boolean is_segment = (dal.Segment_Helper.cast(obj) != null);

		// check partition's segments
		for (dal.Segment s : p.get_Segments()) {
			check_segment(parents, s, obj, is_segment);
		}

		// check online-infrastructure segment
		dal.Segment s = p.get_OnlineInfrastructure();
		if (s != null) {
			check_segment(parents, s, obj, is_segment);

			for (dal.Application a : p.get_OnlineInfrastructureApplications()) {
				if (a.config_object().implementation() == obj.config_object().implementation()) {
					LinkedList<dal.Component> s_list = new LinkedList<dal.Component>();
					s_list.add(s);
					add_path(s_list, parents);
				}
			}
		}

        if (parents.size() > 0)
            return parents.toArray(new Component[0][]);
        else
            throw new config.GenericException("cannot find path(s) between " + p.UID() + '@' + p.class_name() + " and " + obj.UID() + '@' + obj.class_name());
	}


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static void add_computers(LinkedList<dal.Computer> v, dal.ComputerBase cb) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		dal.ComputerSet cs = dal.ComputerSet_Helper.cast(cb);
		if (cs != null) {
			for (dal.ComputerBase c : cs.get_Contains()) {
				add_computers(v, c);
			}
		} else {
			dal.Computer cp = dal.Computer_Helper.cast(cb);
			if (cp != null && cp.get_State() == true) {
				v.add(cp);
			}
		}
	}


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    public static void get_resource_applications(dal.ResourceBase obj, LinkedList<dal.BaseApplication> out, dal.Partition p)
        throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
      {
        if (p == null || obj.disabled(p) == false)
          {

            // test if the resource base can be casted to the application

            dal.BaseApplication a = dal.BaseApplication_Helper.cast(obj);
            if (a != null)
              {
                out.add(a);
              }

            // test if the resource base can contain nested resources

            dal.ResourceSet s = dal.ResourceSet_Helper.cast(obj);
            if (s != null)
              {
                for (dal.ResourceBase o : s.get_Contains())
                  {
                    get_resource_applications(o, out, p);
                  }
              }
          }
      }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    public static String get_value(dal.Variable var, dal.Tag tag) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
        if (tag == null) {
            if (var.get_TagValues().length > 0) {
                throw new config.GenericException("the algorithm was invoked on multi-value " + var.UID() + '@' + var.class_name() + " object without explicit Tag object");
            }
        } else {
            for (dal.TagMapping v : var.get_TagValues()) {
                if (tag.get_SW_Tag().equals(v.get_SW_Tag()) && tag.get_HW_Tag().equals(v.get_HW_Tag()))
                    return v.get_Value();
            }
        }

        return var.get_Value();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	private static void add_repositories(TreeSet<dal.SW_Repository> repositories, dal.SW_Package[] reps) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		for (dal.SW_Package o : reps) {
			dal.SW_Repository r = dal.SW_Repository_Helper.cast(o);
			if (r != null) {
				repositories.add(r);
			}
			add_repositories(repositories, o.get_Uses());
		}
	}

	private static void process_application(TreeSet<dal.SW_Repository> repositories, dal.BaseApplication a) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		if (a != null) {
			try {
				// add repositories used by application
				add_repositories(repositories, a.get_Uses());

				// add repositories linked with the application's program
				dal.ComputerProgram p = a.get_Program();

				if (p != null) {
					dal.SW_Repository r = p.get_BelongsTo();

					if (r != null) {
						repositories.add(r);
					}

					add_repositories(repositories, p.get_Uses());
				}
			} catch (RuntimeException ex) {
				ers.Logger.error(new ers.Issue("Cannot read information about application \'" + a.UID()
						+ "\':\n\twas caused by: " + ex.getMessage()));
			}
		}
	}

	private static void process_segment(TreeSet<dal.SW_Repository> repositories, dal.Segment s, dal.Partition p) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		// check segment's controller
		process_application(repositories, dal.BaseApplication_Helper.cast(s.get_IsControlledBy()));

		// check segment's applications, which are not resources
		for (int i = 0; i < s.get_Applications().length; i++)
			process_application(repositories, s.get_Applications()[i]);

		// add segment's resource applications
		for (dal.ResourceBase o : s.get_Resources()) {
			LinkedList<dal.BaseApplication> resources = new LinkedList<dal.BaseApplication>();
			get_resource_applications(o, resources, p);

			for (dal.BaseApplication j : resources)
				process_application(repositories, j);
		}

		// process applications from nested segments
		for (dal.Segment o : s.get_Segments())
			process_segment(repositories, o, p);
	}

    /**
     * The method creates vector of software repositories used by given
     * partition. A software repository is used, when there is an application
     * referencing it. Use skip_disabled parameter to chose between all-defined
     * and enabled applications. The example shown below prints all used sw
     * repositories including disabled:
     * 
     * <pre>
     * <code>
     *    dal.SW_Repository[] reps = dal.Algorithms.get_used_repositories(p, false);
     *    for(dal.SW_Repository j : reps) {
     *      System.out.println(j + " =&gt; " + j.get_InstallationPath());
     *    }
     *  </code>
     * </pre>
     * 
     * @param p              the partition object
     * @param skip_disabled  if true, skip disabled applications
     * @return               the vector of used dal.SW_Repository objects
     */

	public static dal.SW_Repository[] get_used_repositories(dal.Partition p, boolean skip_disabled) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		dal.OnlineSegment online_segment = p.get_OnlineInfrastructure();

		TreeSet<dal.SW_Repository> repositories = new TreeSet<dal.SW_Repository>(new SW_Repository_Comparator());

		if (online_segment != null) {
			process_segment(repositories, online_segment, (skip_disabled ? p : null));

			for (dal.Application a : p.get_OnlineInfrastructureApplications()) {
				process_application(repositories, a);
			}
		}

		for (dal.Segment i : p.get_Segments())
			process_segment(repositories, i, (skip_disabled ? p : null));

		return repositories.toArray(new dal.SW_Repository[0]);
	}

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    /**
     * Helper method for get_info() implementation.
     * 
     * @param program
     *          the computer program object (input parameter)
     * @param program_names
     *          output list of possible program files
     * @param search_paths
     *          output list of program search paths
     * @param paths_to_shared_libraries
     *          output list of paths to shared libraries
     * @param tag
     *          the tag for the computer program (input parameter)
     * @param host
     *          the host where to run the process (input parameter)
     * @param partition
     *          the partition where to run the process (input parameter)
     */

    static void get_parameters(final dal.ComputerProgram program, final List<String> program_names, final List<String> search_paths, final List<String> paths_to_shared_libraries,
            final dal.Tag tag, final dal.Computer host, final dal.Partition partition)
            throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
      {
          // Check ComputerProgram belong to SW_Package
          final SW_Repository belongs_to = program.get_BelongsTo();
          if (belongs_to == null) {
              throw new config.GenericException("Program " + program.UID() + " does not belong to any SW_Package");
          }

          if (dal.Algorithms.is_compatible(tag, host, partition) == false) {
              throw new config.GenericException("The tag " + tag.UID() + " is not applicable on host " + host.UID() + " with hw tag \"" + host.get_HW_Tag() + "\"");
          }

          // Check the BelongsTo repository (and its subtree) supports the tag
          dal.Algorithms.check_tag(belongs_to, tag);

          final SW_Package[] swps = program.get_Uses();
          if (swps != null) {
              for (final SW_Package swp : swps) {
                  dal.Algorithms.check_tag(swp, tag);
              }
          }
        
          // Find program name (either a script, a binary with no exact implementation or a binary with exact implementation)
          String program_name = "";

          if (program.class_name().equals("Script")) {
              // Script
              program_name = program.get_BinaryName();
              if (program_name.isEmpty() == true)
                  throw new config.GenericException("Program " + program.UID() + " has no BinaryName defined (name of script)");
          } else {
              final Binary binary_program = Binary_Helper.cast(program);
              if (binary_program == null) {
                  throw new config.GenericException("Program " + program.UID() + " is not a ComputerProgram");
              }

              final BinaryFile[] binFiles = binary_program.get_ExactImplementations();
              if ((binFiles == null) || (binFiles.length == 0)) {
                  // Binary with no exact implementation
                  program_name = program.get_BinaryName();
                  if (program_name.isEmpty()) {
                      throw new config.GenericException("Program " + program.UID() + " has no BinaryName defined (no exact implementation)");
                  }
              } else {
                  // Binary with exact implementation
                  for (final BinaryFile j : binFiles) {
                      // Check the Tag matches
                      if (j.get_Tag().equals(tag)) {
                          program_name = j.get_BinaryName();
                          break;
                      }
                  }

                  if (program_name.isEmpty()) {
                      throw new config.GenericException("Program " + program.UID() + " has no exact implementation for tag " + tag.UID());
                  }
              }
          }

          // Make binary tag string (HW and SW)
          StringBuilder binary_tag = new StringBuilder();
          binary_tag.append(tag.get_HW_Tag());
          binary_tag.append("-");
          binary_tag.append(tag.get_SW_Tag());

          // Create possible paths to computer program
          if (program_name.startsWith("/")) {
              // Fully qualified path and name
              program_names.add(program_name);
          } else {
              // Add paths to user-defined repository (Partition RepositoryRoot)
              final String repRoot = partition.get_RepositoryRoot();
              if (repRoot.isEmpty() == false) {
                  if (program.class_name().equals("Script")) {
                      program_names.add(dal.Algorithms.make_path(repRoot, dal.Algorithms.s_share_bin_str, program_name));
                  } else {
                      program_names.add(dal.Algorithms.make_path(repRoot, binary_tag.toString(), dal.Algorithms.s_bin_str, program_name));
                  }
              }

              // Add paths to PathArea if it is non-empty
              final String patchArea = belongs_to.get_PatchArea();
              if (patchArea.isEmpty() == false) {
                  if (program.class_name().equals("Script")) {
                      program_names.add(dal.Algorithms.make_path(patchArea, dal.Algorithms.s_share_bin_str, program_name));
                  } else {
                      program_names.add(dal.Algorithms.make_path(patchArea, binary_tag.toString(), dal.Algorithms.s_bin_str, program_name));
                  }
              }

              // Add paths to BelongsTo repository
              if (program.class_name().equals("Script")) {
                  program_names.add(dal.Algorithms.make_path(belongs_to.get_InstallationPath(), dal.Algorithms.s_share_bin_str, program_name));
              } else {
                  program_names.add(dal.Algorithms.make_path(belongs_to.get_InstallationPath(), binary_tag.toString(), dal.Algorithms.s_bin_str, program_name));
              }
          }

          // Add search paths and paths to shared libraries to user-defined repository if
          // it is used (i.e. Partition RepositoryRoot)
          // Rotate the vectors to make the Repository Root paths first
          final String repRoot = partition.get_RepositoryRoot();
          if (repRoot.isEmpty() == false) {
              search_paths.add(0, dal.Algorithms.make_path(repRoot, binary_tag.toString(), dal.Algorithms.s_bin_str));
              search_paths.add(0, dal.Algorithms.make_path(repRoot, dal.Algorithms.s_share_bin_str));
              paths_to_shared_libraries.add(0, dal.Algorithms.make_path(repRoot, binary_tag.toString(), dal.Algorithms.s_lib_str));
          }

          // Add search paths and paths to shared libraries to used repository
          // and any repositories which they use (recursively)
          final SW_Package[] swp = program.get_Uses();
          if (swp != null) {
              for (final SW_Package i : swp) {
                  dal.Algorithms.get_paths(i, search_paths, paths_to_shared_libraries, binary_tag.toString());
              }
          }

          // Add search paths and paths to shared libraries to repository the program
          // belongs to
          // and any repositories which it uses (recursively)
          dal.Algorithms.get_paths(belongs_to, search_paths, paths_to_shared_libraries, binary_tag.toString());
      }
    
    /**
     * Use generated DAL method dal.ComputerProgram.get_info() instead of this
     * implementation method.
     * 
     * @param program
     *          the computer program object (input parameter)
     * @param environment
     *          output process environment
     * @param program_names
     *          output list of program search paths
     * @param partition
     *          the partition where to run the process (input parameter)
     * @param tag
     *          the tag for the computer program (input parameter)
     * @param host
     *          the host where to run the process (input parameter)
     */

    static void get_info(final dal.ComputerProgram program, 
                         final Map<String, String> environment, 
                         final List<String> program_names,
                         final dal.Partition partition, 
                         final dal.Tag tag, 
                         final dal.Computer host) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
        List<String> search_paths = new ArrayList<>();
        List<String> paths_to_shared_libraries = new ArrayList<>();
        
        dal.Algorithms.get_parameters(program, program_names, search_paths, paths_to_shared_libraries, tag, host, partition);
        
        // Get the environment:
        //  - add environment defined by the partition's attributes (Name, IPCRef, DBPath, DBName, ...)
        //  - add environment defined for the computer program
        //  - add rest of environment defined for the partition
                
        try {
            dal.Algorithms.add_front_partition_environment(environment, partition);

            final dal.Parameter[] params = program.get_ProcessEnvironment();

            if(params != null)
                dal.Algorithms.add_env_vars(environment, Arrays.asList(params), null);

            dal.Algorithms.add_end_partition_environment(environment, partition, null, program, tag);
        }
        catch(config.SystemException | config.NotFoundException ex) {
            throw new config.GenericException("Failed to build environment for Program \"" + program.UID() + '\"', ex);
        }

        dal.Algorithms.set_path(environment, "PATH", search_paths);
        dal.Algorithms.set_path(environment, "LD_LIBRARY_PATH", paths_to_shared_libraries);        
    }

    //
    public static boolean is_compatible(final dal.Tag tag, final dal.Computer host, final dal.Partition partition) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
        if(tag.get_HW_Tag().equals(host.get_HW_Tag())) {
            return true;
        }
        
        final PlatformCompatibility[] info = partition.get_OnlineInfrastructure().get_CompatibilityInfo();
        if((info == null) || (info.length == 0)) {
            return false;
        }
        
        for(final PlatformCompatibility i : info) {
            if(host.get_HW_Tag().equals(i.get_HW_Tag())) {
                final PlatformCompatibility[] pc = i.get_CompatibleWith();
                if(pc != null) {
                    for(final PlatformCompatibility j : pc) {
                        if(tag.get_HW_Tag().equals(j.get_HW_Tag())) {
                            return true;
                        }
                    }
                }  
                
                return false;
            }
        }
        
        return false;
    }

    private static void check_tag(final dal.SW_Package pack, final dal.Tag tag)
            throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
        boolean found = false;

        SW_Repository repository = null;
        SW_ExternalPackage epkg = null;
        if ((repository = SW_Repository_Helper.cast(pack)) != null) {
            // Check through the tags to see if there is a match
            final Tag[] t = repository.get_Tags();
            if (t != null) {
                for (final Tag i : t) {
                    if (i.equals(tag)) {
                        found = true;
                        break;
                    }
                }
            }
        } else if ((epkg = SW_ExternalPackage_Helper.cast(pack)) != null) {
            // Check through the shared library tag mappings to see if there is a match
            {
                final TagMapping[] tms = epkg.get_SharedLibraries();
                if (tms != null) {
                    for (final TagMapping tm : tms) {
                        if (tm.get_HW_Tag().equals(tag.get_HW_Tag()) && tm.get_SW_Tag().equals(tag.get_SW_Tag())) {
                            found = true;
                            break;
                        }
                    }
                }
            }

            // Check through the binaries tag mappings to see if there is a match
            if (found == false) {
                final TagMapping[] tmss = epkg.get_Binaries();
                if (tmss != null) {
                    for (final TagMapping tmm : tmss) {
                        if (tmm.get_HW_Tag().equals(tag.get_HW_Tag()) && tmm.get_SW_Tag().equals(tag.get_SW_Tag())) {
                            found = true;
                            break;
                        }
                    }
                }
            }
        } else {
            throw new config.GenericException("Failed to cast " + pack.UID() + " to SW_Repository or SW_ExternalPackage class");
        }

        if (found == false) {
            throw new config.GenericException("The package " + pack.UID() + " does not support the tag " + tag.UID());
        }

        // Test tags of used packages
        final SW_Package[] pkgs = pack.get_Uses();
        if (pkgs != null) {
            for (final SW_Package p : pkgs) {
                dal.Algorithms.check_tag(p, tag);
            }
        }
    }
    
    private static String make_path(String... elements) {
        final StringBuilder p = new StringBuilder();
        for(final String s : elements) {
            p.append(s);
            p.append('/');
        }
        
        // Remove the trailing "/"
        final int len = p.length();
        if(len > 0) {
            p.deleteCharAt(len - 1);
        }
        
        return p.toString();
    }
    
    static void get_paths(final dal.SW_Package pack, 
                          final List<String> searchPaths, 
                          final List<String> pathsToSharedLibraries,
                          final String binary_tag) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
        final dal.SW_Repository repository = dal.SW_Repository_Helper.cast(pack);
        final dal.SW_ExternalPackage epkg = ((repository != null) ? null : dal.SW_ExternalPackage_Helper.cast(pack));
        
        // Use a set so that we do not have to check for duplicate paths manually every time
        final Set<String> search_paths_ = new LinkedHashSet<>(searchPaths);
        final Set<String> paths_to_shared_libraries_ = new LinkedHashSet<>(pathsToSharedLibraries);
        
        if(repository != null) {
            final String patchArea = repository.get_PatchArea();
            if(patchArea.isEmpty() == false) {
                search_paths_.add(dal.Algorithms.make_path(patchArea, dal.Algorithms.s_share_bin_str));
                search_paths_.add(dal.Algorithms.make_path(patchArea, binary_tag, dal.Algorithms.s_bin_str));
                paths_to_shared_libraries_.add(dal.Algorithms.make_path(patchArea, binary_tag, dal.Algorithms.s_lib_str));
            }
            
            final String instPath = repository.get_InstallationPath();
            if(instPath.isEmpty() == false) {
                search_paths_.add(dal.Algorithms.make_path(instPath, dal.Algorithms.s_share_bin_str));
                search_paths_.add(dal.Algorithms.make_path(instPath, binary_tag, dal.Algorithms.s_bin_str));
                paths_to_shared_libraries_.add(dal.Algorithms.make_path(instPath, binary_tag, dal.Algorithms.s_lib_str));
            }
        } else if(epkg != null) {
            final String patchArea = epkg.get_PatchArea();
            final String instPath = epkg.get_InstallationPath();
            
            {
                final TagMapping[] tm = epkg.get_Binaries();
                if(tm != null) {
                    for(final TagMapping i : tm) {
                        final StringBuilder ep_binary_tag = new StringBuilder();
                        ep_binary_tag.append(i.get_HW_Tag());
                        ep_binary_tag.append("-");
                        ep_binary_tag.append(i.get_SW_Tag());
                        
                        if(binary_tag.equals(ep_binary_tag.toString())) {
                            if(patchArea.isEmpty() == false) {
                                search_paths_.add(dal.Algorithms.make_path(patchArea, i.get_Value()));
                            }
                            
                            search_paths_.add(dal.Algorithms.make_path(instPath, i.get_Value()));
                            
                            break;
                        }
                    }
                }
            }
            
            {
                final TagMapping[] tm = epkg.get_SharedLibraries();
                if(tm != null) {
                    for(final TagMapping j : tm) {
                        final StringBuilder ep_binary_tag = new StringBuilder();
                        ep_binary_tag.append(j.get_HW_Tag());
                        ep_binary_tag.append("-");
                        ep_binary_tag.append(j.get_SW_Tag());
                        
                        if(binary_tag.equals(ep_binary_tag.toString())) {
                            if(patchArea.isEmpty() == false) {
                                paths_to_shared_libraries_.add(dal.Algorithms.make_path(patchArea, j.get_Value()));
                            }
                            
                            paths_to_shared_libraries_.add(dal.Algorithms.make_path(instPath, j.get_Value()));
                            
                            break;
                        }
                    }
                }
            }
        } else {
            throw new config.GenericException("Failed to cast " + pack.toString() + " to SW_Repository or SW_ExternalPackage class");
        }
        
        searchPaths.clear();
        searchPaths.addAll(search_paths_);
        
        pathsToSharedLibraries.clear();
        pathsToSharedLibraries.addAll(paths_to_shared_libraries_);
        
        // Loop over all Uses SW_Package and call get_paths on those
        final SW_Package[] pkgs = pack.get_Uses();
        if(pkgs != null) {
            for(final SW_Package p : pkgs) {
                // Call this function recursively
                dal.Algorithms.get_paths(p, searchPaths, pathsToSharedLibraries, binary_tag);
            }
        }
    }
    
    static void set_path(final Map<String, String> environment, final String var, final List<String> value) {
        final StringBuilder s = new StringBuilder();
        for(final String i : value) {
            s.append(i);
            s.append(":");
        }
        
        final int len = s.length();
        if(len > 0) {
            s.deleteCharAt(len - 1); 
        }
        
        final String old_value = environment.get(var);
        if(old_value != null) {            
            s.append(":");
            s.append(old_value);            
        }
        
        environment.put(var, s.toString());
    }
    
    static void add_end_partition_environment(final Map<String, String> environment,
                                              final dal.Partition partition,
                                              final dal.BaseApplication base_application,
                                              final dal.ComputerProgram computer_program,
                                              final dal.Tag tag) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
        if (s_repository != null && environment.containsKey("TDAQ_DB_VERSION") == false)
          dal.Algorithms.add_env_var(environment, "TDAQ_DB_VERSION", partition.get_DBVersion());

        // Partition needs Environment
        final dal.Parameter[] pEnv = partition.get_ProcessEnvironment();
        if(pEnv != null && pEnv.length > 0) {
            dal.Algorithms.add_env_vars(environment, Arrays.asList(partition.get_ProcessEnvironment()), tag);
        }
        
        // Add environment defined by the sw packages used by application (if != 0) and the computer program
        {
            final Set<dal.SW_Package> packages = new TreeSet<>(new SW_Package_Comparator());
            final List<dal.SW_Package> packages_ordered = new ArrayList<>();
            
            if(base_application != null) {
                final SW_Package[] swp = base_application.get_Uses();
                if(swp != null && swp.length > 0) {
                    dal.Algorithms.get_all_used_sw_packages(Arrays.asList(swp), packages, packages_ordered);
                }
            }
            
            if(computer_program != null) {
                dal.Algorithms.get_all_used_sw_packages(Arrays.asList(computer_program.get_BelongsTo()), packages, packages_ordered);
                final SW_Package[] swp = computer_program.get_Uses();
                if(swp != null && swp.length > 0) {
                    dal.Algorithms.get_all_used_sw_packages(Arrays.asList(swp), packages, packages_ordered);
                }
            }
            
            for(final SW_Package swp : packages_ordered) {
                final dal.Parameter[] pars = swp.get_ProcessEnvironment();
                if(pars != null && pars.length > 0) {
                    dal.Algorithms.add_env_vars(environment, Arrays.asList(pars), tag);
                }
                
                final dal.SW_Repository sr = dal.SW_Repository_Helper.cast(swp);
                if(sr != null) {
                    final String rn = sr.get_InstallationPathVariableName();
                    if(rn.isEmpty() == false) {
                        if(environment.containsKey(rn) == false) {
                            environment.put(rn, sr.get_InstallationPath());
                        } else {
                            // TODO write something
                        }
                    }
                }
            }
            
            // Loop in reverse order
            final ListIterator<dal.SW_Package> rit = packages_ordered.listIterator(packages_ordered.size());
            while(rit.hasPrevious() == true) {
                final dal.SW_Package j = rit.previous();
                final SW_PackageVariable[] swpv = j.get_AddProcessEnvironment();
                if(swpv != null) {
                    for(final dal.SW_PackageVariable x : swpv) {
                        final dal.SW_Package p = j;
                        final dal.SW_PackageVariable v = x;

                        dal.Algorithms.extend_env_var(environment, v, p, p.get_InstallationPath() + "/" + v.get_Suffix());

                        final dal.SW_Repository r = dal.SW_Repository_Helper.cast(p);
                        if(r != null) {
                            final String patchArea = r.get_PatchArea();
                            if(patchArea.isEmpty() == false) {
                                dal.Algorithms.extend_env_var(environment, v, p, patchArea + "/" + v.get_Suffix());
                            }
                        }
                    }
                }
            }
            
            // if the program is Java script, generate Java's CLASSPATH
            if(computer_program != null) {
                final dal.Script script = dal.Script_Helper.cast(computer_program);
                if(script != null) {
                    final String shell = script.get_Shell();
                    if(shell.compareToIgnoreCase("java") == 0) {
                        final StringBuilder class_path = new StringBuilder();
                        final String x = environment.get("CLASSPATH");
                        if(x != null) {
                            class_path.append(x);
                        }
                        
                        final String user_dir = partition.get_RepositoryRoot();
                        for(final dal.SW_Package j : packages_ordered) {
                            final dal.SW_Repository rep = dal.SW_Repository_Helper.cast(j);
                            if(rep != null) {
                                dal.Algorithms.add_classpath(rep, user_dir, class_path);
                            }
                        }
                        
                        environment.put("CLASSPATH", class_path.toString());
                    }
                }
            }
        }
        
        // Set TDAQ_DB and TDAQ_DB_NAME if necessary:
        //  - if not defined, set TDAQ_DB variable
        //  - check db technology
        //    set TDAQ_DB_NAME="RDB", if it is not set and the technology is rdbconfig
        
        String tdaq_db_var = "";
        {
            final String tmp = environment.get("TDAQ_DB");
            if(tmp != null) {
                tdaq_db_var = tmp;
            }
        }
        
        if(partition.get_DBTechnology().equals("rdbconfig")) {
            final String tmp = environment.get("TDAQ_DB_NAME");
            if(tmp != null) {
                if(tdaq_db_var.isEmpty() == true) {
                    dal.Algorithms.add_env_var(environment, "TDAQ_DB", "rdbconfig:" + tmp);
                }
            } else {
                dal.Algorithms.add_env_var(environment, "TDAQ_DB_NAME", "RDB");
                if(tdaq_db_var.isEmpty() == true) {
                    dal.Algorithms.add_env_var(environment, "TDAQ_DB", "rdbconfig:RDB");
                }
            }
        } else {
            if(tdaq_db_var.isEmpty() == true) {
                dal.Algorithms.add_env_var(environment, "TDAQ_DB", "oksconfig:" + partition.get_DBName());
                environment.remove("TDAQ_DB_NAME");
            }
        }
    }
    
    static void add_front_partition_environment(final Map<String, String> environment, final dal.Partition partition)
            throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
      {
        if (s_repository != null) {
            final String s_user_repository = System.getenv("TDAQ_DB_USER_REPOSITORY");

            if (s_user_repository != null && s_user_repository.isEmpty() == false) {
                dal.Algorithms.add_env_var(environment, "TDAQ_DB_PATH", s_user_repository);
            }
            else {
                final String s_mapping_dir = System.getenv("OKS_REPOSITORY_MAPPING_DIR");

                dal.Algorithms.add_env_var(environment, "TDAQ_DB_REPOSITORY", s_repository);
                dal.Algorithms.add_env_var(environment, "TDAQ_DB_PATH", "");

                if (s_mapping_dir != null)
                    dal.Algorithms.add_env_var(environment, "OKS_REPOSITORY_MAPPING_DIR", s_mapping_dir);
            }
        }

        dal.Algorithms.add_env_var(environment, "TDAQ_PARTITION", partition.UID());

        final String IPCRef = partition.get_IPCRef();

        if (IPCRef.isEmpty() == false)
            dal.Algorithms.add_env_var(environment, "TDAQ_IPC_INIT_REF", IPCRef);

        if (s_repository == null) {
            final String DBPath = partition.get_DBPath();

            if (DBPath.isEmpty() == false)
                dal.Algorithms.add_env_var(environment, "TDAQ_DB_PATH", DBPath);
        }

        final String DBName = partition.get_DBName();

        if (DBName.isEmpty() == false)
            dal.Algorithms.add_env_var(environment, "TDAQ_DB_DATA", DBName);
    }
   
    static void add_env_var(final Map<String, String> dict, final String name, final String value) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
        final String s = dal.Algorithms.substitute_variables(value, "$(", ")");
        if(s.length() < 2 || s.charAt(0) != '$' || s.charAt(1) != '(') {
            dict.put(name, s);
        }
    }
    
    static String substitute_variables(final String str_from, final Map<String, String> cvs_map, final String beg, final String end) throws config.GenericException {
        return substitute_variables_impl(str_from, cvs_map, beg, end, false);
    }
    
    static String substitute_variables(final String str_from, final String beg, final String end) throws config.GenericException {
        return substitute_variables_impl(str_from, System.getenv(), beg, end, true);
    }
    
    private static String substitute_variables_impl(final String str_from, final Map<String, String> cvs_map, final String beg, final String end, boolean throwIfNotFound)
            throws config.GenericException {
        final StringBuffer s = new StringBuffer();

        final int max_sub = 128;
        int count = 0;

        try {
            final String begToken = Pattern.quote(beg);
            final String endToken = Pattern.quote(end);

            final Pattern pattern = Pattern.compile(begToken + "(.+?)" + endToken);
            final Matcher matcher = pattern.matcher(str_from);

            while (matcher.find() == true) {
                final String token = matcher.group(1);
                final String replacement = cvs_map.get(token);
                if (replacement != null) {
                    matcher.appendReplacement(s, Matcher.quoteReplacement(replacement));
                } else if (throwIfNotFound) {
                    throw new config.GenericException("Substitution failed for parameter " + token);
                }

                if (++count > max_sub) {
                    matcher.appendTail(s);

                    throw new config.GenericException("Value \"" + str_from + "\" has exceeded the maximum number of substitutions allowed (" + max_sub + "). "
                            + "It might have a circular dependency with substitution variables. After " + max_sub + " substitutions it is \"" + s.toString() + "\"");
                }
            }

            matcher.appendTail(s);
        } catch (IllegalStateException | IllegalArgumentException | IndexOutOfBoundsException ex) {
            throw new config.GenericException("Failed to substiture parameters for value \"" + str_from + "\": " + ex.toString());
        }

        return s.toString();
    }
    
    static void add_env_vars(final Map<String, String> dict, final List<? extends dal.Parameter> envs, final dal.Tag tag) 
    		throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
        for(final dal.Parameter i : envs) {
            final dal.Variable var = dal.Variable_Helper.cast(i);
            if(var != null) {
                dal.Algorithms.add_env_var(dict, var, tag);
            } else {
                final dal.VariableSet vs = dal.VariableSet_Helper.cast(i);
                final dal.Parameter[] vars = vs.get_Contains();
                if(vars != null) {
                    dal.Algorithms.add_env_vars(dict, Arrays.asList(vars), tag);
                }
            }
        }
    }
    
    static void add_env_var2(final Map<String, String> dict, final String name, final String value) {
        final String old = dict.get(name);
        if(old == null) {
            dict.put(name, value);
        }
    }
    
    private static void add_env_var(final Map<String, String> dict, final dal.Variable var, final dal.Tag tag) 
    		throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
        if(dict.containsKey(var.get_Name()) == false) {
            dict.put(var.get_Name(), var.get_value(tag));
        }

    }
    
    private static void extend_env_var(final Map<String, String> environment,
                                       final dal.SW_PackageVariable var,
                                       final dal.SW_Package pack,
                                       final String value) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
        final String res = environment.get(var.get_Name());
        if(res == null) {
            environment.put(var.get_Name(), value);
        } else {
            environment.put(var.get_Name(), value + ":" + res);
        }
    }
    
    private static void get_all_used_sw_packages(final List<? extends dal.SW_Package> in,
                                                 final Set<dal.SW_Package> out,
                                                 final List<dal.SW_Package> out_ordered) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
        for(final dal.SW_Package i : in) {
            final boolean added = out.add(i);
            if(added == true) {
                out_ordered.add(i);
                final dal.SW_Package[] swpkgs = i.get_Uses();
                if(swpkgs != null && swpkgs.length > 0) {
                    dal.Algorithms.get_all_used_sw_packages(Arrays.asList(swpkgs), out, out_ordered);
                }
            }
        }
    }
        
    private static void add_classpath(final dal.SW_Repository rep, final String user_dir, final StringBuilder class_path) 
    		throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
    {
        final dal.SW_Object[] so = rep.get_SW_Objects();
        if(so != null) {
            for(final dal.SW_Object j : so) {
                final dal.JarFile jf = dal.JarFile_Helper.cast(j);
                if(jf != null) {
                    final StringBuilder file = new StringBuilder();
                    final String bn = jf.get_BinaryName();
                                        
                    // check file name is an absolute path
                    if(bn.charAt(0) == '/') {
                        dal.Algorithms.check_file_exists(bn, file);
                    }
                    
                    // check user dir (Repository Root)
                    if((file.length() == 0) && (user_dir.isEmpty() == false)) {
                        dal.Algorithms.check_file_exists(user_dir + "/share/lib/" + bn, file);
                    }
                    
                    // check patch area
                    if(file.length() == 0) {
                        final String patchArea = rep.get_PatchArea();
                        if(patchArea.isEmpty() == false) {
                            dal.Algorithms.check_file_exists(patchArea + "/share/lib/" + bn, file);
                        }
                    }
                    
                    if(file.length() == 0) {
                        dal.Algorithms.check_file_exists(rep.get_InstallationPath() + "/share/lib/" + bn, file);
                    }
                    
                    if(file.length() == 0) {
                        ers.Logger.error( new ers.Issue("Cannot find jar file \"" + bn + "\" described by \"" + j.UID() + "@" + j.class_name() + 
                                "\" that is part of \"" + rep.UID() + "@" + rep.class_name() + "\"") );
                    } 
					else {
						if (class_path.length() != 0) {
							class_path.append(':');
						}
						class_path.append(file.toString());
					}
                }
            }
        }
    }

    private static void check_file_exists(final String path, final StringBuilder file) {
        file.delete(0, file.length());
                
        final File f = new File(path);
        if(f.canRead() == true) {
            file.append(path);
        }
    }
    
    final static String cv_info_name = "RunParams.ConfigVersion";
    
    public static void set_config_version(final String partition_name, final String version, final boolean reload)
            throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
        ipc.Partition partition = new ipc.Partition(partition_name);

        try {
            final dal.ConfigVersionNamed cv = new dal.ConfigVersionNamed(partition, cv_info_name);

            if (version.isEmpty()) {
                cv.remove();
            } else {
                cv.Version = version;
                cv.checkin();
            }
        } catch (final is.RepositoryNotFoundException | is.InfoNotCompatibleException | InfoNotFoundException ex) {
            throw new config.SystemException("cannot " + (version.isEmpty() ? "remove" : "check-in") + " IS data " + cv_info_name + " in IPC partition " + partition_name, ex);
        }

        if (reload) {
            final String[] files = { version };

            final boolean is_initial = (partition_name == null || partition_name.equals(ipc.partition.default_name));

            String rdb_server_name = (is_initial ? "RDB_INITIAL" : "RDB");

            try {
                partition.lookup(rdb.cursor.class, rdb_server_name).reload_database(files);

                if (!is_initial) {
                    rdb_server_name = "RDB_RW";
                    partition.lookup(rdb.writer.class, rdb_server_name).reload_database(files);
                }
            } catch (final ipc.InvalidObjectException ex) {
                throw new config.NotFoundException("database reloading failed because the RDB server " + rdb_server_name + " cannot be found in IPC partition " + partition_name, ex);
            } catch (final ipc.InvalidPartitionException ex) {
                throw new config.NotFoundException("database reloading failed because the IPC partition " + partition_name + " cannot be reached", ex);
            } catch (final rdb.CannotProceed ex) {
                throw new config.GenericException("database reloading failed for RDB server " + rdb_server_name + " in IPC partition " + partition_name + ": " + ex.text, ex);
            }
        }
    }

    public static String get_config_version(final String partition_name) throws config.GenericException, config.SystemException, config.NotFoundException {
    	ipc.Partition partition = new ipc.Partition(partition_name);
    	
        try {
            final dal.ConfigVersionNamed cv = new dal.ConfigVersionNamed(partition, cv_info_name);
            cv.checkout();
            
            if (!cv.Version.isEmpty())
            	return cv.Version;
        }
        catch(final is.RepositoryNotFoundException | is.InfoNotFoundException ex) {
        	throw new config.NotFoundException("cannot find IS data " + cv_info_name + " in IPC partition " + partition_name, ex);
        }
        catch(final is.InfoNotCompatibleException ex) {
        	throw new config.GenericException("cannot checkout IS data " + cv_info_name + " in IPC partition " + partition_name, ex);
        }

		try {
			String env = System.getenv("TDAQ_DB_VERSION");
			return (env != null ? env : "");

		} catch (SecurityException e) {
			throw new config.SystemException("System.getenv(\"TDAQ_DB_VERSION\") failed with SecurityException: " + e.getMessage());
		}

    }
  }


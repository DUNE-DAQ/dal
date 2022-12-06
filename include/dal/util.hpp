#ifndef _dal_util_H_
#define _dal_util_H_

#include <exception>

#include "config/Configuration.hpp"
#include "config/DalObject.hpp"


namespace daq {

namespace core {

    // forward declaration

  class BaseApplication;
  class Partition;
  class SW_Repository;
  class Tag;
  class Computer;


   /**
     *  \brief  Check if given tag can be used on given computer.
     *  
     *  The algorithm reads platforms compatibility description from partition's OnlineSegment.
     *  This allows to describe a host with real hw platform and installed operating system,
     *  and to run on it applications with compatible tags, e.g.:
     *  host with 64 bits SLC5 allows to run applications with x86_64-slc5, i686-slc5 and i686-slc4 tags.
     *
     *  \par Parameters and return value
     *
     *  \param tag             tested tag
     *  \param host            host where the tag is tested
     *  \param partition       partition defining online segment with compatibility info
     *  \return                true, if tag is compatible
     */

  bool is_compatible(const daq::core::Tag& tag, const daq::core::Computer& host, const daq::core::Partition& partition);



    /**
     *  \brief Substitute variables from conversion map or from process environment.
     *
     *  Substitute variables using special syntax like ${FOO}, $(BAR), etc.
     *  If substitution %is defined, then the variables are replaced by the corresponding
     *  substitution value. The substitution values are defined either by the
     *  substitution map, or by the process environment.
     *
     *  \par Parameters and return value
     *
     *  \param value            string containing variables to be substituted
     *  \param conversion_map   pointer to conversion map; if null, the process environment %is used
     *  \param beginning        definition of syntax symbols which delimit the beginning of the variable
     *  \param ending           definition of syntax symbols which delimit the ending of the variable
     *  \return                 Returns the result of substitution.
     *
     *  \par Example
     *
     *  If the conversion map has pair "FOO","BAR", then substitute_variables("/home/${FOO}", cmap, "${", "}") returns "/home/BAR".
     *  Otherwise it returns non-changed value "/home/${FOO}".
     *  <BR>
     *  If there %is environment variable "USER" with value "Online", then substitute_variables("/home/$(USER)", 0, "$()") returns "/home/Online".
     *  Otherwise it returns non-changed value "/home/$(USER)".
     */

  std::string substitute_variables(const std::string& value, const std::map<std::string, std::string> * conversion_map, const std::string& beginning, const std::string& end);

    /**
     *  \brief Implements string converter for database parameters.
     *
     *  The class implements ::Configuration::AttributeConverter for string %type.
     *  It reads parameters defined for given partition object and uses them to
     *  substitute values of database string attributes.
     *
     *  The parameters are stored as a map of substitution keys and values.
     *  If database %is changed, the reset(::Configuration&, const Partition&) method needs to be used.
     *
     *  \par Example
     *
     *  The example shows how to use the converter:
     *
     *  <pre><i>
     *
     *  ::Configuration db(...);  // some code to build configuration database object
     *
     *  const daq::core::Partition * partition = daq::core::get_partition(db, partition_name);
     *  if(partition) {
     *    db.register_converter(new daq::core::SubstituteVariables(db, *partition));
     *  }
     *
     *  </i></pre>
     *
     */

  class SubstituteVariables : public ::Configuration::AttributeConverter<std::string> {

    public:


        /** Build converter object. **/

      SubstituteVariables(const Partition& p)
      {
        reset (p);
      }


        /** Method to reset substitution map in case of database changes. **/

      void reset(const Partition&);


        /** Implementation of convert method. **/

      virtual void convert(std::string& value, const Configuration& conf, const ConfigObject& obj, const std::string& attr_name);


        /** Destroy conversion map. **/

      virtual ~SubstituteVariables() {;}


        /** Return conversion map **/

      const std::map<std::string, std::string> * get_conversion_map() const {return &m_cvt_map;}


    private:

      std::map<std::string, std::string> m_cvt_map;
  };


    /**
     *  \brief Get partition object.
     *
     *  The algorithm %is searching the partition object by given name.
     *  If the name %is empty, then the algorithm takes the name from
     *  the TDAQ_PARTITION environment variable.<BR>
     *
     *  The last parameter of the algorithm can be used to optimise performance
     *  of the DAL in case if a database server config implementation %is used.
     *  The parameter defines how many layers of objects referenced by given 
     *  partition object should be read into client's config cache together with
     *  partition object during single network operation. For example:
     *  - if the parameter %is 0, then only partition object %is read;
     *  - if the parameter %is 1, then partition and first layer segment objects are read;
     *  - if the parameter %is 2, then partition, segments of first and second layers, and application/resources of first layer segments objects are read;
     *  - if the parameter %is 10, then mostly probable all objects referenced by given partition object are read.<BR>
     *
     *  The parameters of the algorithm are:
     *  \param conf      the configuration object with loaded database
     *  \param name      the name of the partition to be loaded (if empty, TDAQ_PARTITION variable %is used)
     *  \param rlevel    optional parameter to optimise performance ("the references level")
     *  \param rclasses  optional parameter to optimise performance ("names of classes which objects are cached")
     *
     *  \return Returns the pointer to the partition object if found, or 0.
     */

  const daq::core::Partition * get_partition(::Configuration& conf, const std::string& name, unsigned long rlevel = 10, const std::vector<std::string> * rclasses = nullptr);


    /**
     *  \brief Get used software repositories.
     *
     *  The algorithm %is searching the sw repositories used by given partition,
     *  checking all active segments and applications.
     *
     *  The method throws daq::core::AlgorithmError exception in case of logical problems found in database
     *  (such as circular dependencies between segments, resources or repositories).
     *
     *  The parameters of the algorithm are:
     *  \param p the partition object
     *
     *  \return The used repositories
     */

  std::set<const daq::core::SW_Repository *> get_used_repositories(const daq::core::Partition& p);


    /**
     *  \brief Add into CLASSPATH JARs defined by JarFile objects.
     *
     *  The function iterates all SW objects of given repository and tests found JarFile objects.
     *  For each JarFile it checks if corresponding JAR file exists in repository root, patch or installation areas.
     *  First readable jar file is added to the class path.
     *
     *  @param[in] rep               the repository with JarFile objects
     *  @param[in] repository_root   the partition's repository root
     *  @param[in,out] class_path    the value of class path
     *
     *  @throw daq::core::NoJarFile is thrown when the jar file is not found or not readable.
     */

  void add_classpath(const daq::core::SW_Repository& rep, const std::string& repository_root, std::string& class_path);


    /**
     * \brief Get OKS GIT version for running partition.
     *
     * The method extracts GIT version for running partition configuration reading value from the RunParams information server.
     * If not set, it tries to extract the value from the TDAQ_DB_VERSION process environment.
     *
     * \param partition the name of the partition
     * \return the configuration version
     *
     *  @throw daq::config::NotFound is thrown if case if partition or information repository does not exist
     *  @throw daq::config::Exception is thrown if case of problems
     */

  std::string
  get_config_version(const std::string& partition);


    /**
     * \brief Set new OKS GIT version for running partition.
     *
     * The method writes version on the RunParams information server and reloads with this version RDB and RDB_RW servers.
     * In initial partition only RDB_INITIAL server is reloaded.
     *
     * \param partition the name of the partition
     * \param version the configuration version
     * \param reload if true, send reload command to RDB and RDB_RW servers
     *
     *  @throw daq::config::NotFound is thrown if case if partition or information repository does not exist
     *  @throw daq::config::Exception is thrown if case of problems
     */

  void
  set_config_version(const std::string& partition, const std::string& version, bool reload);

} /* close namespace core */


  ERS_DECLARE_ISSUE(
    core,
    AlgorithmError,
    ,
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    BadVariableUsage,
    AlgorithmError,
    message,
    ,
    ((std::string)message)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    BadApplicationInfo,
    AlgorithmError,
    "Failed to retrieve information for Application \'" << app_id << "\' from the database: " << message,
    ,
    ((std::string)app_id)
    ((std::string)message)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    BadPartitionID,
    AlgorithmError,
    "There is no partition object with UID = \"" << name << '\"',
    ,
    ((std::string)name)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    SegmentDisabled,
    AlgorithmError,
    "Cannot get information about applications because the segment is disabled",
    ,
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    BadProgramInfo,
    AlgorithmError,
    "Failed to retrieve information for Program \'" << prog_id << "\' from the database: " << message,
    ,
    ((std::string)prog_id)
    ((std::string)message)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    BadHost,
    AlgorithmError,
    "Failed to retrieve application \'" << app_id << "\' from the database: " << message,
    ,
    ((std::string)app_id)
    ((std::string)message)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    NoDefaultHost,
    AlgorithmError,
    "Failed to find default host for segment \'" << seg_id << "\' " << message,
    ,
    ((std::string)seg_id)
    ((std::string)message)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    NoTemplateAppHost,
    AlgorithmError,
    "Both partition default and segment default hosts are not defined for template application \'" << app_id << "\' from segment \'" << seg_id << "\' (will use localhost, that may cause problems presenting info in IGUI for distributed partition).",
    ,
    ((std::string)app_id)
    ((std::string)seg_id)
  )


  ERS_DECLARE_ISSUE_BASE(
    core,
    BadTag,
    AlgorithmError,
    "Failed to use tag \'" << tag_id << "\' because: " << message,
    ,
    ((std::string)tag_id)
    ((std::string)message)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    BadSegment,
    AlgorithmError,
    "Invalid Segment \'" << seg_id << "\' because: " << message,
    ,
    ((std::string)seg_id)
    ((std::string)message)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    GetTemplateApplicationsOfSegmentError,
    AlgorithmError,
    "Failed to get template applications of \'" << name << "\' segment" << message,
    ,
    ((std::string)name)
    ((std::string)message)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    BadTemplateSegmentDescription,
    AlgorithmError,
    "Bad configuration description of template segment \'" << name << "\': " << message,
    ,
    ((std::string)name)
    ((std::string)message)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    CannotGetApplicationObject,
    AlgorithmError,
    "Failed to get application object from name: " << reason,
    ,
    ((std::string)reason)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    CannotFindSegmentByName,
    AlgorithmError,
    "Failed to find segment object \'" << name << "\': " << reason,
    ,
    ((std::string)name)
    ((std::string)reason)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    NotInitedObject,
    AlgorithmError,
    "The " << item << " object " << obj << " was not initialized",
    ,
    ((const char *)item)
    ((void *)obj)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    NotInitedByDalAlgorithm,
    AlgorithmError,
    "The " << obj_id << '@' << obj_class << " object " << address << " was not initialized by DAL algorithm " << algo,
    ,
    ((std::string)obj_id)
    ((std::string)obj_class)
    ((void*)address)
    ((const char *)algo)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    CannotCreateSegConfig,
    AlgorithmError,
    "Failed to create config for segment \'" << name << "\': " << reason,
    ,
    ((std::string)name)
    ((std::string)reason)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    CannotGetParents,
    AlgorithmError,
    "Failed to get parents of \'" << object << '\'',
    ,
    ((std::string)object)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    FoundCircularDependency,
    AlgorithmError,
    "Reach maximum allowed recursion (" << limit << ") during calculation of " << goal << "; possibly there is circular dependency between these objects: " << objects,
    ,
    ((unsigned int)limit)
    ((const char *)goal)
    ((std::string)objects)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    NoJarFile,
    AlgorithmError,
    "Cannot find jar file \'" << file << "\' described by \'" << obj_id << '@' << obj_class << "\' that is part of \'" << rep_id << '@' << rep_class << '\'',
    ,
    ((std::string)file)
    ((std::string)obj_id)
    ((std::string)obj_class)
    ((std::string)rep_id)
    ((std::string)rep_class)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    DuplicatedApplicationID,
    AlgorithmError,
    "Two applications have equal IDs:\n  1) " << first << "\n  2) " << second,
    ,
    ((std::string)first)
    ((std::string)second)
  )

  ERS_DECLARE_ISSUE_BASE(
    core,
    SegmentIncludedMultipleTimes,
    AlgorithmError,
    "The segment \"" << segment << "\" is included by:\n  1) " << first << "\n  2) " << second,
    ,
    ((std::string)segment)
    ((std::string)first)
    ((std::string)second)
  )

} /* close namespace daq */

#endif

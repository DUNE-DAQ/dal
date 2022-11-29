//
//  FILE: dal/src/algorithms.cpp
//
//  Contains implementations of algorithms for generated DAL classes.
//
//
//  Implementation:
//	<Igor.Soloviev@cern.ch> - May 2003
//

#include <strings.h>
#include <sys/stat.h>

#include <list>
#include <set>
#include <iostream>
#include <sstream>
#include <algorithm>

#include <ers/ers.h>
#include <ipc/core.h>
#include <ipc/partition.h>
#include <is/infodictionary.h>
#include <rdb/rdb.hh>
#include <rdb/errors.h>
#include <system/Host.h>

#include <boost/spirit/include/karma.hpp>

#include <config/ConfigObject.h>
#include <config/ConfigAction.h>
#include <config/Configuration.h>
#include <config/map.h>

#include "dal/util.h"

#include "dal/BinaryFile.h"
#include "dal/Binary.h"
#include "dal/Computer.h"
#include "dal/ComputerSet.h"
#include "dal/InfrastructureApplication.h"
#include "dal/InfrastructureTemplateApplication.h"
#include "dal/JarFile.h"
#include "dal/OnlineSegment.h"
#include "dal/Partition.h"
#include "dal/PlatformCompatibility.h"
#include "dal/Rack.h"
#include "dal/Resource.h"
#include "dal/ResourceSetAND.h"
#include "dal/ResourceSetOR.h"
#include "dal/RunControlTemplateApplication.h"
#include "dal/RunControlApplication.h"
#include "dal/Segment.h"
#include "dal/Script.h"
#include "dal/SW_ExternalPackage.h"
#include "dal/SW_PackageVariable.h"
#include "dal/SW_Repository.h"
#include "dal/Tag.h"
#include "dal/TagMapping.h"
#include "dal/TemplateApplication.h"
#include "dal/TemplateSegment.h"
#include "dal/Variable.h"
#include "dal/VariableSet.h"

#include "dal/ConfigVersion.h"

#include "test_circular_dependency.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // the strings below are used in various make_path algorithms

const std::string s_share_bin_str("share/bin");
const std::string s_share_lib("share/lib");
const std::string s_bin_str("bin");
const std::string s_lib_str("lib");

const std::string s_rdbconfig_str("rdbconfig");
const std::string s_rdbconfig_colon_str("rdbconfig:");

const std::string s_classpath_str("CLASSPATH");
const std::string s_tdaq_db_str("TDAQ_DB");
const std::string s_tdaq_db_name_str("TDAQ_DB_NAME");
const std::string s_tdaq_db_path_str("TDAQ_DB_PATH");
const std::string s_tdaq_db_data_str("TDAQ_DB_DATA");
const std::string s_tdaq_db_version_str("TDAQ_DB_VERSION");
const std::string s_tdaq_db_repository_str("TDAQ_DB_REPOSITORY");
const std::string s_tdaq_db_user_repository_str("TDAQ_DB_USER_REPOSITORY");
const std::string s_oks_repository_mapping_dir_str("OKS_REPOSITORY_MAPPING_DIR");
const std::string s_tdaq_partition_str("TDAQ_PARTITION");
const std::string s_tdaq_ipc_init_ref_str("TDAQ_IPC_INIT_REF");

const std::string s_tdaq_application_object_id_str("TDAQ_APPLICATION_OBJECT_ID");
const std::string s_tdaq_application_name_str("TDAQ_APPLICATION_NAME");

const std::string s_path_str("PATH");
const std::string s_ld_library_path_str("LD_LIBRARY_PATH");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const int search_path_default_size(64);
const int paths_to_shared_libraries_default_size(32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef std::map<std::string, std::string> Emap;
typedef std::vector<const daq::core::Parameter *> EnvironmentVars;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // spirit::karma::generate is faster than sprintf() or std::ostringstream

inline void append2str(std::string& s, unsigned short i)
{
  char buf[8];
  char * ptr(buf);
  boost::spirit::karma::generate(ptr, boost::spirit::ushort_, i);
  s.append(buf, ptr - buf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // The functions to recursively find first enabled host

static const daq::core::Computer *
find_enabled(const std::vector<const daq::core::ComputerBase *>& hosts);

static const daq::core::Computer *
find_enabled(const daq::core::ComputerBase * cb)
{
  if (const daq::core::Computer * c = cb->cast<daq::core::Computer>())
    {
      if (c->get_State())
        return c;
    }
  else if (const daq::core::ComputerSet * cs = cb->cast<daq::core::ComputerSet>())
    {
      for (const auto & i : cs->get_Contains())
        {
          if (const daq::core::Computer * c = find_enabled(i))
            return c;
        }
    }

  return nullptr;
}

static const daq::core::Computer *
find_enabled(const std::vector<const daq::core::ComputerBase *>& hosts)
{
  for (const auto & i : hosts)
    if(const daq::core::Computer * c = find_enabled(i))
      return c;

  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // The functions to recursively build vector of computers from computer base object(s)

static void
add_computers(std::vector<const daq::core::Computer *>& v, const daq::core::ComputerBase * cb)
{
  if (const daq::core::Computer * cp = cb->cast<daq::core::Computer>())
    {
      v.push_back(cp);
    }
  else if (const daq::core::ComputerSet * cs = cb->cast<daq::core::ComputerSet>())
    {
      for (const auto & i : cs->get_Contains())
        add_computers(v, i);
    }
}

static void
add_computers(std::vector<const daq::core::Computer *>& v, const std::vector<const daq::core::ComputerBase *>& hosts)
{
  for (const auto & i : hosts)
    add_computers(v, i);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string&
daq::core::Variable::get_value(const Tag * tag) const
{
  if (tag == nullptr)
    {
      if (get_TagValues().size())
        {
          std::ostringstream text;
          text << "the algorithm was invoked on multi-value " << this << " object (file \'" << p_obj.contained_in() << "\') without explicit Tag object";
          throw daq::core::BadVariableUsage(ERS_HERE, text.str());
        }
    }
  else
    {
      for (const auto& i : get_TagValues())
        if (i->get_HW_Tag() == tag->get_HW_Tag() && i->get_SW_Tag() == tag->get_SW_Tag())
          return i->get_Value();
    }

  return get_Value();
}


  /**
   *  Static function to add single variable to the map.
   *  The variable is added only if it is not in the map.
   */

static void
add_env_var(Emap& dict, const daq::core::Variable * var, const daq::core::Tag * tag)
{
  dict.emplace(var->get_Name(), var->get_value(tag));
}


  /**
   *  Static function to add a parameter (variable or set) to the map.
   */

static void
add_env_vars(Emap& dict, const EnvironmentVars& envs, const daq::core::Tag * tag)
{
  for (const auto & i : envs)
    if (const daq::core::Variable * var = i->cast<daq::core::Variable>())
      add_env_var(dict, var, tag);
    else if (const daq::core::VariableSet * vars = i->cast<daq::core::VariableSet>())
      add_env_vars(dict, vars->get_Contains(), tag);
}


  /**
   *  Static function to add special variable to the map.
   */

static void
add_env_var(Emap& dict, const std::string& name, const std::string& value)
{
  static const std::string beg_str("$(");
  static const std::string end_str(")");

  std::string s = daq::core::substitute_variables(value, 0, beg_str, end_str);
  if( s.length() < 2 || s[0] != '$' || s[1] != '(' ) {
    dict[name] = std::move(s);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /**
   *  Static functions to combine 2, 3 and 4 strings into a path.
   */

static std::string
make_path(const std::string& s1, const std::string& s2)
{
  std::string s;
  s.reserve(s1.size() + s2.size() + 1);
  s.append(s1);
  s.push_back('/');
  s.append(s2);
  return s;
}

static std::string
make_path(const std::string& s1, const std::string& s2, const std::string& s3)
{
  std::string s;
  s.reserve(s1.size() + s2.size() + s3.size() + 2);
  s.append(s1);
  s.push_back('/');
  s.append(s2);
  s.push_back('/');
  s.append(s3);
  return s;
}

static void
add_search_path(std::vector<std::string>& paths, std::string&& path)
{
  for (const auto& i : paths)
    if (path == i)
      return;

  paths.emplace_back(std::move(path));
}

//
// Static function to recursively go down the SW_Package tree and add to paths
//

struct BinaryInfo
{
  BinaryInfo(const daq::core::Tag& tag) : m_hw_tag(tag.get_HW_Tag()), m_sw_tag(tag.get_SW_Tag())
  {
    m_bin_path.reserve(32);
    m_lib_path.reserve(32);

    m_bin_path = m_hw_tag;
    m_bin_path.push_back('-');
    m_bin_path.append(m_sw_tag);
    m_bin_path.push_back('/');

    m_lib_path = m_bin_path;

    m_bin_path.append(s_bin_str);
    m_lib_path.append(s_lib_str);
  }

  bool
  is_compatible(const daq::core::TagMapping* mapping) const
  {
    return (m_hw_tag == mapping->get_HW_Tag() && m_sw_tag == mapping->get_SW_Tag());
  }

  const std::string& m_hw_tag;
  const std::string& m_sw_tag;

  std::string m_bin_path;   // ${hw_tag}-${sw_tag}/bin; assume max len 32
  std::string m_lib_path;   // ${hw_tag}-${sw_tag}/lib; assume max len 32
};

static void
get_paths(
    const daq::core::SW_Package* package,
    std::vector<std::string>& search_paths,
    std::vector<std::string>& paths_to_shared_libraries,
    const BinaryInfo& binary_info,
    daq::core::TestCircularDependency& cd_fuse)
{
  if (const daq::core::SW_Repository * repository = package->cast<daq::core::SW_Repository>())
    {
      const std::string& patch_area(repository->get_PatchArea());
      const std::string& installation_path(repository->get_InstallationPath());

      if (patch_area.empty())
        {
          ERS_DEBUG(2, "skip empty \"PatchArea\" of " << repository);
        }
      else
        {
          add_search_path(search_paths,make_path(patch_area, s_share_bin_str));
          add_search_path(search_paths,make_path(patch_area, binary_info.m_bin_path));
          add_search_path(paths_to_shared_libraries,make_path(patch_area, binary_info.m_lib_path));
        }

      if (installation_path.empty())
        {
          ERS_DEBUG( 2 , "skip " << repository << " with empty \"InstallationPath\" attribute");
        }
      else
        {
          add_search_path(search_paths, make_path(installation_path, s_share_bin_str));
          add_search_path(search_paths, make_path(installation_path, binary_info.m_bin_path));
          add_search_path(paths_to_shared_libraries, make_path(installation_path, binary_info.m_lib_path));
        }
    }
  else if (const daq::core::SW_ExternalPackage * epkg = package->cast<daq::core::SW_ExternalPackage>())
    {
      const std::string& package_patch_area(epkg->get_PatchArea());
      const std::string& package_installation_path(epkg->get_InstallationPath());

      for (const auto& i : epkg->get_Binaries())
        {
          if (binary_info.is_compatible(i))
            {
              if (!package_patch_area.empty())
                add_search_path(search_paths, make_path(package_patch_area, i->get_Value()));

              add_search_path(search_paths, make_path(package_installation_path, i->get_Value()));

              break;
            }
        }

      for (const auto& i : epkg->get_SharedLibraries())
        {
          if (binary_info.is_compatible(i))
            {
              if (!package_patch_area.empty())
                add_search_path(paths_to_shared_libraries, make_path(package_patch_area, i->get_Value()));

              add_search_path(paths_to_shared_libraries, make_path(package_installation_path, i->get_Value()));

              break;
            }
        }
    }
  else
    {
      std::ostringstream text;
      text << "failed to cast " << package << " to " << daq::core::SW_Repository::s_class_name << " or " << daq::core::SW_ExternalPackage::s_class_name << " class";
      throw (daq::core::AlgorithmError(ERS_HERE, text.str()));
    }

  // Loop over all Uses SW_Package and call get_paths on those
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, package);
      for (const auto & i : package->get_Uses())
        get_paths(i, search_paths, paths_to_shared_libraries, binary_info, cd_fuse);
    }
}


//
// Static function to recursively go down the SW_Package tree and check tag
// is available in all the tree
//

static void
check_tag(const daq::core::SW_Package* package, const daq::core::Tag& tag, daq::core::TestCircularDependency& cd_fuse)
// throws (daq::core::BadTag)
{
  if (const daq::core::SW_Repository * repository = package->cast<daq::core::SW_Repository>())
    {
      // Check through the tags to see if there is a match
      for (const auto& i : repository->get_Tags())
        if (i == &tag)
          goto test_used_sw_packages;
    }
  else if (const daq::core::SW_ExternalPackage * epkg = package->cast<daq::core::SW_ExternalPackage>())
    {
      // Check through the shared library tag mappings to see if there is a match
      for (const auto& i : epkg->get_SharedLibraries())
        if (i->get_HW_Tag() == tag.get_HW_Tag() && i->get_SW_Tag() == tag.get_SW_Tag())
          goto test_used_sw_packages;

      // Check through the binaries tag mappings to see if there is a match
      for (const auto& i : epkg->get_Binaries())
        if (i->get_HW_Tag() == tag.get_HW_Tag() && i->get_SW_Tag() == tag.get_SW_Tag())
          goto test_used_sw_packages;
    }
  else
    {
      std::ostringstream text;
      text << "failed to cast " << package << " to SW_Repository or SW_ExternalPackage class";
      throw(daq::core::AlgorithmError(ERS_HERE, text.str()));
    }

  {
    std::ostringstream text;
    text << "the " << package << " does not support this tag";
    throw(daq::core::BadTag(ERS_HERE, tag.UID(), text.str()));
  }

  // Test tags of used packages
  test_used_sw_packages:
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, package);

      for (const auto& i : package->get_Uses())
        check_tag(i, tag, cd_fuse);
    }
}


/******************************************************************************
************************** ALGORITHM get_parents() ****************************
******************************************************************************/

  /**
   *  Copy a path 'p' to list of paths 'out'.
   */

inline void
add_path(const std::vector<const daq::core::Component *> & p, std::list<std::vector<const daq::core::Component *> >& out)
{
  out.push_back(p);
}


  /**
   *  Static function to calculate list of components
   *  from the root segment to the lowest component which
   *  the child object (a segment or a resource) belongs.
   */

static void
make_parents_list(
    const ConfigObjectImpl * child,
    const daq::core::ResourceSet * resource_set,
    std::vector<const daq::core::Component *> & p_list,
    std::list< std::vector<const daq::core::Component *> >& out,
    daq::core::TestCircularDependency& cd_fuse)
{
  daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, resource_set);

  // add the resource set to the path
  p_list.push_back(resource_set);

  // check if the application is in the resource relationship, i.e. is a resource or belongs to resource set(s)
  for (const auto& i : resource_set->get_Contains())
    {
      if (i->config_object().implementation() == child)
        {
          add_path(p_list, out);
        }
      else if (const daq::core::ResourceSet * rs = i->cast<daq::core::ResourceSet>())
        {
          make_parents_list(child, rs, p_list, out, cd_fuse);
        }
    }

  // remove the resource set from the path
  p_list.pop_back();
}


static void
make_parents_list(
    const ConfigObjectImpl * child,
    const daq::core::Segment * segment,
    std::vector<const daq::core::Component *> & p_list,
    std::list<std::vector<const daq::core::Component *> >& out,
    bool is_segment,
    daq::core::TestCircularDependency& cd_fuse)
{
  daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, segment);

  // add the segment to the path
  p_list.push_back(segment);

  // check if the application is in the nested segment
  for (const auto& i : segment->get_Segments())
    if (i->config_object().implementation() == child)
      add_path(p_list, out);
    else
      make_parents_list(child, i, p_list, out, is_segment, cd_fuse);

  // check if the application is in the resource relationship, i.e. is a resource or belongs to resource set(s)
  if (!is_segment)
    {
      for (const auto& i : segment->get_Resources())
        if (i->config_object().implementation() == child)
          add_path(p_list, out);
        else if (const daq::core::ResourceSet * resource_set = i->cast<daq::core::ResourceSet>())
          make_parents_list(child, resource_set, p_list, out, cd_fuse);
    }

  // remove the segment from the path

  p_list.pop_back();
}


static void
check_segment(
    std::list< std::vector<const daq::core::Component *> >& out,
    const daq::core::Segment * segment,
    const ConfigObjectImpl * child,
    bool is_segment,
    daq::core::TestCircularDependency& cd_fuse)
{
  daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, segment);

  std::vector<const daq::core::Component *> s_list;

  if (segment->config_object().implementation() == child)
    add_path(s_list,out);

  make_parents_list(child, segment, s_list, out, is_segment, cd_fuse);
}


void
daq::core::Component::get_parents(const daq::core::Partition& partition, std::list<std::vector<const daq::core::Component *>>& parents) const
{
  const ConfigObjectImpl * obj_impl = config_object().implementation();

  const bool is_segment = castable(daq::core::Segment::s_class_name);

  try
    {
      daq::core::TestCircularDependency cd_fuse("component parents", &partition);

      // check partition's segments
      for (const auto& i : partition.get_Segments())
        check_segment(parents, i, obj_impl, is_segment, cd_fuse);

      // check online-infrastructure segment
      const daq::core::Segment * s = partition.get_OnlineInfrastructure();
      check_segment(parents, s, obj_impl, is_segment, cd_fuse);

      // check partition's online-infrastructure
      for (const auto &a : partition.get_OnlineInfrastructureApplications())
        if (a->config_object().implementation() == obj_impl)
          {
            parents.emplace_back(1, s);
            break;
          }

      if (parents.empty())
        ERS_DEBUG(1, "cannot find segment/resource path(s) between Component " << this << " and partition " << &partition << " objects (check this object is linked with the partition as a segment or a resource)");
    }
  catch (ers::Issue & ex)
    {
      ers::error(daq::core::CannotGetParents(ERS_HERE, full_name(), ex));
    }
}


bool
daq::core::is_compatible(const daq::core::Tag& tag, const daq::core::Computer& host, const daq::core::Partition& partition)
{
  if (tag.get_HW_Tag() == host.get_HW_Tag())
    return true;

  for (const auto& info_obj : partition.get_OnlineInfrastructure()->get_CompatibilityInfo())
    {
      if (host.get_HW_Tag() == info_obj->get_HW_Tag())
        {
          for (const auto& j : info_obj->get_CompatibleWith())
            if (tag.get_HW_Tag() == j->get_HW_Tag())
              return true;

          return false;
        }
    }

  return false;
}


static void
set_path(std::map<std::string, std::string>& environment, const std::string& var, std::vector<std::string>& value)
{
  // create colon-separated string from tokens

  std::string s;

  std::string::size_type len = value.size();

  for (const auto& i : value)
    len += i.size();

  s.reserve(len);

  for (const auto& i : value)
    {
      if (s.empty() == false)
        s.push_back(':');

      s.append(i);
    }

  // concatenate above string and a value in map if exists

  std::string& result(environment[var]);

  if (result.empty())
    {
      result = std::move(s);
    }
  else
    {
      std::string old_val = std::move(result);
      result = std::move(s);
      result.push_back(':');
      result.append(old_val);
    }
}


/***************************************************************************/

// helper functions to recursively get all used sw packages

struct UsedPackages
{
  std::set<const daq::core::SW_Package*> m_packages_set;
  std::vector<const daq::core::SW_Package*> m_packages;

  UsedPackages()
  {
    m_packages.reserve(24);
  }

  void
  add(const daq::core::SW_Package* p)
  {
    if (m_packages_set.insert(p).second == true)
      {
        m_packages.push_back(p);
        add(p->get_Uses());
      }
  }

  void
  add(const std::vector<const daq::core::SW_Package*>& in)
  {
    for (const auto & i : in)
      add(i);
  }
};

/***************************************************************************/

static const char *
get_env(const char * name)
{
  const char * val = ::getenv(name);
  if (val && !*val)
    return nullptr;
  return val;
}


  // helper functions to get partition environment
  // note, they cannot be combined together since there is an order of environment build
  // and some code is executed between add_front... and add_end_partition_environment()

static const char * s_repository(get_env(s_tdaq_db_repository_str.c_str()));

static void
add_front_partition_environment(std::map<std::string, std::string>& environment, const daq::core::Partition& partition)
{
  if (s_repository)
    {
      if (const char * s_user_repository = get_env(s_tdaq_db_user_repository_str.c_str()))
        {
          add_env_var(environment, s_tdaq_db_path_str, s_user_repository);
          ERS_DEBUG(2, "Set " << s_tdaq_db_path_str << "=" << s_user_repository << " and unset " << s_tdaq_db_user_repository_str << " and " << s_tdaq_db_repository_str);
        }
      else
        {
          static const char * s_mapping_dir(get_env(s_oks_repository_mapping_dir_str.c_str()));

          add_env_var(environment, s_tdaq_db_repository_str, s_repository);
          add_env_var(environment, s_tdaq_db_path_str, "");
          ERS_DEBUG(2, "Set " << s_tdaq_db_repository_str << '=' << s_repository << " and unset " << s_tdaq_db_path_str);

          if (s_mapping_dir)
            {
              add_env_var(environment, s_oks_repository_mapping_dir_str, s_mapping_dir);
              ERS_DEBUG(2, "Set " << s_oks_repository_mapping_dir_str << '=' << s_mapping_dir);
            }
        }
    }

  add_env_var(environment, s_tdaq_partition_str, partition.UID());

  try
    {
      const std::string& IPCRef = partition.get_IPCRef();
      if (!IPCRef.empty())
        add_env_var(environment, s_tdaq_ipc_init_ref_str, IPCRef);

      if (!s_repository)
        { // is taken only is TDAQ_DB_REPOSITORY is not set
          const std::string& DBPath = partition.get_DBPath();
          if (!DBPath.empty())
            add_env_var(environment, s_tdaq_db_path_str, DBPath);
        }

      const std::string& DBName = partition.get_DBName();
      if (!DBName.empty())
        add_env_var(environment, s_tdaq_db_data_str, DBName);
    }
  catch (daq::config::Generic& ex)
    {
      std::ostringstream text;
      text << "failed to read object " << &partition;
      throw daq::config::Generic(ERS_HERE, text.str().c_str(), ex);
    }
}


static void
extend_env_var(std::map<std::string, std::string>& environment,
               const daq::core::SW_PackageVariable* var,
               const daq::core::SW_Package * package,
               std::string&& value)
{
  std::string& val = environment[var->get_Name()];

  if(val.empty())
    {
      val = std::move(value);
    }
  else
    {
      const std::string old_value(std::move(val));
      val = std::move(value);
      val.push_back(':');
      val.append(old_value);
    }

  ERS_DEBUG(4, "extend environment variable " << var->get_Name() << "=\'" << environment[var->get_Name()] << "\' as defined by object " << var << " linked with " << package);
}


static void
add_end_partition_environment(std::map<std::string, std::string>& environment,
                              const daq::core::Partition& partition,
			      const daq::core::BaseApplication * base_application,
			      const daq::core::ComputerProgram * computer_program,
                              const daq::core::Tag * tag)
{
  // Partition needs Environment
  add_env_vars(environment, partition.get_ProcessEnvironment(), tag);

  if (s_repository && environment.find(s_tdaq_db_version_str) == environment.end())
    add_env_var(environment, s_tdaq_db_version_str, partition.get_DBVersion());

  // Add environment defined by the sw packages used by application (if != 0) and the computer program
  {
    UsedPackages used_sw;

    if (base_application)
      used_sw.add(base_application->get_Uses());

    used_sw.add(computer_program->get_BelongsTo()->cast<daq::core::SW_Package>());
    used_sw.add(computer_program->get_Uses());

    for (const auto &i : used_sw.m_packages)
      add_env_vars(environment, i->get_ProcessEnvironment(), tag);

    for (const auto &j : used_sw.m_packages)
      if (const daq::core::SW_Repository *sr = j->cast<daq::core::SW_Repository>())
        {
          const std::string &rn = sr->get_InstallationPathVariableName();
          if (!rn.empty())
            {
              if (environment.emplace(rn, sr->get_InstallationPath()).second == true)
                {
                  ERS_DEBUG(4, "add environment variable " << rn << "=\'" << sr->get_InstallationPath() << "\' defined by object " << sr);
                }
              else
                {
                  std::ostringstream text;
                  text << "environment variable " << rn << "=\'" << sr->get_InstallationPath() << "\' defined by object " << sr
                      << " was already defined for it; check configuration database";
                  ers::warning(daq::core::BadVariableUsage(ERS_HERE, text.str()));
                }
            }
        }

    for (std::vector<const daq::core::SW_Package*>::const_reverse_iterator j = used_sw.m_packages.rbegin(); j != used_sw.m_packages.rend(); ++j)
      for (const auto &v : (*j)->get_AddProcessEnvironment())
        {
          extend_env_var(environment, v, (*j), make_path((*j)->get_InstallationPath(), v->get_Suffix()));

          // note, new value is prepended, so patch area will have priority
          if (const daq::core::SW_Repository *r = (*j)->cast<daq::core::SW_Repository>())
            {
              if (!r->get_PatchArea().empty())
                extend_env_var(environment, v, (*j), make_path(r->get_PatchArea(), v->get_Suffix()));
            }
        }


    // if the program is Java script, generate Java's CLASSPATH
    if (const daq::core::Script *script = computer_program->cast<daq::core::Script>())
      if (!strcasecmp("java", script->get_Shell().c_str()))
        {
          ERS_DEBUG(5, "application " << base_application << " is Java script");

          std::string class_path;
          std::map<std::string, std::string>::const_iterator x = environment.find(s_classpath_str);
          if (x != environment.end())
            {
              class_path = x->second;
              ERS_DEBUG(5, "CLASSPATH defined via environment: " << class_path);
            }

          const std::string &user_dir(partition.get_RepositoryRoot());

          for (const auto &j : used_sw.m_packages)
            {
              if (const daq::core::SW_Repository *rep = j->cast<daq::core::SW_Repository>())
                {
                  add_classpath(*rep, user_dir, class_path);
                }
            }

          ERS_DEBUG(5, "set final CLASSPATH: " << class_path);
          environment[s_classpath_str] = class_path;
        }
  }


      // Set TDAQ_DB and TDAQ_DB_NAME if necessary:
      //  - if not defined, set TDAQ_DB variable
      //  - check db technology
      //    set TDAQ_DB_NAME="RDB", if it is not set and the technology is rdbconfig

  auto tdaq_db_var_it = environment.find(s_tdaq_db_str);

  if(partition.get_DBTechnology() == s_rdbconfig_str) {
    auto it = environment.find(s_tdaq_db_name_str);
    if(it != environment.end()) {
      if(tdaq_db_var_it == environment.end()) {
	add_env_var(environment, s_tdaq_db_str, std::string(s_rdbconfig_colon_str) + it->second);
      }
    }
    else {
      add_env_var(environment, s_tdaq_db_name_str, "RDB");
      if(tdaq_db_var_it == environment.end()) {
	add_env_var(environment, s_tdaq_db_str, "rdbconfig:RDB");
      }
    }
  }
  else {
    if(tdaq_db_var_it == environment.end()) {
      add_env_var(environment, s_tdaq_db_str, std::string("oksconfig:") + partition.get_DBName());
      environment.erase(s_tdaq_db_name_str);
    }
  }
}


#ifndef ERS_NO_DEBUG
static std::string
mk_app_env_string(std::map<std::string, std::string>& environment)
{
  std::string s;
  for(const auto& i : environment) {
    s += i.first + "=\'" + i.second + "\'\n";
  }
  return s;
}
#endif


/******************************************************************************
***************** ALGORITHM ComputerProgram::get_parameters() *****************
******************************************************************************/

static void get_parameters(
  const daq::core::ComputerProgram * this_cp,
  std::vector<std::string>& program_names,
  std::vector<std::string>& search_paths,
  std::vector<std::string>& paths_to_shared_libraries,
  const daq::core::Tag& tag,
  const daq::core::Computer& host,
  const daq::core::Partition& partition
)
// throw ( BadProgramInfo BadTag)
{
  ERS_DEBUG(4, " CALL get_parameters()"
            << "\n  program   = " << this_cp
            << "\n  tag       = " << &tag
            << "\n  host      = " << &host
            << "\n  partition = " << &partition );

  const daq::core::SW_Repository * belongs_to = nullptr;
  const bool is_script = (this_cp->class_name() == daq::core::Script::s_class_name);
  const std::string& repository_root(partition.get_RepositoryRoot());

  // Check ComputerProgram belong to SW_Package
  try
    {
      belongs_to = this_cp->get_BelongsTo();
    }
  catch (daq::config::Exception& ex)
    {
      throw daq::core::BadProgramInfo(ERS_HERE, this_cp->UID(), "Failed to read SW_Package object", ex);
    }

  // Check the tag is supported by the hardware
  if (!daq::core::is_compatible(tag, host, partition))
    {
      std::ostringstream text;
      text << "this tag is not applicable on host " << host.UID() << " with hw tag \"" << host.get_HW_Tag() << '\"';
      throw daq::core::BadTag(ERS_HERE, tag.UID(), text.str());
    }

  // Check that the software repositories support the tag
  // i.e. the BelongsTo and its subtree and the Uses and their subtree
  try
    {
      daq::core::TestCircularDependency cd_fuse("program tags", this_cp);

      // Check the BelongsTo repository (and its subtree) supports the tag
      check_tag(belongs_to, tag, cd_fuse);

      // Check that the Uses repositories (and their uses subtree) support the tag
      for (const auto& i : this_cp->get_Uses())
        check_tag(i, tag, cd_fuse);
    }
  catch (ers::Issue & ex)
    {
      std::ostringstream text;
      text << this_cp << " is not compatible (running on on host " << &host << ')';
      throw daq::core::BadTag(ERS_HERE, tag.UID(), text.str(), ex );
    }

  // Find program name (either a script, a binary with no exact implementation or a binary with exact implementation)
  std::string program_name;

  if (is_script)
    {
      // Script
      program_name = this_cp->get_BinaryName();
      ERS_DEBUG(6, "the Program name is \"" << program_name << "\" (name of script)");
      if (program_name.empty())
        throw daq::core::BadProgramInfo(ERS_HERE, this_cp->UID(), "program has no BinaryName defined (name of script)" );
      }
    else
      {
        const daq::core::Binary * binary_program = this_cp->cast<daq::core::Binary>();

        if (binary_program == nullptr)
          throw daq::core::BadProgramInfo(ERS_HERE, this_cp->UID(), "program is not a ComputerProgram");

        if (binary_program->get_ExactImplementations().empty())
          {
            // Binary with no exact implementation
            program_name = this_cp->get_BinaryName();
            ERS_DEBUG(6, "the program name is \"" << program_name << "\" (name of binary without exact implementations)");
            if (program_name.empty())
              throw daq::core::BadProgramInfo(ERS_HERE, this_cp->UID(), "program has no BinaryName defined (no exact implementation)" );
          }
        else
          {
            // Binary with exact implementation
            for (const auto& j : binary_program->get_ExactImplementations())
              {
                // Check the Tag matches
                if (j->get_Tag() == &tag)
                  {
                    program_name = j->get_BinaryName();
                    ERS_DEBUG(6, "the program name is \"" << program_name << "\" name of exact implementation binary file " << j );
                    break;
                  }
              }

            if (program_name.empty())
              {
                std::ostringstream text;
                text << "the program " << this_cp << " has no exact implementation for it";
                throw daq::core::BadTag(ERS_HERE, tag.UID(), text.str() );
              }
          }
      }

  // Make binary tag strings
  BinaryInfo binary_info(tag);

  // Create possible paths to computer program
  if (program_name[0] == '/')
    {
      // Fully qualified path and name
      program_names.emplace_back(program_name);
    }
  else
    {
      const std::string& patch_area(belongs_to->get_PatchArea());
      const std::string& installation_path(belongs_to->get_InstallationPath());

      const std::string& path_suffix(is_script ? s_share_bin_str : binary_info.m_bin_path);

      // Add paths to user-defined repository (Partition RepositoryRoot)
      if (!repository_root.empty())
        program_names.emplace_back(std::move(make_path(repository_root, path_suffix, program_name)));

      // Add paths to PathArea if it is non-empty
      if (!patch_area.empty())
        program_names.emplace_back(std::move(make_path(patch_area, path_suffix, program_name)));

      // Add paths to BelongsTo repository
      program_names.emplace_back(std::move(make_path(installation_path, path_suffix, program_name)));
    }


  // Add search paths and paths to shared libraries to user-defined repository if it is used (i.e. Partition RepositoryRoot)
  // Rotate the vectors to make the Repository Root paths first
  if (!repository_root.empty())
    {
      search_paths.emplace_back(std::move(make_path(repository_root, s_share_bin_str)));
      search_paths.emplace_back(std::move(make_path(repository_root, binary_info.m_bin_path)));
      paths_to_shared_libraries.emplace_back(std::move(make_path(repository_root, binary_info.m_lib_path)));

      if (search_paths.size() > 2)
        std::rotate(search_paths.begin(), search_paths.end() - 2, search_paths.end());

      if (paths_to_shared_libraries.size() > 1)
        std::rotate(paths_to_shared_libraries.begin(), paths_to_shared_libraries.end() - 1, paths_to_shared_libraries.end());
    }

  // Add search paths and paths to shared libraries to used repository
  // and any repositories which they use (recursively)
  try
    {
      daq::core::TestCircularDependency cd_fuse("program binary and library paths", this_cp);

      for (const auto& i : this_cp->get_Uses())
        get_paths(i, search_paths, paths_to_shared_libraries, binary_info, cd_fuse);

      // Add search paths and paths to shared libraries to repository the program belongs to
      // and any repositories which it uses (recursively)
      get_paths(belongs_to, search_paths, paths_to_shared_libraries, binary_info, cd_fuse);
    }
  catch (ers::Issue & ex)
    {
      throw daq::core::BadProgramInfo(ERS_HERE, this_cp->UID(), "Failed to get binary and library paths.", ex);
    }
}

/******************************************************************************
 ******************* ALGORITHM ComputerProgram::get_info() ********************
 ******************************************************************************/

void daq::core::ComputerProgram::get_info(
  std::map<std::string, std::string>& environment,
  std::vector<std::string>& program_names,
  const daq::core::Partition& partition,
  const daq::core::Tag& tag,
  const daq::core::Computer& host
) const
{
  ERS_DEBUG(4, " CALL daq::core::ComputerProgram::get_info()" 
	    << "  \n this      = " << this  
	    << "  \n tag       = " << &tag 
	    << "  \n host      = " << &host
	    << "  \n partition = " << &partition );


    // Get the program names and the search paths

  std::vector<std::string> search_paths;
  std::vector<std::string> paths_to_shared_libraries;

  search_paths.reserve(search_path_default_size);
  paths_to_shared_libraries.reserve(paths_to_shared_libraries_default_size);

  get_parameters(this, program_names, search_paths, paths_to_shared_libraries, tag, host, partition);


    // Get the environment:
    //  - add environment defined by the partition's attributes (Name, IPCRef, DBPath, DBName, ...)
    //  - add environment defined for the computer program
    //  - add rest of environment defined for the partition

  try {
    add_front_partition_environment(environment, partition); // throw no_subst_parameter

    add_env_vars(environment, get_ProcessEnvironment(), nullptr);

    add_end_partition_environment(environment, partition, nullptr, this, &tag);
  }
  catch ( daq::config::Generic & ex ) {
     throw daq::core::BadProgramInfo( ERS_HERE, UID(), "failed to build Program environment", ex ) ;
  }

    // add "PATH" and "LD_LIBRARY_PATH" variables

  set_path(environment, s_path_str, search_paths);
  set_path(environment, s_ld_library_path_str, paths_to_shared_libraries);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
get_resourse_apps(
  const daq::core::ResourceBase * obj,
  std::vector<const daq::core::BaseApplication *>& out,
  const daq::core::Partition * p,
  daq::core::TestCircularDependency& cd_fuse)
{
  if (p == nullptr || obj->disabled(*p, true) == false)
    {
      // test if the resource base can be casted to the application
      if (const daq::core::BaseApplication * r = obj->cast<daq::core::BaseApplication>())
        {
          out.push_back(r);
        }

      // test if the resource base can contain nested resources
      if (const daq::core::ResourceSet * s = obj->cast<daq::core::ResourceSet>())
        {
          for (const auto& i : s->get_Contains())
            {
              daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
              get_resourse_apps(i, out, p, cd_fuse);
            }
        }
    }
}

static std::vector<const daq::core::BaseApplication *>
get_resource_applications(const daq::core::ResourceBase * obj, const daq::core::Partition * p = nullptr)
{
  daq::core::TestCircularDependency cd_fuse("resource applications", obj);
  std::vector<const daq::core::BaseApplication *> out;
  get_resourse_apps(obj, out, p, cd_fuse);
  return out;
}

/******************************************************************************
******************* ALGORITHM Segment::get_timeouts() ***********
******************************************************************************/

void
daq::core::Segment::get_timeouts(int& actionTimeout, int& shortActionTimeout) const
  //throws (daq::core::BadSegment)
{
  const daq::core::Partition * partition = get_seg_config(false)->get_partition();

  actionTimeout = shortActionTimeout = 0;

  for (const auto& a : get_infrastructure())
    if (static_cast<int>(a->get_ExitTimeout()) > shortActionTimeout)
      shortActionTimeout = a->get_ExitTimeout();

  for(const auto& i : get_nested_segments())
    {
      if (i->disabled(*partition) == false)
        {
          for (const auto& it : get_infrastructure())
            {
              if (static_cast<int>(it->get_ExitTimeout()) > shortActionTimeout)
                {
                  shortActionTimeout = it->get_ExitTimeout();
                }
            }

          int tmpLongTime, tmpShortTime;
          i->get_timeouts(tmpLongTime, tmpShortTime);

          if (tmpLongTime > actionTimeout)
            actionTimeout = tmpLongTime;
          if (tmpShortTime > shortActionTimeout)
            shortActionTimeout = tmpShortTime;
        }
    }

  for (const auto& a : get_applications())
    {
      if (const daq::core::RunControlApplicationBase* rcApp = a->cast<daq::core::RunControlApplicationBase>())
        {
          if (rcApp->get_ActionTimeout() > actionTimeout)
            actionTimeout = rcApp->get_ActionTimeout();
        }

      if (const daq::core::BaseApplication* slApp = a->cast<daq::core::BaseApplication>())
        {
          if (static_cast<int>(slApp->get_ExitTimeout()) > shortActionTimeout)
            shortActionTimeout = slApp->get_ExitTimeout();
        }

    }

  actionTimeout += get_IsControlledBy()->get_ActionTimeout();
  shortActionTimeout += get_IsControlledBy()->cast<daq::core::BaseApplication>()->get_ExitTimeout();

  ERS_DEBUG(4, "Segment: " << UID() << ": Action Timeout --> " << actionTimeout << "; Exit Timeout --> " << shortActionTimeout);

  return;
}

namespace daq
{
  namespace core
  {

    class BackupHostFactory
    {
    public:
      BackupHostFactory(const daq::core::Segment& seg) :
          m_seg(seg), m_num_of_hosts(seg.get_Hosts().size()), m_count(0)
      {
      }

      const daq::core::Computer *
      get_next()
      {
        size_t idx = m_count++ % m_num_of_hosts;

        if (idx == 0)
          {
            m_count++;
            return m_seg.get_Hosts()[1]->cast<daq::core::Computer>();
          }

        return m_seg.get_Hosts()[idx]->cast<daq::core::Computer>();
      }

      size_t
      get_size() const
      {
        return m_num_of_hosts;
      }

    private:
      const daq::core::Segment& m_seg;
      const size_t m_num_of_hosts;
      size_t m_count;
    };
  }
}




/******************************************************************************
********************** ALGORITHM get_all_applications() ***********************
******************************************************************************/


std::vector<const daq::core::BaseApplication *>
daq::core::Partition::get_all_applications(std::set<std::string> * app_types, std::set<std::string> * use_segments, std::set<const daq::core::Computer *> * use_hosts) const
{
  return get_segment(get_OnlineInfrastructure()->UID())->get_all_applications(app_types, use_segments, use_hosts);
}


void
get_generic_resources(const daq::core::ResourceBase * obj, ::Configuration& db, std::list<const daq::core::Resource *>& out, const daq::core::Partition * p, daq::core::TestCircularDependency& cd_fuse)
{
  if (p == nullptr || obj->disabled(*p) == false)
    {
      if (const daq::core::Resource * r = db.cast<daq::core::Resource>(obj))
        {
          out.push_back(r);
        }
      else if (const daq::core::ResourceSet * s = db.cast<daq::core::ResourceSet>(obj))
        {
          for (const auto& i : s->get_Contains())
            {
              daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
              get_generic_resources(i, db, out, p, cd_fuse);
            }
        }
    }
}

void
daq::core::ResourceBase::get_resources(::Configuration& db, std::list<const Resource *>& out, const daq::core::Partition * p) const
{
  daq::core::TestCircularDependency cd_fuse("generic resources", this);
  get_generic_resources(this, db, out, p, cd_fuse);
}


static std::vector<const daq::core::BaseApplication *>
get_all_referenced(const daq::core::AppConfig * app, const std::vector<const daq::core::BaseApplication*>& refs, const std::vector<const daq::core::BaseApplication *>& all_apps)
{
  std::vector<const daq::core::BaseApplication *> out;

  for (const auto& x : all_apps)
    {
      const ConfigObjectImpl * app_impl(x->get_base_app()->config_object().implementation());

      for (const auto& a : refs)
        {
          if (a->config_object().implementation() == app_impl)
            {
              if (x->is_templated())
                {
                  if (app->get_segment() == x->get_segment())
                    {
                      if (app->get_is_templated())
                        {
                          if (app->get_host() == x->get_host())
                            {
                              out.push_back(x); // both applications are templated; depend only when hosts and segments are equal
                            }
                        }
                      else
                        {
                          out.push_back(x); // the application is not templated, depend on referenced templated applications from the same segment
                        }
                    }
                }
              else
                {
                  out.push_back(x); // referenced application is normal; depend on it in any case
                }
            }
        }
    }

  return out;
}


namespace daq
{
  namespace core
  {
    // This class is a friend of AppConfig and SegConfig
    class AlgorithmUtils
    {

      friend class Partition;

    public:

      static void
      add_applications(daq::core::Segment& seg, const daq::core::Rack * rack, const daq::core::Partition& p, const daq::core::Computer * default_host);

      static void
      add_segments(daq::core::Segment& seg, const daq::core::Partition& p, const std::vector<const daq::core::Segment*>& objs, const daq::core::Rack * rack, const daq::core::Computer * default_host, ::config::map<std::string>& fuse);

      static void
      get_applications(std::vector<const daq::core::BaseApplication *>& out, const daq::core::Segment& seg, std::set<std::string> * app_types, std::set<std::string> * segments, std::set<const daq::core::Computer *> * hosts);

      static AppConfig *
      reset_app_config(daq::core::BaseApplication& app);

      static SegConfig *
      reset_seg_config(daq::core::Segment& seg, const daq::core::Partition* p);

      static const daq::core::Partition*
      get_partition(const daq::core::BaseApplication * app);


    private:

      static void
      add_template_application(const daq::core::TemplateApplication * a, const char * type, daq::core::Segment& seg, std::vector<const daq::core::BaseApplication *>& apps, BackupHostFactory& factory);

      static void
      add_normal_application(const daq::core::Application * a, daq::core::Segment& seg, std::vector<const daq::core::BaseApplication *>& apps);

      static const daq::core::Computer *
      get_host(const daq::core::Segment& seg, const daq::core::BaseApplication * base_app, const daq::core::Application * app = nullptr);

      static bool
      check_app(const daq::core::BaseApplication * app, std::set<std::string> * app_types, std::set<std::string> * use_segments, std::set<const daq::core::Computer *> * use_hosts);

      static void
      check_non_template_segment(const daq::core::Segment& seg, const daq::core::BaseApplication * base_app);

      static void
      set_backup_hosts(const std::string& runs_on, std::vector<const daq::core::Computer *>& template_backup_hosts, BackupHostFactory& factory);

    };
  }
}

const daq::core::Computer *
daq::core::AlgorithmUtils::get_host(const daq::core::Segment& seg, const daq::core::BaseApplication * base_app, const daq::core::Application * app)
{
  if (app)
    {
      if (const daq::core::Computer * h = app->get_RunsOn())
        return h;
    }

  if (!seg.get_hosts().empty())
    return seg.get_hosts().front()->cast<daq::core::Computer>();

  std::ostringstream text;
  text << "to run " << base_app << " (there is no any defined enabled default host for segment, partition or localhost)";
  throw (daq::core::NoDefaultHost(ERS_HERE, seg.UID(), text.str()));
}

void
daq::core::AlgorithmUtils::add_normal_application(const daq::core::Application * a, daq::core::Segment& seg, std::vector<const daq::core::BaseApplication *>& apps)
{
  check_non_template_segment(seg, a);
  daq::core::BaseApplication * app_obj = const_cast<daq::core::BaseApplication *>(seg.configuration().get<daq::core::BaseApplication>(const_cast<ConfigObject&>(a->config_object()), a->UID()));
  daq::core::AppConfig * app_config = daq::core::AlgorithmUtils::reset_app_config(*app_obj);
  app_config->m_host = get_host(seg, a, a);
  app_config->m_segment = &seg;
  app_config->m_base_app = a;
  apps.emplace_back(app_obj);
}

void
daq::core::AlgorithmUtils::add_template_application(const daq::core::TemplateApplication * a, const char * type, daq::core::Segment& seg, std::vector<const daq::core::BaseApplication *>& apps, BackupHostFactory& factory)
{
  const std::vector<const daq::core::Computer*>& hosts(seg.get_hosts());
  int start_idx(0), end_idx(hosts.size());

  const std::string& runs_on(a->get_RunsOn());
  const bool runs_on_first_host(runs_on == daq::core::TemplateApplication::RunsOn::FirstHost || runs_on == daq::core::TemplateApplication::RunsOn::FirstHostWithBackup);

  if (hosts.empty())
    {
      std::ostringstream text;
      text << type << " template application " << a << " may not be run, since segment has no enabled hosts";
      throw daq::core::CannotCreateSegConfig(ERS_HERE, seg.UID(), text.str());
    }

  if (runs_on == daq::core::TemplateApplication::RunsOn::AllButFirstHost)
    {
      if (hosts.size() < 2)
        {
          std::ostringstream text;
          text << type << " template application " << a << " may not be run on \"" << daq::core::TemplateApplication::RunsOn::AllButFirstHost << "\" since segment has " << hosts.size() << " enabled hosts only";
          throw daq::core::CannotCreateSegConfig(ERS_HERE, seg.UID(), text.str());
        }

      start_idx = 1;
    }
  else if(runs_on_first_host)
    {
      end_idx = 1;
    }

  const uint16_t default_num_of_tapps(a->get_Instances());

  std::string app_id(a->UID());
  app_id.push_back(':');
  app_id.append(seg.UID());
  const std::string::size_type app_id_seg_idx = app_id.length();

  for (int i = start_idx; i < end_idx; ++i)
    {
      const daq::core::Computer * h = hosts[i];
      const uint16_t num_of_tapps(default_num_of_tapps ? default_num_of_tapps : h->get_NumberOfCores());

      if(!runs_on_first_host)
        {
          app_id.erase(app_id_seg_idx);
          app_id.push_back(':');
          const std::string& host_uid(h->UID());
          app_id.append(host_uid, 0, host_uid.find('.'));
        }

      const std::string::size_type app_id_host_idx = app_id.length();

      for (unsigned short j = 1; j <= num_of_tapps; ++j)
        {
          if(num_of_tapps > 1)
            {
              app_id.erase(app_id_host_idx);
              app_id.push_back(':');
              append2str(app_id, j);
            }

          daq::core::BaseApplication * app_obj = const_cast<daq::core::BaseApplication *>(seg.configuration().get<daq::core::BaseApplication>(const_cast<ConfigObject&>(a->config_object()), app_id));
          daq::core::AppConfig * app_config = daq::core::AlgorithmUtils::reset_app_config(*app_obj);
          app_config->m_is_templated = true;
          app_config->m_host = h;
          set_backup_hosts(runs_on, app_config->m_template_backup_hosts, factory);
          app_config->m_segment = &seg;
          app_config->m_base_app = a;
          apps.emplace_back(app_obj);
        }
    }
}

void
daq::core::AlgorithmUtils::check_non_template_segment(const daq::core::Segment& seg, const daq::core::BaseApplication * base_app)
{
  if(seg.p_seg_config->m_is_templated)
    {
      std::ostringstream text;
      text << "the segment contains non-template application " << base_app;
      throw daq::core::BadTemplateSegmentDescription(ERS_HERE, seg.UID(), text.str());
    }
}

void
daq::core::AlgorithmUtils::set_backup_hosts(const std::string& runs_on, std::vector<const daq::core::Computer *>& template_backup_hosts, BackupHostFactory& factory)
{
  if (runs_on == daq::core::TemplateApplication::RunsOn::FirstHostWithBackup)
    {
      if (factory.get_size() > 1)
        {
          template_backup_hosts.push_back(factory.get_next());

          if (factory.get_size() > 2)
            {
              template_backup_hosts.push_back(factory.get_next());
            }
        }
    }
}


static void
add_enabled_hosts(std::vector<const daq::core::Computer *>& to, const std::vector<const daq::core::ComputerBase*>& from, unsigned int default_capacity)
{
  std::vector<const daq::core::Computer *> hosts;
  hosts.reserve(default_capacity);
  add_computers(hosts, from);

  for (const auto & h : hosts)
    if (h->get_State())
      to.push_back(h);
}

void
daq::core::AlgorithmUtils::add_applications(daq::core::Segment& seg, const daq::core::Rack * rack, const daq::core::Partition& p, const daq::core::Computer * default_host)
{
  daq::core::SegConfig * seg_config = seg.get_seg_config(false);

  // fill hosts of segment

  if(rack == nullptr)
    {
      add_enabled_hosts(seg_config->m_hosts, seg.get_Hosts(), 4);
    }
  else
    {
      add_enabled_hosts(seg_config->m_hosts, rack->get_Nodes(), 40);

      if(seg_config->m_hosts.size() < 2)
        {
          std::ostringstream text;
          text << "number of enabled computers in \'" << rack << "\' is " << seg_config->m_hosts.size() << " (at least two required)";
          throw daq::core::CannotCreateSegConfig(ERS_HERE, seg.UID(), text.str());
        }
    }

  // if there are no hosts, try to use partition and closest parent segment default host
  if(seg_config->m_hosts.empty() && default_host)
    {
      seg_config->m_hosts.push_back(default_host);
    }

  // check local-host, if there are no hosts
  if(seg_config->m_hosts.empty())
    {
      static std::string local_hostname = System::LocalHost::full_local_name();

      if (const daq::core::Computer * host = seg.configuration().get<daq::core::Computer>(local_hostname))
        {
          if (host->get_State() == true)
            {
              seg_config->m_hosts.push_back(host);
            }
        }
    }


  // backup ....
  BackupHostFactory factory(seg);

  // add controller
    {
      const daq::core::RunControlApplicationBase * ctrl_obj = seg.get_IsControlledBy();
      daq::core::BaseApplication * app_obj = nullptr;
      daq::core::AppConfig * app_config = nullptr;

      if (const daq::core::RunControlApplication * a = ctrl_obj->cast<daq::core::RunControlApplication>())
        {
          check_non_template_segment(seg, a);
          app_obj = const_cast<daq::core::BaseApplication *>(seg.configuration().get<daq::core::BaseApplication>(const_cast<ConfigObject&>(a->config_object()), a->UID()));
          app_config = daq::core::AlgorithmUtils::reset_app_config(*app_obj);
          app_config->m_host = get_host(seg, a, a);
        }
      else if (const daq::core::TemplateApplication * t = ctrl_obj->cast<daq::core::TemplateApplication>())
        {
          const std::string& t_runs_on(t->get_RunsOn());
          if(t_runs_on != daq::core::TemplateApplication::RunsOn::FirstHost && t_runs_on != daq::core::TemplateApplication::RunsOn::FirstHostWithBackup)
            {
              std::ostringstream text;
              text << "controller template application " << t << " may only be run on first host (\"" << t_runs_on << "\" is set instead)";
              throw daq::core::CannotCreateSegConfig(ERS_HERE, seg.UID(), text.str());
            }

          app_obj = const_cast<daq::core::BaseApplication *>(seg.configuration().get<daq::core::BaseApplication>(const_cast<ConfigObject&>(t->config_object()), seg.UID()));
          app_config = daq::core::AlgorithmUtils::reset_app_config(*app_obj);
          app_config->m_is_templated = true;
          app_config->m_host = get_host(seg, t);
          set_backup_hosts(t_runs_on, app_config->m_template_backup_hosts, factory);
        }

      app_config->m_segment = &seg;
      app_config->m_base_app = ctrl_obj->cast<daq::core::BaseApplication>();
      seg_config->m_controller = app_obj;
    }

  // add infrastructure
  for (const auto& x : seg.get_Infrastructure())
    {
      if (seg.configuration().cast<daq::core::Resource>(x) == nullptr)
        {
          if (const daq::core::InfrastructureApplication * a = x->cast<daq::core::InfrastructureApplication>())
            {
              add_normal_application(a, seg, seg_config->m_infrastructure);
            }
          else if (const daq::core::TemplateApplication * t = x->cast<daq::core::TemplateApplication>())
            {
              add_template_application(t, "infrastructure", seg, seg_config->m_infrastructure, factory);
            }
        }
    }

  // add resources
  for (const auto& x : seg.get_Resources())
    {
      // get enabled resources
      for (const auto& j : get_resource_applications(x, &p))
        {
          if (const daq::core::Application * a = j->cast<daq::core::Application>())
            {
              add_normal_application(a, seg, seg_config->m_applications);
            }
          else if (const daq::core::TemplateApplication * t = j->cast<daq::core::TemplateApplication>())
            {
              add_template_application(t, "resource", seg, seg_config->m_applications, factory);
            }
        }
    }

  // add applications
  for (const auto& x : seg.get_Applications())
    {
      if (seg.configuration().cast<daq::core::Resource>(x) == nullptr)
        {
          if (const daq::core::Application * a = x->cast<daq::core::Application>())
            {
              add_normal_application(a, seg, seg_config->m_applications);
            }
          else if (const daq::core::TemplateApplication * t = x->cast<daq::core::TemplateApplication>())
            {
              add_template_application(t, "normal", seg, seg_config->m_applications, factory);
            }
        }
    }
}


static std::string
seg_config_to_name(const std::string& s)
{
  if (s.empty() == true)
    return std::string("partition");
  else
    return std::string("segment \"") + s + "\"";
}

static void
check_mulpiple_inclusion(::config::map<std::string>& fuse, const std::string& id, const std::string& parent)
{
  auto ret = fuse.emplace(id,parent);
  if(ret.second == false)
    {
      throw daq::core::SegmentIncludedMultipleTimes( ERS_HERE, id, seg_config_to_name((*ret.first).second), seg_config_to_name(parent) );
    }
}

void
daq::core::AlgorithmUtils::add_segments(
    daq::core::Segment& seg,
    const daq::core::Partition& p,
    const std::vector<const daq::core::Segment*>& objs,
    const daq::core::Rack * rack,
    const daq::core::Computer * default_host,
    ::config::map<std::string>& fuse)
{
  daq::core::SegConfig * seg_config = seg.get_seg_config(false);

  if(const daq::core::Computer * c = find_enabled(seg.get_Hosts()))
    default_host = c;

  seg_config->m_is_disabled = seg.disabled(p, true);

  if(seg_config->m_is_disabled == false)
    {
      daq::core::AlgorithmUtils::add_applications(seg, rack, p, default_host);
    }

  for(const auto& x : objs)
    {
      if(const daq::core::TemplateSegment * ts = x->cast<TemplateSegment>())
        {
          bool ts_is_disabled = ts->disabled(p, true);

          for(const auto& y : ts->get_Racks())
            {
              std::string id(x->UID());
              id.push_back(':');
              id.append(y->UID());

              bool is_disabled = ts_is_disabled || y->disabled(p, true);

              check_mulpiple_inclusion(fuse, id, seg.p_UID);

              daq::core::Segment * s = const_cast<daq::core::Segment *>(p.configuration().get<daq::core::Segment>(const_cast<ConfigObject&>(ts->config_object()), id));
              daq::core::SegConfig * nested_seg_config = daq::core::AlgorithmUtils::reset_seg_config(*s, &p);
              nested_seg_config->m_is_disabled = is_disabled;
              nested_seg_config->m_is_templated = true;
              nested_seg_config->m_base_segment = x;

              seg_config->m_nested_segments.emplace_back(s);

              if(is_disabled == false)
                {
                  add_applications(*s, y, p, default_host);
                }
            }
        }
      else
        {
          check_mulpiple_inclusion(fuse, x->UID(), seg.p_UID);

          daq::core::Segment * s = const_cast<daq::core::Segment *>(p.configuration().get<daq::core::Segment>(const_cast<ConfigObject&>(x->config_object()), x->UID()));
          daq::core::SegConfig * nested_seg_config = daq::core::AlgorithmUtils::reset_seg_config(*s, &p);
          nested_seg_config->m_is_templated = false;
          nested_seg_config->m_base_segment = x;

          seg_config->m_nested_segments.emplace_back(s);

          add_segments(*s, p, x->get_Segments(), nullptr, default_host, fuse);
        }
    }
}



/**
 *  Function returns true if application's type is listed by the app_types,
 *  the segment is listed in the use_segments and
 *  the host is listed in use_hosts (if containers are defined)
 */

bool
daq::core::AlgorithmUtils::check_app(const daq::core::BaseApplication * app, std::set<std::string> * app_types, std::set<std::string> * use_segments, std::set<const daq::core::Computer *> * use_hosts)
{
  ERS_DEBUG( 5,
    "check_app(app=" << app->UID() << ", seg=" << app->get_segment() << ", host=" << app->get_host() << "):"
    " app_types=" << (app_types && app_types->find(app->class_name()) == app_types->end()) <<
    " use_segments=" << (use_segments && use_segments->find(app->get_segment()->UID()) == use_segments->end()) <<
    " use_hosts=" << (use_hosts && use_hosts->find(app->get_host()) == use_hosts->end())
  );

  return (
    (
      (app_types && app_types->find(app->class_name()) == app_types->end()) ||
      (use_segments && use_segments->find(app->get_segment()->UID()) == use_segments->end()) ||
      (use_hosts && use_hosts->find(app->get_host()) == use_hosts->end())
    ) ? false : true
  );
}

void
daq::core::AlgorithmUtils::get_applications(std::vector<const daq::core::BaseApplication *>& out, const daq::core::Segment& seg, std::set<std::string> * app_types, std::set<std::string> * segments, std::set<const daq::core::Computer *> * hosts)
{
  SegConfig * seg_config = seg.get_seg_config(false);

  // return, if segment is disabled
  if (seg_config->is_disabled() == true)
    {
      ERS_DEBUG( 3, "segment " << seg.UID() << " is disabled" );
      return;
    }

  // check controller
    {
      if (check_app(seg_config->m_controller, app_types, segments, hosts))
        {
          out.push_back(seg_config->m_controller);
        }
    }

  // check infrastructure
  for (const auto& x : seg_config->m_infrastructure)
    if (check_app(x, app_types, segments, hosts))
      out.push_back(x);

  // check applications
  for (const auto& x : seg_config->m_applications)
    if (check_app(x, app_types, segments, hosts))
      out.push_back(x);

  // check nested segments
  for (const auto& x : seg_config->m_nested_segments)
    get_applications(out, *x, app_types, segments, hosts);
}

daq::core::AppConfig *
daq::core::AlgorithmUtils::reset_app_config(daq::core::BaseApplication& app)
{
  if (app.p_app_config)
    app.p_app_config->clear();
  else
    app.p_app_config.reset(new AppConfig());

  return app.p_app_config.get();
}

daq::core::SegConfig *
daq::core::AlgorithmUtils::reset_seg_config(daq::core::Segment& seg, const daq::core::Partition* p)
{
  if (seg.p_seg_config)
    seg.p_seg_config->clear(p);
  else
    seg.p_seg_config.reset(new SegConfig(p));

  return seg.p_seg_config.get();
}

const daq::core::Partition*
daq::core::AlgorithmUtils::get_partition(const daq::core::BaseApplication * app)
{
  return app->get_segment()->get_seg_config(false)->get_partition();
}

const daq::core::Segment *
daq::core::Partition::get_segment(const std::string& name) const
{
  if (m_app_config.m_root_segment == nullptr)
    {
      std::lock_guard<std::mutex> scoped_lock(m_app_config.m_root_segment_mutex);

      if (m_app_config.m_root_segment == nullptr)
        {
          const daq::core::OnlineSegment * onlseg = get_OnlineInfrastructure();

          daq::core::Segment * root_segment = const_cast<daq::core::Segment *>(const_cast<Configuration&>(p_db).get<daq::core::Segment>(const_cast<ConfigObject&>(onlseg->config_object()), onlseg->UID()));

          // reinitialize seg config
          daq::core::AlgorithmUtils::reset_seg_config(*root_segment, this);

          root_segment->p_seg_config->m_base_segment = root_segment;

          const daq::core::Computer * default_host = get_DefaultHost();

          if (default_host && default_host->get_State() == false)
            {
              default_host = nullptr;
            }

          ::config::map<std::string> fuse;
          fuse[root_segment->UID()] = "";

          daq::core::AlgorithmUtils::add_segments(*root_segment, *this, get_Segments(), nullptr, default_host, fuse);

          for (const auto& a : get_OnlineInfrastructureApplications())
            {
              if (const daq::core::ResourceBase * r = a->cast<daq::core::ResourceBase>())
                {
                  if (r->disabled(*this, true) == true)
                    continue;
                }

              std::vector<const daq::core::BaseApplication *>& apps(a->cast<daq::core::InfrastructureBase>() ? root_segment->get_seg_config(false)->m_infrastructure : root_segment->get_seg_config(false)->m_applications);
              daq::core::AlgorithmUtils::add_normal_application(a, *root_segment, apps);
            }

          // check duplicated application IDs

          // FIXME 2022-06-02:
          //   move check to get_all_applications() in next release tdaq-09-05-00
          //   do it once per load/reload modifying ApplicationConfig

          struct Compare {
              bool operator()(const daq::core::BaseApplication *lhs, const daq::core::BaseApplication *rhs) const
              { return (lhs->UID() < rhs->UID()); };
          };

          struct ValidateAppID
          {
            std::map<const daq::core::BaseApplication *, const daq::core::Segment *, Compare> m_map;

            static std::string
            str(const daq::core::BaseApplication * x, const daq::core::Segment * y)
            {
              std::ostringstream s;
              s << '\"' << x << "\" in segment \"" << y->UID() << '\"';
              return s.str();
            }

            void
            check_duplicated(const daq::core::BaseApplication * a, const daq::core::Segment * s)
            {
              auto ret = m_map.emplace(a, s);

              if (ret.second == false)
                throw daq::core::DuplicatedApplicationID( ERS_HERE, ValidateAppID::str(a, s), ValidateAppID::str(ret.first->first, ret.first->second) );
            }

            void
            check_duplicated(const daq::core::Segment *s)
            {
              SegConfig * seg_config = s->get_seg_config(false);

              if (seg_config->is_disabled() == false)
                {
                  check_duplicated(seg_config->get_controller(), s);

                  // check infrastructure
                  for (const auto &x : seg_config->get_infrastructure())
                    check_duplicated(x, s);

                  // check applications
                  for (const auto &x : seg_config->get_applications())
                    check_duplicated(x, s);

                  // check nested segments
                  for (const auto &x : seg_config->get_nested_segments())
                    check_duplicated(x);
                }
            }
          };

          ValidateAppID test;
          test.check_duplicated(root_segment);

          m_app_config.m_root_segment.store(root_segment);
        }
    }

  const daq::core::Segment * seg = p_db.find<daq::core::Segment>(name);

  if(seg == nullptr)
    {
      std::string::size_type idx = name.find(':');

      if (idx != std::string::npos)
        {
          std::string seg_id = name.substr(0, idx);

          seg = p_db.get<Segment>(seg_id);

          if (seg == nullptr)
            {
              std::ostringstream text;
              text << "cannot find template segment object \'" << seg_id << '\'';
              throw daq::core::CannotFindSegmentByName(ERS_HERE, name, text.str());
            }

          if (const daq::core::TemplateSegment * ts = seg->cast<TemplateSegment>())
            {
              std::ostringstream text;
              text << "template segment " << ts << " does not have rack \'" << name.substr(idx + 1) << '\'';
              throw daq::core::CannotFindSegmentByName(ERS_HERE, name, text.str());
            }
          else
            {
              std::ostringstream text;
              text << "object \'" << seg_id << "\' is not template segment";
              throw daq::core::CannotFindSegmentByName(ERS_HERE, name, text.str());
            }
        }
      else
        {
          throw daq::core::CannotFindSegmentByName(ERS_HERE, name, "no such non-template segment object");
        }
    }

  return seg;
}

const daq::core::AppConfig *
daq::core::BaseApplication::get_app_config(bool no_except) const
{
  const BaseApplication * ptr;

  if(typeid(*this) != typeid(BaseApplication))
    {
      if(p_gen_obj)
        {
          ptr = p_gen_obj;
        }
      else
        {
          ptr = p_db.find<daq::core::BaseApplication>(UID());

          if(ptr)
            {
              p_gen_obj.store(ptr);
            }
          else
            {
              if (no_except)
                return nullptr;
              else
                throw daq::core::NotInitedByDalAlgorithm(ERS_HERE, UID(), class_name(), (void*)this, "BaseApplication::get_app_config()");
            }
        }
    }
  else
    {
      ptr = this;
    }

  if(!ptr->p_app_config)
    {
      if (no_except)
        return nullptr;
      else
        throw daq::core::NotInitedByDalAlgorithm(ERS_HERE, UID(), class_name(), (void*)this, "BaseApplication::get_app_config()");
    }

  return ptr->p_app_config.get();

}

daq::core::SegConfig *
daq::core::Segment::get_seg_config(bool check_disabled, bool no_except) const
{
  const Segment * ptr;

  if (typeid(*this) != typeid(Segment))
    {
      if (p_gen_obj)
        {
          ptr = p_gen_obj;
        }
      else
        {
          ptr = p_db.find<daq::core::Segment>(UID());

          if (ptr)
            {
              p_gen_obj.store(ptr);
            }
          else
            {
              if (no_except)
                return nullptr;
              else
                throw daq::core::NotInitedByDalAlgorithm(ERS_HERE, UID(), class_name(), (void*)this, "Segment::get_seg_config()");
            }
        }
    }
  else
    {
      ptr = this;
    }

  if (!ptr->p_seg_config)
    {
      if (no_except)
        return nullptr;
      else
        throw daq::core::NotInitedByDalAlgorithm(ERS_HERE, UID(), class_name(), (void*)this, "Segment::get_seg_config()");
    }

  if (check_disabled && ptr->p_seg_config->is_disabled() && no_except == false)
    {
      throw(daq::core::SegmentDisabled(ERS_HERE));
    }

  return ptr->p_seg_config.get();

}

const daq::core::Computer *
daq::core::BaseApplication::get_host() const
{
  return get_app_config()->get_host();
}

const daq::core::Segment *
daq::core::BaseApplication::get_segment() const
{
  return get_app_config()->get_segment();
}

const daq::core::BaseApplication *
daq::core::BaseApplication::get_base_app() const
{
  return get_app_config()->get_base_app();
}

bool
daq::core::BaseApplication::is_templated() const
{
  return get_app_config()->get_is_templated();
}

bool
daq::core::Segment::is_disabled() const
{
  return get_seg_config(false)->is_disabled();
}

bool
daq::core::Segment::is_templated() const
{
  return get_seg_config(false)->is_templated();
}

const daq::core::Segment *
daq::core::Segment::get_base_segment() const
{
  return get_seg_config(false)->get_base_segment();
}

const daq::core::BaseApplication *
daq::core::Segment::get_controller() const
{
  return get_seg_config(true)->get_controller();
}

const std::vector<const daq::core::BaseApplication *>&
daq::core::Segment::get_infrastructure() const
{
  return get_seg_config(true)->get_infrastructure();
}

const std::vector<const daq::core::BaseApplication *>&
daq::core::Segment::get_applications() const
{
  return get_seg_config(true)->get_applications();
}

const std::vector<const daq::core::Segment*>&
daq::core::Segment::get_nested_segments() const
{
  return get_seg_config(false)->get_nested_segments();
}

const std::vector<const daq::core::Computer*>&
daq::core::Segment::get_hosts() const
{
  return get_seg_config(false)->get_hosts();
}


std::vector<const daq::core::BaseApplication *>
daq::core::BaseApplication::get_initialization_depends_from(const std::vector<const daq::core::BaseApplication *>& all_apps) const
{
  return get_all_referenced(get_app_config(), get_base_app()->get_InitializationDependsFrom(), all_apps);
}

std::vector<const daq::core::BaseApplication *>
daq::core::BaseApplication::get_shutdown_depends_from(const std::vector<const daq::core::BaseApplication *>& all_apps) const
{
  return get_all_referenced(get_app_config(), get_base_app()->get_ShutdownDependsFrom(), all_apps);
}


std::vector<const daq::core::BaseApplication *>
daq::core::Segment::get_all_applications(std::set<std::string> * app_types, std::set<std::string> * segments, std::set<const daq::core::Computer *> * hosts) const
{
  // get all sub-types
  std::set<std::string> all_app_types;

  if (app_types)
    {
      if (app_types->empty())
        {
          app_types = nullptr;
        }
      else
        {
          const ::config::fmap<::config::fset>& all_scs(configuration().superclasses());

          for (const auto& i : *app_types)
            {
              all_app_types.insert(i);

              for (const auto& j : all_scs)
                {
                  if (j.second.find(&DalFactory::instance().get_known_class_name_ref(i)) != j.second.end())
                    {
                      all_app_types.insert(*j.first);
                    }
                }
            }

          app_types = &all_app_types;
        }
    }

  if (segments && segments->empty())
    segments = nullptr;

  if (hosts && hosts->empty())
    hosts = nullptr;

  std::vector<const daq::core::BaseApplication *> out;
  daq::core::AlgorithmUtils::get_applications(out, *this, app_types, segments, hosts);
  return out;
}

static std::vector<const daq::core::Tag*>
get_some_info(const daq::core::BaseApplication * this_app, std::list<const daq::core::Segment *>& s_list)
{
  std::vector<const daq::core::Tag*> tags;

  const daq::core::Partition& partition(*daq::core::AlgorithmUtils::get_partition(this_app));
  const daq::core::Segment * root_segment(partition.get_OnlineInfrastructure()->cast<daq::core::Segment>());
  const daq::core::Computer& host(*this_app->get_host());

  const daq::core::BaseApplication * base_app = this_app->get_base_app();
  const daq::core::Segment * segment = this_app->get_segment();

  ERS_DEBUG( 4, "  this           = " << this_app->UID() << "\n"
                "  partition      = " << &partition << "\n"
                "  parent segment = " << segment << "\n"
                "  host           = " << &host );

    {
      ERS_DEBUG( 4, "Building partition-segment[s] path to the application" );

      std::list<std::vector<const daq::core::Component *>> paths;
      segment->get_parents(partition, paths);

      for (auto& i : paths)
        i.push_back(segment);

      // If still not found then there is a problem
      if (paths.empty())
        {
          throw daq::core::BadApplicationInfo( ERS_HERE, this_app->UID(), "the application is not in the partition control tree" );
        }
      else if (paths.size() > 1)
        {
          std::ostringstream text;
          text << "there are " << paths.size() << " paths from the partition object " << &partition << ":\n";
          for (const auto& i : paths)
            {
              text << " * path including " << i.size() << " components:\n";
              for (const auto& j : i)
              text << "   - " << j << std::endl;
            }
          throw daq::core::BadApplicationInfo( ERS_HERE, this_app->UID(), text.str() );
        }

      std::vector<const daq::core::Component *>& path = paths.front();

      ERS_DEBUG( 5, "add " << root_segment << " as root segment for application " << this_app->UID());
      s_list.push_back(root_segment);

      if (path.size() != 1 || path[0]->UID() != root_segment->UID())
        {
          const std::vector<const daq::core::Segment*> * segs = &root_segment->get_nested_segments();

          for (const auto& i : path)
            {
              const daq::core::TemplateSegment * tseg = i->cast<daq::core::TemplateSegment>();

              if (const daq::core::Segment * seg = i->cast<daq::core::Segment>())
                {
                  const ConfigObjectImpl * seg_config_obj_implementation(seg->config_object().implementation());

                  for (const auto & j : *segs)
                    {
                      if (j->get_base_segment()->config_object().implementation() == seg_config_obj_implementation)
                        {
                          if (tseg != nullptr)
                            {
                              if (segment->UID() != j->UID())
                                {
                                  continue;
                                }
                            }

                          s_list.push_back(j);
                          segs = &j->get_nested_segments();
                          break;
                        }

                      if (&j == &segs->back())
                        {
                          std::ostringstream text;
                          text << "cannot find segment " << seg << " as nested child of " << &partition;
                          throw daq::core::BadApplicationInfo( ERS_HERE, this_app->UID(), text.str() );
                        }
                    }
                }
              else
                {
                  break;
                }
            }
        }
    }

  // If necessary, print out the segment path to application
  if (ers::debug_level() >= 4)
    {
      std::ostringstream text;
      for (std::list<const daq::core::Segment *>::reverse_iterator i = s_list.rbegin(); i != s_list.rend(); ++i)
        {
          text << " * segment " << (*i)->UID() << std::endl;
        }
      text << " * partition: " << &partition << std::endl;
      ERS_DEBUG(4, "PATH to application " << this_app->UID() << " is:\n" << text.str());
    }

  // Check Application has a Program linked to it and that program is linked to a sw repository
  try
    {
      base_app->get_Program()->get_BelongsTo(); // throw an exception if "Program" or "BelongsTo" is not set
    }
  catch (daq::config::Exception& ex)
    {
      throw daq::core::BadApplicationInfo( ERS_HERE, this_app->UID(), "failed to read application's Program object", ex );
    }


  // Get the Tags
  //   - get the tag or possible tags (Application ExplicitTag, DefaultTags
  //     from the segment list, Partition DefaultTags)
  //   - eliminate those which are not supported by the hw

  std::vector<const daq::core::Tag*> tempTags;

  // Get possible tags
  if (base_app->get_ExplicitTag()) {
    // Application ExplicitTag
    tempTags.push_back(base_app->get_ExplicitTag());
  } else {
    // DefaultTags from the segment list
    for (std::list<const daq::core::Segment *>::reverse_iterator i = s_list.rbegin(); i != s_list.rend(); ++i) {
      if(s_list.size() > 1 && *i == root_segment)
        {
          ERS_DEBUG(4, "skip default tags of " << root_segment << " for application " << this_app->UID());
          continue;
        }

      const auto& default_tags((*i)->get_DefaultTags());
      if (!default_tags.empty()) {
        tempTags.insert(tempTags.end(), default_tags.begin(), default_tags.end());
        ERS_DEBUG(4, "use default tags of " << *i << " for application " << this_app->UID());
        break;
      }
    }

    // Partition DefaultTags
    const auto& default_tags(partition.get_DefaultTags());
    if (!default_tags.empty() && tempTags.empty()) {
      tempTags.insert(tempTags.end(), default_tags.begin(), default_tags.end());
      ERS_DEBUG(4, "use default tags of " << &partition << " for application " << this_app->UID());
    }
  }

  // Report error if there are no possible tags
  if (tempTags.empty())
    throw daq::core::BadApplicationInfo( ERS_HERE, this_app->UID(), "there are no Tags defined for the application" ) ;

  // Remove tags which are not supported by the hardware
  {
    // Go through all tags and remove tags if not for this hardware
    for (const auto& i : tempTags)
      if (daq::core::is_compatible(*i, host, partition))
        tags.push_back(i);
      else
        ERS_DEBUG(6, "* remove tag " << i << " which is incompatible with the HW tag " << host.get_HW_Tag());

    // No Tags found to support this hardware
    if (tags.empty())
      {
        std::ostringstream text ;
        text << "application's and/or default tags (";
        for (const auto& i : tempTags)
          {
            if(i != *tempTags.begin()) text << ", ";
            text << i;
          }
        text << ") are not supported by the host " << &host << " (HW tag: \'" << host.get_HW_Tag() << "\')";
        throw daq::core::BadApplicationInfo( ERS_HERE, this_app->UID(), text.str() ) ;
      }


    // Print list of possible Tags
    if(ers::debug_level() >= 6)
      {
        std::ostringstream text;
        text <<" application's tags for " << this_app->UID() << " are:\n";
        for (const auto& i : tags)
          text << " * tag " << i << "\n";
        ERS_DEBUG(6,text.str());
      }
  }

  return tags;
}

static std::string
get_host_and_backup_list(const daq::core::BaseApplication * app)
{
  std::string s(app->get_host()->UID());

  std::vector<const daq::core::Computer *> backup_hosts = app->get_backup_hosts();

  for (const auto& x : backup_hosts)
    {
      s.push_back(',');
      s.append(x->UID());
    }

  return s;
}

const daq::core::Tag *
daq::core::BaseApplication::get_info(std::map<std::string, std::string>& environment, std::vector<std::string>& program_names, std::string & startArgs, std::string & restartArgs) const
{
  const daq::core::Tag * tag = nullptr;
  const daq::core::BaseApplication * base_app = get_base_app();
  const daq::core::ComputerProgram * program = base_app->get_Program();

  const daq::core::Partition& partition(*daq::core::AlgorithmUtils::get_partition(this));
  const daq::core::Computer& host(*get_host());

  std::list<const daq::core::Segment *> s_list;
  std::vector<const daq::core::Tag *> tags = get_some_info(this, s_list) ; // throw BadApplicationInfo

  std::vector<std::string> tmp_paths_to_shared_libraries;
  std::vector<std::string> tmp_search_paths;

  tmp_paths_to_shared_libraries.reserve(search_path_default_size);
  tmp_search_paths.reserve(paths_to_shared_libraries_default_size);

  for (unsigned int i = 0 ; i < tags.size(); i++) {
    try {
      get_parameters(program, program_names, tmp_search_paths, tmp_paths_to_shared_libraries, *(tags[i]), host, partition); // throw BadProgramInfo
      tag = tags[i];
      break;
    }
    catch(const daq::core::BadTag &ex) {
      const int debug_level = 3;
      if(ers::debug_level() >= debug_level) {
        std::ostringstream text;
        text << "cannot use tag " << tags[i];
        ers::debug( daq::core::BadApplicationInfo(ERS_HERE, UID(), text.str(), ex), debug_level);
      }

      if (i == tags.size()-1) {
        throw daq::core::BadApplicationInfo(ERS_HERE, UID(), "No program suited for the possible Tags found.", ex);
      }
    }
    catch(daq::core::BadProgramInfo &ex) {
      throw daq::core::BadApplicationInfo(ERS_HERE, UID(), "No program suited for the possible Tags found.", ex);
    }
    catch(daq::config::Exception& ex) {
      throw daq::core::BadApplicationInfo(ERS_HERE, UID(), "Failed to read application's parameters to get possible Tags." , ex);
    }

  }

  std::vector<std::string> search_paths;
  std::vector<std::string> paths_to_shared_libraries;

  search_paths.reserve(search_path_default_size);
  paths_to_shared_libraries.reserve(paths_to_shared_libraries_default_size);

  // Insert in front paths to shared libraries and search paths for the Uses relationship
  // of the Application object (recursively)
  {
    // Make binary tag strings
    BinaryInfo binary_info(*tag);

    // Check if the partition has RepositoryRoot (in this case copy first item from local copy of paths to libs)
    unsigned int idx = (partition.get_RepositoryRoot().empty() ? 0 : 1);
    unsigned int idx2 = idx * 2;
    if(idx == 1) {
      paths_to_shared_libraries.emplace_back(std::move(tmp_paths_to_shared_libraries[0]));
      search_paths.emplace_back(std::move(tmp_search_paths[0]));
      search_paths.emplace_back(std::move(tmp_search_paths[1]));
    }

    // Append paths to shared libraries from application's repositories
    try {
      daq::core::TestCircularDependency cd_fuse("application binary and library paths", this);
      for (const auto& i : get_Uses()) {
        get_paths(i, search_paths, paths_to_shared_libraries, binary_info, cd_fuse);
      }
    }
    catch(daq::core::FoundCircularDependency &ex) {
      throw daq::core::BadApplicationInfo(ERS_HERE, UID(), "Failed to get binary and library paths.", ex);
    }


    // Copy rest of paths to shared libraries from application's repositories
    while(idx < tmp_paths_to_shared_libraries.size()) {
      if(std::find(paths_to_shared_libraries.begin(), paths_to_shared_libraries.end(), tmp_paths_to_shared_libraries[idx]) == paths_to_shared_libraries.end()) {
        paths_to_shared_libraries.emplace_back(std::move(tmp_paths_to_shared_libraries[idx]));
      }
      idx++;
    }

    // Copy rest of search paths from application's repositories
    while(idx2 < tmp_search_paths.size()) {
      if(std::find(search_paths.begin(), search_paths.end(), tmp_search_paths[idx2]) == search_paths.end()) {
        search_paths.emplace_back(std::move(tmp_search_paths[idx2]));
      }
      idx2++;
    }
  }


    // set application's process environment

  environment.clear();

  try
  {
    add_front_partition_environment(environment, partition); // throw
    ERS_DEBUG( 5, "calculate " << this << " process environment:\n"
                  "add front " << &partition << " object environment\n"
                  << mk_app_env_string(environment) );

    // Application needs Environment
    add_env_vars(environment, get_ProcessEnvironment(), tag);
    ERS_DEBUG( 5, "calculate " << this << " process environment:\n"
                  "add " << this << " object environment\n"
                  << mk_app_env_string(environment) );

    // Application's ComputerProgram needs Environment
    add_env_vars(environment, program->get_ProcessEnvironment(), tag);
    ERS_DEBUG( 5, "calculate " << this << " process environment:\n"
                  "add " << program << " object environment\n"
                  << mk_app_env_string(environment) );

    // Segment list NeedsEnvironment
    std::map<std::string,std::string> parent_var_names;
    for (std::list<const daq::core::Segment *>::reverse_iterator i = s_list.rbegin(); i != s_list.rend(); ++i) {
      add_env_vars(environment, (*i)->get_ProcessEnvironment(), tag);

      for(const auto& j : (*i)->get_infrastructure()) {
        const daq::core::InfrastructureBase * ia = j->get_base_app()->cast<daq::core::InfrastructureBase>();
        const std::string& swv_name(ia->get_SegmentProcEnvVarName());
        if(!swv_name.empty()) {
          try {
              // add value of this segment-wide process environment variable
            const std::string value = (
              ia->get_SegmentProcEnvVarValue() == daq::core::InfrastructureBase::SegmentProcEnvVarValue::AppId ? j->UID() :
              ia->get_SegmentProcEnvVarValue() == daq::core::InfrastructureBase::SegmentProcEnvVarValue::RunsOn ? j->get_host()->UID() :
              get_host_and_backup_list(j)
            );

            ERS_DEBUG(6, j->get_base_app() << " adds segment-wide process environment " << swv_name << " => " << value);
            environment.emplace(swv_name, value);

              // check if one is looking for parent with this name; add and mark if found
            std::map<std::string,std::string>::iterator x = parent_var_names.find(swv_name);
            if(x != parent_var_names.end() && !x->second.empty()) {
              environment.emplace(x->second, value);
              ERS_DEBUG(6, j->get_base_app() << " adds parent segment-wide process environment " << x->second << " => " << value);
              x->second = "";
            }

              // add to parent search list
            const std::string& swv_parent_name(ia->get_SegmentProcEnvVarParentName());
            if(!swv_parent_name.empty()) {
              if(parent_var_names.emplace(swv_name,swv_parent_name).second == true) {
                ERS_DEBUG(6, j->get_base_app() << " requires to add parent segment-wide process environment " << swv_parent_name << " (set for " << swv_name << ')');
              }
            }
          }
          catch(ers::Issue& ex) {
            throw daq::core::BadApplicationInfo( ERS_HERE, UID(), "failed to build Application environment", ex ) ;
          }
        }
      }

      ERS_DEBUG( 5, "calculate " << this << " process environment:\n"
                    "add " << *i << " object environment\n"
                    << mk_app_env_string(environment) );
    }

    add_end_partition_environment(environment, partition, base_app, program, tag);
    ERS_DEBUG( 5, "calculate " << base_app << " process environment:\n"
                  "add end " << &partition << " object environment\n"
                  << mk_app_env_string(environment) );

    // Set Application ID and name variables
    add_env_var(environment, s_tdaq_application_object_id_str, base_app->UID());
    add_env_var(environment, s_tdaq_application_name_str, UID());

    ERS_DEBUG( 5, "final " << base_app << " process environment:\n"
                  "add TDAQ_APPLICATION_OBJECT_ID and TDAQ_APPLICATION_NAME variables to environment\n"
               << mk_app_env_string(environment)  );
  }
  catch  ( daq::config::Generic & ex ) {
    throw daq::core::BadApplicationInfo( ERS_HERE, UID(), "failed to build Application environment", ex ) ;
  }

    // Add "PATH" and "LD_LIBRARY_PATH" variables

  set_path(environment, s_path_str, search_paths);
  set_path(environment, s_ld_library_path_str, paths_to_shared_libraries);

  // Resolve the command line options
  static const std::string beg_env_str("env(");
  static const std::string end_env_str(")");
  std::string sa = program->get_DefaultParameters();
  sa.push_back(' ');
  std::string rsa = sa;
  sa.append(base_app->get_Parameters());
  rsa.append(base_app->get_RestartParameters());
  startArgs = daq::core::substitute_variables(sa, &environment, beg_env_str, end_env_str);
  restartArgs = daq::core::substitute_variables(rsa, &environment, beg_env_str, end_env_str);

  return tag;
}


std::vector<const daq::core::Computer *>
daq::core::AppConfig::get_backup_hosts() const
{
  if(const daq::core::Application * a = m_base_app->cast<daq::core::Application>())
    {
      std::vector<const daq::core::Computer *> result;
      add_computers(result, a->get_BackupHosts());
      return result;
    }
  else
    {
      return m_template_backup_hosts;
    }
}

std::vector<const daq::core::Computer *>
daq::core::BaseApplication::get_backup_hosts() const
{
  return get_app_config()->get_backup_hosts();
}

daq::core::ApplicationConfig::ApplicationConfig(::Configuration& db) :
    m_db(db), m_root_segment(nullptr)
{
  ERS_DEBUG(2, "construct the object " << (void *)this);
  m_db.add_action(this);
}

daq::core::ApplicationConfig::~ApplicationConfig()
{
  ERS_DEBUG(2, "destroy the object " << (void *)this);
  m_db.remove_action(this);
}

/******************************************************************************
 ******************* ALGORITHM BaseApplication::get_output_error_directory() **
 ******************************************************************************/

std::string
daq::core::Partition::get_log_directory() const
{
  ERS_DEBUG(4, " CALL " << this << "::get_log_directory()");

  std::string log_path = get_LogRoot();

  if (log_path.empty() == true)
    {
      log_path = "/tmp/logs";
    }

  // Add to this base path the partition name
  log_path += "/" + UID();

  return log_path;
}


// adds Parameters used in segment to the vector
static void
add_parameters( std::vector<const daq::core::Parameter*>& params, const daq::core::Segment& segment, daq::core::TestCircularDependency& cd_fuse )
{
  const std::vector<const daq::core::Parameter*>& seg_params(segment.get_Parameters());
  params.insert(params.end(), seg_params.begin(), seg_params.end() ) ;

  for(const auto& i : segment.get_Segments())
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
      add_parameters(params, *i, cd_fuse) ;
    }
}

static void
add_vars(std::map<std::string, std::string>& cvt_map, const std::vector<const daq::core::Parameter*>& params, daq::core::TestCircularDependency& cd_fuse)
{
  for (const auto& i : params)
    {
      if (const daq::core::Variable * var = i->cast<daq::core::Variable>())
        {
          const std::string& v = var->get_value(); // note, algorithm is used here; it returns empty value for multi-value variable
          cvt_map[var->get_Name()] = v;
          if (v.find('$') != std::string::npos || var->get_Name().find('$') != std::string::npos)
            {
              const_cast<daq::core::Variable *>(var)->DalObject::unread();
            }
        }
      else if (const daq::core::VariableSet * vars = i->cast<daq::core::VariableSet>())
        {
          daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, vars);
          add_vars(cvt_map, vars->get_Contains(), cd_fuse);
        }
    }
}

void
daq::core::SubstituteVariables::reset(const Partition& p)
{
  m_cvt_map.clear();

  m_cvt_map[s_tdaq_partition_str] = p.UID();                      // insert name-of-partition parameter
  m_cvt_map["TDAQ_LOGS_ROOT"] = p.get_LogRoot();                  // insert partition log root
  m_cvt_map["TDAQ_LOGS_PATH"] = p.get_LogRoot() + "/" + p.UID() ; // insert TDAQ logs path

  try
    {
      daq::core::TestCircularDependency cd_fuse("segments substitution parameters", &p);
      std::vector<const daq::core::Parameter*> params = p.get_Parameters();

      if (const daq::core::Segment* oseg = p.get_OnlineInfrastructure())
        {
          daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, oseg);
          add_parameters(params, *oseg, cd_fuse);
        }

      for (const auto& i : p.get_Segments())
        {
          daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
          add_parameters(params, *i, cd_fuse);
        }

      add_vars(m_cvt_map, params, cd_fuse);

      // get list of used sw repositories and add parameters, referencing their installation
      for (const auto& i : get_used_repositories(p))
        {
          const std::string& rn = i->get_InstallationPathVariableName();
          if (!rn.empty())
            {
              if(m_cvt_map.emplace(rn, i->get_InstallationPath()).second == true)
                {
                  ERS_DEBUG(4, "add substitute variables conversion map parameter " << rn << "=\'" << i->get_InstallationPath() << "\' defined by object " << *i);
                }
              else
                {
                  std::ostringstream text;
                  text << "substitute variables conversion map parameter " << rn << "=\'" << i->get_InstallationPath() << "\' defined by object " << *i << " was already defined; check configuration database";
                  ers::warning(daq::core::BadVariableUsage(ERS_HERE, text.str()));
                }
            }
        }
    }
  catch(ers::Issue& ex)
    {
      throw daq::config::Generic(ERS_HERE, "Failed to substitute parameters from the database", ex);
    }

  // Recursive substitution of the variables before they are used
  static const std::string beg_str("${");
  static const std::string end_str("}");
  std::string subst_s;
  std::map<std::string, std::string>::const_iterator map_iter = m_cvt_map.begin();
  while (map_iter != m_cvt_map.end())
    {
      try
        {
          subst_s = substitute_variables(map_iter->second, &m_cvt_map, beg_str, end_str);
        }
      catch (daq::config::Exception& ex)
        {
          std::ostringstream text;
          text << "Failed to calculate variable \'" << map_iter->first << '\'';
          throw daq::config::Generic(ERS_HERE, text.str().c_str(), ex);
        }

      if (subst_s != map_iter->second)
        {
          m_cvt_map[map_iter->first] = std::move(subst_s);
        }
      map_iter++;
    }

  if (ers::debug_level() >= 3)
    {
      std::ostringstream text;
      text << "Variables substitution map contains " << m_cvt_map.size() << " entries:\n";
      map_iter = m_cvt_map.begin();
      while (map_iter != m_cvt_map.end())
        {
          text << " [" << map_iter->first << "] => " << map_iter->second << std::endl;
          map_iter++;
        }

      ERS_DEBUG(3, text.str());
    }

  // unread all objects, which may appear in template cache

  p.configuration().unread_template_objects();
}

void
daq::core::SubstituteVariables::convert(std::string& s, const Configuration&, const ConfigObject& o, const std::string& a)
{
  ERS_DEBUG(5, "convert attribute \'" << a << "\' value \'" << s << "\' of object " << &o);

  static const std::string beg_str("${");
  static const std::string end_str("}");

  s = substitute_variables(s, &m_cvt_map, beg_str, end_str);

  ERS_DEBUG(5, "return value \'" << s << '\'');
}

std::string
daq::core::substitute_variables(const std::string& str_from, const std::map<std::string, std::string> * cvs_map, const std::string& beg, const std::string& end)
{
  std::string s(str_from);

  std::string::size_type pos = 0;       // position of tested string index
  std::string::size_type p_start = 0;   // beginning of variable
  std::string::size_type p_end = 0;     // beginning of variable

  int subst_count(1);
  const int max_subst(128);             // max allowed number of substitutions

  while(
   ((p_start = s.find(beg, pos)) != std::string::npos) &&
   ((p_end = s.find(end, p_start + beg.size())) != std::string::npos)
  ) {
    std::string var(s, p_start + beg.size(), p_end - p_start - beg.size());

    if(++subst_count > max_subst) {
      std::ostringstream text;
      text << "Value \'" << str_from << "\' has exceeded the maximum number of substitutions allowed (" << max_subst << "). "
              "It might have a circular dependency with substitution variables. "
              "After " << max_subst << " substitutions it is \'" << s << '\'';
      throw daq::config::Generic(ERS_HERE, text.str().c_str());
    }

    if(cvs_map) {
      std::map<std::string, std::string>::const_iterator j = cvs_map->find(var);
      if(j != cvs_map->end()) {
        s.replace(p_start, p_end - p_start + end.size(), j->second);
      }
    }
    else {
      if(char * env = getenv(var.c_str())) {
        s.replace(p_start, p_end - p_start + end.size(), env);
      }
      else {
        std::ostringstream text;
        text << "substitution failed for parameter \'" << std::string(s, p_start, p_end - p_start + end.size()) << '\'';
        throw daq::config::Generic(ERS_HERE, text.str().c_str());
      }
    }

    pos = p_start + 1;
  }

  return s;
}

////////////////////////////////////////////////////////////////////////////////////

  // class Tokenizer is used for easy parsing of string containing several tokens

namespace dal {
class Tokenizer {

  public:

    Tokenizer(const std::string&, const char *);
    std::string next();

  private:

    std::string p_string;
    const char * p_delimeters;
    std::string::size_type p_idx;

};

Tokenizer::Tokenizer(const std::string& s, const char * d) :
  p_string(s),
  p_delimeters(d),
  p_idx(p_string.find_first_not_of(p_delimeters))
{
}

std::string
Tokenizer::next()
{
  if(p_idx == std::string::npos) return "";
  std::string::size_type end_idx = p_string.find_first_of(p_delimeters, p_idx);
  if(end_idx == std::string::npos) end_idx=p_string.length();
  std::string token = p_string.substr(p_idx, end_idx - p_idx);
  p_idx = p_string.find_first_not_of(p_delimeters, end_idx);
  return token;
}
}

////////////////////////////////////////////////////////////////////////////////////

const daq::core::Partition *
daq::core::get_partition(::Configuration& conf, const std::string& pname, unsigned long rlevel, const std::vector<std::string> * rclasses)
{
  static std::set<std::string> s_already_processed_partitions; // keep list of already processed partitions
  static std::mutex s_mutex;

  std::string name = pname;

  if (name.empty())
    {
      s_already_processed_partitions.clear(); // re-set the already processed partitions list to allow re-read referenced objects, when it is needed

      if (const char * s = getenv("TDAQ_PARTITION"))
        {
          name = s;
        }
      else
        {
          ers::error(daq::core::BadPartitionID(ERS_HERE, name, std::runtime_error("No partition UID provided. What is the UID of the partition you are looking for?")));
          return nullptr;
        }
    }

  // code below can be used for performance measurements

  if (const char * rl = get_env("DAL_GET_PARTITION_REF_LEVEL"))
    {
      rlevel = atoi(rl);
      ERS_DEBUG(3, " set ref-level parameter = " << rlevel << " (was read from non-empty environment variable \"DAL_GET_PARTITION_REF_LEVEL\")");
    }

  // reset reference parameter if the method was called for this partition

  std::ostringstream cname_ss;
  cname_ss << name << '@' << (void *)&conf;
  const std::string cname(cname_ss.str());

  std::lock_guard<std::mutex> scoped_lock(s_mutex);

  if (s_already_processed_partitions.find(cname) != s_already_processed_partitions.end())
    {
      rlevel = 0;
      ERS_DEBUG(3, " set ref-level parameter = 0 (the method get_partition() has been called already for partition@configuration \'" << cname << "\')");
    }
  else
    {
      s_already_processed_partitions.insert(cname);
    }

  std::vector<std::string> dummu;

  if (rlevel)
    {
      if (const char * rn = get_env("DAL_GET_PARTITION_REF_CLASS_NAMES"))
        {
          rclasses = &dummu;
          dal::Tokenizer t(rn, ",");
          std::string token;
          std::string names;
          while (!(token = t.next()).empty())
            {
              dummu.push_back(token);
              names += token + '\n';
            }
          ERS_DEBUG(3, " set ref-class-names parameter = " << names << " (was read from non-empty environment variable \"DAL_GET_PARTITION_REF_CLASS_NAMES\")");
        }
      else if (getenv("DAL_GET_PARTITION_AVOID_DEF_REF_CLASS_NAMES") == nullptr)
        {
          rclasses = &dummu;
          dummu =
            {
              daq::core::Partition::s_class_name,        // the partition object
              daq::core::Tag::s_class_name,              // tags, tags mappings (always needed)
              daq::core::Segment::s_class_name,          // segments
              daq::core::ResourceSet::s_class_name,      // resource sets
              daq::core::Rack::s_class_name,             // racks
              daq::core::BaseApplication::s_class_name,  // applications
              daq::core::ComputerBase::s_class_name,     // computers and computer sets
              daq::core::SW_Package::s_class_name,       // sw repositories, packages
              daq::core::SW_Object::s_class_name,        // sw objects
              daq::core::BinaryFile::s_class_name,       // sw object extensions
              daq::core::Parameter::s_class_name         // parameters for substitution
            };
          ERS_DEBUG(3, " set default ref-class-names parameter (to get info about applications)");
        }
    }

  const daq::core::Partition * p = conf.get<daq::core::Partition>(name, false, true, rlevel, rclasses);

  if (!p)
    {
      ers::error(daq::core::BadPartitionID(ERS_HERE, name));
    }

  return p;
}


////////////////////////////////////////////////////////////////////////////////////


  // add vector of repository objects to the set of repositories

static void
add_repositories(std::set<const daq::core::SW_Repository *>& repositories, const std::vector<const daq::core::SW_Package*>& reps, daq::core::TestCircularDependency& cd_fuse)
{
  for (const auto& i : reps)
    {
      if (const daq::core::SW_Repository * r = i->cast<daq::core::SW_Repository>())
        repositories.insert(r);

      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
      add_repositories(repositories, i->get_Uses(), cd_fuse);
    }
}


  // process repositories linked with application

static void
process_application(std::set<const daq::core::SW_Repository *>& repositories, const daq::core::BaseApplication * a)
{
  if (a)
    {
      try
        {
          daq::core::TestCircularDependency cd_fuse("used repositories", a);

          // add repositories used by application
          add_repositories(repositories, a->get_Uses(), cd_fuse);

          // add repositories linked with the application's program
          if (const daq::core::ComputerProgram * p = a->get_Program())
            {
              if (const daq::core::SW_Repository * r = p->get_BelongsTo())
                repositories.insert(r);

              daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, p);
              add_repositories(repositories, p->get_Uses(), cd_fuse);
            }

        }
      catch (ers::Issue& ex)
        {
          ers::error(daq::core::BadApplicationInfo(ERS_HERE,a->UID(),"db problem",ex));
        }
    }
}


  // process applications of segment

static void
process_segment(std::set<const daq::core::SW_Repository *>& repositories, const daq::core::Segment& s, daq::core::TestCircularDependency& cd_fuse)
{
  // check segment's controller
  process_application(repositories, s.get_IsControlledBy()->cast<daq::core::BaseApplication>());

  // check segment's applications, which are not resources
  for (const auto & i : s.get_Applications())
    process_application(repositories, i);

  // check segment's infrastructure applications
  for (const auto& i : s.get_Infrastructure())
    process_application(repositories, i->cast<daq::core::BaseApplication>());

  // add segment's resource applications
  for (const auto& i : s.get_Resources())
    for (const auto& j : get_resource_applications(i))
      process_application(repositories, j);

  // process applications from nested segments
  for (const auto& i : s.get_Segments())
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
      process_segment(repositories, *i, cd_fuse);
    }
}

std::set<const daq::core::SW_Repository *>
daq::core::get_used_repositories(const daq::core::Partition& p)
{
  std::set<const daq::core::SW_Repository *> repositories;

  daq::core::TestCircularDependency cd_fuse("used segments and repositories", &p);

  if (const daq::core::OnlineSegment * online_segment = p.get_OnlineInfrastructure())
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, online_segment);
      process_segment(repositories, *online_segment, cd_fuse);

      for (const auto &a : p.get_OnlineInfrastructureApplications())
        process_application(repositories, a);
    }

  for (const auto& i : p.get_Segments())
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
      process_segment(repositories, *i, cd_fuse);
    }

  return repositories;
}

static void
check_file_exists(const std::string& path, std::string & file)
{
  ERS_DEBUG(6, "try path \'" << path << '\'');

  struct stat buffer;
  if (stat(path.c_str(), &buffer) == 0)
    file = path;
}


void
daq::core::add_classpath(const daq::core::SW_Repository& rep, const std::string& user_dir, std::string& class_path)
{
  for (const auto& j : rep.get_SW_Objects())
    {
      if (const daq::core::JarFile *jf = j->cast<daq::core::JarFile>())
        {
          std::string file;

          const std::string& bn = jf->get_BinaryName();

          // check file name is an absolute path
          if (bn[0] == '/')
            check_file_exists(bn, file);

          // check user dir (Repository Root)
          if (file.empty() && !user_dir.empty())
            check_file_exists(make_path(user_dir, s_share_lib, bn), file);

          // check patch area
          if (file.empty() && !rep.get_PatchArea().empty())
            check_file_exists(make_path(rep.get_PatchArea(), s_share_lib, bn), file);

          if (file.empty())
            check_file_exists(make_path(rep.get_InstallationPath(), s_share_lib, bn), file);

          if (file.empty())
            {
              ers::error(daq::core::NoJarFile(ERS_HERE, bn, j->UID(), j->class_name(), rep.UID(), rep.class_name()));
            }
          else
            {
              if (!class_path.empty())
                class_path.push_back(':');
              class_path.append(file);
            }
        }
    }
}

/******************************************************************************
********** ALGORITHMS get_config_version() and get_config_version() ***********
******************************************************************************/

static std::string cv_info_name("RunParams.ConfigVersion");


std::string
daq::core::get_config_version(const std::string& partition)
{
  IPCPartition p(partition);
  ISInfoDictionary dict(p);

  ConfigVersion cv;
  cv.Version = "";

  try
    {
      ERS_DEBUG(1, "try to read IS value " << cv_info_name);
      dict.findValue(cv_info_name, cv);
    }
  catch (const daq::is::InfoNotFound & ex)
    {
      throw daq::config::NotFound(ERS_HERE, "is value", cv_info_name.c_str(), ex);
    }
  catch (const daq::is::RepositoryNotFound & ex)
    {
      throw daq::config::NotFound(ERS_HERE, "is repository", cv_info_name.c_str(), ex);
    }
  catch (const daq::is::Exception & ex)
    {
      std::ostringstream text;
      text << "failed to read IS object \"" << cv_info_name << '\"';
      throw daq::config::Generic(ERS_HERE, text.str().c_str(), ex);
    }

  if (!cv.Version.empty())
    return cv.Version;

  if(const char * env = getenv(s_tdaq_db_version_str.c_str()))
    return env;

  return cv.Version; // empty string
}

void
daq::core::set_config_version(const std::string& partition, const std::string& version, bool reload)
{
  IPCPartition p(partition);
  ISInfoDictionary dict(p);

  try
    {
      if(version.empty())
        {
          ERS_DEBUG(1, "remove " << cv_info_name);
          dict.remove(cv_info_name);
        }
      else
        {
          ConfigVersion cv;
          cv.Version = version;
          ERS_DEBUG(1, "write " << version << " to " << cv_info_name);
          dict.checkin(cv_info_name, cv);
        }
    }
  catch (const daq::is::RepositoryNotFound & ex)
    {
      throw daq::config::NotFound(ERS_HERE, "is repository", cv_info_name.c_str(), ex);
    }
  catch (const daq::is::Exception & ex)
    {
      std::ostringstream text;
      text << "failed to " << (version.empty() ? "remove" : "check-in") << " IS object \"" << cv_info_name << '\"';
      throw daq::config::Generic(ERS_HERE, text.str().c_str(), ex);
    }


  if (reload)
    {
      const bool is_initial(partition == ::ipc::partition::default_name);
      const char * rdb_server_name(is_initial ? "RDB_INITIAL" : "RDB");

      try
        {
          ::rdb::RDBNameList files;
          files.length(1);
          files[0] = CORBA::string_dup(version.c_str());
          ERS_DEBUG(1, "reload \"" << rdb_server_name << '@' << partition << "\" with value " << version);
          p.lookup<::rdb::cursor>(rdb_server_name)->reload_database(files);

          if (!is_initial)
            {
              rdb_server_name = "RDB_RW";
              ERS_DEBUG(1, "reload \"" << rdb_server_name << '@' << partition << "\" with value " << version);
              p.lookup<::rdb::writer>(rdb_server_name)->reload_database(files);
            }
        }
      catch (CORBA::SystemException & ex)
        {
          std::ostringstream text;
          text << "failed to reload rdb server \"" << rdb_server_name << '@' << partition << '\"';
          throw daq::config::Generic(ERS_HERE, text.str().c_str(), daq::ipc::CorbaSystemException(ERS_HERE, &ex));
        }
      catch (::rdb::CannotProceed & ex )
        {
          std::ostringstream text;
          text << "failed to reload rdb server \"" << rdb_server_name << '@' << partition << "\": " << ex.text;
          throw daq::config::Generic(ERS_HERE, text.str().c_str());
        }
      catch ( const daq::ipc::Exception & ex)
        {
          std::ostringstream text;
          text << "lookup failed for rdb server \"" << rdb_server_name << '@' << partition << '\"';
          throw daq::config::Generic(ERS_HERE, text.str().c_str(), ex);
        }
    }
}

std::string
daq::core::Partition::get_config_version()
{
  return ::daq::core::get_config_version(UID());
}

void
daq::core::Partition::set_config_version(const std::string& version, bool reload)
{
  ::daq::core::set_config_version(UID(), version, reload);
}

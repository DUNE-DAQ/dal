#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include "config/ConfigObject.h"
#include "config/Configuration.h"
#include "config/ConfigurationPointer.h"

#include "dal/BaseApplication.h"
#include "dal/Partition.h"
#include "dal/ComputerProgram.h"
#include "dal/Tag.h"
#include "dal/Computer.h"
#include "dal/OnlineSegment.h"
#include "dal/TemplateSegment.h"
#include "dal/TemplateApplication.h"
#include "dal/ResourceBase.h"
#include "dal/Resource.h"
#include "dal/Variable.h"
#include "dal/util.h"

#include <string>

//
// This is the C++ helper function that will instantiate the correct DAL objects
// in the C++ world and then call the DAL algorithm and return the result.
//
// Compared to the original C++ algorithm it has one additional parameter for
// the configuration database itself.
//
// Objects in the database are never passed directly from Python to C++, just their name
// is given as a string. This is enough to retrieve the corresponding C++ object
// from the database.
//
// We also use this wrapper to return the result of the DAL algorithm directly as string
// rather than passing in a string reference that is modified. The latter is not very
// Pythonic...
//


namespace
{

  class Helper_class
  {

  public:

    std::string m_id;
    std::string m_class_name;
    Helper_class(const std::string id, const std::string class_name) :
        m_id(id), m_class_name(class_name)
    {
      ;
    }
  };

  class AppConfigHelper
  {

  public:

    AppConfigHelper(const daq::core::BaseApplication * app) :
        m_app(app)
    {
    }

    const std::string&
    get_app_id() const
    {
      return m_app->UID();
    }

    Helper_class *
    get_base_app() const
    {
      return new Helper_class(m_app->get_base_app()->UID(), m_app->get_base_app()->class_name());
    }

    Helper_class *
    get_host() const
    {
      return new Helper_class(m_app->get_host()->UID(), m_app->get_host()->class_name());
    }

    Helper_class *
    get_base_seg() const
    {
      return new Helper_class(m_app->get_segment()->get_base_segment()->UID(), m_app->get_segment()->get_base_segment()->class_name());
    }

    const std::string&
    get_seg_id()
    {
      return m_app->get_segment()->UID();
    }

    boost::python::list
    get_backup_hosts() const
    {
      boost::python::list result;

      for (const auto& i : m_app->get_backup_hosts())
        result.append(Helper_class(i->UID(), i->class_name()));

      return result;
    }

    boost::python::list
    get_initialization_depends_from(boost::python::list all_apps) const
    {
      std::vector<const daq::core::BaseApplication *> apps;

      const boost::python::ssize_t n = boost::python::len(all_apps);
      for (boost::python::ssize_t i = 0; i < n; i++)
        {
          AppConfigHelper& tmp = boost::python::extract<AppConfigHelper&>(PyList_GetItem(all_apps.ptr(), i));
          apps.emplace_back(tmp.m_app);
        }

      boost::python::list result;

      for (const auto& x : m_app->get_initialization_depends_from(apps))
        result.append(AppConfigHelper(x));

      return result;
    }

    boost::python::list
    get_shutdown_depends_from(boost::python::list all_apps) const
    {
      std::vector<const daq::core::BaseApplication *> apps;

      const boost::python::ssize_t n = boost::python::len(all_apps);
      for (boost::python::ssize_t i = 0; i < n; i++)
        {
          AppConfigHelper& tmp = boost::python::extract<AppConfigHelper&>(PyList_GetItem(all_apps.ptr(), i));
          apps.emplace_back(tmp.m_app);
        }

      boost::python::list result;

      for (const auto& x : m_app->get_shutdown_depends_from(apps))
        result.append(AppConfigHelper(x));

      return result;
    }

    bool
    is_templated() const
    {
      return m_app->is_templated();
    }

    // FIXME: 2019-01-11 remove partition_name parameter
    boost::python::dict
    get_info()
    {
      std::map<std::string, std::string> environment;
      std::vector<std::string> program_names;
      std::string startArgs, restartArgs;

      const daq::core::Tag * tag = m_app->get_info(environment, program_names, startArgs, restartArgs);

      boost::python::dict dictionary;

      dictionary["tag"] = tag->UID();

      boost::python::dict env;
      for (auto & i : environment)
          env[i.first] = i.second;
      dictionary["environment"] = env;

      boost::python::list program_list;
      for (auto & i : program_names)
          program_list.append(i);
      dictionary["programNames"] = program_list;

      dictionary["startArgs"] = startArgs;
      dictionary["restartArgs"] = restartArgs;

      return dictionary;
    }

    const daq::core::BaseApplication * m_app;
  };

  class SegConfigHelper
  {

  private:

    static boost::python::list
    vec2list(const std::vector<const daq::core::BaseApplication *>& vec)
    {
      boost::python::list result;

      for (const auto& x : vec)
        result.append(AppConfigHelper(x));

      return result;
    }

  public:

    SegConfigHelper(const daq::core::Segment * seg) :
        m_segment(seg)
    {
    }

    const std::string&
    get_seg_id() const
    {
      return m_segment->UID();
    }

    boost::python::list
    get_all_applications(std::set<std::string> * app_types = nullptr, std::set<std::string> * use_segments = nullptr, std::set<const daq::core::Computer *> * use_hosts = nullptr) const
    {
      return vec2list(m_segment->get_all_applications(app_types, use_segments, use_hosts));
    }

    AppConfigHelper *
    get_controller() const
    {
      return new AppConfigHelper(m_segment->get_controller());
    }

    boost::python::list
    get_infrastructure() const
    {
      return vec2list(m_segment->get_infrastructure());
    }

    boost::python::list
    get_applications() const
    {
      return vec2list(m_segment->get_applications());
    }

    boost::python::list
    get_nested_segments()
    {
      boost::python::list result;

      for (const auto& x : m_segment->get_nested_segments())
        result.append(SegConfigHelper(x));

      return result;
    }

    boost::python::list
    get_hosts() const
    {
      boost::python::list result;

      for (const auto& i : m_segment->get_hosts())
        result.append(Helper_class(i->UID(), i->class_name()));

      return result;
    }

    Helper_class *
    get_base_seg() const
    {
      return new Helper_class(m_segment->get_base_segment()->UID(), m_segment->get_base_segment()->class_name());
    }

    bool
    is_disabled() const
    {
      return m_segment->is_disabled();
    }

    bool
    is_templated() const
    {
      return m_segment->is_templated();
    }

    boost::python::dict
    get_timeouts()
    {
      int actionTimeout, shortActionTimeout;

      m_segment->get_timeouts(actionTimeout, shortActionTimeout);

      boost::python::dict dictionary;
      dictionary["actionTimeout"] = actionTimeout;
      dictionary["shortActionTimeout"] = shortActionTimeout;

      return dictionary;
    }


    const daq::core::Segment * m_segment;
  };

  std::string
  partition_get_log_directory(const std::string& self_name, python::ConfigurationPointer& db)
  {
    return db.get()->get<daq::core::Partition>(self_name)->get_log_directory();
  }

  boost::python::list
  component_get_parents(const std::string& self_name, python::ConfigurationPointer& db, const std::string& partition_name)
  {
    Configuration *conf = db.get();

    const daq::core::Component *self = conf->get<daq::core::Component>(self_name);

    const daq::core::Partition* partition = conf->get<daq::core::Partition>(partition_name);

    std::list<std::vector<const daq::core::Component *>> parents;
    self->get_parents(*partition, parents);

    boost::python::list parent_ids;

    for (auto & i : parents)
      {
        boost::python::list small_list;

        for (auto & j : i)
          small_list.append(Helper_class(j->UID(), j->class_name()));

        parent_ids.append(small_list);
      }

    return parent_ids;
  }

  bool
  component_disabled(const std::string& self_name, python::ConfigurationPointer& db, const std::string& partition_name)
  {
    Configuration *conf = db.get();

    return conf->get<daq::core::Component>(self_name)->disabled(*conf->get<daq::core::Partition>(partition_name));
  }

  boost::python::list
  computer_program_get_info(const std::string& self_name, python::ConfigurationPointer& db, const std::string& partition_name, const std::string& tag_name, const std::string& host_name)
  {
    Configuration *conf = db.get();

    std::map < std::string, std::string > environment;
    std::vector < std::string > program_names;

    conf->get<daq::core::ComputerProgram>(self_name)->get_info(
        environment,
        program_names,
        *conf->get<daq::core::Partition>(partition_name),
        *conf->get<daq::core::Tag>(tag_name),
        *conf->get<daq::core::Computer>(host_name));

    boost::python::list result;
    boost::python::dict dictionary;

    for (typename std::map<std::string, std::string>::iterator iter = environment.begin(); iter != environment.end(); ++iter)
      dictionary[iter->first] = iter->second;

    boost::python::list mini_list;
    for (auto & i : program_names)
      mini_list.append(i);

    result.append(dictionary);
    result.append(mini_list);

    return result;
  }

  std::string
  variable_get_value(const std::string& self_name, python::ConfigurationPointer& db, const std::string& tag_name)
  {
    Configuration *conf = db.get();

    return conf->get<daq::core::Variable>(self_name)->get_value(conf->get<daq::core::Tag>(tag_name));
  }

  boost::python::list
  partition_get_all_applications(const std::string& self_name, python::ConfigurationPointer& db, boost::python::list app_types, boost::python::list use_segments, boost::python::list use_hosts)
  {
    Configuration *conf = db.get();

    const daq::core::Partition *self = daq::core::get_partition(*conf, self_name);

    std::set<std::string> app_types_set;
    std::set<std::string> use_segments_set;
    std::set<const daq::core::Computer *> use_hosts_set;

    for (unsigned int i = 0; i < boost::python::len(app_types); i++)
      app_types_set.insert(boost::python::extract<std::string>(app_types[i]));

    for (unsigned int i = 0; i < boost::python::len(use_segments); i++)
      use_segments_set.insert(boost::python::extract<std::string>(use_segments[i]));

    for (unsigned int i = 0; i < boost::python::len(use_hosts); i++)
      use_hosts_set.insert(conf->get<daq::core::Computer>(boost::python::extract<std::string>(use_hosts[i])));

    boost::python::list result;

    for (const auto& a : self->get_all_applications(&app_types_set, &use_segments_set, &use_hosts_set))
      result.append(AppConfigHelper(a));

    return result;
  }

  SegConfigHelper *
  partition_get_segment(const std::string& self_name, python::ConfigurationPointer& db, const std::string& seg_name)
  {
    return new SegConfigHelper(daq::core::get_partition(*db.get(), self_name)->get_segment(seg_name));
  }



BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(get_all_applications_overloads, get_all_applications, 0, 3)

BOOST_PYTHON_MODULE(libdal_algo_helper)
{
    // Helper class is visible on the python side
    using namespace boost::python;
    class_<Helper_class > ("Helper_class", init<const std::string&, const std::string&>())
    .def_readwrite("id", &Helper_class::m_id)
    .def_readwrite("className", &Helper_class::m_class_name);

    const char * app_config_helper_class_description =
        "The proxy object to access properties of an application.\n"
        "Such objects are initialized by the DAL algorithms dal.Partition.get_all_applications or dal.Partition.get_segment and include normal and template applications.\n";

    class_<AppConfigHelper>("AppConfig", app_config_helper_class_description, init<const daq::core::BaseApplication *>())
    .def("get_app_id", &AppConfigHelper::get_app_id, return_value_policy<copy_const_reference>(), "Get application id")
    .def("get_base_app_helper", &AppConfigHelper::get_base_app, return_value_policy<manage_new_object>(), "Get base application object (i.e. used to create template application)")
    .def("get_host_helper", &AppConfigHelper::get_host, return_value_policy<manage_new_object>(), "Get host where application is running")
    .def("get_base_seg_helper", &AppConfigHelper::get_base_seg, return_value_policy<manage_new_object>(), "Get base segment object (i.e. used to create template segment)")
    .def("get_seg_id", &AppConfigHelper::get_seg_id, return_value_policy<copy_const_reference>(), "Get segment id this application belongs to")
    .def("get_backup_hosts_helper", &AppConfigHelper::get_backup_hosts, "Get backup hosts")
    .def("get_initialization_depends_from_helper", &AppConfigHelper::get_initialization_depends_from, "Get applications need to be started before this application; pass as a parameter all applications for given segment or partition", args("all_apps"))
    .def("get_shutdown_depends_from_helper", &AppConfigHelper::get_shutdown_depends_from, "Get applications need to be running before this application is shut down; pass as a parameter all applications for given segment or partition", args("all_apps"))
    .def("is_templated", &AppConfigHelper::is_templated, "Return true if this application is a template application")
    .def("get_info_helper", &AppConfigHelper::get_info, "Get information required to run the application")
    ;

    const char * seg_config_helper_class_description =
        "The proxy object to access properties of a segment.\n"
        "Such objects are initialized by the DAL algorithm dal.Partition.get_segment and include normal and template segments.\n";

    const char * seg_config_get_all_applications_description =
        "Get all applications for given and nested segments.\n"
        "The parameters allows to select applications by:\n"
        " * app_types - names of the base application classes,\n"
        " * use_segments - return applications for given segments,\n"
        " * use_hosts - return applications running on given hosts.\n"
        "The method is used to implement partition's get_all_applications() algorithm returning description of all applications running in the partition.\n"
        "In this case the algorithm is run on partition's online segment.";

    class_<SegConfigHelper>("SegConfig", seg_config_helper_class_description, init<const daq::core::Segment *>())
    .def("get_seg_id", &SegConfigHelper::get_seg_id, return_value_policy<copy_const_reference>(), "get segment id")
    .def("get_all_applications_helper", &SegConfigHelper::get_all_applications, get_all_applications_overloads(seg_config_get_all_applications_description, args("app_types", "use_segments", "use_hosts"))[with_custodian_and_ward<1,3>()])
    .def("get_controller_helper", &SegConfigHelper::get_controller, return_value_policy<manage_new_object>(), "Get segment controller")
    .def("get_infrastructure_helper", &SegConfigHelper::get_infrastructure, "Get segment infrastructure applications")
    .def("get_applications_helper", &SegConfigHelper::get_applications, "Get segment applications")
    .def("get_nested_segments_helper", &SegConfigHelper::get_nested_segments, "Get nested segments")
    .def("get_hosts_helper", &SegConfigHelper::get_hosts, "Get hosts for given segment")
    .def("get_base_seg_helper", &SegConfigHelper::get_base_seg, return_value_policy<manage_new_object>(), "Get base segment object (i.e. used to create template segment)")
    .def("is_templated", &SegConfigHelper::is_templated, "Return true if this segment is a template segment")
    .def("is_disabled", &SegConfigHelper::is_disabled, "Return true if this segment is disabled")
    .def("get_timeouts", &SegConfigHelper::get_timeouts, "Return run control action timeouts")
    ;

    // Function bindings
    def("partition_get_log_directory", partition_get_log_directory);
    def("component_get_parents", component_get_parents);
    def("component_disabled", component_disabled);
    def("computer_program_get_info", computer_program_get_info);
    def("variable_get_value", variable_get_value);
    def("partition_get_all_applications", partition_get_all_applications);
    def("partition_get_segment", partition_get_segment, return_value_policy<manage_new_object>());
}

}

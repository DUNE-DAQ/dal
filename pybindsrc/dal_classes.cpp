/**
 * @file dal_classes.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "dal_pybind_utils.hpp"

#include "dal/BaseApplication.hpp"
#include "dal/Partition.hpp"
#include "dal/ComputerProgram.hpp"
#include "dal/Tag.hpp"
#include "dal/Computer.hpp"
#include "dal/OnlineSegment.hpp"
#include "dal/TemplateSegment.hpp"
#include "dal/TemplateApplication.hpp"
#include "dal/ResourceBase.hpp"
#include "dal/Resource.hpp"
#include "dal/Variable.hpp"
#include "dal/util.hpp"

#include "oksdbinterfaces/Configuration.hpp"

#include "ers/Issue.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace py = pybind11;

namespace dunedaq::dal::python {

  using AppInfo_t = std::variant<std::string, std::vector<std::string>, std::map<std::string, std::string>>;
  using ComputerProgramInfo_t = std::variant<std::vector<std::string>, std::map<std::string, std::string>>;

  struct ObjectLocator {
    
    ObjectLocator(const std::string& id_arg, const std::string& class_name_arg) :
      id(id_arg), class_name(class_name_arg) 
    {}

    const std::string id;
    const std::string class_name;
  };

  class AppConfigHelper {

  public:
    AppConfigHelper(const dunedaq::dal::BaseApplication* app):
      m_app(app)
    {
      check_ptrs( {m_app});
    }

    const std::string& get_app_id() const {
      check_ptrs( { m_app } );
      return m_app->UID();
    }

    ObjectLocator* get_base_app() const {
      check_ptrs( {m_app});
      check_ptrs( {m_app->get_base_app()} );
      return new ObjectLocator(m_app->get_base_app()->UID(), m_app->get_base_app()->class_name());
    }

    ObjectLocator* get_host() const {
      check_ptrs( {m_app});
      check_ptrs( {m_app->get_host()} );
      return new ObjectLocator(m_app->get_host()->UID(), m_app->get_host()->class_name());
    }

    ObjectLocator* get_base_seg() const {
      check_ptrs( {m_app});
      check_ptrs( {m_app->get_segment() });
      check_ptrs( {m_app->get_segment()->get_base_segment() } );
      return new ObjectLocator(m_app->get_segment()->get_base_segment()->UID(), m_app->get_segment()->get_base_segment()->class_name());
    }

    const std::string& get_seg_id() const {
      check_ptrs( {m_app});
      check_ptrs( {m_app->get_segment() });
      return m_app->get_segment()->UID();
    }

    std::vector<ObjectLocator> get_backup_hosts() const {
      std::vector<ObjectLocator> backup_hosts;
      for (const auto& host : m_app->get_backup_hosts()) {
	check_ptrs({host});
	backup_hosts.push_back(ObjectLocator(host->UID(), host->class_name()));
      }
      return backup_hosts;
    }

    bool is_templated() const {
      check_ptrs( {m_app});
      return m_app->is_templated();
    }

    std::unordered_map<std::string, AppInfo_t> get_info() {
      std::unordered_map<std::string, AppInfo_t> app_info_collection;
    
      std::map<std::string, std::string> environment;
      std::vector<std::string> program_names;
      std::string start_args, restart_args;
    
      const dunedaq::dal::Tag * tag = m_app->get_info(environment, program_names, start_args, restart_args);
      check_ptrs({tag});

      app_info_collection["tag"] = tag->UID();
      app_info_collection["environment"] = environment;
      app_info_collection["programNames"] = program_names;
      app_info_collection["startArgs"] = start_args;
      app_info_collection["restartArgs"] = restart_args;
    
      return app_info_collection;
    }

  private:

    const dunedaq::dal::BaseApplication* m_app;
  };

  class SegConfigHelper {

  public:
    SegConfigHelper(const dunedaq::dal::Segment* seg) :
      m_segment(seg) {
    }

    std::string get_seg_id() const { 
      check_ptrs({m_segment});
      return m_segment->UID();
    }

    std::vector<AppConfigHelper> get_all_applications(std::set<std::string>* app_types = nullptr, 
						      std::set<std::string>* use_segments = nullptr, 
						      std::set<const dunedaq::dal::Computer *>* use_hosts = nullptr) const {
      check_ptrs({m_segment});
      return app_translator(m_segment->get_all_applications(app_types, use_segments, use_hosts));
    }

    AppConfigHelper* get_controller() const {
      check_ptrs({m_segment});
      return new AppConfigHelper(m_segment->get_controller());
    }

    std::vector<AppConfigHelper> get_infrastructure() const {
      check_ptrs({m_segment});
      return app_translator(m_segment->get_infrastructure());
    }

    std::vector<AppConfigHelper> get_applications() const {
      check_ptrs({m_segment});
      return app_translator(m_segment->get_applications());
    }

    std::vector<SegConfigHelper> get_nested_segments() const {
      std::vector<SegConfigHelper> nested_segments;

      check_ptrs({m_segment});
      for (const auto& seg : m_segment->get_nested_segments()) {
	nested_segments.emplace_back(SegConfigHelper(seg));
      }
      
      return nested_segments;
    }

    std::vector<ObjectLocator> get_hosts() const {
      std::vector<ObjectLocator> hosts;

      check_ptrs({m_segment});
      for (const auto& host : m_segment->get_hosts()) {
	check_ptrs({host});
	hosts.emplace_back(ObjectLocator(host->UID(), host->class_name()));
      }

      return hosts;
    }

    ObjectLocator* get_base_seg() const {
      check_ptrs({m_segment});
      check_ptrs({m_segment->get_base_segment()});

      return new ObjectLocator(m_segment->get_base_segment()->UID(), m_segment->get_base_segment()->class_name());
    }

    bool is_disabled() const {
      check_ptrs({m_segment});
      return m_segment->is_disabled();
    }

    bool is_templated() const {
      check_ptrs({m_segment});
      return m_segment->is_templated();
    }

    std::unordered_map<std::string, int> get_timeouts() const {
      int action_timeout = -999;
      int shortaction_timeout = -999;
      
      check_ptrs({m_segment});
      m_segment->get_timeouts(action_timeout, shortaction_timeout);

      std::unordered_map<std::string, int> timeouts;
      timeouts["actionTimeout"] = action_timeout;
      timeouts["shortActionTimeout"] = shortaction_timeout;

      return timeouts;
    }

  private:

    static std::vector<AppConfigHelper> app_translator(const std::vector<const dunedaq::dal::BaseApplication *>& apps_in) {
      std::vector<AppConfigHelper> apps_out;

      for (const auto& app : apps_in) {
	apps_out.emplace_back(AppConfigHelper(app));
      }

      return apps_out;
    }
    
    const dunedaq::dal::Segment* m_segment;
  };

    std::vector<AppConfigHelper> 
    partition_get_all_applications(const Configuration& db, 
				   const std::string& partition_name,
				   std::set<std::string> app_types,
				   std::set<std::string> use_segments,
				   std::set<std::string> use_hosts) {
      const dunedaq::dal::Partition* partition = dunedaq::dal::get_partition(const_cast<Configuration&>(db), partition_name);

      check_ptrs({partition});

      std::set<const dunedaq::dal::Computer *> use_hosts_concrete;
      for (const auto& hostname : use_hosts) {
	auto computer_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Computer>(hostname);
	check_ptrs({computer_ptr});
	use_hosts_concrete.insert(computer_ptr);
      }

      std::vector<AppConfigHelper> apps;
      for (const auto& app : partition->get_all_applications(&app_types, &use_segments, &use_hosts_concrete)) {
	apps.emplace_back(AppConfigHelper(app));
      }

      return apps;
    }

  std::vector<std::vector<ObjectLocator>> component_get_parents(const Configuration& db, const std::string& partition_id, const std::string& component_id) {
    const dunedaq::dal::Component* component_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Component>(component_id);
    const dunedaq::dal::Partition* partition_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Partition>(partition_id);
    check_ptrs( {component_ptr, partition_ptr});

    std::list<std::vector<const dunedaq::dal::Component*>> parents;
    std::vector<std::vector<ObjectLocator>> parent_ids;

    component_ptr->get_parents(*partition_ptr, parents);

    for (const auto& parent : parents) { 
      std::vector<ObjectLocator> parents_components;
      
      for (const auto& ancestor_component_ptr : parent) { 
	check_ptrs( {ancestor_component_ptr} );
	parents_components.emplace_back( ObjectLocator(ancestor_component_ptr->UID(), 
						       ancestor_component_ptr->class_name()) );
      }
      parent_ids.emplace_back(parents_components);
    }
    
    return parent_ids;
  }

  bool component_disabled(const Configuration& db, const std::string& partition_id, const std::string& component_id) {
    const dunedaq::dal::Component* component_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Component>(component_id);
    const dunedaq::dal::Partition* partition_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Partition>(partition_id);

    check_ptrs({component_ptr});
    check_ptrs({partition_ptr});

    return component_ptr->disabled(*partition_ptr);
  }

  std::string partition_get_log_directory(const Configuration& db, const std::string& partition_id) {
    const dunedaq::dal::Partition* partition_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Partition>(partition_id);
    check_ptrs( {partition_ptr} );
    return partition_ptr->get_log_directory();
  }

  SegConfigHelper* partition_get_segment(const Configuration& db, const std::string& partition_id, const std::string& seg_name) {
    auto partition_ptr = dunedaq::dal::get_partition(const_cast<Configuration&>(db), partition_id);
    check_ptrs( {partition_ptr} );
    auto segptr = partition_ptr->get_segment(seg_name);
    check_ptrs( {segptr} );
    return new SegConfigHelper(segptr);
  }

  std::string variable_get_value(const Configuration& db, const std::string& variable_id, const std::string& tag_id) {
    const dunedaq::dal::Variable* variable_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Variable>(variable_id);
    check_ptrs( {variable_ptr} );
    const dunedaq::dal::Tag* tag_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Tag>(tag_id);
    check_ptrs( {tag_ptr} );
    return variable_ptr->get_value(tag_ptr);
  }

  std::vector<ComputerProgramInfo_t> computer_program_get_info(const Configuration& db, const std::string& partition_id, const std::string& prog_id, const std::string& tag_id, const std::string& host_id) {

    std::vector<ComputerProgramInfo_t> proginfo_collection;

    std::map<std::string, std::string> environment;
    std::vector<std::string> program_names;

    auto partition_ptr = dunedaq::dal::get_partition(const_cast<Configuration&>(db), partition_id);
    auto prog_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::ComputerProgram>(prog_id);
    auto tag_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Tag>(tag_id);
    auto host_ptr = const_cast<Configuration&>(db).get<dunedaq::dal::Computer>(host_id);
    check_ptrs( {partition_ptr, prog_ptr, tag_ptr, host_ptr } );

    prog_ptr->get_info(environment, program_names, *partition_ptr, *tag_ptr, *host_ptr);
		       
    proginfo_collection.emplace_back(environment);
    proginfo_collection.emplace_back(program_names);
    
    return proginfo_collection;
  }


void
register_dal_classes(py::module& m)
{
  py::class_<ObjectLocator>(m, "ObjectLocator")
    .def(py::init<const std::string&, const std::string&>())
    .def_readonly("id", &ObjectLocator::id)
    .def_readonly("class_name", &ObjectLocator::class_name)
    ;

  py::class_<AppConfigHelper>(m, "AppConfigHelper")
    .def(py::init<const dunedaq::dal::BaseApplication*>())
    .def("get_app_id", &AppConfigHelper::get_app_id, "Wrapper for BaseApplication::UID()")
    .def("get_base_app", &AppConfigHelper::get_base_app, "Return identifying info on the BaseApplication")
    .def("get_host", &AppConfigHelper::get_host, "Return identifying info on the host")
    .def("get_base_seg", &AppConfigHelper::get_base_seg, "Return identifying info on the base segment")
    .def("get_seg_id", &AppConfigHelper::get_seg_id, "Return the ID of the segment")
    .def("get_backup_hosts", &AppConfigHelper::get_backup_hosts, "Computers where the application can be restarted in case of problems")
    .def("is_templated", &AppConfigHelper::is_templated, "Is the application templated")
    .def("get_info", &AppConfigHelper::get_info, "Get information required to run the application")
    ;

  py::class_<SegConfigHelper>(m, "SegConfigHelper")
    .def(py::init<const dunedaq::dal::Segment*>())
    .def("get_seg_id", &SegConfigHelper::get_seg_id, "get segment id")
    .def("get_all_applications", &SegConfigHelper::get_all_applications, "Get all applications in the segment")
    .def("get_controller", &SegConfigHelper::get_controller, "Get segment controller")
    .def("get_infrastructure", &SegConfigHelper::get_infrastructure, "Get segment infrastructure applications")
    .def("get_applications", &SegConfigHelper::get_applications, "Get segment applications")
    .def("get_nested_segments", &SegConfigHelper::get_nested_segments, "Get nested segments")
    .def("get_hosts", &SegConfigHelper::get_hosts, "Get hosts for given segment")
    .def("get_base_seg", &SegConfigHelper::get_base_seg, "Get base segment object (i.e. used to create template segment)")
    .def("is_templated", &SegConfigHelper::is_templated, "Return true if this segment is a template segment")
    .def("is_disabled", &SegConfigHelper::is_disabled, "Return true if this segment is disabled")
    .def("get_timeouts", &SegConfigHelper::get_timeouts, "Return run control action timeouts")
    ;

  m.def("partition_get_all_applications", &partition_get_all_applications, "Get list of applications in the requested partition");
  m.def("partition_get_log_directory", partition_get_log_directory);
  m.def("partition_get_segment", partition_get_segment);

  m.def("component_get_parents", &component_get_parents, "Get the Component-derived class instances of the parent(s) of the Component-derived object in question");
  m.def("component_disabled", &component_disabled, "Determine if a Component-derived object (e.g. a Segment) has been disabled");

  m.def("variable_get_value", &variable_get_value, "Get the value stored in an object of class Variable");

  m.def("computer_program_get_info", &computer_program_get_info, "Get details about a computer program");
}


} // namespace dunedaq::dal::python

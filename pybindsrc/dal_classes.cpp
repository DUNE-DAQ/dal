/**
 * @file dal_classes.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

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

#include "config/Configuration.hpp"

#include "ers/Issue.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace py = pybind11;

namespace dunedaq {

  ERS_DECLARE_ISSUE(dal, NullPointerReturned, "A null pointer was returned",)

} // namespace dunedaq

namespace dunedaq::dal::python {

  using AppInfo_t = std::variant<std::string, std::vector<std::string>, std::map<std::string, std::string>>;

  void check_ptrs(std::vector<const void*> ptrs) {
    for (const auto& ptr : ptrs) {
      if (!ptr) {
	throw NullPointerReturned(ERS_HERE);
      }
    }
  }

  struct ObjectLocator {
    
    ObjectLocator(const std::string& id_arg, const std::string& class_name_arg) :
      id(id_arg), class_name(class_name_arg) 
    {}

    const std::string id;
    const std::string class_name;
  };

  class AppConfigHelper {

  public:
    AppConfigHelper(const daq::core::BaseApplication* app):
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
    
      const daq::core::Tag * tag = m_app->get_info(environment, program_names, start_args, restart_args);
      check_ptrs({tag});

      app_info_collection["tag"] = tag->UID();
      app_info_collection["environment"] = environment;
      app_info_collection["programNames"] = program_names;
      app_info_collection["startArgs"] = start_args;
      app_info_collection["restartArgs"] = restart_args;
    
      return app_info_collection;
    }

  private:

    const daq::core::BaseApplication* m_app;
  };

    std::vector<AppConfigHelper> 
    partition_get_all_applications(const Configuration& db, 
				   const std::string& partition_name,
				   std::set<std::string> app_types,
				   std::set<std::string> use_segments,
				   std::set<std::string> use_hosts) {
      const daq::core::Partition* partition = daq::core::get_partition(const_cast<Configuration&>(db), partition_name);

      check_ptrs({partition});

      std::set<const daq::core::Computer *> use_hosts_concrete;
      for (const auto& hostname : use_hosts) {
	auto computer_ptr = const_cast<Configuration&>(db).get<daq::core::Computer>(hostname);
	check_ptrs({computer_ptr});
	use_hosts_concrete.insert(computer_ptr);
      }

      std::vector<AppConfigHelper> apps;
      for (const auto& app : partition->get_all_applications(&app_types, &use_segments, &use_hosts_concrete)) {
	apps.emplace_back(AppConfigHelper(app));
      }

      return apps;
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
    .def(py::init<const daq::core::BaseApplication*>())
    .def("get_app_id", &AppConfigHelper::get_app_id, "Wrapper for BaseApplication::UID()")
    .def("get_base_app", &AppConfigHelper::get_base_app, "Return identifying info on the BaseApplication")
    .def("get_host", &AppConfigHelper::get_host, "Return identifying info on the host")
    .def("get_base_seg", &AppConfigHelper::get_base_seg, "Return identifying info on the base segment")
    .def("get_seg_id", &AppConfigHelper::get_seg_id, "Return the ID of the segment")
    .def("get_backup_hosts", &AppConfigHelper::get_backup_hosts, "Computers where the application can be restarted in case of problems")
    .def("is_templated", &AppConfigHelper::is_templated, "Is the application templated")
    .def("get_info", &AppConfigHelper::get_info, "Get information required to run the application")
    ;

  m.def("partition_get_all_applications", &partition_get_all_applications, "Get list of applications in the requested partition");

}


} // namespace dunedaq::config::python

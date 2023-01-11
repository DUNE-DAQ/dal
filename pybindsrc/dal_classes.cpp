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

#include <string>
#include <vector>
#include <iostream>

namespace py = pybind11;

namespace dunedaq {

  ERS_DECLARE_ISSUE(dal, NullPointerReturned, "A null pointer was returned",)

} // namespace dunedaq

namespace dunedaq::dal::python {

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
    {}

    const std::string& get_app_id() const {
      check_ptrs( { m_app } );
      return m_app->UID();
    }

  private:

    const daq::core::BaseApplication* m_app;
  };

    std::vector<AppConfigHelper> 
    partition_get_all_applications(const Configuration& db, 
				   const std::string& partition_name) {
      const daq::core::Partition* partition = daq::core::get_partition(const_cast<Configuration&>(db), partition_name);

      if (!partition) {
	throw NullPointerReturned(ERS_HERE);
      }

      std::vector<AppConfigHelper> apps;
      for (const auto& app : partition->get_all_applications()) {
	apps.emplace_back(AppConfigHelper(app));
      }

      return apps;
    }

void
register_dal_classes(py::module& m)
{
  py::class_<ObjectLocator>(m, "ObjectLocator")
    .def(py::init<const std::string&, const std::string&>())
    ;

  py::class_<AppConfigHelper>(m, "AppConfigHelper")
    .def(py::init<const daq::core::BaseApplication*>())
    .def("get_app_id", &AppConfigHelper::get_app_id, "Wrapper for BaseApplication::UID()")
    ;

  m.def("partition_get_all_applications", &partition_get_all_applications, "Get list of applications in the requested partition");

}


} // namespace dunedaq::config::python

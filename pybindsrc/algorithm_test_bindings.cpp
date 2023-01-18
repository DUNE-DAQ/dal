/**
 * @file algorithm_test_bindings.cpp
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

#include "config/Configuration.hpp"

#include "ers/Issue.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <iostream>

namespace py = pybind11;

namespace {

  std::string print_app(const daq::core::BaseApplication* app) {
    check_ptrs({app});
    return app->UID() + "@" + app->class_name() + " on " + app->get_host()->UID() + "@" + app->get_host()->class_name();
  }

  std::string print_segment(const daq::core::Segment* seg)
  {
    check_ptrs({seg});

    std::string out;

    out += std::string("segment: ") + seg->UID() + '\n';

    out += std::string("controller: ") + print_app(seg->get_controller()) + '\n';

    if(!seg->get_infrastructure().empty()) {
      out.append("infrastructure:\n");
      for(const auto& x : seg->get_infrastructure()) {
	out += print_app(x) + '\n';
      }
    } else {
      out.append("no infrastructure\n");
    }

    if(!seg->get_applications().empty()) {
      out.append("applications:\n");
      for(const auto& x : seg->get_applications()) {
	out += print_app(x) + '\n';
      }
    } else {
      out.append("no applications\n");
    }

    if(!seg->get_hosts().empty()) {
      out.append("hosts:\n");
      for(const auto& x : seg->get_hosts()) {
	out += x->UID() + "@" + x->class_name();
      }
    } else {
      out.append("no hosts\n");
    }

    if(!seg->get_nested_segments().empty()) {
      out.append("nested segments:\n");
      for(const auto& x : seg->get_nested_segments()) {
	out += print_segment(x);
      }
    } else {
      out.append("no nested segments\n");
    }

    return out;
  }
  
  std::string print_segment_timeout(const daq::core::Segment* seg) {
    check_ptrs({seg});

    int action_timeout, shortaction_timeout;

    seg->get_timeouts(action_timeout, shortaction_timeout);

    std::string out = std::string("segment ") + seg->UID() + " actionTimeout: " + std::to_string(action_timeout) + ", shortActionTimeout" + std::to_string(shortaction_timeout) + '\n';

    for(const auto& x : seg->get_nested_segments()) {
        out += print_segment_timeout(x);
    }

    return out;
  }

} // namespace ""

namespace dunedaq::dal::python {

  std::string get_parents_test(const Configuration& db, const::std::string partition_id, const std::string& component_id ) {
    
    const daq::core::Component* component_ptr = const_cast<Configuration&>(db).get<daq::core::Component>(component_id);
    const daq::core::Partition* partition_ptr = const_cast<Configuration&>(db).get<daq::core::Partition>(partition_id.c_str());

    check_ptrs( {component_ptr, partition_ptr});

    std::list< std::vector<const daq::core::Component*>> parents;
    component_ptr->get_parents(*partition_ptr, parents);

    std::string line;

    for (const auto& parent: parents) {
      line += "[";
      for (const auto& ancestor_component_ptr : parent) {
	check_ptrs( {ancestor_component_ptr});
	line += "<" + std::string(ancestor_component_ptr->UID()) + "@" + std::string(ancestor_component_ptr->class_name()) + ">";
      }
    }
    
    return line;
  }

  std::string get_log_directory_test(const Configuration& db, const::std::string partition_id) {
    return const_cast<Configuration&>(db).get<daq::core::Partition>(partition_id)->get_log_directory();
  }

  std::string get_segment_test(const Configuration& db, const::std::string partition_id, const std::string& seg_name) {
    auto partition_ptr = const_cast<Configuration&>(db).get<daq::core::Partition>(partition_id);
    check_ptrs({partition_ptr});
    
    return print_segment(partition_ptr->get_segment(seg_name));
  }

  std::string get_value_test(const Configuration& db, const std::string& variable_id, const std::string& tag_id) {

    const daq::core::Variable *variable_ptr = const_cast<Configuration&>(db).get<daq::core::Variable>(variable_id);
    const daq::core::Tag * tag_ptr = const_cast<Configuration&>(db).get<daq::core::Tag>(tag_id);

    check_ptrs({variable_ptr, tag_ptr});

    return variable_ptr->get_value(tag_ptr);
  }

  bool disabled_test(const Configuration& db, const::std::string partition_id, const std::string& component_id) {
    const daq::core::Component* component_ptr = const_cast<Configuration&>(db).get<daq::core::Component>(component_id);
    const daq::core::Partition* partition_ptr = const_cast<Configuration&>(db).get<daq::core::Partition>(partition_id);

    check_ptrs({component_ptr});
    check_ptrs({partition_ptr});

    return component_ptr->disabled(*partition_ptr);
  }

  std::string get_timeouts_test(const Configuration& db, const::std::string partition_id, const std::string& segment_id) {
    const daq::core::Partition* partition_ptr = const_cast<Configuration&>(db).get<daq::core::Partition>(partition_id);
    check_ptrs({partition_ptr});

    const daq::core::Segment* segment_ptr = partition_ptr->get_segment(segment_id);
    check_ptrs({segment_ptr});

    return print_segment_timeout(segment_ptr);
  }
  
  
  void
  register_algorithm_test_bindings(py::module& m)
  {
    m.def("get_parents_test", &get_parents_test, "To test against the component_get_parents binding");
    m.def("get_log_directory_test", &get_log_directory_test, "To test against the partition_get_log_directory binding");
    m.def("get_segment_test", &get_segment_test, "To test against the partition_get_segment binding");
    m.def("get_value_test", &get_value_test, "To test against the variable_get_value binding");
    m.def("disabled_test", &disabled_test, "To test against the component_disabled binding");
    m.def("get_timeouts_test", &get_timeouts_test, "To test the SegConfigHelper get_timeouts binding");
  }

} // namespace dunedaq::dal::python

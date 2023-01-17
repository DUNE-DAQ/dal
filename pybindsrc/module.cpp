/**
 * @file module.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace dunedaq::dal::python {

extern void
register_dal_classes(py::module&);

extern void
register_algorithm_test_bindings(py::module&);

PYBIND11_MODULE(_daq_dal_py, m)
{

  m.doc() = "Python interface to the dal package"; 

  register_dal_classes(m);
  register_algorithm_test_bindings(m);
}

} // namespace dunedaq::dal::python

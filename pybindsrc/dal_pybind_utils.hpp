#ifndef DAL_PYBINDSRC_DAL_PYBIND_UTILS_HPP_
#define DAL_PYBINDSRC_DAL_PYBIND_UTILS_HPP_

#include "ers/Issue.hpp"

namespace dunedaq {

  ERS_DECLARE_ISSUE(dal, NullPointerReturned, "A null pointer was returned",)

} // namespace dunedaq

namespace {

  void check_ptrs(const std::vector<const void*> ptrs) {
    for (const auto& ptr : ptrs) {
      if (!ptr) {
	throw dunedaq::dal::NullPointerReturned(ERS_HERE);
      }
    }
  }
} // namespace


#endif

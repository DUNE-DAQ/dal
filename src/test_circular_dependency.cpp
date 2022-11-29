#include <config/DalObject.h>

#include "test_circular_dependency.h"

void
daq::core::TestCircularDependency::push(const DalObject * object)
{
  if(p_index < p_limit) {
    p_objects[p_index++] = object;
  }
  else {
    std::ostringstream s;
    for(unsigned int i = 0; i < p_index; ++i) {
      if(i != 0) s << ", ";
      s << p_objects[i];
    }

    throw daq::core::FoundCircularDependency(ERS_HERE, p_limit, p_goal, s.str());
  }
}

#include "oksdbinterfaces/DalObject.hpp"

#include "test_circular_dependency.hpp"

void
dunedaq::dal::TestCircularDependency::push(const DalObject * object)
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

    throw dunedaq::dal::FoundCircularDependency(ERS_HERE, p_limit, p_goal, s.str());
  }
}

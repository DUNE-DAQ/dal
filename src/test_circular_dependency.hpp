#ifndef _daq_core_test_circular_dependency_H_
#define _daq_core_test_circular_dependency_H_


#include "dal/util.hpp"

namespace dunedaq {
  namespace oksdbinterfaces {
    class DalObject;
  }
}

namespace dunedaq::dal {

    class TestCircularDependency {

      friend class AddTestOnCircularDependency;

      public:

        TestCircularDependency(const char * goal, const dunedaq::oksdbinterfaces::DalObject * first_object) :
            p_goal(goal), p_index(0)
        {
          p_objects[p_index++] = first_object;
        }


      private:

        /// \throw dunedaq::dal::FoundCircularDependency
        void push(const dunedaq::oksdbinterfaces::DalObject * object);

        void
        pop()
        {
          p_index--;
        }

          /// maximum recursion level
        enum {
	  p_limit = 64
	};

        const char * p_goal;
        unsigned int p_index;
        const dunedaq::oksdbinterfaces::DalObject * p_objects[p_limit];

    };

    class AddTestOnCircularDependency {

      public:

        AddTestOnCircularDependency(TestCircularDependency& fuse, const dunedaq::oksdbinterfaces::DalObject * obj) : p_fuse(fuse) { p_fuse.push(obj); }
        ~AddTestOnCircularDependency() { p_fuse.pop(); }


      private:

        TestCircularDependency& p_fuse;
    };
} // dunedaq::dal


#endif

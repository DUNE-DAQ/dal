#include "config/ConfigObject.h"
#include "config/Configuration.h"
#include "config/ConfigurationPointer.h"
#include "dal/util.h"

#include "boost/python.hpp"

#include "dal/Application.h"
#include "dal/TemplateApplication.h"
#include "dal/ComputerProgram.h"
#include "dal/Partition.h"
#include "dal/BaseApplication.h"
#include "dal/Partition.h"
#include "dal/ComputerProgram.h"
#include "dal/Tag.h"
#include "dal/Computer.h"
#include "dal/Segment.h"
#include "dal/TemplateSegment.h"
#include "dal/TemplateApplication.h"
#include "dal/ResourceBase.h"
#include "dal/Resource.h"
#include "dal/Variable.h"

#include <iostream>
#include <string> 
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>


   // Database is taken from the envıroment variable. Generated  partition is used. db and partition is used commonly for all tests.
    const std::string partition_name = "dal_test";
    const std::string data = "oksconfig:" + partition_name + ".data.xml";
    Configuration db(data);

    const daq::core::Partition * partition = daq::core::get_partition(db, partition_name);
   // db.register_converter(new daq::core::SubstituteVariables(db, *partition));


boost::python::list get_parents_test(std::string name){
	boost::python::list result;
	// Get pointer to ourselves
	const daq::core::Component *self = db.get<daq::core::Component>(name);

	std::list< std::vector<const daq::core::Component *> > parents ; 
	unsigned int j;
	std::string line ;
	self->get_parents(*partition, parents);
	if (parents.size()){
		for (auto & i : parents){
			line.append("[") ;
			if (i.size())
			{
				for (j = 0 ; j < i.size(); j ++ ){
					line.append( "<");
					line.append( i[j]->UID() );
					line.append ("@" );
					line.append( i[j]->class_name());
					line.append(">");
					}	

			}

		}
	}
	result.append(line);
	return result;
} 


std::string get_log_directory_test(const std::string& self_name)
{
    return db.get<daq::core::Partition>(self_name)->get_log_directory();
}

bool disabled_test(const std::string& self_name)
{

    // Get pointer to ourselves
	const daq::core::Component *self = db.get<daq::core::Component>(self_name);

    // Call the DAL algorithm.

    return self->disabled( *partition);;
}


static std::string
print_segment_timeout(const daq::core::Segment * seg)
{
  int actionTimeout, shortActionTimeout;

  seg->get_timeouts(actionTimeout, shortActionTimeout);

  std::string out = std::string("segment ") + seg->UID() + " actionTimeout: " + std::to_string(actionTimeout) + ", shortActionTimeout" + std::to_string(shortActionTimeout) + '\n';

  if(seg->get_nested_segments().empty() == false)
    {
      for(const auto& x : seg->get_nested_segments())
        out += print_segment_timeout(x);
    }

  return out;
}


std::string get_timeouts_test(const std::string& self_name)
{
  return print_segment_timeout(partition->get_segment(self_name));
}

	std::string get_value_test(const std::string& self_name, const std::string& tag_name) {

		// Get pointer to ourselves
		const daq::core::Variable *self = db.get<daq::core::Variable>(self_name);

		// Get the DAL object to the argument, here a 'Tag'
		const daq::core::Tag * tag = db.get<daq::core::Tag>(tag_name);

		return self->get_value(tag);
	}

        static std::string
        print_app(const daq::core::BaseApplication * app)
        {
          return app->UID() + "@" + app->class_name() + " on " + app->get_host()->UID() + "@" + app->get_host()->class_name();
        }

        static std::string
        print_segment(const daq::core::Segment * seg)
        {
          std::string out;

          out += std::string("segment: ") + seg->UID() + '\n';

          out += std::string("controller: ") + print_app(seg->get_controller()) + '\n';

          if(seg->get_infrastructure().empty() == false)
            {
              out.append("infrastructure:\n");
              for(const auto& x : seg->get_infrastructure())
                out += print_app(x) + '\n';
            }
          else
            {
              out.append("no infrastructure\n");
            }

          if(seg->get_applications().empty() == false)
            {
              out.append("applications:\n");
              for(const auto& x : seg->get_applications())
                out += print_app(x) + '\n';
            }
          else
            {
              out.append("no applications\n");
            }

          if(seg->get_hosts().empty() == false)
            {
              out.append("hosts:\n");
              for(const auto& x : seg->get_hosts())
                out += x->UID() + "@" + x->class_name();
            }
          else
            {
              out.append("no hosts\n");
            }

          if(seg->get_nested_segments().empty() == false)
            {
              out.append("nested segments:\n");
              for(const auto& x : seg->get_nested_segments())
                out += print_segment(x);
            }
          else
            {
              out.append("no nested segments\n");
            }

            return out;
        }

	std::string
        get_segment_test(const std::string& seg_name)
        {
          return print_segment(partition->get_segment(seg_name));
        }


	boost::python::list get_info_test(const std::string& self_name,const std::string& tag_name, const std::string& host_name)
	{
          const daq::core::ComputerProgram *self = db.get<daq::core::ComputerProgram>(self_name);
          const daq::core::Tag * tag = db.get<daq::core::Tag>(tag_name);
          const daq::core::Computer * host = db.get<daq::core::Computer>(host_name);

          std::map<std::string, std::string> environment;
          std::vector<std::string> program_names;

          self->get_info(environment, program_names, *partition, *tag, *host);

          boost::python::dict dictionary;
          for (const auto& iter : environment)
            dictionary[iter.first] = iter.second;

          boost::python::list mini_list;
          for (const auto & i : program_names)
            mini_list.append(i);

          boost::python::list result;

          result.append(dictionary);
          result.append(mini_list);

          return result;
        }

BOOST_PYTHON_MODULE(libdal_algo_tester)
{
	using namespace boost::python;
    def("get_parents_test", get_parents_test);
    def("get_log_directory_test", get_log_directory_test);
    def("disabled_test",disabled_test);
    def("get_timeouts_test",get_timeouts_test);
    def("get_value_test", get_value_test);
    def("get_segment_test", get_segment_test);
    def("get_info_test", get_info_test);    

}

//
//  FILE: src/dal_dump_apps.cpp
//
//  Example program which shows how to use user-defined algorithms
//  in the user's code. In particular:
//    - find root partition
//    - calculate application's program, command line, environment,
//      binary search paths and paths to shared libraries.
//
//  For command line arguments see function usage() or run the program
//  with --help.
//
//  Implementation:
//	<Igor.Soloviev@cern.ch> - May 2003
//

#include <stdlib.h>
#include <thread>
#include <fstream>
#include <sstream>

#include <boost/program_options.hpp>

#include "oksdbinterfaces/Configuration.hpp"

#include "dal/BaseApplication.hpp"
#include "dal/OnlineSegment.hpp"
#include "dal/Partition.hpp"
#include "dal/Segment.hpp"
#include "dal/util.hpp"


static void
print_info(
  const std::vector<std::string>& file_names,
  const std::map<std::string, std::string>& environment,
  std::ostream& f
)
{

    // print program names

  f << " - possible program file names:";

  if(file_names.empty()) {
    f << " no\n";
  }
  else {
    f << std::endl;
    for(std::vector<std::string>::const_iterator j = file_names.begin(); j != file_names.end(); ++j) {
      f << "    * " << *j << std::endl;
    }
  }


    // print environment

  f << " - environment variables:";

  if(environment.empty()) {
    f << " no\n";
  }
  else {
    f << std::endl;
    for(std::map<std::string, std::string>::const_iterator j = environment.begin(); j != environment.end(); ++j) {
      f << "    * " << j->first << "=\"" << j->second << "\"\n";
    }
  }
}

static void
get_configuration(Configuration& db, const std::string& partition_name, const std::string& object_id, const std::string& app_name, const std::string& segment_id, bool subst, int count)
{
  try {
      // find partition || exit, if there is no partition object

    const dunedaq::dal::Partition * partition = dunedaq::dal::get_partition(db, partition_name);
    if(!partition) return;

    std::ostringstream fname;
    fname << "/tmp/test-" << count << ".out";
    std::ofstream f(fname.str());

      // register variables converter

    if(subst) {
      db.register_converter(new dunedaq::dal::SubstituteVariables(*partition));
    }


      // get application object (a normal application or template application)

    const dunedaq::dal::BaseApplication * b_app = (object_id.empty() ? 0 : db.get<dunedaq::dal::BaseApplication>(object_id));

    if(!object_id.empty() && !b_app) {
      f << "ERROR: cannot get object \'" << object_id << "\' of class \'BaseApplication\'\n";
      return;
    }


      // get application(s)

    std::set<std::string> segments;

    if(!segment_id.empty()) {
      segments.insert(segment_id);
    }

    const dunedaq::dal::Segment * root_seg = partition->get_segment(partition->get_OnlineInfrastructure()->UID());
    std::vector<const dunedaq::dal::BaseApplication *> objects = root_seg->get_all_applications(nullptr, (!segments.empty() ? &segments : nullptr), nullptr);

    unsigned int count = 0;

    for (const auto& i : objects) {
      if(b_app && (i->get_base_app()->cast<dunedaq::dal::BaseApplication>() != b_app)) continue;
      if(!app_name.empty() && app_name != i->UID()) continue;

      count++;

      if(i->is_templated()) {
        f << "### (" << count << ") template application " << i << " ###" << std::endl;
      }
      else {
        f << "### (" << count << ") application " << i << " ###" << std::endl;
      }

        // get application parameters (tag, environment set, application host,
        // possible program names, paths to programs and shared libs)

      try {
        std::vector<std::string> file_names;
        std::map<std::string, std::string> environment;
        std::string a, b;

        (*i).get_info(environment, file_names, a, b) ;

        f << " - command line start args:\n    " << a << "\n"
             " - command line restart args:\n    " << b << std::endl;

        print_info(file_names, environment, f);
      }
      catch ( dunedaq::dal::AlgorithmError & ex ) {
        ers::error( ex ) ;  // report a problem
      }
    }

    if(count == 0) {
      if(b_app) {
        f << "the application " << b_app << " is not running in the partition; it is disabled or not included into partition" << std::endl;
      }
      else if(!app_name.empty()) {
        f << "the application with name \'" << app_name << "\' is not running in the partition; it is disabled or not included into partition" << std::endl;
      }
      else if(!segment_id.empty()) {
        f << "the applications of segment " << segment_id << " are not running in the partition; the segment or it\'s applications are disabled or the segment is not included into partition" << std::endl;
      }
    }

    f.close();
    TLOG() << "finish with file " << fname.str();
  }
  catch (ers::Issue & ex) {
    TLOG() << "ERROR [thread " << count << "]: caught " << ex;
  }
}

int
main(int argc, char *argv[])
{
  boost::program_options::options_description desc("Similar to dal_dump_apps, but run algorithms in multiple threads (to test config and DAL). Available options are:");

  std::string db_name;
  std::string partition_name;
  std::string object_id;
  std::string app_name;
  std::string segment_id;
  int thread_num;

  bool subst = false;

  try
    {
      desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&db_name), "name of the database")
        ("partition-id,p", boost::program_options::value<std::string>(&partition_name)->required(), "name of the partition object")
        ("application-id,a", boost::program_options::value<std::string>(&object_id), "identity of the application object (if not provided, dump all applications)")
        ("application-name,n", boost::program_options::value<std::string>(&app_name), "name of the application object (if not provided, dump all applications)")
        ("application-segment-id,g", boost::program_options::value<std::string>(&segment_id), "identity of the application's segment object (if defined, print apps of this segment)")
        ("threads-number,t", boost::program_options::value<int>(&thread_num)->default_value(2), "number of threads")
        ("substitute-variables,s","substitute database parameters")
        ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return EXIT_SUCCESS;
        }

      boost::program_options::notify(vm);

      if (vm.count("substitute-variables"))
        {
          subst = true;
        }
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }


  std::cout << "run " << thread_num << " threads" << std::endl;

  if(thread_num < 1)
    {
      std::cerr << "ERROR: invalid number of threads" << std::endl;
      return 1;
    }

  try
    {
      Configuration conf(db_name);

      std::vector<std::thread> v;
      v.reserve(thread_num);

      for (int i = 1; i <= thread_num; ++i)
        {
          v.push_back(std::thread(get_configuration, std::ref(conf), std::ref(partition_name), std::ref(object_id), std::ref(app_name), std::ref(segment_id), subst, i));
        }

      for (int i = 0; i < thread_num; ++i)
        {
          v[i].join();
        }
    }
  catch (ers::Issue & ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return (EXIT_FAILURE);
    }

  return 0;
}

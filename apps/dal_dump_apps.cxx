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
//  Modified:
//

#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "config/Configuration.hpp"

#include "dal/Application.hpp"
#include "dal/OnlineSegment.hpp"
#include "dal/Partition.hpp"

#include "dal/util.hpp"




static void
print_info(const std::vector<std::string>& file_names, const std::map<std::string, std::string>& environment)
{

  // print program names

  std::cout << " - possible program file names:";

  if (file_names.empty())
    {
      std::cout << " no\n";
    }
  else
    {
      std::cout << std::endl;
      for (const auto& j : file_names)
        {
          std::cout << "    * " << j << std::endl;
        }
    }

  // print environment

  std::cout << " - environment variables:";

  if (environment.empty())
    {
      std::cout << " no\n";
    }
  else
    {
      std::cout << std::endl;
      for (const auto& j : environment)
        {
          std::cout << "    * " << j.first << "=\"" << j.second << "\"\n";
        }
    }
}

int
main(int argc, char *argv[])
{
  boost::program_options::options_description desc("Example of dunedaq::dal::Application::get_info() algorithm usage. By default the algorithm is applied to all applications used by the partition.");

  std::string db_name;
  std::string partition_name;
  std::string object_id;
  std::string app_name;
  std::string segment_id;

  bool subst = false;


  try
    {
      desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&db_name), "name of the database")
        ("partition-id,p", boost::program_options::value<std::string>(&partition_name)->required(), "name of the partition object")
        ("application-id,a", boost::program_options::value<std::string>(&object_id), "identity of the application object (if not provided, dump all applications)")
        ("application-name,n", boost::program_options::value<std::string>(&app_name), "name of the application object (if not provided, dump all applications)")
        ("application-segment-id,g", boost::program_options::value<std::string>(&segment_id), "identity of the application's segment object (if defined, print apps of this segment)")
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


    // work with configuration

  try
    {

      // load configuration

      Configuration conf(db_name);

      // find partition || exit, if there is no partition object

      const dunedaq::dal::Partition * partition = dunedaq::dal::get_partition(conf, partition_name);

      if (!partition)
        return EXIT_FAILURE;

      // register variables converter

      if (subst)
        {
          conf.register_converter(new dunedaq::dal::SubstituteVariables(*partition));
        }

      // get application object (a normal application or template application)

      const dunedaq::dal::BaseApplication * b_app = (object_id.empty() ? nullptr : conf.get<dunedaq::dal::BaseApplication>(object_id));

      if (!object_id.empty() && !b_app)
        {
          std::cerr << "ERROR: cannot get object \'" << object_id << "\' of class \'BaseApplication\'\n";
          return EXIT_FAILURE;
        }

      // get application(s)

      std::set<std::string> segments;

      if (!segment_id.empty())
        {
          segments.insert(segment_id);
        }

      const dunedaq::dal::Segment * root_seg = partition->get_segment(partition->get_OnlineInfrastructure()->UID());
      std::vector<const dunedaq::dal::BaseApplication *> objects = root_seg->get_all_applications(0, (!segments.empty() ? &segments : 0), 0);

      unsigned int count = 0;

      for (const auto& i : objects)
        {
          if (b_app && (i->get_base_app()->cast<dunedaq::dal::BaseApplication>() != b_app))
            continue;
          if (!app_name.empty() && app_name != i->UID())
            continue;

          count++;

          if (i->get_base_app()->cast<dunedaq::dal::Application>())
            {
              std::cout << "### (" << count << ") application " << i->get_base_app() << " ###\n";
            }
          else
            {
              std::cout << "### (" << count << ") template application " << i->UID() << " ###\n";
            }

          // get application parameters (tag, environment set, application host,
          // possible program names, paths to programs and shared libs)

          try
            {
              std::vector<std::string> file_names;
              std::map<std::string, std::string> environment;
              std::string a, b;
              i->get_info(environment, file_names, a, b);

              std::cout << " - command line start args:\n    " << a << "\n - command line restart args:\n    " << b << std::endl;

              print_info(file_names, environment);
            }
          catch (dunedaq::dal::AlgorithmError & ex)
            {
              ers::error(ex);  // report a problem
            }
        }

      if (count == 0)
        {
          if (b_app)
            std::cout << "the application " << b_app << " is not running in the partition; it is disabled or not included into partition\n";
          else if (!app_name.empty())
            std::cout << "the application with name \'" << app_name << "\' is not running in the partition; it is disabled or not included into partition\n";
          else if (!segment_id.empty())
            std::cout << "the applications of segment " << segment_id << " are not running in the partition; the segment or it\'s applications are disabled or the segment is not included into partition\n";
        }
    }
  catch (ers::Issue & ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

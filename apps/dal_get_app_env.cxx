//
//  FILE: src/dal_get_app_env.cpp
//
//  The program prints out environment variables defined for it in the configuration database.
//
//  For command line arguments see function usage() or run the program with --help.
//
//  Implementation:
//	<Igor.Soloviev@cern.ch> - September 2005
//

#include <boost/program_options.hpp>

#include <config/Configuration.h>

#include "dal/BaseApplication.h"
#include "dal/Segment.h"
#include "dal/OnlineSegment.h"
#include "dal/Partition.h"

#include "dal/util.h"


int main(int argc, char *argv[])
{
  std::string data;
  std::string partition_name;
  std::string object_id;
  std::string syntax;

  try
    {
      boost::program_options::options_description desc("Print to the standard out in a shell source form environment variables for given application. Available options are:");

      desc.add_options()
          ("data,d", boost::program_options::value<std::string>(&data), "database name")
          ("partition-name,p", boost::program_options::value<std::string>(&partition_name)->required(), "partition name")
          ("application-id,a", boost::program_options::value<std::string>(&object_id)->required(), "application name")
          ("shell-syntax,s", boost::program_options::value<std::string>(&syntax)->default_value("sh"), "shell syntax used to set environment variable value: \'sh\' (Bourne shell) or \'csh\' (C shell)")
          ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return EXIT_FAILURE;
        }

      boost::program_options::notify(vm);

      if (syntax != "sh" && syntax != "csh")
        {
          std::cerr << "ERROR: unknown syntax \'" << syntax << "\'. Use either \'sh\', or \'csh\'.\n";
          return EXIT_FAILURE;
        }
    }
  catch (std::exception& ex)
    {
      std::cerr << "ERROR: failed to parse command line: " << ex.what() << std::endl;
      return EXIT_FAILURE;
    }

  try
    {
      Configuration db(data);

      // find partition; exit, if there is no partition object
      if (const daq::core::Partition * partition = daq::core::get_partition(db, partition_name))
        {
          db.register_converter(new daq::core::SubstituteVariables(*partition));

          std::set<std::string> segments;

          const daq::core::Segment * root_seg = partition->get_segment(partition->get_OnlineInfrastructure()->UID());

          for (const auto& i : root_seg->get_all_applications())
            {
              if (i->UID() == object_id)
                {
                  std::vector<std::string> file_names;
                  std::map<std::string, std::string> environment;
                  std::string startArgs, restartArgs;

                  // get application info
                  i->get_info(environment, file_names, startArgs, restartArgs);

                  // print environment
                  for (const auto & j : environment)
                    {
                      if (syntax == "sh")
                        std::cout << "export " << j.first << "=\"" << j.second << "\"\n";
                      else if (syntax == "csh")
                        std::cout << "setenv " << j.first << " \"" << j.second << "\"\n";
                    }

                  return EXIT_SUCCESS;
                }
            }
        }
      else
        {
          std::cerr << "ERROR: cannot find partition \'" << partition_name << "\'" << std::endl;
          return EXIT_FAILURE;

        }
    }
  catch (ers::Issue & ex)
    {
      std::cerr << "ERROR: " << ex << std::endl;
      return EXIT_FAILURE;
    }

  std::cerr << "ERROR: cannot find application \'" << object_id << "\'" << std::endl;

  return EXIT_SUCCESS;
}

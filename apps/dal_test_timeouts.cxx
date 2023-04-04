// This test program calculates timeouts to perform transactions for a segment
// Author: G. Lehmann
// Date: 04-01-2005 

#include <boost/program_options.hpp>

#include "dal/Partition.hpp"
#include "dal/Segment.hpp"
#include "dal/util.hpp"

using namespace dunedaq::oksdbinterfaces;

int
main(int argc, char *argv[])
{
  // parse parameters

  boost::program_options::options_description cmdl("Test for timeout calculation algorithm using the following options");

  std::string db_name, segment_name, partition_name;

  try
    {
      cmdl.add_options()
          ("data,d", boost::program_options::value<std::string>(&db_name)->required(), "mandatory name of the database using format \"plugin:db-name\"")
          ("partition,p", boost::program_options::value<std::string>(&partition_name)->required(), "mandatory name of partition")
          ("segment,s", boost::program_options::value<std::string>(&segment_name)->required(), "mandatory name of segment")
          ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdl), vm);

      if (vm.count("help"))
        {
          std::cout << cmdl << std::endl;
          return EXIT_SUCCESS;
        }

      boost::program_options::notify(vm);
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }



  try
    {
      dunedaq::oksdbinterfaces::Configuration conf(db_name);

      if (const dunedaq::dal::Partition * p = dunedaq::dal::get_partition(conf, partition_name))
        {
          if (const dunedaq::dal::Segment * s = p->get_segment(segment_name))
            {
              int longT, shortT;
              s->get_timeouts(longT, shortT);
              std::cout << "Segment: " << segment_name << ": Action Timeout --> " << longT << "; Short Timeout --> " << shortT << std::endl;
            }
          else
            {
              std::cerr << "Failed to find segment object with ID \"" << segment_name << "\"\n";
              return (EXIT_FAILURE);
            }
        }
      else
        {
          std::cerr << "Failed to find partition object with name \"" << partition_name << "\"\n";
          return (EXIT_FAILURE);
        }
    }
  catch (dunedaq::oksdbinterfaces::Exception & ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return (EXIT_FAILURE);
    }
  catch (dunedaq::dal::AlgorithmError & ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return (EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}

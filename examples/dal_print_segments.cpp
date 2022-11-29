#include <boost/program_options.hpp>

#include <config/Configuration.h>

#include <dal/OnlineSegment.h>
#include <dal/Partition.h>
#include <dal/ResourceSet.h>
#include <dal/Segment.h>

#include <dal/util.h>

namespace po = boost::program_options;

static void
print_resource(const daq::core::ResourceBase * res, unsigned int recursion_level)
{
  std::cout << std::string(recursion_level * 2, ' ') << res->UID() << std::endl;

  if (const daq::core::ResourceSet * rset = res->cast<daq::core::ResourceSet>())
    {
      for (const auto & r : rset->get_Contains())
        print_resource(r, recursion_level + 1);
    }
}

static void
print_segment(const daq::core::Segment * seg, unsigned int recursion_level)
{
  std::cout << std::string(recursion_level * 2, ' ') << seg->UID() << std::endl;

  for (const auto & s : seg->get_nested_segments())
    print_segment(s, recursion_level + 1);

  for (const auto & r : seg->get_Resources())
    print_resource(r, recursion_level + 1);
}

int
main(int argc, char *argv[])
{
    // prepare command line parser

  po::options_description desc("This program prints out tree of segments and their resources.");

  std::string data;
  std::string name;

  // parse command line
  try
    {
      desc.add_options()
          ("data,d", po::value<std::string>(&data), "database name")
          ("partition-name,p", po::value<std::string>(&name)->required(), "partition name")
          ("help,h", "Print help message");

      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return EXIT_FAILURE;
        }

      po::notify(vm);
    }
  catch (std::exception& ex)
    {
      std::cerr << "ERROR: failed to parse command line: " << ex.what() << std::endl;
      return EXIT_FAILURE;
    }


  try
    {
      Configuration db(data);

      if (const daq::core::Partition * p = daq::core::get_partition(db, name))
        {
          db.register_converter(new daq::core::SubstituteVariables(*p));
          print_segment(p->get_segment(p->get_OnlineInfrastructure()->UID()), 0);
        }
      else
        {
          std::cerr << "ERROR: cannot find partition " << name << std::endl;
          return EXIT_FAILURE;
        }
    }
  catch (ers::Issue & ex)
    {
      std::cerr << "ERROR: " << ex << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

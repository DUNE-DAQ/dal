#include <boost/program_options.hpp>

#include "dal/util.hpp"
#include "exit_status.hpp"

int
main(int argc, char **argv)
{

  std::string partition;

  boost::program_options::options_description desc(
      "Get configuration version. The partition infrastructure has to be running. The program reads version from information service.\n\n"
      "Available options are:");

  try
    {
      desc.add_options()
        ("partition,p", boost::program_options::value<std::string>(&partition)->required(), "name of partition")
        ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return __SuccessExitStatus__;
        }

      boost::program_options::notify(vm);
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return __CommandLineErrorExitStatus__;
    }

  try
    {
      std::string version = ::dunedaq::dal::get_config_version(partition);
      std::cout << version << std::endl;
    }
  catch (const dunedaq::oksdbinterfaces::NotFound & ex)
    {
      std::cerr << ex << std::endl;
      auto params = ex.parameters();
      return (params["type"] == "is value" ? __InfoNotFoundExitStatus__ : __RepositoryNotFoundExitStatus__);
    }
  catch (const dunedaq::oksdbinterfaces::Exception & ex)
    {
      std::cerr << "ERROR: " << ex << std::endl;
      return __FailureExitStatus__;
    }

  return __SuccessExitStatus__;
}

#include <boost/program_options.hpp>

#include "dal/util.hpp"
#include "exit_status.hpp"

int
main(int argc, char **argv)
{

  std::string partition;
  std::string version;
  bool reload;

  boost::program_options::options_description desc(
      "Set configuration version. The partition infrastructure has to be running. The program publishes new version in information service and reloads remote database service.\n\n"
      "Available options are:");

  try
    {
      desc.add_options()
        ("version,v", boost::program_options::value<std::string>(&version)->required(), "set configuration version or erase if empty")
        ("partition,p", boost::program_options::value<std::string>(&partition)->required(), "name of partition")
        ("reload,r", "reload database service")
        ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return __SuccessExitStatus__;
        }

      reload = vm.count("reload");

      boost::program_options::notify(vm);
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return __CommandLineErrorExitStatus__;
    }

  try
    {
      ::dunedaq::dal::set_config_version(partition, version, reload);
    }
  catch (const dunedaq::oksdbinterfaces::NotFound & ex)
    {
      std::cerr << ex << std::endl;
      return __RepositoryNotFoundExitStatus__;
    }
  catch (const dunedaq::oksdbinterfaces::Exception & ex)
    {
      std::cerr << "ERROR: " << ex << std::endl;
      return __FailureExitStatus__;
    }

  return __SuccessExitStatus__;
}

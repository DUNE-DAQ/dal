#include <sstream>
#include <boost/program_options.hpp>

#include <ipc/core.h>

#include "config/Configuration.hpp"

#include "dal/Partition.hpp"
#include "dal/BaseApplication.hpp"
#include "dal/Computer.hpp"
#include "dal/Segment.hpp"

#include "dal/util.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
main(int argc, char *argv[])
{
  try
    {
      IPCCore::init(argc, argv);
    }
  catch (ers::Issue & ex)
    {
      ers::warning(ers::Message(ERS_HERE, ex));
    }

  boost::program_options::options_description desc("This program prints out results of algorithms calculating dependencies of application initialisation and shutdown");

  std::string data, partition_name, app_id;

  try
    {
      std::vector<std::string> app_types_list;
      std::vector<std::string> segments_list;

      desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&data), "name of the database")
        ("partition-name,p", boost::program_options::value<std::string>(&partition_name)->required(), "name of the partition object")
        ("application-id,a", boost::program_options::value<std::string>(&app_id), "id of application")
        ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
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
      Configuration db(data);

      const daq::core::Partition * partition = daq::core::get_partition(db, partition_name);

      if(!partition) return 1;

      db.register_converter(new daq::core::SubstituteVariables(*partition));

      std::vector<const daq::core::BaseApplication *> all_apps = partition->get_all_applications();

      std::set<std::string> t_classes = {"TemplateApplication"};

      for(const auto& a : all_apps)
        {
          if(a->UID() == app_id)
            {
              std::cout << "found " << a->get_base_app() << std::endl;

              std::set<std::string> segments = {a->get_segment()->UID()};
              std::set<const daq::core::Computer*> hosts = {a->get_host()};

              std::vector<const daq::core::BaseApplication *> apps2 = partition->get_all_applications(&t_classes, &segments, &hosts);

              if (!apps2.empty())
                {
                  std::cout << "segment " << a->get_segment() << " contains " << apps2.size() << " template applications running on " << a->get_host() << ":\n";
                  for (const auto &x : apps2)
                    std::cout << " - " << x->UID() << std::endl;
                }

              std::cout << "application " << a->UID() << " initialization dependencies:\n";
              for (const auto &x : a->get_initialization_depends_from(all_apps))
                std::cout << " - " << x->UID() << std::endl;

              std::cout << "application " << a->UID() << " shutdown dependencies:\n";
              for (const auto &x : a->get_shutdown_depends_from(all_apps))
                std::cout << " - " << x->UID() << std::endl;
            }
        }
    }
  catch (ers::Issue & ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return (EXIT_FAILURE);
    }

  return 0;
}


#include <chrono>


#include <boost/program_options.hpp>

#include "config/Configuration.hpp"

#include "dal/BaseApplication.hpp"
#include "dal/Component.hpp"
#include "dal/Partition.hpp"

#include "dal/util.hpp"

template<class T>
void
stop_and_report(T &tp, const std::string& name)
{
  std::cout << "TEST \"" << name << "\" => " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - tp).count() / 1000. << " ms\n";
}


int
main(int argc, char *argv[])
{

  boost::program_options::options_description desc(
      "This program prints out results of disabled() algorithm applied to all objects of classes derived from Component class and to the objects of application configuration");

  std::string data;
  std::string partition_name;

  std::vector<std::string> disabled;
  bool use_algorithm = false;
  bool get_applications = false;
  bool substitute_vars = false;

  try
    {
      std::vector<std::string> app_types_list;
      std::vector<std::string> segments_list;

      desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&data)->required(), "database name")
        ("partition-name,p", boost::program_options::value<std::string>(&partition_name)->required(), "name of the partition object")
        ("disabled,D", boost::program_options::value<std::vector<std::string>>(&disabled)->multitoken(), "run disabled test on these components")
        ("use-algorithm,O", "use optimal get_partition")
        ("get-applications,A", "run get_segment")
        ("substitute-vars,S", "register variables converter")
        ("help,h", "print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return EXIT_SUCCESS;
        }

      if (vm.count("use-algorithm"))
        use_algorithm = true;

      if (vm.count("get-applications"))
        get_applications = true;

      if (vm.count("substitute-vars"))
        substitute_vars = true;

      boost::program_options::notify(vm);
    }
  catch (std::exception &ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }

  try
    {
      ::Configuration db(data);

      auto tp = std::chrono::steady_clock::now();

      const daq::core::Partition *partition = use_algorithm ? daq::core::get_partition(db, partition_name) : db.get<daq::core::Partition>(partition_name);

      stop_and_report(tp, use_algorithm ? "daq::core::get_partition()" : "get<daq::core::Partition>()");

      if (!partition)
        return EXIT_FAILURE;

      if (substitute_vars)
        {
          auto tp2 = std::chrono::steady_clock::now();
          db.register_converter(new daq::core::SubstituteVariables(*partition));
          std::ostringstream text;
          stop_and_report(tp2, "daq::core::SubstituteVariables()");
        }

      for (const auto &x : disabled)
        if (const daq::core::Component *obj = db.get<daq::core::Component>(x))
          {
            auto tp2 = std::chrono::steady_clock::now();
            std::ostringstream text;
            text << "disabled(" << obj << ") = " << std::boolalpha << obj->disabled(*partition);
            stop_and_report(tp2, text.str());
          }

      if (get_applications)
        {
          auto tp2 = std::chrono::steady_clock::now();
          std::ostringstream text;
          text << "get_all_applications(): " << partition->get_all_applications().size();
          stop_and_report(tp2, text.str());
        }

      stop_and_report(tp, "TOTAL");
    }
  catch (ers::Issue &ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

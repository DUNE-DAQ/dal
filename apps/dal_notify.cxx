//
//  FILE: src/dal_notify.cpp
//
//  Example program which shows how to subscribe on application changes
//  and receive notification. It also demonstrates how to switch between
//  OKS and RDB implementations.
//
//  For command line arguments see function usage() or run the program
//  with --help.
//
//  Implementation:
//	<Igor.Soloviev@cern.ch> - June 2003
//
//  Modified:
//

#include <signal.h>
#include <unistd.h>

#include <boost/program_options.hpp>


  // include headers describing abstract configuration interface

#include "config/Change.hpp"
#include "config/Configuration.hpp"


  // include generated headers

#include "dal/Application.hpp"
#include "dal/Component.hpp"
#include "dal/RunControlApplication.hpp"
#include "dal/Partition.hpp"
#include "dal/Variable.hpp"

#include "dal/util.hpp"


  // keep pointers to objects used to test substitution (with command line option -s)

const daq::core::Partition * partition   = nullptr;
daq::core::SubstituteVariables * sv_obj  = nullptr;

void
pre_cb(void * parameter)
{
  std::cout << "PRE-MODIFICATION CALLBACK" << std::endl;

  if (partition)
    {
      std::cout << "The partition object before changes is:\n";
      partition->print(4, true, std::cout);
    }
  else
    std::cout << "The partition object is not set\n";
}


  /**
   *  The callback function is called when changes occurred.
   *  It contains all changes.
   */

void
cb1(const std::vector<ConfigurationChange *> & changes, void * /*parameter*/)
{
  if(sv_obj && partition) {
    std::cout << "CALLBACK 1 calls reset() on SubstituteVariables object to update parameters conversion map" << std::endl;
    sv_obj->reset(*partition);
  }

  std::cout << "CALLBACK 1 (report all changes):\n";

    // iterate changes sorted by classes

  for(auto& j : changes) {

    // get class name
    std::cout << "- there are changes in class \"" << j->get_class_name() << "\"\n";

    // print class name
    for(auto& i : j->get_modified_objs()) {
      std::cout << "  * object \""  << i << "\" was modified\n";
    }

    // print removed objects
    for(auto& i : j->get_removed_objs()) {
      std::cout << "  * object \""  << i << "\" was removed\n";
    }

    // print created objects
    for(auto& i : j->get_created_objs()) {
      std::cout << "  * object \""  << i << "\" was created\n";
    }
  }

  std::cout.flush();
}


void
cb2(const std::vector<ConfigurationChange *> & changes, void * parameter)
{
  try
    {
      std::cout << "CALLBACK 2 (report changes in known classes):\n";

      // the pointer to configuration was passed as user-defined
      // callback parameter (see wait_notification() method below)

      Configuration * configuration = reinterpret_cast<Configuration *>(parameter);

      for (const auto& j : changes)
        {

          // get class name

          const std::string& class_name = j->get_class_name();
          std::cout << "  Changes in class \"" << class_name << "\" are:\n";

          std::vector<std::string>::const_iterator i;

          // print modified objects of known classes (Application, Variable, Partition)
          // and skip others

          for (const auto& i : j->get_modified_objs())
            {
              if (class_name == "Application")
                {
                  if (const daq::core::Application * obj = configuration->get<daq::core::Application>(i))
                    obj->print(4, true, std::cout);
                }
              else if (class_name == "Variable")
                {
                  if (const daq::core::Variable * obj = configuration->get<daq::core::Variable>(i))
                    obj->print(4, true, std::cout);
                }
              else if (class_name == "Partition")
                {
                  if (const daq::core::Partition * obj = configuration->get<daq::core::Partition>(i))
                    obj->print(4, true, std::cout);

                  // print all disabled to test work of auto disabling algorithm
                  if (partition)
                    {
                      std::vector<const daq::core::Component*> objs;
                      configuration->get(objs);

                      std::cout << std::boolalpha << "Found " << objs.size() << " objects of classes derived from Component class.\n"
                          "Apply disabled(p=" << partition << ") algorithm to each of them.\n";
                      for (const auto &x : objs)
                        {
                          try
                            {
                              std::cout << " * disabled(" << partition << ") on object " << x << " returns " << x->disabled(*partition) << std::endl;
                            }
                          catch (...)
                            {
                              ;
                            }
                        }
                    }
                }
              else
                {
                  std::cout << "*** Warning: ignore change for object " << i << " of " << class_name << " class ***\n";
                }

              if (class_name == "RunControlApplication")
                {
                  if (const daq::core::RunControlApplication * obj = configuration->get<daq::core::RunControlApplication>(i))
                    obj->print(4, true, std::cout);
                }
            }
        }

      std::cout.flush();
    }
  catch (dunedaq::config::Exception & ex)
    {
      std::cerr << "ERROR: " << ex << std::endl;
    }
}


extern "C" void signal_handler(int sig)
{
  std::cout << "dal_notify caught signal " << sig << std::endl;	
}


  /**
   *  The main function.
   */

int main(int argc, char *argv[])
{


    // parse command line

  std::string db_name;

  bool subscribe_app = false;
  bool subscribe_var = false;
  bool subscribe_prt = false;
  
  std::string partition4substitution;

  std::vector<std::string> app_objects;
  std::vector<std::string> var_objects;
  std::vector<std::string> prt_objects;


    // parse command line parameters

  boost::program_options::options_description cmdl("Example of selective subscription on changes in dal Application, Environment and Partition classes using the following options");

  try
    {
      cmdl.add_options()
          ("data,d", boost::program_options::value<std::string>(&db_name), "database name")
          ("partition-for-substitution,s", boost::program_options::value<std::string>(&partition4substitution), "partition object to test update of partition's parameters (auto subscribe on changes in Variable class)")
          ("applications,a", boost::program_options::value<std::vector<std::string> >(&app_objects)->multitoken()->zero_tokens(), "subscribe on changes of the given application objects or all if empty")
          ("environment-variables,e", boost::program_options::value<std::vector<std::string> >(&var_objects)->multitoken()->zero_tokens(), "subscribe on changes of the given environment variable objects or all if empty")
          ("partitions,p", boost::program_options::value<std::vector<std::string> >(&prt_objects)->multitoken()->zero_tokens(), "subscribe on changes of the given partition objects or all if empty")
          ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdl), vm);

      if (vm.count("help"))
        {
          std::cout << cmdl << std::endl;
          return EXIT_SUCCESS;
        }

      boost::program_options::notify(vm);

      if (vm.count("applications") && app_objects.empty())
        subscribe_app = true;

      if (vm.count("environment-variables") && var_objects.empty())
        subscribe_var = true;

      if (vm.count("partitions") && prt_objects.empty())
        subscribe_prt = true;
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }

  try
    {
      Configuration conf(db_name);

      signal(SIGINT,signal_handler);
      signal(SIGTERM,signal_handler);


      // find partition and register conversion object, if update of a parameter to be tested
      if (!partition4substitution.empty())
        {
          partition = daq::core::get_partition(conf, partition4substitution);

          if (!partition)
            {
              std::cerr << "ERROR: cannot find partition: \"" << partition4substitution << "\"\n\n";
              return (EXIT_FAILURE);
            }

          sv_obj = new daq::core::SubstituteVariables(*partition);
          conf.register_converter(sv_obj);

          subscribe_var = true;
        }


      // create two subscription criteria

      ::ConfigurationSubscriptionCriteria c1;
      ::ConfigurationSubscriptionCriteria c2;


      // subscribe on all changes in classes

      if (subscribe_app)
        {
          c1.add(daq::core::Application::s_class_name);
          std::cout << "subscribe on any changes in class Application\n";
        }

      if (subscribe_var)
        {
          c1.add(daq::core::Variable::s_class_name);
          std::cout << "subscribe on any changes in class Variable\n";
        }

      if (subscribe_prt)
        {
          c1.add(daq::core::Partition::s_class_name);
          std::cout << "subscribe on any changes in class Partition\n";
        }

      // subscribe on changes of objects in the Application class
      for (const auto& i : app_objects)
        {
          if (const daq::core::Application * obj = conf.get<daq::core::Application>(i))
            {
              c2.add(*obj);
              std::cout << "subscribe on modifications of object \"" << i << "\" from Application class\n";
            }
          else
            {
              std::cerr << "ERROR: Can not find Application object with id \"" << i << "\"\n";
              return (EXIT_FAILURE);
            }
        }


      // subscribe on changes of objects in the Environment class
      for (const auto& i : var_objects)
        {
          if (const daq::core::Variable * obj = conf.get<daq::core::Variable>(i))
            {
              c2.add(*obj);
              std::cout << "subscribe on modifications of object \"" << i << "\" from Variable class\n";
            }
          else
            {
              std::cerr << "ERROR: Can not find Variable object with id \"" << i << "\"\n";
              return (EXIT_FAILURE);
            }
        }


      // subscribe on changes of objects in the Partition class
      for (const auto& i : prt_objects)
        {
          if (const daq::core::Partition * obj = conf.get<daq::core::Partition>(i))
            {
              c2.add(*obj);
              std::cout << "subscribe on modifications of object \"" << i << "\" from Partition class\n";
            }
          else
            {
              std::cerr << "ERROR: Can not find Partition object with id \"" << i << "\"\n";
              return (EXIT_FAILURE);
            }
        }

      // wait for notification on changes
      // note, that there is no return from second method in case of success

      Configuration::CallbackId id1 = conf.subscribe(c1, cb1, reinterpret_cast<void *>(&conf));
      conf.subscribe(c2, cb2, reinterpret_cast<void *>(&conf));

      Configuration::CallbackId id3 = conf.subscribe(pre_cb, nullptr);

      try
        {
          pause();
        }
      catch (...)
        {
          ;
        }

      std::cout << "Exiting dal_notify ..." << std::endl;

      conf.unsubscribe(id1);
      conf.unsubscribe(id3);

      conf.unsubscribe(); // unsubscribe cb2
    }
  catch (dunedaq::config::Exception & ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return (EXIT_FAILURE);
    }

  return 0;
}

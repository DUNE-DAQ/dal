#include <sstream>
#include <set>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "config/Configuration.hpp"

#include "dal/BaseApplication.hpp"
#include "dal/Component.hpp"
#include "dal/OnlineSegment.hpp"
#include "dal/Partition.hpp"

#include "dal/util.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
run_test(const dunedaq::dal::Component * obj, const dunedaq::dal::Partition * partition)
{
  TLOG_DEBUG(1) <<  "call disabled(" << partition << ") on object " << obj ;

  try
    {
      bool result = obj->disabled(*partition);
      std::cout << " * disabled(" << partition << ") on object " << obj << " returns " << result << std::endl;
    }
  catch (ers::Issue& ex)
    {
      std::cerr << "ERROR: disabled(" << partition << ") algorithm on object " << obj << " fails: " << ex << std::endl;
    }
}

static void
test_application(const dunedaq::dal::BaseApplication * a, const dunedaq::dal::Partition * partition)
{
  if (const dunedaq::dal::Component * c = a->cast<dunedaq::dal::Component>())
    {
      TLOG_DEBUG(1) <<  "call disabled(" << partition << ") on resource application " << a ;

      try
        {
          bool result = c->disabled(*partition);

          if (a->get_base_app() != a)
            {
              bool result2 = a->get_base_app()->cast<dunedaq::dal::Component>()->disabled(*partition, true);

              if (result != result2)
                {
                  std::cerr << " * ERROR: disabled(" << partition << ") on resource application object " << a << " returns " << result << ", but on base application " << a->get_base_app() << " the result is " << result2 << std::endl;
                  return;
                }
            }

          std::cout << " * disabled(" << partition << ") on resource application object " << a << " returns " << result << std::endl;
        }
      catch (...)
        {
          ;
        }
    }
}

static void
test_segment(const dunedaq::dal::Segment * s, const dunedaq::dal::Partition * partition)
{

  TLOG_DEBUG(1) <<  "call disabled(" << partition << ") on segment " << s ;
  try
    {
      std::cout << " * disabled(" << partition << ") on segment object " << s << " returns " << s->disabled(*partition) << std::endl;

      for (const auto& ns : s->get_nested_segments())
        test_segment(ns, partition);
    }
  catch (...)
    {
      ;
    }
}



int
main(int argc, char *argv[])
{

  boost::program_options::options_description desc("This program prints out results of disabled() algorithm applied to all objects of classes derived from Component class and to the objects of application configuration");

  std::vector<std::string> data_list;
  std::string partition_name;
  std::vector<std::string> disabled_list;
  std::vector<std::string> enabled_list;

  bool test_applications = false;
  bool test_segments = false;

  try
    {
      std::vector<std::string> app_types_list;
      std::vector<std::string> segments_list;

      desc.add_options()
        ("data,d", boost::program_options::value<std::vector<std::string>>(&data_list), "names of the database (run test sequentially on each database file)")
        ("partition-name,p", boost::program_options::value<std::string>(&partition_name)->required(), "name of the partition object")
        ("disabled,D", boost::program_options::value<std::vector<std::string>>(&disabled_list)->multitoken(), "identities of components disabled by user")
        ("enabled,E", boost::program_options::value<std::vector<std::string>>(&enabled_list)->multitoken(), "identities of components enabled by user (override DB disabling)")
        ("test-applications,a", "test resource applications (including templated)")
        ("test-segments,s", "test segments (including templated)")
        ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return EXIT_SUCCESS;
        }

      if (vm.count("test-applications"))
        test_applications = true;

      if (vm.count("test-segments"))
        test_segments = true;

      boost::program_options::notify(vm);
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }

  for (size_t i=0; i < data_list.size(); ++i)
    {
      const std::string& data = data_list[i];
      if (data_list.size() != 1)
        std::cout << "=====================================================================================\nTEST " << data << std::endl;

      try
        {
          ::Configuration db(data);

          const dunedaq::dal::Partition * partition = dunedaq::dal::get_partition(db, partition_name);

          if(!partition) return 1;

          db.register_converter(new dunedaq::dal::SubstituteVariables(*partition));

          std::vector<const dunedaq::dal::Component *> objs;

          db.get(objs);

          std::cout << "Apply disabled() algorithm on " << objs.size() << " objects of base Component class.\n";

          std::cout << std::boolalpha;

          for (const auto& i : objs)
            run_test(i, partition);

          const dunedaq::dal::Segment* rootSeg = nullptr;
          std::vector<const dunedaq::dal::BaseApplication *> apps;

          if (test_segments || test_applications)
            rootSeg = partition->get_segment(partition->get_OnlineInfrastructure()->UID());

          if (test_segments)
            {
              std::cout << "Test partition segments\n";
              test_segment(rootSeg, partition);
            }

          if (test_applications)
            {
              apps = rootSeg->get_all_applications();
              std::cout << "Apply disabled algorithm on all resources applications\n";
              for(const auto& app : apps)
                test_application(app, partition);
            }

          if(!disabled_list.empty() || !enabled_list.empty())
            {
              std::cout << "There are " << disabled_list.size() << " objects disabled and there are " << enabled_list.size() << " objects enabled by user.\nApply user configuration and repeat the test.\n";

              std::set<const dunedaq::dal::Component *> user_disabled;
              std::set<const dunedaq::dal::Component *> user_enabled;

              for (size_t i=0; i < disabled_list.size(); ++i)
                {
                  try
                    {
                      if(const dunedaq::dal::Component * c = db.get<dunedaq::dal::Component>(disabled_list[i]))
                        {
                          user_disabled.insert(c);
                        }
                    }
                  catch(ers::Issue & ex)
                    {
                      std::cerr << "ERROR: trying to get component \'" << disabled_list[i] << "\' caught " << ex << std::endl;
                    }
                }

              for (size_t i=0; i < enabled_list.size(); ++i)
                {
                  try
                    {
                      if(const dunedaq::dal::Component * c = db.get<dunedaq::dal::Component>(enabled_list[i]))
                        {
                          user_enabled.insert(c);
                        }
                    }
                  catch(ers::Issue & ex)
                    {
                      std::cerr << "ERROR: trying to get component \'" << enabled_list[i] << "\' caught " << ex << std::endl;
                    }
                }

              if (!user_disabled.empty())
                partition->set_disabled(user_disabled);

              if (!user_enabled.empty())
                partition->set_enabled(user_enabled);

              std::cout << "Re-apply disabled() algorithm on " << objs.size() << " objects of base Component class.\n";
              for (const auto& i : objs)
                run_test(i, partition);

              if (test_segments || test_applications)
                rootSeg = partition->get_segment(partition->get_OnlineInfrastructure()->UID());

              if (test_segments)
                {
                  std::cout << "Re-test partition segments\n";
                  test_segment(rootSeg, partition);
                }

              if (test_applications)
                {
                  apps = rootSeg->get_all_applications();
                  std::cout << "Re-apply disabled algorithm on all resources applications\n";
                  for(const auto& app : apps)
                    test_application(app, partition);
                }

            }
        }
      catch (ers::Issue & ex)
        {
          std::cerr << "Caught " << ex << std::endl;
          return (EXIT_FAILURE);
        }
    }

  return 0;
}

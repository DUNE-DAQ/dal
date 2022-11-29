#include <iostream>
#include <string>

#include <boost/program_options.hpp>

#include <config/Configuration.h>
#include <config/ConfigObject.h>

#include "dal/Computer.h"
#include "dal/Module.h"
#include "dal/Rack.h"

ERS_DECLARE_ISSUE(
  dal_test_rw,
  ConfigException,
  "caught daq::config::Exception exception",
)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
  void
  set_value(ConfigObject& o, const std::string& name, T value)
  {
    o.set_by_val(name, value);
  }

template<class T>
  void
  set_ref(ConfigObject& o, const std::string& name, T value)
  {
    o.set_by_ref(name, value);
  }

template<class T>
  void
  check_value(ConfigObject& o, const std::string& name, T v1)
  {
    T v2;
    o.get(name, v2);
    if (v1 != v2)
      {
        std::cerr << "ERROR reading attribute: \'" << name << '\'' << std::endl;
      }
    else
      {
        std::cout << "TEST " << name << " of " << o << " is OK\n";
      }
  }

void
check_object(ConfigObject& o, const std::string& name, ConfigObject * o1)
{
  ConfigObject o2;
  o.get(name, o2);
  if (o1 == nullptr)
    {
      if (!o2.is_null())
        {
          std::cerr << "ERROR reading relationship: \'" << name << "\' (read an object instead of NULL)" << std::endl;
        }
    }
  else
    {
      if (o2.is_null())
        {
          std::cerr << "ERROR reading relationship: \'" << name << "\' (read NULL instead of object)" << std::endl;
        }
      else
        {
          if (!(o2 == *o1))
            {
              std::cerr << "ERROR reading relationship: \'" << name << "\' (read and wrote objects are different)" << std::endl;
            }
        }
    }

  std::cout << "TEST value of " << name << " relationship of object " << o << " is OK: read " << o2 << std::endl;
}

void
check_objects(ConfigObject& o, const std::string& name, const std::vector<const ::ConfigObject*> o1)
{
  std::vector<ConfigObject> o2;

  o.get(name, o2);

  if (o1.size() != o2.size())
    {
      std::cerr << "ERROR reading relationship: \'" << name << "\' (read vector of different size)" << std::endl;
    }
  else
    {
      for (unsigned int i = 0; i < o1.size(); ++i)
        {
          if (!(*o1[i] == o2[i]))
            {
              std::cerr << "ERROR reading relationship: \'" << name << "\' (objects " << i << " are different)" << std::endl;
            }
        }
    }

  std::cout << "TEST values of " << name << " relationship of object " << o << " is OK: read ";
  for (unsigned int i = 0; i < o1.size(); ++i)
    {
      if (i != 0)
        std::cout << ", ";
      std::cout << o2[i];
    }
  std::cout << std::endl;
}


#define INIT(T, X, V)            \
for(T v = X - 16; v <= X;) {     \
  V.push_back(++v);              \
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static std::string
create_data(::Configuration& db, const std::string& name, const std::string& suffix)
{
  static const std::string schema_name("daq/schema/core.schema.xml");
  static const std::string rack_name("rack");
  static const std::string lfs_name("lfs");
  static const std::string node_name("node");

  std::string f(name);
  f += suffix;

  db.create(f, std::list<std::string>(1, schema_name));

  daq::core::Rack * rack = const_cast<daq::core::Rack *>(db.create<daq::core::Rack>(f, rack_name + suffix));

  std::vector<const daq::core::ComputerBase*> rack_nodes;
  std::vector<const daq::core::Computer*> rack_lfs;

  rack_lfs.push_back(const_cast<daq::core::Computer *>(db.create<daq::core::Computer>(*rack, lfs_name + suffix)));
  rack_nodes.push_back(const_cast<daq::core::Computer *>(db.create<daq::core::Computer>(*rack, node_name + suffix + "-1")));
  rack_nodes.push_back(const_cast<daq::core::Computer *>(db.create<daq::core::Computer>(*rack, node_name + suffix + "-2")));

  rack->set_LFS(rack_lfs);
  rack->set_Nodes(rack_nodes);

  return f;
}


int
main(int argc, char *argv[])
{
  const std::string schema_name("daq/schema/core.schema.xml");

  std::string data_name, plugin_spec;

  boost::program_options::options_description cmdl("The utility tests creation of files and objects using different plugins, ant tests validity of objects after include removals and composite parent destruction using the following options");

  try
    {
      cmdl.add_options()
          ("data,d", boost::program_options::value<std::string>(&data_name)->required(), "name of data file to be created")
          ("database,s", boost::program_options::value<std::string>(&plugin_spec)->required(), "database specification: config plugin (oksconfig | rdbconfig:server-name)")
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
      ::Configuration db(plugin_spec);

      // create 7 db files each containing 1 racks, 1 LFS and 2 nodes

      std::string f = create_data(db, data_name, "");
      std::string f1 = create_data(db, data_name, ".1");
      std::string f2 = create_data(db, data_name, ".2");
      std::string f11 = create_data(db, data_name, ".11");
      std::string f12 = create_data(db, data_name, ".12");
      std::string f21 = create_data(db, data_name, ".21");
      std::string f22 = create_data(db, data_name, ".22");

      // top-level file includes 2 files, which include 2 or 3 leave files

      db.add_include(f1, f11);
      db.add_include(f1, f12);

      db.add_include(f2, f21);
      db.add_include(f2, f22);
      db.add_include(f2, f12);

      db.add_include(f, f1);
      db.add_include(f, f2);

      db.commit("test");

      // create map of nodes (key: UID, value: object ptr)

      std::map<std::string, const daq::core::Computer*> nodes_info;

        {
          std::vector<const daq::core::Computer*> nodes;

          db.get(nodes);

          for (const auto& i : nodes)
            nodes_info[i->UID()] = i;
        }

      // remove include f1 from root file, that results several files to be closed
      // and several template objects to be invalidated

      db.remove_include(f, f1);

      std::cout << "TEST NODES AFTER REMOVAL OF " << f1 << " INCLUDE:\n";
      for (auto i = nodes_info.begin(); i != nodes_info.end();)
        {
          try
            {
              i->second->get_State();  // cause exception if object was deleted
              std::cout << " - object " << i->second << " is OK\n";
              ++i;
            }
          catch (daq::config::DeletedObject& ex)
            {
              std::cout << " - node \'" << i->first << "\' was deleted\n";
              nodes_info.erase(i++);
            }
        }

      // get list of racks, no deleted objects should be there

      std::cout << "\nTEST RACKS:\n";
      std::vector<const daq::core::Rack*> racks;
      db.get(racks);
      for (const auto& i : racks)
        {
          std::cout << " - object " << i << std::endl;
        }

      daq::core::Computer * a_good_computer(nullptr);

      // now destroy a rack and see which nodes are invalidated as well

      std::cout << "\nTEST NODES AFTER DESTRUCTION OF " << racks[0] << " OBJECT:\n";
      db.destroy(*racks[0]);

      for (const auto& i : nodes_info)
        {
          try
            {
              i.second->get_State();  // cause exception if object was deleted
              if (i.second->config_object().contained_in() != f21 && i.second->config_object().contained_in() != f22)
                {
                  std::cout << " - skip node \'" << i.first << "\' contained in " << i.second->config_object().contained_in() << "\n";
                  continue;
                }
              std::cout << " - object " << i.second << " is OK\n";
              a_good_computer = const_cast<daq::core::Computer *>(i.second);
            }
          catch (daq::config::DeletedObject& ex)
            {
              std::cout << " - node \'" << i.first << "\' was deleted\n";
            }
        }

      std::cout << std::endl;

      // move object

      if (a_good_computer)
        {
          const std::string& from(a_good_computer->config_object().contained_in());
          const std::string& to(from == f22 ? f21 : f22);

          std::cout << "\nTEST: move " << a_good_computer << " from " << from << " to " << to;

          a_good_computer->move(to);

          if (a_good_computer->config_object().contained_in() == to)
            {
              std::cout << " (OK)\n";
            }
          else
            {
              std::cout << " (FAILED)\n"
                  "contained_in() returned " << a_good_computer->config_object().contained_in() << " instead of expected " << to << std::endl;
            }
        }

      db.commit("test2");

      // rename object (also check changes in base classes following inheritance hierarchy)

      const daq::core::HW_Object * hw_object = a_good_computer->cast<daq::core::HW_Object>();

      std::string old_uid(a_good_computer->UID());
      std::ostringstream new_uid_s;
      new_uid_s << static_cast<const void*>(&old_uid);
      std::string new_uid = new_uid_s.str();

      std::cout << "\nTEST: rename object " << a_good_computer << " to \'" << new_uid << "\'\n";
      a_good_computer->rename(new_uid);

      std::cout
        << " * UID changed in class of object: " << ((a_good_computer->UID() == new_uid) ? "(OK)" : "(FAILED)") << std::endl
        << " * UID changed in base class: " << ((hw_object->UID() == new_uid) ? "(OK)" : "(FAILED)") << std::endl
        << " * can get object by ID in the class of object: " << ((db.get<daq::core::Computer>(new_uid) == a_good_computer) ? "(OK)" : "(FAILED)") << std::endl
        << " * can get object by ID in the base class: " << ((db.get<daq::core::HW_Object>(new_uid) == hw_object) ? "(OK)" : "(FAILED)") << std::endl
        << " * cannot get object by old ID in the class of object: " << ((db.get<daq::core::Computer>(old_uid) == nullptr) ? "(OK)" : "(FAILED)") << std::endl
        << " * cannot get object by old ID in the base class: " << ((db.get<daq::core::HW_Object>(old_uid) == nullptr) ? "(OK)" : "(FAILED)") << std::endl;

      // try to create module with UID of computer
      // the call has to fail, since both classes are derived from the sane base class (HW_Object)

      try
        {
          std::cout << "\nTEST: try to create module with id of existing computer object " << a_good_computer->UID() << ' ';
          daq::core::Module * bad_module = const_cast<daq::core::Module *>(db.create<daq::core::Module>(*a_good_computer, a_good_computer->UID()));
          std::cout << " => the object " << bad_module << " was created: (FAILED)\n";
        }
      catch (daq::config::Exception & ex)
        {
          std::cout << " => the object was not created, caught exception \"" << ex.what() << "\": (OK)\n";
        }

      return EXIT_SUCCESS;
    }
  catch (daq::config::Exception & ex)
    {
      ers::fatal(dal_test_rw::ConfigException(ERS_HERE, ex));
    }

  return (EXIT_FAILURE);
}

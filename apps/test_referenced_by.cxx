#include "config/Configuration.hpp"

int
main(int argc, char **argv)
{
  try
    {
      ::Configuration db("oksconfig:/tmp/be_test.data.xml");  // oks_merge -o /tmp/be_test.data.xml -s /tmp/be_test.schema.xml daq/partitions/be_test.data.xml

      ConfigObject ipc_server;
      db.get("InfrastructureApplication", "ipc-server", ipc_server);

      std::cout << "Object " << ipc_server << std::endl;
      ipc_server.print_ref(std::cout, db);

        {
          std::vector<ConfigObject> objs;

          ipc_server.referenced_by(objs);

          std::cout << "is referenced by " << objs.size() << " objects:\n";

          for (std::vector<ConfigObject>::const_iterator i = objs.begin(); i != objs.end(); ++i)
            std::cout << " - " << *i << std::endl;
        }

      ConfigObject root_controller;
      db.get("RunControlApplication", "RootController", root_controller);

      std::vector<ConfigObject> objs;
      root_controller.get("InitializationDependsFrom", objs);

      std::vector<const ::ConfigObject*> objs2;
      for (int j = 0; j < objs.size(); ++j)
        {
          if (objs[j].UID() != ipc_server.UID())
            objs2.push_back(&objs[j]);
          else
            std::cout << "skip object " << objs[j] << std::endl;
        }

      root_controller.set_objs("InitializationDependsFrom", objs2);

        {
          std::vector<ConfigObject> objs;

          ipc_server.referenced_by(objs);

          std::cout << ipc_server << " is referenced by " << objs.size() << " objects:\n";

          for (std::vector<ConfigObject>::const_iterator i = objs.begin(); i != objs.end(); ++i)
            std::cout << " - " << *i << std::endl;
        }

      ConfigObject crate1, crate2;
      ConfigObject module1_1, module1_2, module2_1, module2_2, module3_x;

      db.create(root_controller, "Crate", "1", crate1);
      db.create(root_controller, "Crate", "2", crate2);
      db.create(root_controller, "Module", "1.1", module1_1);
      db.create(root_controller, "Module", "1.2", module1_2);
      db.create(root_controller, "Module", "2.1", module2_1);
      db.create(root_controller, "Module", "2.2", module2_2);
      db.create(root_controller, "Module", "3.x", module3_x);

      std::vector<const ::ConfigObject*> modules1;
      std::vector<const ::ConfigObject*> modules2_1;
      std::vector<const ::ConfigObject*> modules2_2;

      modules1.push_back(&module1_1);
      modules1.push_back(&module1_2);
      modules1.push_back(&module3_x);

      modules2_1.push_back(&module2_1);
      modules2_1.push_back(&module2_2);

      modules2_2.push_back(&module2_1);
      modules2_2.push_back(&module3_x);

      crate1.set_objs("Modules", modules1);
      crate2.set_objs("Modules", modules2_1);

      std::cout << "Object " << crate1 << std::endl;
      crate1.print_ref(std::cout, db);
      std::cout << std::endl;

      std::cout << "Object " << crate2 << std::endl;
      crate2.print_ref(std::cout, db);
      std::cout << std::endl;

      std::vector<const ::ConfigObject*> all_modules;
      all_modules.push_back(&module1_1);
      all_modules.push_back(&module1_2);
      all_modules.push_back(&module3_x);
      all_modules.push_back(&module2_1);
      all_modules.push_back(&module2_2);

      for (const auto& x : all_modules)
        {
          std::vector<ConfigObject> objs;

          x->referenced_by(objs);

          std::cout << x << " is referenced by " << objs.size() << " objects:\n";

          for (const auto& i : objs)
            std::cout << " - " << i << std::endl;
        }

      try
        {
          crate2.set_objs("Modules", modules2_2);
        }
      catch (daq::config::Exception & ex)
        {
          std::cerr << "Expected error: " << ex << std::endl;
        }

      std::cout << "Object " << crate1 << std::endl;
      crate1.print_ref(std::cout, db);
      std::cout << std::endl;

      std::cout << "Object " << crate2 << std::endl;
      crate2.print_ref(std::cout, db);
      std::cout << std::endl;

      for (const auto& x : all_modules)
        {
          std::vector<ConfigObject> objs;

          x->referenced_by(objs);

          std::cout << x << " is referenced by " << objs.size() << " objects:\n";

          for (const auto& i : objs)
              std::cout << " - " << i << std::endl;
        }

    }
  catch (daq::config::Exception & ex)
    {
      std::cerr << "ERROR: caught " << ex << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}

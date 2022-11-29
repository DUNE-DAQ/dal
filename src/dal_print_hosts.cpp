//
//  confdb_print_hosts.cpp
//
//  This programs retrieves the list of computers from the database
//  for all the applications of a partition, prints the list to standard
//  output stream, then exits.
//
//  Created:
//	- Igor Soloviev 18 August 1999 (on the base of rc_printHosts.cc)
//
//  Modifications:
//
//	- Igor Soloviev (ATD Back-End group) - Nov 2000
//		- convert from RW Tools.h++ to STL
//
//	- Igor Soloviev (ATD OnlineSW group) - Sep 2000
//		- add option to print host remote login command
//		- add option to define partition default host
//
//	- Igor Soloviev (ATD OnlineSW group) - Dec 2003
//		- port to generated DAL
//
//      - Igor Soloviev (ATLAS C&C WG) - Sep 2007
//              - remove options working with partition default host
//                since the Partition::get_all_applications() calculates
//                the localhost internally
//	- Giovanna Lehmann Miotto (ATLAS C&C WG) - Jul 2009
//		- add option to print host for a single application

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "config/Configuration.h"

#include "dal/BaseApplication.h"
#include "dal/Computer.h"
#include "dal/CustomLifetimeApplicationBase.h"
#include "dal/Partition.h"

#include "dal/util.h"


static void
printHost(const daq::core::Computer * host, bool print_rl_cmd, bool print_tag)
{
  std::cout << host->UID();

  if(print_rl_cmd) {
    std::cout << ' ' << host->get_RLogin();
  }

  if(print_tag) {
    std::cout << ' ' << host->get_HW_Tag();
  }

  std::cout << std::endl;
}


int
main(int argc, char **argv)
{
  std::string data;
  std::string partition;
  std::string application;

  bool ignore_non_restartable = false;
  bool ignore_started_at_boot = false;
  bool ignore_started_at_sor = false;
  bool ignore_started_at_eor = false;
  bool ignore_started_by_user = false;
  bool ignore_stopped_at_sor = false;
  bool ignore_stopped_at_eor = false;
  bool ignore_stopped_at_shut = false;
  bool ignore_stopped_by_user = false;
  bool print_rl_cmd = false;
  bool print_tag = false;
  bool print_all = false;

  boost::program_options::options_description desc(
      "This program prints out the set of hosts which are used by the applications for given configuration. "
      "The configuration is defined by the partition object defined by object id provided via -p parameter. "
      "Use any combination of controlled/restartable/start/stop options to ignore applications of required type. "
      "If no such options given, ALL hosts used by the partition are printed. "
      "Use -L option to print host's remote-login-command. Use -T option to print host's hardware tag. "
      "The databases implementation and name are defined by -d option or by the TDAQ_DB environment variable.\n\n"
      "Available options are:");

  try
    {
      desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&data), "name of the database")
        ("partition-id,p", boost::program_options::value<std::string>(&partition)->required(), "name of the partition object")
        ("application-id,a", boost::program_options::value<std::string>(&application), "prints host only for this application")
        ("ignore-non-restartable,R","ignore applications which are not restarted if failed or dead")
        ("ignore-started-at-boot,B","ignore applications which are started at boot")
        ("ignore-started-at-sor,S","ignore applications which are started at start of run")
        ("ignore-started-at-eor,E","ignore applications which are started at end of run")
        ("ignore-started-by-user,U","ignore applications which are started by user")
        ("ignore-stopped-at-sor,C","ignore applications which are stopped at start of run")
        ("ignore-stopped-at-eor,P","ignore applications which are stopped at end of run")
        ("ignore-stopped-at-shutdown,W","ignore applications which are stopped at shutdown")
        ("ignore-stopped-by-user,V","ignore applications which are stopped by user")
        ("print-remote-login-command,L","prints host remote login command")
        ("print-binary-tag,T","prints host hardware tag")
        ("all,A","prints any defined host including non-used by the partition")
        ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return EXIT_SUCCESS;
        }

      boost::program_options::notify(vm);

      if (vm.count("ignore-non-restartable"))
        ignore_non_restartable = true;

      if (vm.count("ignore-started-at-boot"))
        ignore_started_at_boot = true;

      if (vm.count("ignore-started-at-sor"))
        ignore_started_at_sor = true;

      if (vm.count("ignore-started-at-eor"))
        ignore_started_at_eor = true;

      if (vm.count("ignore-started-by-user"))
        ignore_started_by_user = true;

      if (vm.count("ignore-stopped-at-sor"))
        ignore_stopped_at_sor = true;

      if (vm.count("ignore-stopped-at-eor"))
        ignore_stopped_at_eor = true;

      if (vm.count("ignore-stopped-at-shutdown"))
        ignore_stopped_at_shut = true;

      if (vm.count("ignore-stopped-by-user"))
        ignore_stopped_by_user = true;

      if (vm.count("print-remote-login-command"))
        print_rl_cmd = true;

      if (vm.count("print-binary-tag"))
        print_tag = true;

      if (vm.count("all"))
        print_all = true;
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }

  try
    {
      Configuration conf(data);

      const daq::core::Partition * p = daq::core::get_partition(conf, partition);

      if(!p)
        return 1;

      conf.register_converter(new daq::core::SubstituteVariables(*p));

      // print all hosts defined by the configuration
      if (print_all)
        {
          std::vector<const daq::core::Computer*> hosts;
          conf.get(hosts);

          for (const auto& i : hosts)
            printHost(i, print_rl_cmd, print_tag);
        }
      else
        {

          // remember visited hosts
          std::string appName(application);
          std::set<const daq::core::Computer *, std::less<const daq::core::Computer *> > hosts;

          // iterate over the list of applications to retrieve host names and add them to "hosts"
          for(auto & i : p->get_all_applications())
            {
              const daq::core::BaseApplication * a = i->get_base_app();

 	      // search only for one specific application
	      if (!appName.empty() && (i->UID() != appName))
	        continue;

              // ignore non-restartable applications if required
              if(
                ignore_non_restartable && a->get_IfExitsUnexpectedly() != daq::core::BaseApplication::IfExitsUnexpectedly::Restart &&
                a->get_IfFailsToStart() != daq::core::BaseApplication::IfFailsToStart::Restart
              ) continue;

              if(const daq::core::CustomLifetimeApplicationBase * ca = a->cast<daq::core::CustomLifetimeApplicationBase>())
                {
                  // ignore started at certain moment if required
                  if(
                    (ignore_started_at_boot && ca->get_Lifetime() == daq::core::CustomLifetimeApplicationBase::Lifetime::Boot_Shutdown)       ||
                    (ignore_started_at_sor  && ca->get_Lifetime() == daq::core::CustomLifetimeApplicationBase::Lifetime::SOR_EOR)             ||
                    (ignore_started_at_eor  && ca->get_Lifetime() == daq::core::CustomLifetimeApplicationBase::Lifetime::EOR_SOR)             ||
                    (ignore_started_by_user && ca->get_Lifetime() == daq::core::CustomLifetimeApplicationBase::Lifetime::UserDefined_Shutdown)
                  ) continue;


                  // ignore stopped at certain moment if required
                  if(
                    (ignore_stopped_at_sor  && ca->get_Lifetime() == daq::core::CustomLifetimeApplicationBase::Lifetime::EOR_SOR)             ||
                    (ignore_stopped_at_eor  && ca->get_Lifetime() == daq::core::CustomLifetimeApplicationBase::Lifetime::SOR_EOR)             ||
                    (ignore_stopped_at_shut && ca->get_Lifetime() == daq::core::CustomLifetimeApplicationBase::Lifetime::Boot_Shutdown)       ||
                    (ignore_stopped_by_user && ca->get_Lifetime() == daq::core::CustomLifetimeApplicationBase::Lifetime::UserDefined_Shutdown)
                  ) continue;
                }

              const daq::core::Computer * host = i->get_host();

              if (hosts.find(host) == hosts.end())
                {
                  hosts.insert(host);
                  printHost(host, print_rl_cmd, print_tag);
                }
            }
        }
    }
  catch (daq::config::Exception & ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return (EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}

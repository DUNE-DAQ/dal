//
//  FILE: src/dal_create_db_connection_files.cpp
//
//  Author: G. Lehmann Miotto
//
//  Implementation:
//
//  Modified:
//

#include <fstream>
#include <filesystem>
#include <stdlib.h>

#include <boost/program_options.hpp>

#include "ers/ers.h"

#include "ipc/core.h"

#include "config/Configuration.h"

#include "dal/Partition.h"
#include "dal/TriggerConfiguration.h"
#include "dal/DBConnection.h"
#include "dal/TriggerDBConnection.h"

#include "dal/util.h"

typedef   std::map<std::string,std::string> TranslationMap;

namespace daq
{
  ERS_DECLARE_ISSUE_BASE(
    core,
    BadDBType,
    AlgorithmError,
    "Failed to identify DB type \'" << type << "\' from the database object \'" << dbt << "\'",
    ,
    ((std::string)type)
    ((std::string) dbt)
  )

  ERS_DECLARE_ISSUE( core,
    CantCreateDirectory,
    "Directory '" << directory << "' can not be created",
    ((std::string)directory )
    )

  ERS_DECLARE_ISSUE( core,
    CannotChangePermissions,
    "File permissions on " << file << " can not be changed because: " << reason,
    ((std::string)file )
    ((std::string)reason )
    )
}

 std::string lowcase(const std::string & mixed)
   {
      std::string lowcase;
      lowcase.reserve(mixed.length());
      for ( std::string::size_type i = 0; i< mixed.length(); ++i)
      {
         lowcase += std::tolower(mixed[i]);
      }
      return lowcase;
   }


int main(int ac, char *av[])
{

   try {
      IPCCore::init(ac, av);
   }
   catch(ers::Issue & e) {
      ers::fatal(e);
      abort();
   }

  // take defaults from environment
  std::string tdaq_partition(""), tdaq_db(""), lookup("");
  if (getenv("TDAQ_PARTITION"))
     tdaq_partition = getenv("TDAQ_PARTITION");
  if (getenv("TDAQ_DB"))
    tdaq_db = getenv("TDAQ_DB");
  if (getenv("CORAL_DBLOOKUP_PATH"))
    lookup = getenv("CORAL_DBLOOKUP_PATH");

  // parse commandline parameters

  boost::program_options::options_description desc("Allowed options", 128);
  desc.add_options()
    ("help,h", "help message")
    ("partition,p",   boost::program_options::value<std::string>(&tdaq_partition)->default_value(tdaq_partition), "partition name (TDAQ_PARTITION)")
    ("database,d",    boost::program_options::value<std::string>(&tdaq_db)->default_value(tdaq_db), "database (TDAQ_DB)")
    ("lookup_path,l", boost::program_options::value<std::string>(&lookup)->default_value(lookup), "authorization path (CORAL_DBLOOKUP_PATH)")
    ;

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(ac,av,desc), vm);
  boost::program_options::notify(vm);

  if(vm.count("help")) {
    desc.print(std::cout);
    std::cout << std::endl;
    exit (0);
  }
  if(vm.count("partition")) {
    tdaq_partition = vm["partition"].as<std::string>();
    ERS_DEBUG(0, "Partition Name: " << tdaq_partition);
  }
  if(vm.count("database")) {
    tdaq_db = vm["database"].as<std::string>();
    ERS_DEBUG(0, "Database Name: " << tdaq_db);
  }

  if(vm.count("lookup_path")) {
    lookup = vm["lookup_path"].as<std::string>();
    ERS_DEBUG(0, "Path for dblookup.xml: " << lookup);
  }

  // Create translation maps between DB values and content to add
  TranslationMap type;
  type["MySQL"] = "mysql";
  type["Oracle"] = "oracle";
  type["SQLite"] = "sqlite_file";
  type["Coral"] = "coral";

  TranslationMap host; // will be extended later
  host["LOCAL_HOST"] = "127.0.0.1";
  if(getenv("TDAQ_CORAL_PROXY_HOST")) {
    host["RACK_HOST"] = getenv("TDAQ_CORAL_PROXY_HOST");
  }

  // Instanciate the DB
  try {
    
    ::Configuration db(tdaq_db);

    // find partition and register variables converter
    // exit, if there is no partition object

    const daq::core::Partition * partition = daq::core::get_partition(db, tdaq_partition);
    if(!partition) return 1;
    db.register_converter(new daq::core::SubstituteVariables(*partition));
    
    // Get Trigger Configuration 
    const daq::core::TriggerConfiguration * tc = partition->get_TriggerConfiguration();
    

    if (!tc) return 1;

    // Get DBConnections
    const daq::core::TriggerDBConnection * tdbc = tc->get_TriggerDBConnection();
    std::vector<const daq::core::DBConnection*> odbc = tc->get_DBConnections();
    
    std::filesystem::path lookuppath(lookup);
    if (!exists(lookuppath)) {
      try
	{
	  std::filesystem::create_directories(lookuppath);
	  std::filesystem::permissions(lookuppath, std::filesystem::perms::owner_all | std::filesystem::perms::group_all | std::filesystem::perms::others_all);
	}
      catch (std::exception & ex)
	{
	  ers::fatal(daq::core::CantCreateDirectory(ERS_HERE, lookuppath.string(), ex ) );
	  return 1;
	}
    }
    
    std::ofstream lookup_xml(lookuppath.string() + "/dblookup.xml");

    lookup_xml << "<?xml version=\"1.0\" ?>\n\n";
    lookup_xml << "<servicelist>\n\n";

    if ( tdbc ) {
      if(type.find(tdbc->get_Type()) == type.end()) {
        daq::core::BadDBType e(ERS_HERE,tdbc->get_Type(), tdbc->UID() );
        ERS_LOG(e);
      }
      else {
        if(tdbc->get_Alias().empty()) {
	   lookup_xml <<  "<logicalservice name=\"" << tdbc->get_Name() <<"\">\n";
	}
	else {
	   lookup_xml <<  "<logicalservice name=\"" << tdbc->get_Alias() <<"\">\n";
	}
        lookup_xml << "<service name=\"" << type[tdbc->get_Type()] <<"://";
     
	std::string ho = tdbc->get_Server()  ;
	if (host.find(ho) == host.end()) {
	  lookup_xml << tdbc->get_Server() ;
        }
        else {
	  lookup_xml << host[ho] ; 
        }

        if (!tdbc->get_Port().empty()) {
	   lookup_xml << ":" << tdbc->get_Port() ;
	} 
	lookup_xml << "/" ; 
        
	lookup_xml <<  tdbc->get_Name() << "\" accessMode=\"read\" authentication=\"password\" />\n";
        lookup_xml << "</logicalservice>\n\n";
      }
    }
    for(unsigned int i=0; i< odbc.size(); i++) {
      if(type.find(odbc[i]->get_Type()) == type.end()) {
	daq::core::BadDBType e(ERS_HERE,odbc[i]->get_Type(), odbc[i]->UID() );
	ers::error(e);
      }
      else {
        if(odbc[i]->get_Alias().empty()) {
           lookup_xml <<  "<logicalservice name=\"" << odbc[i]->get_Name() <<"\">\n";
        }
        else {
           lookup_xml <<  "<logicalservice name=\"" << odbc[i]->get_Alias() <<"\">\n";
        }
	lookup_xml << "<service name=\"" << type[odbc[i]->get_Type()] <<"://";        
	std::string ho = odbc[i]->get_Server()  ;
	if (host.find(ho) == host.end()) {
	  lookup_xml << odbc[i]->get_Server() ;
        }
        else {
	  lookup_xml << host[ho] ; 
        }

        if (!odbc[i]->get_Port().empty()) {
          lookup_xml <<  ":" << odbc[i]->get_Port() ;        
        }
        lookup_xml << "/" ;
	lookup_xml << odbc[i]->get_Name() 
		   << "\" accessMode=\"read\" authentication=\"password\" />\n";
	lookup_xml << "</logicalservice>\n\n";
      }
    }
    
    lookup_xml << "</servicelist>\n";    
    lookup_xml.close();
    try {
      std::filesystem::permissions(lookuppath / "dblookup.xml", std::filesystem::perms::owner_all | std::filesystem::perms::group_all | std::filesystem::perms::others_all);
    }
    catch (std::exception &e) {
    }
    ERS_DEBUG(0, "Exiting now");
  }
  catch (ers::Issue & ex) {
    ers::fatal(ex);
    sleep(3);
    return (EXIT_FAILURE);
  }
  return 0;
   
}

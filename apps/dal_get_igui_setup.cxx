//
//  dal_get_igui_setup.cpp
//
//
//  Created:
//	- Igor Soloviev 28 November 2006
//

#include <set>
#include <map>

#include <fstream>
#include <sstream>

#include <boost/program_options.hpp>

#include <ipc/core.h>

#include <config/Configuration.h>

#include "dal/util.h"

#include "dal/SW_Repository.h"
#include "dal/Partition.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // list of possible return values

enum ConfdbAddHostReturnValues {
  __NoErrors__ = 0 ,
  __Error__BadCommandLine__ = 100 ,
  __Error__DBOpenFailed__ ,
  __Error__CannotFindPartitionObject__ ,
  __Error__Bad_Jar_File__ ,
  __Error__Config_Exception__
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /** Failed to parse command line. */

ERS_DECLARE_ISSUE(
  dal_get_igui_setup,
  BadCommandLine,
  "Bad command line" << why,
  ((const char*)why)
)

  /** Cannot find partition object */

ERS_DECLARE_ISSUE(
  dal_get_igui_setup,
  PartitionObjectNotFound,
  "Cannot find partition object with UID = \'" << id << '\'',
  ((std::string)id)
)

  /** Caught Config exception. */

ERS_DECLARE_ISSUE(
  dal_get_igui_setup,
  CaughtConfigException,
  "Caught config exception",
)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
main(int argc, char * argv[])
{
  try
    {
      IPCCore::init(argc, argv);
    }
  catch (ers::Issue & ex)
    {
      ers::warning(ers::Message(ERS_HERE, ex));
    }

  ConfdbAddHostReturnValues return_status = __NoErrors__;

  boost::program_options::options_description desc("Creates list of JAR files and properties for IGUI setup.. Available options are:");

  std::string data;
  std::string partition_name;
  std::string syntax;

  try
    {
      desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&data), "name of the database")
        ("partition-id,p", boost::program_options::value<std::string>(&partition_name)->required(), "name of the partition object")
        ("syntax,s", boost::program_options::value<std::string>(&syntax)->required(), "shell syntax used to set environment variables")
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
      ers::error(dal_get_igui_setup::BadCommandLine(ERS_HERE, ex.what()));
      return __Error__BadCommandLine__;
    }

  bool use_sh;

  if(syntax == "sh")
    use_sh = true;
  else if(syntax == "csh")
    use_sh = false;
  else
    {
      std::ostringstream str;
      str << ": bad name of shell syntax (\'" << syntax << "\'); the supported shell syntax is either \'sh\' or \'csh\'";
      ers::error(dal_get_igui_setup::BadCommandLine(ERS_HERE, str.str().c_str()));
      return __Error__BadCommandLine__;
    }

  try
    {
      ::Configuration db(data);

      const daq::core::Partition * p = daq::core::get_partition(db, partition_name);

      if (!p)
        {
          ers::error(dal_get_igui_setup::PartitionObjectNotFound(ERS_HERE,partition_name));
          return __Error__CannotFindPartitionObject__;
        }


      db.register_converter(new daq::core::SubstituteVariables(*p));

      // get set of software repositories files
      std::set<const daq::core::SW_Repository *> repositories = daq::core::get_used_repositories(*p);

      // build list of is-info files
      std::string properties;
      std::string class_path;

      std::map< std::string, std::set<std::string> > d_properties; // for properties in format "-Dfoo=bar1 -Dfoo=bar2" => "-Dfoo=bar1:bar2"

      const std::string& user_dir = p->get_RepositoryRoot();

      for (const auto& i : repositories)
        {
          if (!i->get_IGUIProperties().empty())
            {
              for (const auto& j : i->get_IGUIProperties())
                {
                  std::string::size_type pos = j.find('=');
                  if (pos != std::string::npos && pos > 2 && j[0] == '-' && j[1] == 'D')
                    {
                      std::string key = j.substr(0, pos + 1);
                      std::string value = j.substr(pos + 1);
                      ERS_DEBUG(3, "insert key=\'" << key << "\', value=\'" << value << "\'");
                      d_properties[key].insert(value);
                      continue;
                    }

                  if (!properties.empty())
                    properties += ' ';

                  properties += j;
                }
            }

          try
            {
              daq::core::add_classpath(*i, user_dir, class_path);
            }
          catch(ers::Issue& ex)
            {
              ers::error(ex);
              return_status = __Error__Bad_Jar_File__;
            }
        }

      for (const auto& i : d_properties)
        {
          std::string value;

          for (const auto& j : i.second)
            {
              if(!value.empty())
                value += ':';
              else
                value=i.first;

              value += j;
            }

          if (!properties.empty())
            properties += ' ';

          properties += value;
        }


      if(use_sh)
        {
          std::cout
            << "export __IGUI_PROPERTIES__=\"" << properties << "\"\n"
            << "export __IGUI_CLASSPATH__=\"" << class_path << "\"\n";
        }
      else
        {
          std::cout
            << "setenv __IGUI_PROPERTIES__ \"" << properties << "\"\n"
            << "setenv __IGUI_CLASSPATH__ \"" << class_path << "\"\n";
        }
    }
  catch (const daq::config::Exception & ex)
    {
      ers::error(dal_get_igui_setup::CaughtConfigException(ERS_HERE,ex));
      return __Error__Config_Exception__;
    }

  return return_status;
}

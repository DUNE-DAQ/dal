//
//  dal_load_is_info_files.cpp
//
//
//  Created:
//	- Igor Soloviev 13 January 2004
//
//  Modifications:
//	- Moved algorithm to get used sw repositories to the library (IS, 28/11/2006)
//
//

#include <fstream>
#include <set>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "config/Configuration.hpp"

#include "dal/SW_Repository.hpp"
#include "dal/Partition.hpp"
#include "dal/util.hpp"

namespace po = boost::program_options;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // list of possible return values

enum ConfdbAddHostReturnValues {
  __NoErrors__ = 0 ,
  __Error__CmdlParserFailed__ = 100 ,
  __Error__Bad_IS_Info_File__ ,
  __Error__Config_Exception__
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  /** Failed to parse command line. */

ERS_DECLARE_ISSUE(
  dal_load_is_info_files,
  BadCommandLine,
  "Failed to parse command line",
)


  /** Cannot find is-info-file exception. */

ERS_DECLARE_ISSUE(
  dal_load_is_info_files,
  NoInfoFile,
  "cannot find IS info file \'" << file << "\' required by the \'" << obj_id << '@' << obj_class << "\' with installation-path \'" << path << '\'',
  ((std::string)file)
  ((std::string)obj_id)
  ((std::string)obj_class)
  ((std::string)path)
)




  /** Caught Config exception. */

ERS_DECLARE_ISSUE(
  dal_load_is_info_files,
  ConfigException,
  "Caught config exception",
)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
check_file_exists(const std::string& path, std::string & file)
{
  std::ifstream f(path.c_str());
  if(f.good()) {
    file = path;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
main(int argc, char * argv[])
{
  ConfdbAddHostReturnValues return_status = __NoErrors__;

  po::options_description desc("List IS info files defined for given partition");

  std::string data;
  std::string partition_name;

  // parse command line
  try
    {
      desc.add_options()
      (
          "data,d",
          po::value<std::string>(&data),
          "name of the database containing partition object (also defined by TDAQ_DB)"
      )
      (
          "partition-name,p",
          po::value<std::string>(&partition_name)->required(),
          "name of the partition containing IS info files to be loaded"
      )
      (
          "help,h",
          "Print help message"
      );

      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return return_status;
        }

      po::notify(vm);
    }
  catch (std::exception& ex)
    {
      ers::fatal(dal_load_is_info_files::BadCommandLine(ERS_HERE, ex));
      return __Error__CmdlParserFailed__;
    }


  std::string repository_objects;  // keep list of objects to be reported in case of error

  try {

      // create configuration

    ::Configuration db(data);


      // find partition

    const daq::core::Partition * p = daq::core::get_partition(db, partition_name);
    if(!p) return 1;


      // register variables converter

    db.register_converter(new daq::core::SubstituteVariables(*p));


      // get set of software repositories files

    std::set<const daq::core::SW_Repository *> repositories = daq::core::get_used_repositories(*p);


      // build list of is-info files

    std::set<std::string> files;

    const std::string& user_dir = p->get_RepositoryRoot();

    {
      for(auto & i : repositories) {
        if(!i->get_ISInfoDescriptionFiles().empty()) {
          if(!repository_objects.empty()) repository_objects += ", ";
          repository_objects += '\"' + i->UID() + '@' + i->class_name() + '\"';
          for(auto & j : i->get_ISInfoDescriptionFiles()) {
            std::string file;

              // check file name is an absolute path

            if(j[0] == '/') {
              check_file_exists(j, file);
            }

              // check user dir (Repository Root)

            if(file.empty() && !user_dir.empty()) {
              check_file_exists(user_dir + '/' + j, file);
            }

            if(file.empty() && !i->get_PatchArea().empty()) {
              check_file_exists(i->get_PatchArea() + '/' + j, file);
            }

            if(file.empty()) {
              check_file_exists(i->get_InstallationPath() + '/' + j, file);
            }

            if(file.empty()) {
              ers::error(dal_load_is_info_files::NoInfoFile(ERS_HERE,j,i->UID(),i->class_name(),i->get_InstallationPath()));
              return_status = __Error__Bad_IS_Info_File__;
            }
            else {
              files.insert(file);
            }
          }
        }
      }
    }

    for(auto & j : files)
      {
        std::cout << j << std::endl;
      }
  }
  catch ( daq::config::Exception & ex ) {
    ers::error(dal_load_is_info_files::ConfigException(ERS_HERE,ex));
    return __Error__Config_Exception__;
  }


  return return_status;
}

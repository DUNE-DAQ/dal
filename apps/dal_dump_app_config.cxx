#include <sstream>

#include <boost/program_options.hpp>

#include "oksdbinterfaces/Configuration.hpp"

#include "dal/util.hpp"

#include "dal/Partition.hpp"
#include "dal/BaseApplication.hpp"
#include "dal/Computer.hpp"
#include "dal/Segment.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // The type TableRow describes table's row with 6 items, separator and fill chars.
  // It is used to print separator line, title row and application parameters

struct TableRow
{
  char m_fill;
  char m_separator;
  std::string m_items[6];
  TableRow(const char fill, const char separator) :
      m_fill(fill), m_separator(separator)
  {
    ;
  }

  TableRow(const char fill, const char separator, unsigned short num, const dunedaq::dal::BaseApplication * app, const dunedaq::dal::Computer * host, const dunedaq::dal::Segment * seg, const std::string& seg_id, const std::string& app_id) :
      m_fill(fill), m_separator(separator)
  {
    std::ostringstream num_s;
    num_s << num;
    m_items[0] = num_s.str();
    m_items[1] = app->UID() + '@' + app->class_name();
    if (!DalObject::is_null(host))
      m_items[2] = host->UID() + '@' + host->class_name();
    else
      m_items[2] = "(null)";
    m_items[3] = seg->UID() + '@' + seg->class_name();
    m_items[4] = seg_id;
    m_items[5] = app_id;
  }

  TableRow(const char fill, const char separator, const dunedaq::dal::Computer& host) :
      m_fill(fill), m_separator(separator)
  {
    m_items[0] = m_items[1] = m_items[3] = m_items[4] = m_items[5] = "";
    m_items[2] = host.UID() + '@' + host.class_name();
  }

  TableRow(const char fill, const char separator, const std::string& s1, const std::string& s2, const std::string& s3, const std::string& s4, const std::string& s5, const std::string& s6) :
      m_fill(fill), m_separator(separator)
  {
    m_items[0] = s1;
    m_items[1] = s2;
    m_items[2] = s3;
    m_items[3] = s4;
    m_items[4] = s5;
    m_items[5] = s6;
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
main(int argc, char *argv[])
{

  std::string data;
  std::string partition_name;
  std::set<std::string> app_types;
  std::vector<std::string> hosts_list;
  std::set<const dunedaq::dal::Computer *> hosts;
  std::set<std::string> segments;
  bool backup = false;

  boost::program_options::options_description desc("This program prints out applications and their configuration parameters using Partition::get_all_applications() algorithm");

  try
    {
      std::vector<std::string> app_types_list;
      std::vector<std::string> segments_list;

      desc.add_options()
        ("data,d", boost::program_options::value<std::string>(&data), "name of the database")
        ("partition-name,p", boost::program_options::value<std::string>(&partition_name)->required(), "name of the partition object")
        ("application-types,t", boost::program_options::value<std::vector<std::string> >(&app_types_list)->multitoken(), "filter out all applications except given classes (and their subclasses)")
        ("hosts,c", boost::program_options::value<std::vector<std::string> >(&hosts_list)->multitoken(), "filter out all applications except those which run on given hosts")
        ("segments,s", boost::program_options::value<std::vector<std::string> >(&segments_list)->multitoken(), "filter out all applications except those which belong to given segments")
        ("show-backup-hosts,b","print backup hosts")
        ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return EXIT_SUCCESS;
        }

      boost::program_options::notify(vm);

      if (vm.count("show-backup-hosts"))
        {
          backup = true;
        }

      for (const auto& x : app_types_list)
        app_types.insert(x);

      for (const auto& x : segments_list)
        segments.insert(x);
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }

  try
    {
      Configuration db(data);

      const dunedaq::dal::Partition * partition = dunedaq::dal::get_partition(db, partition_name);

      if (!partition)
        return EXIT_FAILURE;

      db.register_converter(new dunedaq::dal::SubstituteVariables(*partition));

      std::set<const dunedaq::dal::Computer *> hosts;

      for (const auto& x : hosts_list)
        {
          if (const dunedaq::dal::Computer * host = db.get<dunedaq::dal::Computer>(x))
            {
              hosts.insert(host);
            }
          else
            {
              std::cerr << "ERROR: cannot find computer \'" << x << "\'\n";
              return (EXIT_FAILURE);
            }
        }

      std::vector<const dunedaq::dal::BaseApplication *> apps = partition->get_all_applications((app_types.empty() ? nullptr : &app_types), (segments.empty() ? nullptr : &segments), (hosts.empty() ? nullptr : &hosts));

      std::cout << "Got " << apps.size() << " applications:\n";

      std::vector<TableRow> rows;
      rows.reserve(apps.size()+4);

      // fill the table
        {
          rows.push_back(TableRow('=', '='));
          rows.push_back(TableRow(' ', '|', "num","Application Object","Host","Segment","Segment unique id","Application unique id"));
          rows.push_back(TableRow('=', '='));

          unsigned long count = 1;
          for (const auto& i : apps)
            {
              rows.push_back(TableRow(' ', '|', count++, i->get_base_app(), i->get_host(), i->get_segment()->get_base_segment(), i->get_segment()->UID(), i->UID()));

              if (backup)
                {
                  for (const auto& x : i->get_backup_hosts())
                    {
                      rows.push_back(TableRow(' ', '|', *x));
                    }
                }
            }

          rows.push_back(TableRow('=', '='));
        }

      // detect columns widths
      unsigned short cw[] = {1,1,1,1,1,1};

      for (const auto& i: rows)
        {
          for (unsigned short j = 0; j < sizeof(cw)/sizeof(unsigned short); ++j)
            {
              unsigned short len = i.m_items[j].size();

              if(len > cw[j])
                cw[j]=len;
            }
        }


        // print table
      bool align_left[] = {false, true, true, true, true, true};

      for (const auto& i: rows)
        {
          std::cout.fill(i.m_fill);
          for (unsigned short j = 0; j < sizeof(cw)/sizeof(unsigned short); ++j)
            {
              std::cout << i.m_separator << i.m_fill;
              std::cout.setf((align_left[j] ? std::ios::left : std::ios::right), std::ios::adjustfield);
              std::cout.width(cw[j]);
              std::cout << i.m_items[j] << i.m_fill;
            }
          std::cout << i.m_separator << std::endl;
        }
    }
  catch (ers::Issue & ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return (EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}

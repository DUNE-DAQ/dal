#include "config/Schema.hpp"
#include "config/Configuration.hpp"
#include "config/ConfigObject.hpp"

#include "dal/util.hpp"

#include "dal/Partition.hpp"
#include "dal/Segment.hpp"
#include "dal/ResourceBase.hpp"
#include "dal/RunControlApplicationBase.hpp"

#include "RODBusydal/BusyChannel.hpp"
#include "RODBusyModule/BusyTimeoutAction.h"
#include "RODBusydal/RODBusyModule.hpp"

     std::string m_rcd_name;
     std::map <std::string, std::string> m_busy_source;
     std::map <std::string, std::string> m_rcd_map;
     std::map <std::string, std::string> m_controller_map;

     Configuration* m_oks_db;
     const dunedaq::dal::Partition* m_partition;

     std::string m_current_rcd;
     std::string m_current_controller;
     std::map <const dunedaq::dal::Segment *, bool> m_is_linked;

bool navigate_segments (const dunedaq::dal::Segment *segment,
                        const dunedaq::dal::Segment *target)
{
  std::vector <const dunedaq::dal::Segment *> vseg;
  std::vector <const dunedaq::dal::Segment *>::iterator iseg;

  vseg = segment->get_Segments ();

  for (iseg = vseg.begin (); iseg != vseg.end (); ++iseg)
    {
    std::cout << "compare[s] " << (*iseg) << " == " << target << ": (" << (void *)(*iseg) << " and " << (void *)target << ") returns " << ((*iseg) == target) << std::endl;

    if ((*iseg) == target)
      return true;
    else if (navigate_segments ((*iseg), target))
      return true;
    }
  return false;
}

bool find_segment (const dunedaq::dal::Partition* partition,
		   const dunedaq::dal::Segment *target)
{
  std::vector <const dunedaq::dal::Segment *> vseg;
  std::vector <const dunedaq::dal::Segment *>::iterator iseg;

  vseg = partition->get_Segments ();

  for (iseg = vseg.begin (); iseg != vseg.end (); ++iseg)
  {
      std::cout << "compare[p] " << (*iseg) << " == " << target << ": (" << (void *)(*iseg) << " and " << (void *)target << ") returns " << ((*iseg) == target) << std::endl;

    if ((*iseg) == target)
      return true;
    else if (navigate_segments ((*iseg), target))
      return true;
  }

  return false;
}

void navigate_relationships (Configuration *confDB,
			     ConfigObject *o,
			     const dunedaq::dal::Partition *partition,
			     bool debug)
{
  std::string class_name = o->class_name ();

  // If the object is a resource, then check if it is enabled

  const dunedaq::dal::ResourceBase *res_obj = confDB->get <dunedaq::dal::ResourceBase> (class_name);

  if (res_obj)
    if (res_obj->disabled (*partition))
      return;

  std::vector <ConfigObject> values;
  const std::string relationship_name = "*";
  bool check_composite_only = false;
  unsigned long rlevel = 0;
  const std::vector <std::string> *rclasses = 0;

  try
  {
    o->referenced_by (values, relationship_name, check_composite_only,
		      rlevel, rclasses);
  }

  catch (...)
  {
    return;
  }

  std::vector <ConfigObject>::iterator iobj;

  if (debug)
  {
    std::cout << "Object " << o->full_name () << " is referenced by : ";

    for (iobj = values.begin (); iobj !=values.end () ; ++iobj)
    {
      ConfigObject *conf_obj = &(*iobj);
      std::cout << conf_obj->full_name () << ",";
    }

    std::cout << std::endl;
  }

  for (iobj = values.begin (); iobj !=values.end () ; ++iobj)
  {
    if (iobj->class_name () == "Segment")
    {
      const dunedaq::dal::Segment *s = confDB->get <dunedaq::dal::Segment> (iobj->UID ());

      // Is that segment disabled ?

      if (s->disabled (*partition))
	continue;

      // Is that segment part of the partition ?

      std::pair <const dunedaq::dal::Segment *, bool> is_linked;
      std::map <const dunedaq::dal::Segment *, bool>::iterator link_it;

      link_it = m_is_linked.find (s);

      if (link_it != m_is_linked.end ())
      {
	is_linked = *link_it;
      }
      else
      {
	is_linked.first = s;
	is_linked.second = find_segment (partition, s);

	m_is_linked.insert (is_linked);
      }

      if (! is_linked.second)
	continue;

      // Find the controller

      const dunedaq::dal::RunControlApplicationBase *rc = s->get_IsControlledBy ();

      m_current_controller = rc->UID ();
      return;
    }
    else if (iobj->class_name () != "BusyChannel" &&
	     iobj->class_name () != "RODBusyModule" &&
             iobj->full_name () != o->full_name ())
    {
      ConfigObject *conf_obj = &(*iobj);

      navigate_relationships (confDB, conf_obj, partition, debug);

      if (iobj->class_name () == "RCD")
      {
	const dunedaq::dal::ResourceBase *rapp = confDB->get <dunedaq::dal::ResourceBase> (iobj->UID ());
	if (! rapp->disabled (*partition))
	{
	  m_current_rcd = rapp->UID ();
	  return;
	}
      }
    }
  }
}

void setup (Configuration *confDB,
	    const dunedaq::dal::Partition *partition,
	    std::string& rcd_uid,
	    std::vector <std::string>& busychannel_uid_vector,
	    std::vector <bool>& enabled_config,
	    bool debug)
{
  m_busy_source.clear ();
  m_rcd_map.clear ();
  m_controller_map.clear ();
  m_is_linked.clear ();

  m_oks_db = confDB;
  m_rcd_name = rcd_uid;
  m_partition = partition;

  // Get the vector of busy sources

  register int i;
  std::vector <std::string>::iterator ichannel;

  for (i = 0, ichannel = busychannel_uid_vector.begin ();
       ichannel != busychannel_uid_vector.end (); ++ichannel, ++i)
  {
    if (enabled_config[i])
    {
      const RODBusydal::BusyChannel *bc = confDB->get <RODBusydal::BusyChannel> ((*ichannel));
      const dunedaq::dal::ResourceBase *rb = bc->get_BusySource ();
      std::string obj_name = rb->UID () + "@" + rb->class_name ();
      std::pair<std::string, std::string> busy_channel_pair;

      busy_channel_pair.first = busychannel_uid_vector[i];
      busy_channel_pair.second = obj_name;

      m_busy_source.insert (busy_channel_pair);

      // Get the busy source config object

      ConfigObject *conf_obj = const_cast <ConfigObject *> (&(rb->config_object ()));

      std::cout << "=> busy source: " << conf_obj << std::endl;

      // Navigate back to the segment

      m_current_controller = "";
      navigate_relationships (confDB, conf_obj, partition, debug);

      // Fill maps

      std::pair <std::string, std::string> rcd_map_element;
      std::pair <std::string, std::string> controller_map_element;

      rcd_map_element.first = busychannel_uid_vector[i];
      rcd_map_element.second = m_current_rcd;

      controller_map_element.first = busychannel_uid_vector[i];
      controller_map_element.second = m_current_controller;

      m_rcd_map.insert (rcd_map_element);
      m_controller_map.insert (controller_map_element);
    }
  }

  m_is_linked.clear ();

  if (debug)
  {
    std::map <std::string, std::string>::iterator j, k;

    for (j = m_rcd_map.begin (), k = m_controller_map.begin ();
	 k != m_controller_map.end (); ++j, ++k)
      std::cout << (*k).first << " " 
		<< (*k).second << " "
		<< (*j).second << std::endl;
  }
}

void usage()
{
  std::cout <<
    "usage: dal_cmp_ptr -p | --partition-id partition-id\n"
    "                  [-d | --data database-name]\n"
    "                  [-r | --rcd-id rcd-id]\n"
    "\n"
    "Options/Arguments:\n"
    "       -d database-name  name of the database (ignore TDAQ_DB variable)\n"
    "       -p partition-name name of the partition object\n"
    "       -r rcd-id         identity of the RCD\n"
    "\n";
}

int main(int argc, char *argv[])
{
  std::string db_name;
  std::string partition_name;
  std::string rcd_uid;

    // parse command line parameters

  {
    for(int i = 1; i < argc; i++) {
      if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
        usage();
        return (EXIT_SUCCESS);
      }
      else if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--data")) {
        if(++i == argc) {
          std::cerr << "ERROR: no data file provided\n\n";
          return EXIT_FAILURE;
        }
        else {
          db_name = argv[i];
        }
      }
      else if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--partition-id")) {
        if(++i == argc) {
          std::cerr << "ERROR: no partition-id provided\n\n";
          return EXIT_FAILURE;
        }
        else {
          partition_name = argv[i];
        }
      }
      else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--rcd-id")) {
        if(++i == argc) {
          std::cerr << "ERROR: no rcd  id provided\n\n";
          return EXIT_FAILURE;
        }
        else {
          rcd_uid = argv[i];
        }
      }
      else {
        std::cerr << "ERROR: unexpected parameter: \"" << argv[i] << "\"\n\n";
        usage();
        return EXIT_FAILURE;
      }
    }
  }

  try
    {
      Configuration db(db_name);

      // find partition; exit, if there is no partition object
      const dunedaq::dal::Partition * partition = dunedaq::dal::get_partition(db, partition_name);
      if (!partition)
        return EXIT_FAILURE;

      // register variables converter
      db.register_converter(new dunedaq::dal::SubstituteVariables(*partition));

      std::vector <std::string> busychannel_uid_vector(16);
      std::vector <bool> enabled_config(16);

      for (int ic = 0; ic<16; ++ic) {
          busychannel_uid_vector[ic] = "invalid";
          enabled_config[ic]=false;
      }

      const RODBusydal::RODBusyModule* dbModule = db.get<RODBusydal::RODBusyModule>(rcd_uid);

      if(!dbModule)
        {
          std::cerr << "ERROR: cannot find RODBusyModule with ID = " << rcd_uid << std::endl;
          return 1;
        }

      for(const auto& it : dbModule->get_Contains()){
          if(const RODBusydal::BusyChannel* bc = db.cast<RODBusydal::BusyChannel> (it)){
              std::cout << "item " << bc << std::endl;
              int channelnumber = bc->get_Id();
              busychannel_uid_vector[channelnumber] = bc->UID();
              enabled_config[channelnumber] = true;
              std::cout << "busychannel_uid_vector [" << channelnumber << "] = " << bc->UID() << std::endl;
          }
      }


      setup (&db, partition, rcd_uid, busychannel_uid_vector, enabled_config, true);
    }
  catch (ers::Issue & ex)
    {
      std::cerr << "ERROR: caught " << ex << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}



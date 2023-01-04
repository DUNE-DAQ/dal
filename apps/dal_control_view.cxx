//
//  FILE: src/dal_control_view.cpp
//
//  Example program which shows how to subscribe on application changes
//  and receive notification.
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

#include "config/Change.hpp"
#include "config/Configuration.hpp"

#include "dal/util.hpp"

#include "dal/OnlineSegment.hpp"
#include "dal/Resource.hpp"
#include "dal/ResourceSet.hpp"
#include "dal/Partition.hpp"


// the struct stores callback's parameters
struct ControlCallbackInfo
{
  bool state;
  const daq::core::Segment * segment;
  const daq::core::ResourceBase * resource;
  std::list<std::vector<const daq::core::Component *>> parents;
  const daq::core::Partition& p;

  ControlCallbackInfo(const daq::core::Segment * s, const daq::core::ResourceBase * r, const daq::core::Partition& part) :
      segment(s), resource(r), p(part)
  {
    state = (segment ? segment->disabled(p) : resource->disabled(p));
  }
};


// the pointer is used by signal handler to unsubscribe and by the callbacks
::Configuration * conf = nullptr;


  // ************************************ SIGNAL HANDLER ************************************* //
  

extern "C" void
signal_handler(int)
{
  if (conf)
    {
      std::cout << "Interrupting dal_control_view ...\n";
      conf->unsubscribe();
      conf->unload();
      exit(0);
    }
}

  // ************************************ PRINT FUNCTIONS ************************************ //

  // the function to print "Disabled()" value as '+' or '-'

char
bool2sign(bool value)
{
  return (value ? '-' : '+');
}

  // the function to print a resource

void
print_resource(const daq::core::ResourceBase& resource, const daq::core::Partition& p, bool parent_state, size_t margin)
{
  std::string str(margin, ' ');

  if (DalObject::is_null(&resource))
    {
      std::cout << str << "(null)\n";
    }
  else
    {
      if (const daq::core::Resource * r = conf->cast<daq::core::Resource>(&resource))
        {
          std::cout << str << r << '(' << bool2sign(parent_state) << bool2sign(r->disabled(p)) << ")\n";
        }
      else if (const daq::core::ResourceSet * s = conf->cast<daq::core::ResourceSet>(&resource))
        {
          std::cout << str << s << '(' << bool2sign(parent_state) << bool2sign(s->disabled(p)) << ")\n";

          parent_state = (
              parent_state == true
                ? parent_state            // if parent was disabled, then propagate parent's state
                : s->disabled(p)          // otherwise propagate this resource set state
              );

          for (const auto& i : s->get_Contains())
            print_resource(*i, p, parent_state, margin + 2);
        }
    }
}

  // the function to print a segment

void
print_segment(const daq::core::Segment& segment, const daq::core::Partition& p, bool parent_state, size_t margin)
{
  const std::string str(margin, ' ');

  if (DalObject::is_null(&segment))
    {
      std::cout << str << "(null)\n";
    }
  else
    {
      std::cout << str << &segment << '(' << bool2sign(parent_state) << bool2sign(segment.disabled(p)) << ")\n";

      parent_state = (
          parent_state == true
            ? parent_state           // if parent was disabled, then propagate parent's state
            : segment.disabled(p)    // otherwise propagate this segment state
          );

      for (const auto& i : segment.get_Segments())
        print_segment(*i, p, parent_state, margin + 2);

      for (const auto& j : segment.get_Resources())
        print_resource(*j, p, parent_state, margin + 2);
    }
}

  // the function to print a partition

void
print_partition(const daq::core::Partition& partition)
{
  // print out online-infrastructure segment
  std::cout << "  ONLINE INFRASTRUCTURE SEGMENT:\n";
  print_segment(*partition.get_OnlineInfrastructure(), partition, false, 4);

  // print out nested segments
  std::cout << "  NESTED SEGMENTS:\n";
  for (const auto& i : partition.get_Segments())
    print_segment(*i, partition, false, 4);
}


  // *************************************** CALLBACKS *************************************** //

  // the callback to print out any partition change

void
cb_partition(const std::vector<ConfigurationChange *> & /*changes*/, void * parameter)
{
  std::cout << "something was changed, print out new partition control view:\n";
  print_partition(*reinterpret_cast<const daq::core::Partition *>(parameter));
}

  // the callback to be invoked when a segment or resource changed

void
cb(const std::vector<ConfigurationChange *> & changes, void * parameter)
{
  ControlCallbackInfo * cb_info = reinterpret_cast<ControlCallbackInfo *>(parameter);

  std::cout << "got notification for object ";
  if (cb_info->segment)
    std::cout << cb_info->segment;
  else
    std::cout << cb_info->resource;
  std::cout << " changes\n";

  // calculate new state of the monitored object:
  // firstly get state of the object, then check it's parents

  bool new_state = (cb_info->segment ? cb_info->segment->disabled(cb_info->p) : cb_info->resource->disabled(cb_info->p));

  // iterate changes sorted by classes

  for (const auto& j : changes)
    {

      // get class name

      const std::string& class_name = j->get_class_name();

      if (class_name != "Segment")
        {
          if (cb_info->segment)
            {
              std::cerr << "*** ERROR: Oops, got changes in class \"" << class_name << "\" which is not a segment ***\n";
              continue;
            }
          else if (class_name.find("Resource") == std::string::npos)
            {
              std::cerr << "*** ERROR: Oops, got changes in class \"" << class_name << "\" which is not a segment or a resource ***\n";
              continue;
            }
        }

      std::vector<std::string>::const_iterator i;

      // print modified objects
      // note, we assume that parents were not removed!

      if (j->get_modified_objs().size())
        {
          for (const auto& i : j->get_modified_objs())
            std::cout << " * " << class_name << " \'" << i << "\' was modified\n";

          if (class_name == "Segment")
            {
              for (const auto& x : cb_info->parents)
                {
                  for (const auto& y : x)
                    {
                      if (y->disabled(cb_info->p))
                        new_state = true;
                    }
                }
            }
        }
    }

  if (new_state != cb_info->state)
    {
      std::cout << " * the object's state now is: " << bool2sign(new_state) << std::endl;
      cb_info->state = new_state;
    }
  else
    {
      std::cout << " * the object's state was not changed\n";
    }

  std::cout << std::endl;
  std::cout.flush();
}


  // ********************************** SUBSCRIPTION UTILITY ********************************* //

template<class T> const T *
subscribe_object(const daq::core::Partition& partition, const std::string& id)
{
    // get segment or resource object

  if (const T * obj = conf->template get<T>(id))
    {
        // create segment callback info structure, which holds informations
        // about present segment's state and the segment's pointer

      ControlCallbackInfo * cb_info = new ControlCallbackInfo(
        reinterpret_cast<const daq::core::Segment *> ( (T::s_class_name == "Segment") ? obj : 0 ),
        reinterpret_cast<const daq::core::ResourceBase *>( (T::s_class_name.find("Resource") != std::string::npos) ? obj : 0 ),
        partition
      );


        // create subscription criteria

      ::ConfigurationSubscriptionCriteria criteria;

      std::cout << " * create callback to subscribe on changes of the " << obj << " (" << bool2sign(cb_info->state) << ')';


        // subscribe on object's change

      criteria.add(*obj);


        // get segment's parents and subscribe on their changes

      obj->get_parents(partition, cb_info->parents);
      if (cb_info->parents.size() > 0)
        {
          std::cout << " and it's parents:\n";

          for (const auto& x : cb_info->parents)
            {
              std::cout << "  - path:\n";
              for (const auto& j : x)
                {
                  criteria.add(*j);
                  if (j->disabled(partition))
                    cb_info->state = true;
                  std::cout << "     * " << j << " (" << bool2sign(j->disabled(partition)) << ")\n";
                }
            }
        }
      else
        {
          std::cout << std::endl;
        }


        // subscribe on changes

      conf->subscribe(criteria, cb, reinterpret_cast<void *>(cb_info));

      return obj;
    }
  else
    {
      std::cerr << "ERROR: Can not find " << T::s_class_name << " object with id \"" << id << "\"\n";
      return nullptr;
    }
}


  /**
   *  The main function.
   */

int
main(int argc, char **argv)
{

    // database and partition names passed via command line

  std::string db_name;
  std::string partition_name;


    // lists of segments and resources passed via command line

  std::vector<std::string> segment_objects;
  std::vector<std::string> resource_objects;


    // set to true if the list of segments or the list of resources is empty

  bool subscribe_all_segments = false;
  bool subscribe_all_resources = false;


    // print partition control tree

  bool print_partition_on_any_change = false;

  // parse command line parameters

  boost::program_options::options_description cmdl("Example of subscription on a partition control view changes. "
      "The application prints out the partition control view after start and updates it "
      "each time after any database modification, if -a command line option is used.\n"
      "The application subscribes on changes of given resources and segments as it is defined\n"
      "by the -e and -s options.\n"
      "When a resource or segment is printed out, it's state is presented as (xy), where:\n"
      "* x='+' when all parents are enabled and x='-' when a parent is disabled;\n"
      "* y='+' when the object itself is enabled and y='-' when it is disabled.\n"
      "Available options are");

  try
    {
      cmdl.add_options()
          ("data,d", boost::program_options::value<std::string>(&db_name), "database name")
          ("partition-id,p", boost::program_options::value<std::string>(&partition_name)->required(), "name of the partition object")
          ("notify-segments-changes,s", boost::program_options::value<std::vector<std::string> >(&segment_objects)->multitoken()->zero_tokens(), "subscribe on changes of the state of given segments or on all segments if empty")
          ("notify-resources-changes,e", boost::program_options::value<std::vector<std::string> >(&resource_objects)->multitoken()->zero_tokens(), "subscribe on changes of the state of given resources or on all resources if empty")
          ("print-partition-on-any-change,a","refresh partition control view after any change")
          ("help,h", "Print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdl), vm);

      if (vm.count("help"))
        {
          std::cout << cmdl << std::endl;
          return EXIT_SUCCESS;
        }

      boost::program_options::notify(vm);

      if (vm.count("print-partition-on-any-change"))
        print_partition_on_any_change = true;

      if (vm.count("notify-segments-changes") && segment_objects.empty())
        subscribe_all_segments = true;

      if (vm.count("notify-resources-changes") && resource_objects.empty())
        subscribe_all_resources = true;
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }


  try
    {
      ::Configuration db(db_name);
      conf = &db;

      const daq::core::Partition * partition = daq::core::get_partition(*conf, partition_name);
      if (!partition)
        return EXIT_FAILURE;


      // register handlers for user's signals
      signal(SIGINT,signal_handler);
      signal(SIGTERM,signal_handler);


      // subscribe on any partition changes
      if (print_partition_on_any_change)
        {
          ::ConfigurationSubscriptionCriteria criteria;  // empty criteria = notify about any changes
          conf->subscribe(criteria, cb_partition, const_cast<void *>(reinterpret_cast<const void *>(partition)));
        }

      std::vector<const daq::core::Segment *> p2;

      // subscribe on changes of objects in the Segment class
      if (subscribe_all_segments)
        {
          std::vector<const daq::core::Segment *> segments;
          conf->get(segments);
          for (const auto& i : segments)
            {
              if (subscribe_object<daq::core::Segment>(*partition, i->UID()) == 0)
                return EXIT_FAILURE;
            }
        }
      else
        {
          for (const auto& i : segment_objects)
            {
              if (subscribe_object<daq::core::Segment>(*partition, i) == 0)
                return EXIT_FAILURE;
            }
        }


      // subscribe on changes of objects in the Resource class
      if (subscribe_all_resources)
        {
          std::vector<const daq::core::ResourceBase *> resources;
          conf->get(resources);
          for(const auto& i : resources)
            {
              if (subscribe_object<daq::core::ResourceBase>(*partition, i->UID()) == 0)
                return EXIT_FAILURE;
            }
        }
      else
        {
          for(const auto& i : resource_objects)
            {
              if(subscribe_object<daq::core::ResourceBase>(*partition, i) == 0)
                return EXIT_FAILURE;
            }
        }

      std::cout << "The partition control view:\n";
      print_partition(*partition);

      if (segment_objects.empty() && resource_objects.empty() && !subscribe_all_segments && !subscribe_all_resources && !print_partition_on_any_change)
        {
          std::cout << "There is no any subscription provided via command line, exiting ...\n";
          return EXIT_SUCCESS;
        }

      // wait for notification on changes
      while (true)
        sleep(1);
    }
  catch (dunedaq::config::Exception & ex)
    {
      std::cerr << "Caught " << ex << std::endl;
      return (EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}

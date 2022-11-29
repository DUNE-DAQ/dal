#ifndef _dal_seg_config_H_
#define _dal_seg_config_H_

#include <string>
#include <vector>

#include "dal/Segment.h"
#include "dal/app-config.h"


namespace daq
{
  namespace core
  {

    // forward declarations

    class Computer;
    class Partition;
    class Segment;

    /**
     * \brief The class describes segment configuration parameters
     *
     *  The class provides methods to get description of nested segments and applications.
     *  An object of SegConfig class should be created by daq::core::Partition::get_segment() algorithm.
     *  Above algorithm allows to limit depth of nested segments description (e.g. for efficiency reasons)
     *  using optional \e depth parameter. If depth parameter is set to 0, then get description of this
     *  segment only (no description of nested segments is provided even if there are such segments in database).
     *  The Run Control may set depth parameter equal to 1 to get information about controller and infrastructure
     *  of nested segments to set them up. If depth parameter is used with default value, the algorithm returns
     *  description of all nested segments.
     *
     *  There are four main use cases:
     *  - to get tree of segments (all disabled and enabled segments are returned)
     *  - to get description of enabled applications of a segment
     *  - to get description of all enabled applications belonging to this and nested segments
     *  - to get description of segment hosts
     *
     *  \par Descriptions of Segment and Nested Segments
     *
     *  Each segment has configuration object, describing the segment in database. The object is returned by the get_segment() method.
     *  Each segment has name (usually is equal to unique ID of configuration object). If the segment is created from template segment object,
     *  then the name is different from UID and it is equal to colon-separated IDs of templated segment and rack configuration objects.
     *  The nested segments can be accessed using get_nested_segments().
     *  The enabled / disabled status of segment can be checked using is_disabled() method.
     *
     *  \par Descriptions of Segment Applications
     *
     *  Each segment has 3 types of applications:
     *  - the segment controller (is described by IsControlledBy relationship); use get_controller() method to access it
     *  - the segment infrastructure applications (are described by the Infrastructure relationship); use get_infrastructure() method to access them
     *  - the segment applications (are described by the Applications and Resources relationships); use get_applications() method to access them
     *
     *  Only enabled application are put into SegConfig description.
     *
     *  \par Descriptions of All Applications
     *
     *  Use get_all_applications() method to get description of applications in given and chosen nested segments (as defined by depth parameter).
     *  It is possible to provide precise selection criteria by application class types, names of nested segments and hosts.
     *  The get_all_applications() method invoked on online segment is used to get all applications of partition (see
     *  daq::core::Partition::get_all_applications() algorithm).
     *
     *  \par Segment Hosts
     *
     *  The segment hosts are described by the Hosts relationship. For templated segments the Hosts are defined by the hosts of corresponding rack object.
     *  Only enabled (i.e. switched "On") hosts are described.
     *  The first enabled host is considered as "infrastructure" or "default" host.
     *  It is used to run applications without explicitly defined "runs on" relationship.
     **/

    class SegConfig
    {

      friend class Partition;
      friend class AlgorithmUtils;

    public:

      /**
       *  The constructor should only used by the Partition::get_segment() algorithm.
       *  It cannot be made truly private by efficiency reasons.
       */

      SegConfig(const Partition * p) :
          m_partition(p), m_base_segment(nullptr), m_controller(nullptr), m_is_disabled(true), m_is_templated(false)
      {
        ;
      }


      /**
       * Get partition object.
       */

      const Partition *
      get_partition() const
      {
        return m_partition;
      }

      /**
       * Get segment database object.
       */

      const Segment *
      get_base_segment() const
      {
        return m_base_segment;
      }

      /**
       * Get segment controller.
       */

      const BaseApplication *
      get_controller() const
      {
        return m_controller;
      }

      /**
       *  Get infrastructure applications of this segment.
       *  Include applications created from the \e Infrastructure relationship.
       */

      const std::vector<const BaseApplication *>&
      get_infrastructure() const
      {
        return m_infrastructure;
      }

      /**
       *  Get segment applications.
       *  Include applications created from \e Resources and \e Applications relationships.
       */

      const std::vector<const BaseApplication *>&
      get_applications() const
      {
        return m_applications;
      }

      /**
       *  Get nested segments.
       *  Include generated template segments.
       */

      const std::vector<const Segment *>&
      get_nested_segments() const
      {
        return m_nested_segments;
      }

      /**
       *  Get hosts of the segment.
       *  Such hosts are used to run applications without explicitly defined host via "RunsOn" relationship
       */

      const std::vector<const Computer *>&
      get_hosts() const
      {
        return m_hosts;
      }

      /**
       *  Get disabled status.
       *  \return \b true if the segment is disabled and \b false if it is not
       */

      bool
      is_disabled() const
      {
        return m_is_disabled;
      }

      /**
       *  Get disabled status.
       *  \return \b true if the segment is templated and \b false if it is not
       */

      bool
      is_templated() const
      {
        return m_is_templated;
      }


    private:

      const daq::core::Partition * m_partition;
      const daq::core::Segment * m_base_segment;
      const BaseApplication * m_controller;
      std::vector<const BaseApplication *> m_infrastructure;
      std::vector<const BaseApplication *> m_applications;
      std::vector<const Segment *> m_nested_segments;
      std::vector<const daq::core::Computer *> m_hosts;
      bool m_is_disabled;
      bool m_is_templated;

      void
      clear(const Partition * p)
      {
        m_partition = p;
        m_base_segment = nullptr;
        m_controller = nullptr;
        m_infrastructure.clear();
        m_applications.clear();
        m_nested_segments.clear();
        m_hosts.clear();
        m_is_disabled = false;
        m_is_templated = false;
      }

    };

  }
}

#endif

#ifndef _dal_app_config_H_
#define _dal_app_config_H_

#include <string>
#include <vector>

namespace daq {
  namespace core {

      // forward declarations

    class BaseApplication;
    class Computer;
    class Segment;
    class Partition;
    class Tag;
    class BackupHostFactory;

    /**
     * \brief The class describes application configuration parameters
     *
     *  The class provides methods to get description an application.
     *
     *  It hides differences between normal (i.e. created from \b Application configuration class) and template
     *  (i.e. created from \b TemplateApplication configuration class) applications.
     *  In case of normal application a name (or ID) of application is equal to the unique ID of corresponding
     *  configuration object. For template application it is generated taking into account segment, host and
     *  instance ID, e.g.:
     *  - "segment-name:host-id" for template controller
     *  - "application-id:segment-name:host-id" for template application with instance number = 1
     *  - "application-id:segment-name:host-id:instance-id" for others
     *
     *  The main properties of an application are:
     *  - use method get_app_id() to get application id
     *  - use method get_base_app() to get configuration database object for this application
     *  - use method get_host() to get host where application is running
     *  - use method get_segment() to get configuration object describing segment this application belongs to
     *  - use method get_seg_id() to get segment id (is different from segment configuration object ID for template segments)
     *
     *  An object of AppConfig class may only be created by the SegConfig::get_all_applications() and
     *  the daq::core::Partition::get_all_applications() algorithms. A user should not create an object of AppConfig class
     *  (constructor remains public by efficiency reasons).
     **/

    class AppConfig
    {

      friend class AlgorithmUtils;
      friend class Partition;

    public:

      /**
       *  The constructor may only used by the SegConfig::get_all_applications() algorithm to create normal application.
       *  It cannot be made truly private by efficiency reasons.
       **/
      AppConfig(const BaseApplication * app, const Computer * host, const daq::core::Segment * seg);

      /**
       *  The constructor may only used by the SegConfig::get_all_applications() algorithm to create templated application.
       *  It cannot be made truly private by efficiency reasons.
       **/
      AppConfig(const BaseApplication * app, const Computer * host, const daq::core::Segment * seg, BackupHostFactory& factory);

      /**
       * Get base application object used to configure the application.
       * \throw Throw daq::core::NotInitedObject if the object was not initialized and cannot be used.
       */
      const BaseApplication *
      get_base_app() const
      {
        return m_base_app;
      }

      /**
       * Get the host where the application runs on.
       * \throw Throw daq::core::NotInitedObject if the object was not initialized and cannot be used.
       */
      const Computer *
      get_host() const
      {
        return m_host;
      }

      /**
       * Get segment that the application belongs.
       * \throw Throw daq::core::NotInitedObject if the object was not initialized and cannot be used.
       */
      const Segment *
      get_segment() const
      {
        return m_segment;
      }

      /**
       *  Get backup hosts for this application.
       *
       *  The method returns vector of computers where the application can be restarted in case of problems.
       *
       *  For normal applications the backup hosts are defined via "BackupHosts" relationship.
       *  For template applications with "RunsOn" attribute set to "FirstHostWithBackup" the backup hosts are
       *  randomly chosen from list of segment hosts; there are no backup hosts for other types of template "RunsOn".
       *
       *  \throw Throw daq::config::Exception in case of problems
       */

      std::vector<const Computer *>
      get_backup_hosts() const;

      bool
      get_is_templated() const
      {
        return m_is_templated;
      }

    private:

      const BaseApplication * m_base_app;
      const Computer * m_host;
      const Segment * m_segment;
      bool m_is_templated;
      std::vector<const daq::core::Computer *> m_template_backup_hosts;

      void
      clear()
      {
        m_base_app = nullptr;
        m_host = nullptr;
        m_segment = nullptr;
        m_is_templated = false;
        m_template_backup_hosts.clear();
      }

      AppConfig()
      {
        clear();
      }

    };

  }
}

#endif

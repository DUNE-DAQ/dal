#ifndef _dal_application_config_H_
#define _dal_application_config_H_

#include <atomic>
#include <mutex>

#include "config/ConfigAction.hpp"

class Configuration;

namespace daq
{
  namespace core
  {
    class Segment;
    class Partition;

    class ApplicationConfig : public ConfigAction
    {
      friend class Partition;

    private:

      ::Configuration& m_db;
      mutable std::atomic<const daq::core::Segment*> m_root_segment;
      mutable std::mutex m_root_segment_mutex;

      void
      __clear() noexcept
      {
        std::lock_guard<std::mutex> scoped_lock(m_root_segment_mutex);
        m_root_segment.store(nullptr);
      }

    public:

      ApplicationConfig(::Configuration& db);

      virtual
      ~ApplicationConfig();

      void
      notify(std::vector<ConfigurationChange *>& /*changes*/) noexcept
      {
        __clear();
      }

      void
      load() noexcept
      {
        __clear();
      }

      void
      unload() noexcept
      {
        __clear();
      }

      void
      update(const ConfigObject& /*obj*/, const std::string& /*name*/) noexcept
      {
        __clear();
      }

    };
  }
}

#endif

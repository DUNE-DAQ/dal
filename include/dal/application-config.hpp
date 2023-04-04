#ifndef _dal_application_config_H_
#define _dal_application_config_H_

#include <atomic>
#include <mutex>

#include "oksdbinterfaces/ConfigAction.hpp"

namespace dunedaq {
  namespace oksdbinterfaces {
    class Configuration;
  }
}

namespace dunedaq::dal {

    class Segment;
    class Partition;

    class ApplicationConfig : public dunedaq::oksdbinterfaces::ConfigAction
    {
      friend class Partition;

    private:

      dunedaq::oksdbinterfaces::Configuration& m_db;
      mutable std::atomic<const dunedaq::dal::Segment*> m_root_segment;
      mutable std::mutex m_root_segment_mutex;

      void
      __clear() noexcept
      {
        std::lock_guard<std::mutex> scoped_lock(m_root_segment_mutex);
        m_root_segment.store(nullptr);
      }

    public:

      ApplicationConfig(dunedaq::oksdbinterfaces::Configuration& db);

      virtual
      ~ApplicationConfig();

      void
      notify(std::vector<dunedaq::oksdbinterfaces::ConfigurationChange *>& /*changes*/) noexcept
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
      update(const dunedaq::oksdbinterfaces::ConfigObject& /*obj*/, const std::string& /*name*/) noexcept
      {
        __clear();
      }

    };
} // namespace dunedaq::dal

#endif

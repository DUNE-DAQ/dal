#ifndef _dal_application_config_H_
#define _dal_application_config_H_

#include <atomic>
#include <mutex>

#include "conffwk/ConfigAction.hpp"

namespace dunedaq {
  namespace conffwk {
    class Configuration;
  }
}

namespace dunedaq::dal {

    class Segment;
    class Partition;

    class ApplicationConfig : public dunedaq::conffwk::ConfigAction
    {
      friend class Partition;

    private:

      dunedaq::conffwk::Configuration& m_db;
      mutable std::atomic<const dunedaq::dal::Segment*> m_root_segment;
      mutable std::mutex m_root_segment_mutex;

      void
      __clear() noexcept
      {
        std::lock_guard<std::mutex> scoped_lock(m_root_segment_mutex);
        m_root_segment.store(nullptr);
      }

    public:

      ApplicationConfig(dunedaq::conffwk::Configuration& db);

      virtual
      ~ApplicationConfig();

      void
      notify(std::vector<dunedaq::conffwk::ConfigurationChange *>& /*changes*/) noexcept
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
      update(const dunedaq::conffwk::ConfigObject& /*obj*/, const std::string& /*name*/) noexcept
      {
        __clear();
      }

    };
} // namespace dunedaq::dal

#endif

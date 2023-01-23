#ifndef _dal_disabled_components_H_
#define _dal_disabled_components_H_

#include <string>
#include <vector>

#include "config/Configuration.hpp"
#include "config/ConfigAction.hpp"

#include "dal/Component.hpp"

namespace dunedaq::dal {

    class Partition;
    class ResourceSet;
    class Segment;

    class DisabledComponents : public ConfigAction
    {

      friend class Partition;
      friend class Component;

    private:

      struct SortStringPtr
      {
        bool
        operator()(const std::string * s1, const std::string * s2) const
        {
          return (*s1 < *s2);
        }
      };

      ::Configuration& m_db;

      unsigned long m_num_of_slr_enabled_resources;
      unsigned long m_num_of_slr_disabled_resources;

      std::set<const std::string *, SortStringPtr> m_disabled;
      std::set<const dunedaq::dal::Component *> m_user_disabled;
      std::set<const dunedaq::dal::Component *> m_user_enabled;

      void
      __clear() noexcept
      {
        m_disabled.clear();
        m_user_disabled.clear();
        m_user_enabled.clear();
        m_num_of_slr_enabled_resources = 0;
        m_num_of_slr_disabled_resources = 0;
      }

    public:

      DisabledComponents(::Configuration& db);

      virtual
      ~DisabledComponents();

      void
      notify(std::vector<ConfigurationChange *>& /*changes*/) noexcept;

      void
      load() noexcept;

      void
      unload() noexcept;

      void
      update(const ConfigObject& obj, const std::string& name) noexcept;

      void
      reset() noexcept;

      size_t
      size() noexcept
      {
        return m_disabled.size();
      }

      void
      disable(const dunedaq::dal::Component& c)
      {
        m_disabled.insert(&c.UID());
      }

      bool
      is_enabled(const dunedaq::dal::Component* c);

      bool
      is_enabled_short(const dunedaq::dal::Component* c)
      {
        return (m_disabled.find(&c->UID()) == m_disabled.end());
      }

      void
      disable_children(const dunedaq::dal::ResourceSet&);

      void
      disable_children(const dunedaq::dal::Segment&);

      static unsigned long
      get_num_of_slr_resources(const dunedaq::dal::Partition& p);

    };
} // namespace dunedaq::daq

#endif

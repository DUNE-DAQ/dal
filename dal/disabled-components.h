#ifndef _dal_disabled_components_H_
#define _dal_disabled_components_H_

#include <string>
#include <vector>

#include "config/Configuration.h"
#include "config/ConfigAction.h"

#include "dal/Component.h"

namespace daq
{
  namespace core
  {

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
      std::set<const daq::core::Component *> m_user_disabled;
      std::set<const daq::core::Component *> m_user_enabled;

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
      disable(const daq::core::Component& c)
      {
        m_disabled.insert(&c.UID());
      }

      bool
      is_enabled(const daq::core::Component* c);

      bool
      is_enabled_short(const daq::core::Component* c)
      {
        return (m_disabled.find(&c->UID()) == m_disabled.end());
      }

      void
      disable_children(const daq::core::ResourceSet&);

      void
      disable_children(const daq::core::Segment&);

      static unsigned long
      get_num_of_slr_resources(const daq::core::Partition& p);

    };
  }
}

#endif

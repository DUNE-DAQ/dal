#include "dal/Application.h"
#include "dal/Partition.h"
#include "dal/ResourceSet.h"
#include "dal/ResourceSetAND.h"
#include "dal/ResourceSetOR.h"
#include "dal/Segment.h"
#include "dal/TemplateApplication.h"
#include "dal/OnlineSegment.h"
#include "dal/util.h"
#include "dal/disabled-components.h"

#include "test_circular_dependency.h"


daq::core::DisabledComponents::DisabledComponents(::Configuration& db) :
  m_db(db),
  m_num_of_slr_enabled_resources(0),
  m_num_of_slr_disabled_resources(0)
{
  ERS_DEBUG(2, "construct the object " << (void *)this );
  m_db.add_action(this);
}

daq::core::DisabledComponents::~DisabledComponents()
{
  ERS_DEBUG(2, "destroy the object " << (void *)this);
  m_db.remove_action(this);
}

void
daq::core::DisabledComponents::notify(std::vector<ConfigurationChange *>& /*changes*/) noexcept
{
  ERS_DEBUG(2, "reset partition components because of notification callback on object " << (void *)this);
  __clear();
}

void
daq::core::DisabledComponents::load() noexcept
{
  ERS_DEBUG(2, "reset partition components because of configuration load on object " << (void *)this);
  __clear();
}

void
daq::core::DisabledComponents::unload() noexcept
{
  ERS_DEBUG(2, "reset partition components because of configuration unload on object " << (void *)this);
  __clear();
}

void
daq::core::DisabledComponents::update(const ConfigObject& obj, const std::string& name) noexcept
{
  ERS_DEBUG(2, "reset partition components because of configuration update (obj = " << obj << ", name = \'" << name << "\') on object " << (void *)this);
  __clear();
}

void
daq::core::DisabledComponents::reset() noexcept
{
  ERS_DEBUG(2, "reset disabled by explicit user call");
  m_disabled.clear(); // do not clear s_user_disabled && s_user_enabled !!!
}

bool
daq::core::DisabledComponents::is_enabled(const daq::core::Component * c)
{
  if (const daq::core::Segment * seg = c->cast<daq::core::Segment>())
    {
      if (daq::core::SegConfig * conf = seg->get_seg_config(false, true))
        {
          return !conf->is_disabled();
        }
    }
  else if (const daq::core::BaseApplication * app = c->cast<daq::core::BaseApplication>())
    {
      if (const daq::core::AppConfig * conf = app->get_app_config(true))
        {
          const daq::core::BaseApplication * base = conf->get_base_app();
          if (base != app && is_enabled_short(base->cast<daq::core::Component>()) == false)
            return false;
        }
    }

  return is_enabled_short(c);
}


void
daq::core::Partition::set_disabled(const std::set<const daq::core::Component *>& objs) const
{
  m_disabled_components.m_user_disabled.clear();

  for (const auto& i : objs)
      m_disabled_components.m_user_disabled.insert(i);

  m_disabled_components.m_num_of_slr_disabled_resources = m_disabled_components.m_user_disabled.size();

  m_disabled_components.reset();

  m_app_config.__clear();
}

void
daq::core::Partition::set_enabled(const std::set<const daq::core::Component *>& objs) const
{
  m_disabled_components.m_user_enabled.clear();

  for (const auto& i : objs)
    m_disabled_components.m_user_enabled.insert(i);

  m_disabled_components.m_num_of_slr_enabled_resources = m_disabled_components.m_user_enabled.size();

  m_disabled_components.reset();

  m_app_config.__clear();
}

void
daq::core::DisabledComponents::disable_children(const daq::core::ResourceSet& rs)
{
  for (auto & i : rs.get_Contains())
    {
      // FIXME 2022-04-22: implement efficient castable() method working with pointers
      //if (i->castable(&daq::core::TemplateApplication::s_class_name) == false)
      if (i->cast<daq::core::TemplateApplication>() == nullptr)
        {
          ERS_DEBUG(6, "disable resource " << i << " because it's parent resource-set " << &rs << " is disabled");
          disable(*i);
        }
      else
        {
          ERS_DEBUG(6, "do not disable template resource application " << i << " (it's parent resource-set " << &rs << " is disabled)");
        }

      if (const daq::core::ResourceSet * rs2 = i->cast<daq::core::ResourceSet>())
        {
          disable_children(*rs2);
        }
    }
}

void
daq::core::DisabledComponents::disable_children(const daq::core::Segment& s)
{
  for (auto & i : s.get_Resources())
    {
      // FIXME 2022-04-22: implement efficient castable() method working with pointers
      //if (i->castable(&daq::core::TemplateApplication::s_class_name) == false)
      if (i->cast<daq::core::TemplateApplication>() == nullptr)
        {
          ERS_DEBUG(6, "disable resource " << i << " because it's parent segment " << &s << " is disabled");
          disable(*i);
        }
      else
        {
          ERS_DEBUG(6, "do not disable template resource application " << i << " (it's parent segment " << &s << " is disabled)");
        }

      if (const daq::core::ResourceSet * rs = i->cast<daq::core::ResourceSet>())
        {
          disable_children(*rs);
        }
    }

  for (auto & j : s.get_Segments())
    {
      ERS_DEBUG(6, "disable segment " << j << " because it's parent segment " << &s << " is disabled");
      disable(*j);
      disable_children(*j);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace daq {
  ERS_DECLARE_ISSUE_BASE(
    core,
    ReadMaxAllowedIterations,
    AlgorithmError,
    "Has exceeded the maximum of iterations allowed (" << limit << ") during calculation of disabled objects",
    ,
    ((unsigned int)limit)
  )
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // fill data from resource sets

static void fill(
  const daq::core::ResourceSet& rs,
  std::vector<const daq::core::ResourceSetOR *>& rs_or,
  std::vector<const daq::core::ResourceSetAND *>& rs_and,
  daq::core::TestCircularDependency& cd_fuse
)
{
  if (const daq::core::ResourceSetAND * r1 = rs.cast<daq::core::ResourceSetAND>())
    {
      rs_and.push_back(r1);
    }
  else if (const daq::core::ResourceSetOR * r2 = rs.cast<daq::core::ResourceSetOR>())
    {
      rs_or.push_back(r2);
    }

  for (auto & i : rs.get_Contains())
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
      if (const daq::core::ResourceSet * rs2 = i->cast<daq::core::ResourceSet>())
        {
          fill(*rs2, rs_or, rs_and, cd_fuse);
        }
    }
}


  // fill data from segments

static void fill(
  const daq::core::Segment& s,
  std::vector<const daq::core::ResourceSetOR *>& rs_or,
  std::vector<const daq::core::ResourceSetAND *>& rs_and,
  daq::core::TestCircularDependency& cd_fuse
)
{
  for (auto & i : s.get_Resources())
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
      if (const daq::core::ResourceSet * rs = i->cast<daq::core::ResourceSet>())
        {
          fill(*rs, rs_or, rs_and, cd_fuse);
        }
    }

  for (auto & j : s.get_Segments())
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, j);
      fill(*j, rs_or, rs_and, cd_fuse);
    }
}


  // fill data from partition

static void fill(
  const daq::core::Partition& p,
  std::vector<const daq::core::ResourceSetOR *>& rs_or,
  std::vector<const daq::core::ResourceSetAND *>& rs_and,
  daq::core::TestCircularDependency& cd_fuse
)
{
  if (const daq::core::OnlineSegment * onlseg = p.get_OnlineInfrastructure())
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, onlseg);
      fill(*onlseg, rs_or, rs_and, cd_fuse);

      // NOTE: normally application may not be ResourceSet, but for some "exotic" cases put this code
      for (auto &a : p.get_OnlineInfrastructureApplications())
        {
          if (const daq::core::ResourceSet * rs = a->cast<daq::core::ResourceSet>())
            {
              fill(*rs, rs_or, rs_and, cd_fuse);
            }
        }
    }

  for (auto & i : p.get_Segments())
    {
      daq::core::AddTestOnCircularDependency add_fuse_test(cd_fuse, i);
      fill(*i, rs_or, rs_and, cd_fuse);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool
daq::core::Component::disabled(const daq::core::Partition& partition, bool skip_check) const
{
  // fill disabled (e.g. after partition changes)

  if (partition.m_disabled_components.size() == 0)
    {
      if (partition.get_Disabled().empty() && partition.m_disabled_components.m_user_disabled.empty())
        {
          return false;  // the partition has no disabled components
        }
      else
        {
          // get two lists of all partition's resource-set-or and resource-set-and
          // also test any circular dependencies between segments and resource sets
          daq::core::TestCircularDependency cd_fuse("component \'is-disabled\' status", &partition);
          std::vector<const daq::core::ResourceSetOR *> rs_or;
          std::vector<const daq::core::ResourceSetAND *> rs_and;
          fill(partition, rs_or, rs_and, cd_fuse);

          // calculate explicitly and implicitly (nested) disabled components
            {
              std::vector<const daq::core::Component *> vector_of_disabled;
              vector_of_disabled.reserve(partition.get_Disabled().size() + partition.m_disabled_components.m_user_disabled.size());

              // add user disabled components, if any
              for (auto & i : partition.m_disabled_components.m_user_disabled)
                {
                  vector_of_disabled.push_back(i);
                  ERS_DEBUG(6, "disable component " << i << " because it is explicitly disabled by user");
                }

              // add partition-disabled components ignoring explicitly enabled by user
              for (auto & i : partition.get_Disabled())
                {
                  ERS_DEBUG(6, "check component " << i << " explicitly disabled in partition");

                  if (partition.m_disabled_components.m_user_enabled.find(i) == partition.m_disabled_components.m_user_enabled.end())
                    {
                      vector_of_disabled.push_back(i);
                      ERS_DEBUG(6, "disable component " << i << " because it is explicitly disabled in partition");
                    }
                  else
                    {
                      ERS_DEBUG(6, "skip component " << i << " because it is enabled by user");
                    }
                }

              // fill set of explicitly and implicitly (segment/resource-set containers) disabled components
              for (auto & i : vector_of_disabled)
                {
                  partition.m_disabled_components.disable(*i);

                  if (const daq::core::ResourceSet * rs = i->cast<daq::core::ResourceSet>())
                    {
                      partition.m_disabled_components.disable_children(*rs);
                    }
                  else if (const daq::core::Segment * seg = i->cast<daq::core::Segment>())
                    {
                      partition.m_disabled_components.disable_children(*seg);
                    }
                }
            }

          for (unsigned long count = 1; true; ++count)
            {
              const unsigned long num(partition.m_disabled_components.size());

              ERS_DEBUG(6, "before auto-disabling iteration " << count << " the number of disabled components is " << num);

              for (const auto& i : rs_or)
                {
                  if (partition.m_disabled_components.is_enabled_short(i))
                    {
                      // check ANY child is disabled
                      for (auto & i2 : i->get_Contains())
                        {
                          if (!partition.m_disabled_components.is_enabled_short(i2))
                            {
                              ERS_DEBUG(6, "disable resource-set-OR " << i << " because it's child " << i2 << " is disabled");
                              partition.m_disabled_components.disable(*i);
                              partition.m_disabled_components.disable_children(*i);
                              break;
                            }
                        }
                    }
                }

              for (const auto& j : rs_and)
                {
                  if (partition.m_disabled_components.is_enabled_short(j))
                    {
                      const std::vector<const daq::core::ResourceBase*> &resources = j->get_Contains();

                      if (!resources.empty())
                        {
                          // check ANY child is enabled
                          bool found_enabled = false;
                          for (auto & j2 : resources)
                            {
                              if (partition.m_disabled_components.is_enabled_short(j2))
                                {
                                  found_enabled = true;
                                  break;
                                }
                            }
                          if (found_enabled == false)
                            {
                              ERS_DEBUG(6, "disable resource-set-AND " << j << " because all it's children are disabled");
                              partition.m_disabled_components.disable(*j);
                              partition.m_disabled_components.disable_children(*j);
                            }
                        }
                    }
                }

              if (partition.m_disabled_components.size() == num)
                {
                  ERS_DEBUG(6, "after " << count << " iteration(s) auto-disabling algorithm found no newly disabled sets, exiting loop ...");
                  break;
                }

              unsigned int iLimit(1000);

              if (count > iLimit)
                {
                  ers::error(daq::core::ReadMaxAllowedIterations(ERS_HERE, iLimit));
                  break;
                }
            }
        }
    }

  bool result(skip_check ? !partition.m_disabled_components.is_enabled_short(this) : !partition.m_disabled_components.is_enabled(this));
  ERS_DEBUG( 6, "disabled(" << this << ") returns " << std::boolalpha << result );
  return result;
}

unsigned long
daq::core::DisabledComponents::get_num_of_slr_resources(const daq::core::Partition& p)
{
  return (p.m_disabled_components.m_num_of_slr_enabled_resources + p.m_disabled_components.m_num_of_slr_disabled_resources);
}

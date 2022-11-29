package dal;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedList;
import java.util.TreeMap;
import java.util.TreeSet;


/**
 * The class Component_Comparator is required to use
 * TreeSet<dal.Component>, i.e. to define a comparator for two
 * dal.Component objects
 */

class Component_Comparator implements Comparator<dal.Component>
  {
    public int compare(dal.Component a, dal.Component b)
      {
        return b.UID().compareTo(a.UID());
      }
  }


  /**
   *  The <b>Resources</b> class provides interface to give status of components (segments and resources) disabling.
   *  A calculation of disabling information for a big partition requires a lot of CPU resources.
   *  By this reason such calculation is done once for all components at first request and is stored inside cache map.
   *  Above cache is cleaned on each database reload or modification (is implemented using config.ConfigAction mechanism).
   *  <p>
   *  By efficiency reasons an object of such class is used as a singleton per partition object.
   *  The following algorithms are available to user:
   *  <ul>
   *   <li><code>{@link dal.Partition#set_disabled(dal.Component[])}</code> - temporarily set given components as 'disabled' without modifying partition object in database
   *   <li><code>{@link dal.Partition#set_enabled(dal.Component[])}</code> - temporarily set given components as 'enabled' without modifying partition object in database
   *   <li><code>{@link Component#disabled(dal.Partition)}</code> - return true, if given component is disabled in a scope of the partition
   *   <li><code>{@link Component#why_disabled(dal.Partition, String, boolean)}</code> - give textual description why given component is disabled
   *  </ul>
   *  <p>
   *
   *  @author  "http://consult.cern.ch/xwho/people/432778"
   *  @since   TDAQ release 02-00-01
   */

public class Resources implements config.ConfigAction {

    /**
     * the [key] - disabled component
     * the [value] is:
     *  # null                                        => the component is explicitly disabled
     *  # else either:
     *    - parent                                    => [value] is disabled 
     *    - the key is RS-OR                          => [value] is one of disabled child
     *    - the key is RS-AND and "[value] == [key]"  => all children are disabled
     */

  private TreeMap<String,dal.Component> s_disabled;

  private TreeSet<dal.Component> s_user_disabled;
  private TreeSet<dal.Component> s_user_enabled;
  

    /**
     *  Create new object to store in cache disabled status of components,
     *  to allow user disabling and enabling and to get explanation
     *  "why given component is disabled".
     *
     *  @param db  configuration database object
     */

  public Resources(config.Configuration db) {
    clear_all_disabled();
    db.add_config_action(this);
  }

    /**
     *  The method temporarily marks given components as 'disabled'
     *  without modifying partition object in database.
     *  Above is overwritten by consequent set_disabled() call.
     *  It is automatically clean when database is updated or reload.
     *
     *  @param objs  vector of components to be temporarily disabled
     */

    public synchronized void set_disabled(dal.Component objs[])
      {
        if (s_user_disabled != null)
          {
            s_user_disabled.clear();
          }
        else
          {
            s_user_disabled = new TreeSet<dal.Component>(new Component_Comparator());
          }

        for (dal.Component o : objs)
          {
            s_user_disabled.add(o);
          }

        if (s_disabled != null)
          {
            s_disabled.clear();
            s_disabled = null;
          }
      }


    /**
     *  The method temporarily marks given components as 'enabled'
     *  without modifying partition object in database.
     *  Above is overwritten by consequent set_enabled() call.
     *  It is automatically clean when database is updated or reload.
     *
     *  @param objs  vector of components to be temporarily enabled.
     */

    public synchronized void set_enabled(dal.Component objs[])
      {
        if (s_user_enabled != null)
          {
            s_user_enabled.clear();
          }

        if (objs.length > 0)
          {
            s_user_enabled = new TreeSet<dal.Component>(new Component_Comparator());
            for (dal.Component o : objs)
              {
                s_user_enabled.add(o);
              }
          }
        else
          {
            s_user_enabled = null;
          }

        if (s_disabled != null)
          {
            s_disabled.clear();
            s_disabled = null;
          }
      }


    /**
     *  The method returns 'disabled' status of a component in a scope of partition.
     *
     *  Since the calculation of disabled status requires a lot of CPU resources,
     *  it is done once for all components of the partition, and the result is stored in cache map.
     *  This map is automatically updated on database modification or reload.
     *
     *  @param obj         the component to be tested
     *  @param p           partition object
     *  @param skip_check  skip check of application/segment configuration
     *
     *  @return <b>true</b> if component is disabled
     */

  public synchronized boolean get_disabled(dal.Component obj, dal.Partition p, boolean skip_check) throws config.GenericException, config.SystemException, config.NotFoundException, config.NotValidException {

      // fill disabled (e.g. after partition changes)

    if(s_disabled == null) {
      if(p.get_Disabled().length > 0 || (s_user_disabled != null && s_user_disabled.size() > 0)) {
        s_disabled = new TreeMap<String,dal.Component>();

          // get two lists of all partition's resource-set-or and resource-set-and
        LinkedList<dal.ResourceSetOR> rs_or = new LinkedList<dal.ResourceSetOR>();
        LinkedList<dal.ResourceSetAND> rs_and = new LinkedList<dal.ResourceSetAND>();
        fill(p, rs_or, rs_and);

        ArrayList<dal.Component> disabled_list = new ArrayList<dal.Component>();
        
        // add user disabled components, if any
        if(s_user_disabled != null) {
          for(dal.Component c : s_user_disabled) {
            disabled_list.add(c);
          }
        }
        
        // add partition-disabled components ignoring explicitly enabled by user
        for(dal.Component c : p.get_Disabled()) {
          if(s_user_enabled == null || s_user_enabled.contains(c) == false) {
            disabled_list.add(c);
          }
        }

        // fill map of implicitly (via segment/resource-set containers) disabled components
        for(dal.Component c : disabled_list)
          {
            disable(c,null);
            
            dal.Segment seg = dal.Segment_Helper.cast(c);
            if(seg != null) {
              disable_children(seg);
            }
            else {
              dal.ResourceSet rs = dal.ResourceSet_Helper.cast(c);
              if(rs != null) {
                disable_children(rs);
              }
            }
          }

        for(int count = 1; true; count++) {
          int num = s_disabled.size();

          for( dal.ResourceSetOR rs : rs_or) {
            if(is_enabled(rs)) {
              // check ANY child is disabled
              for(dal.ResourceBase rb : rs.get_Contains()) {
                if(!is_enabled(rb)) {
                  disable(rs,dal.Component_Helper.cast(rb));
                  disable_children(rs);
                  break;
                }
              }
            }
          }

          for( dal.ResourceSetAND rs : rs_and) {
            if(is_enabled(rs) && rs.get_Contains().length > 0) {
	          // check ANY child is enabled
              boolean found_enabled = false;
              for(dal.ResourceBase rb : rs.get_Contains()) {
                if(is_enabled(rb)) {
                  found_enabled = true;
                  break;
                }
              }
              if(found_enabled == false) {
                disable(rs,dal.Component_Helper.cast(rs)); // i.e. all children are disabled
                disable_children(rs);
              }
            }
          }

          if(s_disabled.size() == num) {
            break;
          }

          final int iLimit = 1000;
  
          if(count > iLimit)
            throw new config.GenericException("exceeded the maximum of iterations allowed (" + iLimit + ") during calculation of disabled objects");
        }
      }
      else {
        return false;  // the partition has no disabled components and PartitionComponents is not inited
      }
    }

    return(skip_check ? !is_enabled_short(obj) : !is_enabled(obj));
  }


    /**
     *  The method returns textual description of a reason why given component was disabled.
     *
     *  Example:
     * <ul>
     * <li> explicitly disabled resource: <pre>
     *  object 'BCM_ROD_Spare@BCM_ROD_Module' is disabled because
     *    it is explicitly disabled </pre>
     *
     * <li> disabled because of direct parent: <pre>
     *  object 'TRTBarrelC_LTP_BusyChannel_BC0@BusyChannel' is disabled because
     *    it's parent TRTBarrelC_LTP@LTPModule is disabled because:
     *      it is explicitly disabled </pre>
     *
     * <li> disabled because of chain of parents: <pre>
     *  object 'PixelL12_TIM4_busy@BusyChannel' is disabled because
     *    it's parent PixelL12_RODBusy@RODBusyModule is disabled because:
     *      it's parent PixelL12_LTP@LTPModule is disabled because:
     *        it's parent PixelDisks_LTP@LTPModule is disabled because:
     *          it's parent PixelL0_LTP@LTPModule is disabled because:
     *            it's parent PixelLTPi_Global@LTPiModule is disabled because:
     *              it's parent RCDPixelTtcCratePitL0@RCD is disabled because:
     *                it's parent PixelTtcCrateBLayer@Segment is disabled because:
     *                  it's parent Pixel@Segment is disabled because:
     *                    it is explicitly disabled </pre>
     *
     * <li> resource-set OR disabling: <pre>
     *  object 'ROS-TIL-LBC-ROL-16@ResourceSetOR' is disabled because
     *    it is "resource-set-OR" and at least one child TileLaserModule@TileLaserModule is disabled because:
     *      it's parent TileLaserRCD@RCD is disabled because:
     *        it is explicitly disabled </pre>
     *
     * <li> resource-set AND disabling (dummy example): <pre>
     *  object 'ROBIN-TRT-ECC-03-3@RobinReadoutModule' is disabled because
     *    it is "resource-set-AND" and all children (2) are disabled:
     *      [1] component ROL-TRT-ECC-03-341402@RobinDataChannel is disabled because:
     *        it's parent rod341402_ResourceSet@ResourceSetOR is disabled because:
     *          it is "resource-set-OR" and at least one child rod341402@TRTROD05Module is disabled because:
     *            it is explicitly disabled
     *      [2] component dummy@RobinDataChannel is disabled because:
     *        it is explicitly disabled </pre>
     *  </ul>
     *
     *  Since the calculation of disabled status requires a lot of CPU resources,
     *  it is done once for all components of the partition, and the result is stored in cache map.
     *  This map is automatically updated on database modification or reload.
     *
     *  @param c          the disabled component
     *  @param prefix     put such prefix before any new output line
     *  @param detailed   if true, give detailed explanation why given component is disabled (use recursion for parents)
     *
     *  @return text explaining why component is disabled
     */

  public synchronized String why_disabled(dal.Component c, String prefix, boolean detailed) throws config.GenericException, config.SystemException, config.NotFoundException, config.NotValidException {
    String prefix2 = (detailed ? new String(prefix + "  ") : null);

    dal.Component v = s_disabled.get(c.UID());
    if(v != null) {
      dal.ResourceSetOR rs_or = dal.ResourceSetOR_Helper.cast(c);
      if(rs_or != null) {
        for(int i = 0; i < rs_or.get_Contains().length; i++) {
          dal.ResourceBase rb_obj = rs_or.get_Contains()[i];
          if(v.UID().compareTo(rb_obj.UID()) == 0 && v.class_name().compareTo(rb_obj.class_name()) == 0) {
            return new String( prefix + 
              "it is \"resource-set-OR\" and at least one child " + v.UID() + '@' + v.class_name() + " is disabled" +
              (detailed ? (" because:\n" + why_disabled(v, prefix2, true)) : "")
            );
          }
        }
      }
      else {
        dal.ResourceSetAND rs_and = dal.ResourceSetAND_Helper.cast(c);
        if(rs_and != null && c.UID().compareTo(v.UID()) == 0 && c.class_name().compareTo(v.class_name()) == 0) {
          StringBuffer out = new StringBuffer( prefix +
            "it is \"resource-set-AND\" and " + (rs_and.get_Contains().length > 0 ? "all children (" + rs_and.get_Contains().length + ") are disabled:" : "it has no children")
          );

          String prefix4 = (detailed ? new String(prefix + "    ") : null);

          for(int i = 0; i < rs_and.get_Contains().length; i++) {
            dal.ResourceBase rb_obj = rs_and.get_Contains()[i];
            out.append( '\n' + prefix + 
               "  [" + (i+1) + "] component " + rb_obj.UID() + '@' + rb_obj.class_name() + " is disabled" +
               (detailed ? (" because:\n" + why_disabled(rb_obj, prefix4, true)) : "")
            );
          }

          return out.toString();
        }
      }

      return( prefix + 
        "it\'s parent " + v.UID() + '@' + v.class_name() + " is disabled" +
        (detailed ? (" because:\n" + why_disabled(v, prefix2, true)) : "")
      );
    }
    else {
      if(is_enabled(c)) {
        throw new config.SystemException(new String("Component " + c.UID() + '@' + c.class_name() + " is enabled"));
      }
      else {
        return new String(prefix + "it is explicitly disabled");
      }
    }
  }


    /** config.ConfigAction: clean cache on any database update */

  public synchronized void notify( config.Change[] changes ) {
    clear_all_disabled();
  }


    /** config.ConfigAction: clean cache on any database unload */

  public synchronized void unload( ) {
    clear_all_disabled();
  }


    /** config.ConfigAction: clean cache on any database load */

  public synchronized void load() {
    clear_all_disabled();
  }


    /** config.ConfigAction: clean cache on any database modification */

  public synchronized void update( config.ConfigObject obj, String name ) {
    clear_all_disabled();
  }

  synchronized boolean is_enabled_short(dal.Component c) {
    return (!s_disabled.containsKey(c.UID()));
  }
  
	private synchronized boolean is_enabled(dal.Component c) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
		dal.Segment seg = dal.Segment_Helper.cast(c);

		if (seg != null) {
			dal.SegConfig conf = seg.get_seg_config(false, true);

			if (conf != null) {
				return !conf.is_disabled();
			}
		} else {
			dal.BaseApplication app = dal.BaseApplication_Helper.cast(c);

			if (app != null) {
				dal.AppConfig conf = app.get_app_config(true);

				if (conf != null) {
					dal.BaseApplication base = conf.get_base_app();

					if (base != app && is_enabled_short(dal.Component_Helper.cast(base)) == false) {
						return false;
					}
				}
			}
		}

		return is_enabled_short(c);
	}

  private synchronized void clear_all_disabled() {
    s_disabled = null;
    s_user_enabled = null;

    if(s_user_disabled != null) {
      s_user_disabled.clear();
    }
  }

    /**
     *  This internal method is used to mark component as disabled.
     *  Note: check the key is not in the map to avoid loop dependencies between values.
     */

    private synchronized void disable(dal.Component c, dal.Component reason)
      {
        if (!s_disabled.containsKey(c.UID()))
          {
            s_disabled.put(c.UID(), reason);
          }
      }

    private void disable_children(dal.ResourceSet rs) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
      {
        dal.Component c = dal.Component_Helper.cast(rs);

        for (dal.ResourceBase o : rs.get_Contains())
          {
            if (dal.TemplateApplication_Helper.cast(o) == null)
              {
                disable(o, c);
              }
            dal.ResourceSet rs2 = dal.ResourceSet_Helper.cast(o);
            if (rs2 != null)
              {
                disable_children(rs2);
              }
          }
      }

    private void disable_children(dal.Segment s) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
      {
        dal.Component c = dal.Component_Helper.cast(s);

        for (dal.ResourceBase o : s.get_Resources())
          {
            if (dal.TemplateApplication_Helper.cast(o) == null)
              {
                disable(o, c);
              }
            dal.ResourceSet rs = dal.ResourceSet_Helper.cast(o);
            if (rs != null)
              {
                disable_children(rs);
              }
          }

        for (dal.Segment o : s.get_Segments())
          {
            disable(o, c);
            disable_children(o);
          }
      }

    // fill data from resource sets
    private static void fill(dal.ResourceSet rs, LinkedList<dal.ResourceSetOR> rs_or, LinkedList<dal.ResourceSetAND> rs_and) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
      {
        dal.ResourceSetAND r1 = dal.ResourceSetAND_Helper.cast(rs);
        if (r1 != null)
          {
            rs_and.add(r1);
          }
        else
          {
            dal.ResourceSetOR r2 = dal.ResourceSetOR_Helper.cast(rs);
            if (r2 != null)
              {
                rs_or.add(r2);
              }
          }

        for (dal.ResourceBase o : rs.get_Contains())
          {
            dal.ResourceSet rs2 = dal.ResourceSet_Helper.cast(o);
            if (rs2 != null)
              {
                fill(rs2, rs_or, rs_and);
              }
          }
      }

    // fill data from segments
    private static void fill(dal.Segment s, LinkedList<dal.ResourceSetOR> rs_or, LinkedList<dal.ResourceSetAND> rs_and) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
      {
        for (dal.ResourceBase o : s.get_Resources())
          {
            dal.ResourceSet rs = dal.ResourceSet_Helper.cast(o);
            if (rs != null)
              {
                fill(rs, rs_or, rs_and);
              }
          }

        for (dal.Segment o : s.get_Segments())
          {
            fill(o, rs_or, rs_and);
          }
      }

    // fill data from partition
    private static void fill(dal.Partition p, LinkedList<dal.ResourceSetOR> rs_or, LinkedList<dal.ResourceSetAND> rs_and) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException
      {
        if (p.get_OnlineInfrastructure() != null)
          {
            fill(p.get_OnlineInfrastructure(), rs_or, rs_and);

            for (dal.Application o : p.get_OnlineInfrastructureApplications())
              {
                dal.ResourceSet rs = dal.ResourceSet_Helper.cast(o);
                if (rs != null)
                  {
                    fill(rs, rs_or, rs_and);
                  }
              }
          }

        for (dal.Segment o : p.get_Segments())
          {
            fill(o, rs_or, rs_and);
          }
      }
  }

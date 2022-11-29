package dal;

import java.util.Enumeration;
import java.util.Hashtable;


   /**
    *  Implements string converter for database parameters.
    *  
    *  The class implements config.AttributeConverter for string type.
    *  It reads parameters defined for given partition and nested segments and uses them to
    *  substitute values of database string attributes.
    *
    *  The parameters are stored as a map of substitution keys and values.
    *  If database is changed, the reset(config.Configuration, dal.Partition) method has to be called.
    *
    *  For debug purposes use TDAQ_ERS_DEBUG_LEVEL=1.
    *
    *  @author "http://consult.cern.ch/xwho/people/432778"
    */


public class SubstituteVariables implements config.AttributeConverter {

  private java.util.Hashtable<String,String> m_parameters;

  private String substitute_variables(String value, java.util.Hashtable<String,String> cvt_map, String symbols) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
    String beg = symbols.substring(0, 2);
    String end = symbols.substring(2);

    StringBuffer s = new StringBuffer(value);

    int pos = 0;       // position of tested string index
    int p_start = 0;   // beginning of variable
    int p_end = 0;     // beginning of variable

    int rcount = 1;

    while(
      ((p_start = s.indexOf(beg, pos)) != -1) &&
      ((p_end = s.indexOf(end, p_start + 2)) != -1)
    ) {
      if(++rcount > 1000) {
        throw new config.GenericException(
          "Value \'" + value + "\' has exceeded the maximum number of substitutions allowed (1000). " +
          "It might have a circular dependency with substitution variables. " +
          "After 1000 substitutions it is \'" + s.toString() + '\''
	);
      }

      String var = s.substring(p_start + 2, p_end);

      if(cvt_map != null) {
        String v = (String)cvt_map.get(var);
        if(v != null) {
          s.replace(p_start, p_end + 1, v);
        }
      }
      else {
          String v = System.getenv(var);
          if(v != null) {
            s.replace(p_start, p_end + 1, v);
          }
      }

      pos = p_start + 1;
    }

    return s.toString();
  }

  private void add_parameter(String name, String value) {
    m_parameters.put(name, value);
    ers.Logger.debug(1, new ers.Issue("set parameter \"" + name + "\" = \"" + value + '\"' ));
  }

  private void add_parameters(dal.Parameter[] objs) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
    for(int i = 0; i < objs.length; i++) {
        // check if parameter is variable
      dal.Variable x = dal.Variable_Helper.cast(objs[i]);
      if(x != null) {
        String val = x.get_value(null);
        add_parameter(x.get_Name(), val);
          // un-read variable, if it is built on top of other variables
        if(x.get_Name().indexOf("$") != -1 || val.indexOf("$") != -1) {
          x.unread(false);
        }
        continue;
      }

        // check if parameter is set-of-variables
      dal.VariableSet s = dal.VariableSet_Helper.cast(objs[i]);
      if(s != null) {
        add_parameters(s.get_Contains());
      }
    }
  }

  private void add_segment_parameters(dal.Segment obj) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
    add_parameters(obj.get_Parameters());
    for(int i = 0; i < obj.get_Segments().length; i++) {
      add_segment_parameters(obj.get_Segments()[i]);
    }
  }

    /**
     *  The method resets parameters used for variables substitution.
     *
     *  The method to be called each time the database has been reloaded or modified to get new parameters.
     * 
     *  @param p   the partition object, which parameters are used for substitution
     *
     */

  public void reset(dal.Partition p) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {

    // clean old parameters if any
    m_parameters.clear();


    // insert TDAQ-specific parameters
    String user_name = System.getenv("USER");

    if(user_name == null || user_name.length() == 0) {
      user_name = new String("unknown");
    }

    add_parameter("TDAQ_PARTITION", p.UID());
    add_parameter("TDAQ_LOGS_ROOT", p.get_LogRoot());
    add_parameter("TDAQ_LOGS_PATH", p.get_LogRoot() + "/" + p.UID());

    // add partition's parameters
    add_parameters(p.get_Parameters());


    // add segment's parameters
    add_segment_parameters(p.get_OnlineInfrastructure());
    for(dal.Segment i : p.get_Segments()) {
      add_segment_parameters(i);
    }


    // get list of used sw repositories and add parameters, referencing their installation
    {
      dal.SW_Repository[] repositories = dal.Algorithms.get_used_repositories(p, false);

      for(dal.SW_Repository j : repositories) {
        String rn = j.get_InstallationPathVariableName();
        if(rn.length() > 0) {
          add_parameter(rn, j.get_InstallationPath());
        }
      }
    }


    // read all objects again (may have attributes to be substituted)
    p.configuration_object().unread_all_objects(false);


    // recursive substitution of the variables before they are used
    for (Enumeration<String> e = m_parameters.keys() ; e.hasMoreElements() ;) {
      String key = e.nextElement();
      String value = m_parameters.get(key);

      try {
        String n = new String(substitute_variables(value, m_parameters, "${}"));
        if(n.equals(value) == false) {
          ers.Logger.debug(1, new ers.Issue("replace \"" + value + "\" by  \"" + n + "\" using parameter \"" + key + '\"'));
          m_parameters.put(key, n);
        }
      }
      catch (config.ConfigException ex) {
          throw new config.GenericException("Failed to calculate variable \'" + key + "\'", ex);
      }
    }

  }

    /**
     *  The constructor of the variables substitution object using partition's parameters.
     *
     *  The conversion object to be registered with config.register_converter() method.
     *  The example is shown below:
     *  <pre><code>
     *    config.Configuration db = new config.Configuration("test DB");
     *    dal.Partition p = dal.Algorithms.get_partition(db, "test partition");
     *    db.register_converter(new dal.SubstituteVariables(p));
     *  </code></pre>
     *
     *  @param p   the partition object, which parameters are used for substitution
     *
     */

  public SubstituteVariables(dal.Partition p) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
    m_parameters = new Hashtable<String,String>();
    reset(p);
  }


    /** The code, which does actual conversion. It is invoked by the config.Configuration object */ 

  public Object convert(Object value, config.Configuration db, config.ConfigObject obj, String attr_name) throws config.GenericException, config.NotFoundException, config.NotValidException, config.SystemException {
    return (Object)(new String(substitute_variables((String)value, m_parameters, "${}")));
  }


    /** The object converts attributes of string type. */

  public Class get_class() {
    return String.class;
  }
}

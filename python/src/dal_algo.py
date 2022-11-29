import libdal_algo_helper
import pm
from pm.dal import dal, DFdal
import config


def _partition_get_log_directory_wrapper(self, db):
    """ wrapper for partition_get_log_directory implementing dal.Partition.get_log_directory algorithm
    """
    return libdal_algo_helper.partition_get_log_directory(self.id, db._obj)


def _component_get_parents_wrapper(self, db, partition):
    """ wrapper for component_get_parents implementing dal.Component.get_parents algorithm
    """
    parents = libdal_algo_helper.component_get_parents(self.id, db._obj, partition.id )
    parent_list =[]
    for p in parents:
        component_list =[]
        for c in p:
            component_list.append(db.get_dal(c.className, c.id))         
        parent_list.append(component_list)
    return parent_list


def _component_disabled_wrapper(self, db, partition):
    """ wrapper for component_disabled implementing dal.Component.disabled algorithm
    """
    return libdal_algo_helper.component_disabled(self.id, db._obj, partition.id) 


def _computer_program_get_info_wrapper(self, db, partition, tag, host):
    """ wrapper for computer_program_get_info implementing dal.ComputerProgram.get_info algorithm
    """
    result = {'environment' : [], 'program_names' : []}
    returned_list = libdal_algo_helper.computer_program_get_info(self.id, db._obj, partition.id, tag.id, host.id)
    for item in returned_list[0]:
        result['environment'].append( item)
    for item in returned_list[1]:
        result['program_names'].append(item)
    return result


def set_disabled_wrapper(self, db, to_be_disabled):
    """ wrapper for set_disabled
    """
    libdal_algo_helper.set_disabled(self.id, db._obj, to_be_disabled)


def set_enabled_wrapper(self, db, to_be_enabled):
    """ wrapper for set_enabled
    """
    libdal_algo_helper.set_enabled(self.id, db._obj, to_be_enabled)


def _variable_get_value_wrapper(self, db, tag):
    """ wrapper for variable_get_value
    """
    return libdal_algo_helper.variable_get_value(self.id, db._obj, tag.id)


def _partition_get_all_applications_wrapper(self, db, app_types, use_segments, use_hosts):
    """ wrapper for partition_get_all_applications implementing dal.Partition.get_all_applications algorithm
    """
    result = [] 

    if (app_types is None) :
        app_types = []

    if (use_segments is None) :
        use_segments = []

    if (use_hosts is None) :
        use_hosts = []
        
    apps = libdal_algo_helper.partition_get_all_applications(self.id, db._obj, app_types, use_segments, use_hosts)
    
    for item in apps:
        item.db = db

    return apps


# add configuration object required to create DAL objects afterwards
def _partition_get_segment_wrapper(self, db, name):
    """ wrapper for partition_get_segment implementing dal.Partition.get_segment algorithm
    """
    seg = libdal_algo_helper.partition_get_segment(self.id, db._obj,str(name))
    seg.db = db
    return seg

# create application DAL object from helper one
def _appconfig_get_base_app_wrapper(self):
    """ wrapper for get_base_app_helper
    """
    result = self.get_base_app_helper()
    return self.db.get_dal(result.className, result.id)

# create computer DAL object from helper one
def _appconfig_get_host_wrapper(self):
    """ wrapper for get_host_helper
    """
    result = self.get_host_helper()
    return self.db.get_dal(result.className, result.id)

# create segment DAL object from helper one
def _appconfig_get_base_seg_wrapper(self):
    """ wrapper for get_base_seg_helper
    """
    result = self.get_base_seg_helper()
    return self.db.get_dal(result.className, result.id)

# create vector of computer DAL objects from helper ones
def _appconfig_get_backup_hosts_wrapper(self):
    """ wrapper for get_backup_hosts_helper
    """
    returned_list = self.get_backup_hosts_helper()
    result = []
    for item in returned_list:
        result.append(self.db.get_dal(item.className, item.id))
    return result

# add configuration object required to create DAL objects afterwards
def _appconfig_get_initialization_depends_from_helper_wrapper(self, apps):
    """ wrapper for get_initialization_depends_from_helper
    """
    result = self.get_initialization_depends_from_helper(apps)
    for item in result:
        item.db = self.db
    return result

# add configuration object required to create DAL objects afterwards
def _appconfig_get_shutdown_depends_from_wrapper(self, apps):
    """ wrapper for get_shutdown_depends_from_helper
    """
    result = self.get_shutdown_depends_from_helper(apps)
    for item in result:
        item.db = self.db
    return result

# create tag DAL objects from its id
def _appconfig_get_info_wrapper(self):    
    """ wrapper for get_info_helper
    """
    result = self.get_info_helper()      
    result["tag"] = self.db.get_dal("Tag", result["tag"])
    return result

def _segconfig_get_all_applications_wrapper(self):
    """ wrapper for get_all_applications_helper
    """
    result = self.get_all_applications_helper()
    for item in result:
        item.db = self.db
    return result

def _segconfig_get_controller_wrapper(self):
    """ wrapper for get_controller_helper
    """
    result = self.get_controller_helper()
    result.db = self.db
    return result

def _segconfig_get_infrastructure_wrapper(self):
    """ wrapper for get_infrastructure_helper
    """
    result = self.get_infrastructure_helper()
    for item in result:
        item.db = self.db
    return result

def _segconfig_get_applications_wrapper(self):
    """ wrapper for get_applications_helper
    """
    result = self.get_applications_helper()
    for item in result:
        item.db = self.db
    return result

def _segconfig_get_nested_segments_wrapper(self):
    """ wrapper for get_nested_segments_helper
    """
    result = self.get_nested_segments_helper()
    for item in result:
        item.db = self.db
    return result

def _segconfig_get_hosts_wrapper(self):
    """ wrapper for get_hosts_helper
    """
    returned_list = self.get_hosts_helper()
    result = []
    for item in returned_list:
        result.append(self.db.get_dal(item.className, item.id))
    return result

def _segconfig_get_base_seg_wrapper(self):
    """ wrapper for get_base_seg_helper
    """
    result = self.get_base_seg_helper()
    return self.db.get_dal(result.className, result.id)


dal.Component.disabled = _component_disabled_wrapper
dal.Component.get_parents = _component_get_parents_wrapper

dal.ComputerProgram.get_info = _computer_program_get_info_wrapper

dal.Partition.get_all_applications = _partition_get_all_applications_wrapper
dal.Partition.get_segment = _partition_get_segment_wrapper
dal.Partition.get_log_directory = _partition_get_log_directory_wrapper

dal.Variable.get_value = _variable_get_value_wrapper

libdal_algo_helper.AppConfig.get_base_app = _appconfig_get_base_app_wrapper
libdal_algo_helper.AppConfig.get_host = _appconfig_get_host_wrapper
libdal_algo_helper.AppConfig.get_base_seg = _appconfig_get_base_seg_wrapper
libdal_algo_helper.AppConfig.get_backup_hosts = _appconfig_get_backup_hosts_wrapper
libdal_algo_helper.AppConfig.get_initialization_depends_from = _appconfig_get_initialization_depends_from_helper_wrapper
libdal_algo_helper.AppConfig.get_shutdown_depends_from = _appconfig_get_shutdown_depends_from_wrapper
libdal_algo_helper.AppConfig.get_info = _appconfig_get_info_wrapper

libdal_algo_helper.SegConfig.get_all_applications = _segconfig_get_all_applications_wrapper
libdal_algo_helper.SegConfig.get_controller = _segconfig_get_controller_wrapper
libdal_algo_helper.SegConfig.get_infrastructure = _segconfig_get_infrastructure_wrapper
libdal_algo_helper.SegConfig.get_applications = _segconfig_get_applications_wrapper
libdal_algo_helper.SegConfig.get_nested_segments = _segconfig_get_nested_segments_wrapper
libdal_algo_helper.SegConfig.get_hosts = _segconfig_get_hosts_wrapper
libdal_algo_helper.SegConfig.get_base_seg = _segconfig_get_base_seg_wrapper


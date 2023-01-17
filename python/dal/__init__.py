import config
import os
from ._daq_dal_py import * 


scriptsdir=os.path.dirname(os.path.realpath(__file__))

# MUST IMPROVE PATH FINDING
assert os.path.exists(f'{scriptsdir}/../../../bin/core.schema.xml')
dal_classes = config.dal.module('dal_classes', f'{scriptsdir}/../../../bin/core.schema.xml')

Partition = dal_classes.Partition
Component = dal_classes.Component

def setify(arg):
    if arg is None:
        return set()
    elif type(arg) is str:
        return set( {arg} )
    else:
        return set(arg)

def _partition_get_all_applications_wrapper(self, db, app_types, use_segments, use_hosts):
    return partition_get_all_applications(db._obj, self.id, setify(app_types), setify(use_segments), setify(use_hosts))

def _partition_get_log_directory_wrapper(self, db):
    return partition_get_log_directory(db._obj, self.id)

def _partition_get_segment_wrapper(self, db, segname):
    return partition_get_segment(db._obj, self.id, segname)

def _component_get_parents_wrapper(self, db, partition):

    parents = []
    try:
        parents = component_get_parents(db._obj, self.id, partition.id)
    except RuntimeError:
        print('Swallowing the "A null pointer was returned" exception')
    parent_list = []
    for p in parents:
        component_list = []
        for c in p:
            component_list.append(db.get_dal(c.class_name, c.id))
        parent_list.append(component_list)
    return parent_list

Partition.get_all_applications = _partition_get_all_applications_wrapper
Partition.get_log_directory = _partition_get_log_directory_wrapper
Partition.get_segment = _partition_get_segment_wrapper

Component.get_parents = _component_get_parents_wrapper

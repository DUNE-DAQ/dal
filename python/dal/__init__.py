#from __future__ import absolute_import
import config
import os
from ._daq_dal_py import partition_get_all_applications


scriptsdir=os.path.dirname(os.path.realpath(__file__))

# MUST IMPROVE PATH FINDING
assert os.path.exists(f'{scriptsdir}/../../../bin/core.schema.xml')
dal_classes = config.dal.module('dal_classes', f'{scriptsdir}/../../../bin/core.schema.xml')

Partition = dal_classes.Partition

def setify(arg):
    if arg is None:
        return set()
    elif type(arg) is str:
        return set( {arg} )
    else:
        return set(arg)

def partition_get_all_applications_wrapper(self, db, app_types, use_segments, use_hosts):
    return partition_get_all_applications(db._obj, self.id, setify(app_types), setify(use_segments), setify(use_hosts))

Partition.get_all_applications = partition_get_all_applications_wrapper

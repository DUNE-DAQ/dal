#from __future__ import absolute_import
import config
import os
from ._daq_dal_py import partition_get_all_applications


scriptsdir=os.path.dirname(os.path.realpath(__file__))

# MUST IMPROVE PATH FINDING
assert os.path.exists(f'{scriptsdir}/../../../bin/core.schema.xml')
dal_classes = config.dal.module('dal_classes', f'{scriptsdir}/../../../bin/core.schema.xml')

Partition = dal_classes.Partition

def partition_get_all_applications_tweak(self, db, partition_name):
    return partition_get_all_applications(db._obj, partition_name)

Partition.get_all_applications = partition_get_all_applications_tweak

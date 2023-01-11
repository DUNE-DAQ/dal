#!/bin/env python

import config
import dal
import os
#from config._daq_config_py import _Configuration

# MUST IMPROVE PATH FINDING!
scriptsdir=os.path.dirname(os.path.realpath(__file__))
db = config.Configuration(f"oksconfig:{scriptsdir}/dal_testing.data.xml")

# #apps = dal.partition_get_all_applications(db, "JohnsPartition")

partition = db.get_dal("Partition", "JohnsPartition")

apps = partition.get_all_applications(db, "JohnsPartition")
# print(len(apps))


print("")
for app in apps:
    print(app.get_app_id())


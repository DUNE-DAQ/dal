#!/bin/env python

import config
import dal
import os

# MUST IMPROVE PATH FINDING!
scriptsdir=os.path.dirname(os.path.realpath(__file__))

print(f"\nReading in {scriptsdir}/dal_testing.data.xml:\n")
db = config.Configuration(f"oksconfig:{scriptsdir}/dal_testing.data.xml")

partition = db.get_dal("Partition", "JohnsPartition")

apps = partition.get_all_applications(db, "JohnsPartition")


for app in apps:
    print(app.get_app_id())

print("")


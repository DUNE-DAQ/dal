#!/bin/env python3

import config
import os

schemafile=f'{os.environ["DAL_SHARE"]}/schema/dal/tutorial.schema.xml'
datafile="tutorial.data.xml"

# binds a new dal into the module named "tutorial"               
tutorial = config.dal.module('tutorial', schemafile)

db = config.Configuration("oksconfig")
db.create_db(datafile, [schemafile])

readout_app = tutorial.ReadoutApplication("DummyReadout", 
                                          Name="/full/pathname/of/readout/executable",
                                          SubDetector="PMT")

runcontrol_app = tutorial.RunControlApplication("DummyRC",
                                                Name="/full/pathname/of/RC/executable",
                                                ApplicationsControlled=[readout_app])

db.update_dal(readout_app)
db.update_dal(runcontrol_app)
db.commit()

print(f"""
Please take a look at {datafile} for the data file this script 
created using OKS classes from {schemafile}
""")


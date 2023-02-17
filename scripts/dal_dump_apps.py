#!/usr/bin/env python
import argparse
import sys
import oksdbinterfaces
import dal    
import os
from argparse import RawTextHelpFormatter

def print_info(file_names,environment):
    print(" - possible program file names:")
    if file_names is None :
       print("no")
    else:
        for file in file_names:
           print("    * " + file)

    print(" - environment variables:")
    if environment is None :
       print("no")
    else:
        for key in sorted(environment):
            
           print("    * " + key + "=\"" + environment[key] + "\"")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="""Example of dunedaq::dal::Application::get_info() algorithm usage. The database name is defined either\n
       the -d command line parameter, or by the TDAQ_DB environment variable in format \"impl:parameter\",\n
       e.g. \"oksconfig:/tmp/my-db.xml\". By default the algorithms are applied to all applications used by\n
       the partition are their results are printed out.\n\n
       usage: dal_dump_apps.py [-d | --data database-name]\n
                         [-p | --partition-id partition-id]\n
                         [-a | --application-id app-id]\n
                         [-n | --application-name app-name]\n
                         [-g | --application-segment-id seg-id]\n
        \n""", formatter_class=RawTextHelpFormatter)
    parser.add_argument("-d", "--data", help = "name of the database (ignore TDAQ_DB variable)" )
    parser.add_argument("-p", "--partition_id", help = "name of the partition object")
    parser.add_argument("-a", "--application_id", help = "identity of the application object (if not provided, dump all applications")
    parser.add_argument("-n", "--application_name", help = "name of the application object (if not provided, dump all applications)")
    parser.add_argument("-g", "--application_segment_id", help = "identity of the application's segment object (if defined,print apps of this segment")  
 
    args = parser.parse_args()

    for required_env_variable in ["TDAQ_IPC_INIT_REF", "TDAQ_DB_PATH", "TDAQ_DB_DATA"]:
        if required_env_variable not in os.environ:
            os.environ[required_env_variable] = "DUMMY_TDAQ_ENV_VALUE"

    # Open database
    db = oksdbinterfaces.Configuration(args.data)
    #Get the application object
    app = 0
    if args.application_id : 
        app = db.get_dal('BaseApplication', args.application_id)
    if args.application_name :
        app = db.get_dal('BaseApplication', args.application_name.split(':')[0])  
        args.application_id = args.application_name
    # get Partition object
    partition = db.get_dal('Partition', args.partition_id)
    app_config_list = partition.get_all_applications(db, None, args.application_segment_id, None)  

    count = 0

    for i in app_config_list :
        if app and type(app) != int and (i.get_base_app().id != app.id): # recall app is initialized to the integer 0
            continue           
        if  i.is_templated() :
            if ((args.application_id is not None) ) and ( not (   args.application_name == i.app_id) ): #watch out for these 
                continue
        else :
            if (args.application_id is not None)  and ( not (   app.id == i.get_app_id()) ):
                continue

        count += 1
        if (':' in str(i.get_app_id()) ) or ('Template' in str(i.get_base_app().class_name) ) : # if it is type template app
           print("### (" + str(count) + ") template application " + i.get_app_id() + " ###")
        else: # if templated
           print("### (" + str(count) + ") application \'" + i.get_app_id() + '@' +i.get_base_app().class_name + "\' ###")

        info = i.get_info() 

        print(" - command line start args:\n    " +  info['startArgs'])
        print(" - command line restart args:\n    " +  info['restartArgs'])
        print_info(info['programNames'], info['environment'])

    if count == 0 :
        if app is not None and type(app) != int:  # recall app is initialized to the integer 0
           print("the application " + app.id + " is not running in the partition; it is disabled or not included into partition")
        elif args.application_name is not None:
           print("the application with name \'" + args.application_name + "\' is not running in the partition; it is disabled or not included into partition")
        elif args.application_segment_id is not None :
           print("the applications of segment " + args.application_segment_id + " are not running in the partition; the segment or it\'s applications are disabled or the segment is not included into partition")


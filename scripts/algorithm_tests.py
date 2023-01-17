#!/bin/env python

from __future__ import print_function
import dal
import sys
import config
import os
import subprocess
import filecmp

test_output_verbose = True


def check (result, test_name) :
    if result == True:
        print("")
        print(test_name + " is successful ! ")
    else :
        print("")
        print(test_name + " failed ! ")
        raise RuntimeError("")  

def print_app(app):
    return app.get_app_id() + "@" + app.get_base_app().class_name + " on " + app.get_host().id + "@" + app.get_host().class_name

def print_segment(seg):
    res = "segment: " + seg.get_seg_id() + '\n'

    res += "controller: " + print_app(seg.get_controller()) + '\n'

    infr= seg.get_infrastructure()
    if ( (infr is not None) and (not (len(infr) == 0)) ):
        res += "infrastructure:\n"
        for i in range (0,len(infr)) :
            res += print_app(infr[i]) + '\n'
    else:
        res += "no infrastructure\n"

    apps= seg.get_applications()
    if ( (apps is not None) and (not (len(apps) == 0)) ):
        res += "applications:\n"
        for i in range (0,len(apps)) :
            res += print_app(apps[i]) + '\n'
    else:
        res += "no applications\n"

    hosts= seg.get_hosts()
    if ( (hosts is not None) and (not (len(hosts) == 0)) ):
        res += "hosts:\n"
        for i in range (0,len(hosts)) :
            res += hosts[i].id + "@" + hosts[i].class_name + '\n'
    else:
        res += "no hosts\n"

    nested_segs=seg.get_nested_segments()
    if ( (nested_segs is not None) and (not (len(nested_segs) == 0)) ):
        res += "nested segments:\n"
        for i in range (0,len(nested_segs)) :
            res += print_segment(nested_segs[i])
    else:
        res += "no nested segments\n"

    return res

def get_parents_test_case () :
    
    res1 = ""
    components =  db.get_dals('Component')

    for i in range (0,len(components)) :
        res1 += str(components[i].get_parents(db, partition)) 
        res1 +='\n'
    
    res2 = ""
    for i in range (0,len(components)) : 

        res2+='['
        res2+=str(dal.get_parents_test(db._obj, partition.id, components[i].id)) 
        res2+= ']'  

    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")

    if test_output_verbose:
        print("")
        print("===================================RES1===================================")
        print(res1)
        print("===================================RES2===================================")
        print(res2)
        print("")

    return (res1 == res2) 


def get_log_directory_test_case () :

    res1 = ""
    parts =  db.get_dals('Partition')
    print("Number of partitions to be tested : " + str(len(parts)))
    for i in range (0,len(parts)) : 
        res1 += str(parts[i].get_log_directory(db)) 
        res1 +='\n'
    
    res2 = ""
    for i in range (0,len(parts)) : 
        res2+=str(dal.get_log_directory_test(db._obj, parts[i].id)) 

    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")

    if test_output_verbose:
        print("")
        print("===================================RES1===================================")
        print(res1)
        print("===================================RES2===================================")
        print(res2)
        print("")

    return (res1 == res2) 


def get_segment_test_case () :
    segs =  partition.Segments   
    print("Number of Segments to be tested : " + str(len(segs)+1))

    root_seg = partition.get_segment(db, partition.OnlineInfrastructure.id)

    res1 = print_segment(root_seg)
    for i in range (0,len(segs)) :
        res1 += print_segment(partition.get_segment(db, segs[i].id))
        res1 +='\n'

    res2 = dal.get_segment_test(db._obj, partition.id, root_seg.get_seg_id())
    for i in range (0,len(segs)) :
        res2+=str(dal.get_segment_test(db._obj, partition.id, segs[i].id))

    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")

    if test_output_verbose:
        print("")
        print("===================================RES1===================================")
        print(res1)
        print("===================================RES2===================================")
        print(res2)
        print("")

    return (res1 == res2) 

if __name__ == '__main__':
    global db
    global partition

    scriptsdir=os.path.dirname(os.path.realpath(__file__))

    db_file=f"{scriptsdir}/dal_testing.data.xml"
    assert os.path.exists(db_file)
    db = config.Configuration(f"oksconfig:{db_file}")

    partition=db.get_dal("Partition", "JohnsPartition")

    check( get_log_directory_test_case(), "get_log_directory_test_case")
    check( get_segment_test_case(), "get_segment_test_case")
    check( get_parents_test_case(), "get_parents_test_case")

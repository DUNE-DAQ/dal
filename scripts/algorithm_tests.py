#!/bin/env python

from __future__ import print_function
import dal
import sys
import oksdbinterfaces
import os
import subprocess
import filecmp

test_output_verbose = True

def print_output(res1, res2):
    if test_output_verbose:
        print("")
        print("=====================Output When Python Bindings to C++ Used======================")
        print("")
        print(res1)
        print("")
        print("===========================Output When Original C++ Used==========================")
        print("")
        print(res2)
        print("")
        print("==================================================================================")

def check (result, test_name) :
    if result == True:
        print("")
        print(test_name + ": PASS")
    else :
        print("")
        print(test_name + ": FAIL")
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

def print_segment_timeout(seg):
    res = "segment " + seg.get_seg_id() + " actionTimeout: " + str(seg.get_timeouts()['actionTimeout']) + ", shortActionTimeout" + str(seg.get_timeouts()['shortActionTimeout'] ) + '\n'

    nested_segs=seg.get_nested_segments()
    if ( (nested_segs is not None) and (not (len(nested_segs) == 0)) ):
        for i in range (0,len(nested_segs)) :
            res += print_segment_timeout(nested_segs[i])

    return res

def get_timeouts_test_case () :
    root_seg = partition.get_segment(db, partition.OnlineInfrastructure.id)

    res1 = print_segment_timeout(root_seg)
    res2 = dal.get_timeouts_test(db._obj, partition.id, root_seg.get_seg_id())

    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")

    print_output(res1, res2)

    return (res1 == res2) 


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

    print_output(res1, res2)

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

    print_output(res1, res2)

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

    print_output(res1, res2)

    return (res1 == res2) 

def get_disabled_test_case():

    res1 = ""
    components = db.get_dals('Component')    
    print("Number of components to be tested : " + str(len(components)))
    for i in range (0,len(components)) : 
        res1 += str(components[i].disabled(db, partition.id)) 
        res1 +='\n'

    res2 = ""
    for i in range (0,len(components)) : 
        res2+=str(dal.disabled_test(db._obj, partition.id, components[i].id)) 
    
    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")

    print_output(res1, res2)
    return (res1 == res2) 

def get_value_test_case():
 
    res1 = ""
    variables =  db.get_dals('Variable')    
    print("Number of Variables to be tested : " + str(len(variables)))
     
    for i in range (0,len(variables)) :
        if len(variables[i].TagValues) != 0:
           tag = variables[i].TagValues[0] 
           res1 += str(variables[i].get_value(db, tag)) 
           res1 +='\n'
 
    res2 = ""
    for i in range (0,len(variables)) : 
        if len(variables[i].TagValues) != 0:
           tag = variables[i].TagValues[0]
           
           res2+=str(dal.get_value_test(db._obj, variables[i].id, tag.id) )

    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")

    print_output(res1, res2)

    return (res1 == res2) 



if __name__ == '__main__':
    global db
    global partition

    if "TDAQ_DB_PATH" not in os.environ:
        os.environ["TDAQ_DB_PATH"] = os.environ["DUNEDAQ_SHARE_PATH"]

    scriptsdir=os.path.dirname(os.path.realpath(__file__))

    db_file=f"{scriptsdir}/dal_testing.data.xml"
    assert os.path.exists(db_file)
    db = oksdbinterfaces.Configuration(f"oksconfig:{db_file}")

    partition=db.get_dal("Partition", "ToyPartition")

    print(f"\n\nRUNNING TEST \"get_log_directory_test_case\"")
    check( get_log_directory_test_case(), "get_log_directory_test_case")

    print(f"\n\nRUNNING TEST \"get_segment_test_case\"")
    check( get_segment_test_case(), "get_segment_test_case")

    print(f"\n\nRUNNING TEST \"get_value_test_case\"")
    check( get_value_test_case(), "get_value_test_case")

    print(f"\n\nRUNNING TEST \"get_parents_test_case\"")
    check( get_parents_test_case(), "get_parents_test_case")

    print(f"\n\nRUNNING TEST \"get_disabled_test_case\"")
    check( get_disabled_test_case(), "get_disabled_test_case")

    print(f"\n\nRUNNING TEST \"get_timeouts_test_case\"")
    check( get_timeouts_test_case(), "get_timeouts_test_case")

    print("""
n.b. newlines have been removed from the test output above; the important 
thing is that the output agrees between calls to the C++ function and their
Python bindings
""")

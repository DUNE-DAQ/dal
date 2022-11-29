from __future__ import print_function
from builtins import str
from builtins import range
import libdal_algo_tester
import sys
import pm
from pm.dal import dal, DFdal
import pm.project
import config
import dal_algo
import os
import subprocess
import filecmp

def check (result, test_name) :
    if result == True:
        print(test_name + " is successful ! ")
    else :
        print(test_name + " failed ! ")
        raise RuntimeError("")  


def get_parents_test_case () :

    res1 = ""
    components =  db.get_dals('Component')
    for i in range (0,len(components)) : 
        res1 += str(components[i].get_parents(db, partition)) 
        res1 +='\n'
    
    res2 = ""
    for i in range (0,len(components)) : 

        res2+='['
        res2+=str(libdal_algo_tester.get_parents_test(components[i].id)) 
        res2+= ']'  

    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")

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
        res2+=str(libdal_algo_tester.get_log_directory_test(parts[i].id)) 

    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")

    return (res1 == res2) 


def disabled_test_case () :

    res1 = ""
    components =  db.get_dals('Component')    
    print("Number of components to be tested : " + str(len(components)))
    for i in range (0,len(components)) : 
        res1 += str(components[i].disabled(db, partition)) 
        res1 +='\n'

    res2 = ""
    for i in range (0,len(components)) : 
        res2+=str(libdal_algo_tester.disabled_test(components[i].id)) 
    
    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")
    return (res1 == res2) 

def get_tag(comp):
 for item in comp.ProcessEnvironment:
   if "VariableSet" in str(type(item)):
       for var in item.Contains :
            if len(var.TagValues)!= 0 :
               tag = var.TagValues[0]
               return tag
   else:
       if len(item.TagValues)!= 0 :
            tag = item.TagValues[0]
            return tag


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
    res2 = libdal_algo_tester.get_timeouts_test(root_seg.get_seg_id())

    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")

    return (res1 == res2) 


def get_value_test_case () :
 
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
           res2+=str(libdal_algo_tester.get_value_test(variables[i].id, tag.id) )
 
    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")
    return (res1 == res2) 

def print_app(app):
    return app.get_app_id() + "@" + app.get_base_app().className() + " on " + app.get_host().id + "@" + app.get_host().className()

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
            res += hosts[i].id + "@" + hosts[i].className() + '\n'
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

def get_segment_test_case () :
    segs =  partition.Segments   
    print("Number of Segments to be tested : " + str(len(segs)+1))

    root_seg = partition.get_segment(db, partition.OnlineInfrastructure.id)
    
    res1 = print_segment(root_seg)
    for i in range (0,len(segs)) :
        res1 += print_segment(partition.get_segment(db, segs[i].id))
        res1 +='\n'

    res2 = libdal_algo_tester.get_segment_test(root_seg.get_seg_id())
    for i in range (0,len(segs)) : 
        res2+=str(libdal_algo_tester.get_segment_test(segs[i].id)) 

    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","")
    return (res1 == res2) 


def get_info_test_case () :

    res1 = []
    comp =  db.get_dals('Script')    
    print("Number of Scripts to be tested : " + str(len(comp)))
    tag = 0
    computer = db.get_dals('Computer')[0]

    for i in range (0,len(comp)) : 
        tag = 0
        tag = get_tag(comp[i])
        computer = 0
        #getcomputer
        if not ( tag == 0 or computer == 0 ) :
            dicti = comp[i].get_info(db, partition, tag, computer)
            res1 += str(dicti)
            res1 +='\n'

    res1.sort()
    res1 = str(res1)
    res2 = []
    for i in range (0,len(comp)) : 
        tag = 0
        tag = get_tag(comp[i])
        computer = 0
        #getcomputer
        if not ( tag == 0 or computer == 0 ) :
           res2+=str(libdal_algo_tester.get_info_test(comp[i].id, tag.id,computer.id) )
    
    res2.sort()
    res2 = str(res2)
    
    res1 = res1.replace(",","").replace("[","").replace("]","").replace(" ","").replace("\n","").replace("'","").replace("paths_to_shared_libraries","").replace("program_names","").replace("search_paths","").replace("{","").replace("}","").replace(":","").replace("\\n","")
    res2 = res2.replace(",","").replace("[","").replace("]","").replace(" ","").replace("'","").replace("\n","").replace("{","").replace("}","")

    return (res1 == res2) 


if __name__ == '__main__':
    global partition_name
    global database
    stdout = sys.stdout
    
    cwd=os.getcwd()

    partition_name='dal_test'
    db_file=cwd+'/'+partition_name+'.data.xml'

    if not os.path.isfile(db_file):
        subprocess.call(["pm_part_hlt.py", "-p", partition_name])

    database = 'oksconfig:'+db_file
    os.environ["TDAQ_DB_DATA"] = database
    os.environ["TDAQ_DB_VERSION"] = ''

    # Test dal_dump_app_config.py utility

    python_out_app_config=cwd+'/'+'python_dump_app_config.out'
    cpp_out_app_config=cwd+'/'+'cpp_dump_app_config.out'

    with open(python_out_app_config, "w") as f1:
        cmd1 = ["dal_dump_app_config.py", "-d", database ,  "-p" , partition_name]
        subprocess.call(cmd1, stdout=f1)
 
    with open(cpp_out_app_config, "w") as f2:
        cmd2 = ["dal_dump_app_config", "-d", database, "-p", partition_name]
        subprocess.call(cmd2, stdout=f2)
 
    check((os.stat(cpp_out_app_config).st_size != 0) and filecmp.cmp(python_out_app_config, cpp_out_app_config), "dump_app_config python utility test")


    # Test dal_dump_apps.py utility

    python_dump_apps_out=cwd+'/'+'python_dump_apps.out'
    cpp_dump_apps_out=cwd+'/'+'cpp_dump_apps.out'

    with open(python_dump_apps_out, "w") as f1:
        cmd1 = ["dal_dump_apps.py", "-d", database ,  "-p" ,  partition_name]
        subprocess.call(cmd1, stdout=f1)

    with open(cpp_dump_apps_out, "w") as f2:
        cmd2 = ["dal_dump_apps", "-d", database, "-p", partition_name]
        subprocess.call(cmd2, stdout=f2)

    check((os.stat(cpp_dump_apps_out).st_size != 0) and filecmp.cmp(cpp_dump_apps_out, python_dump_apps_out), "dump_apps python utility test")


    # Test other algorithms

    db = pm.project.Project(database)
    partition = db.get_dal('Partition', partition_name)

    check( get_parents_test_case(), "get_parents_test_case")
    check( get_log_directory_test_case(), "get_log_directory_test_case")
    check( disabled_test_case(), "disabled_test_case")
    check( get_timeouts_test_case(), "get_timeouts_test_case") 
    check( get_segment_test_case(), "get_segment_test_case")
    check( get_value_test_case(), "get_value_test_case")
    check( get_info_test_case(), "get_info_test_case")

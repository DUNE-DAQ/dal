# An Introduction to OKS

## Overview

OKS (Object Kernel Support) is a suite of packages [originally
written](https://gitlab.cern.ch/atlas-tdaq-software/oks) for the ATLAS
data acquisition effort. Its features include:
* The ability to define object types in XML (known as OKS "classes"), off of which C++ and Python code can automatically be generated
* Support for class Attributes, Relationships, and Methods. Attributes and Relationships are automatically generated; Methods allow developers to add behavior to classes
* The ability to create instances of classes, modify them, read them into the XML file acting as a database and retrieve them from the database

As of this writing (Jan-18-2023) the packages have been refactored
only to the extent that they work within the DUNE DAQ framework
although further changes within the context of DUNE's needs could be
useful. Also, unfortunately most documentation for OKS is
access-protected and only available to ATLAS collaborators. However,
this document provides a taste of what OKS has to offer.

## Getting Started

To get started working with the DUNE-repurposed OKS packages, you'll want to [set up a work area](https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-buildtools/). 
Then you'll want to get your hands on the following repos:
* daq-cmake
* okssystem
* config
* oks
* genconfig
* oks_utils
* dal
* oksconfig

Note that for daq-cmake, you'll want to switch over to the `johnfreeman/oks_support` branch; this is because of the addition of the `daq_generate_dal` 
function which generates code from the XML files. For the other repos, you want the `develop` branches which are the default upon cloning. As dependencies
exist between the OKS packages you'll also want to edit the `sourcecode/dbt-build-order.cmake` file, adding the following at the end of the `build_order` list:
```
                "okssystem"
                "config"
                "oks"
                "genconfig"
                "oks_utils"
                "dal"
                "oksconfig"
```
Once you've made these additions to your work area, you can proceed to build everything using the usual daq-buildtools commands. 

With the packages built, it's time to run some tests to make sure things are in working order. These include:
* `test_configuration.py`: A test script in the config package. Tests that you can create objects, save them to a database, read them back, and remove them from a database.
* `test_dal.py`: Also from the config package. Test that you can change the values of objects, and get expected errors if you assign out-of-range values. 
* `algorithm_tests.py`: A script from the dal package. Test that Python bindings to class Methods implemented in C++ work as expected. 

If anything goes wrong during the tests, it will be self-evident. Make a note of what happened and contact John Freeman, `jcfree@fnal.gov`. 

## A Look at OKS Databases: XML-Represented classes and objects

While ATLAS has various database implementations (Oracle-based, etc.), for the DUNE DAQ we only need their basic database format, which is an XML file. There are generally two types of database file: the kind that defines classes, which by convention have the `*.class.xml` extension, and the kind that define instances of those classes (i.e. objects) and have `*.data.xml` extensions. The class files are known from ATLAS as "schema files" and the object files are known from ATLAS as "data files". A good way to get a feel for these files is to run the following command:
```
oks_tutorial simple.schema.xml simple.data.xml
```
Just looking at the screen output of this application, you can learn a lot, in particular that the files appear to describe a company made up of people and departments represented as objects. To look at how this is represented, if you open `simple.schema.xml` and scroll past a lot preliminary material you don't need to worry about, you'll see a class called "Department", which consists of an Attribute (the name of the Department) and a Relationship (its staff). The Relationship here refers to the department's Employees, which is a count of Employee instances that can range from "zero" ("one" would have made more sense) to "many" (i.e., as many as you want). Scrolling down a little further, you see the Employee class has a superclass called Person, meaning an Employee contains the Attributes, etc. a Person would have but additionally with an Attribute called Salary and a Relationship called Works At. If we look at `simple.data.xml` you see it describes a department called EP with an employee named Maria and a department called IT which contains Alexander and Michel. Then there are three unaffiliated people, Mick, Peter, and a baby. Given that the baby's Birthday Attribute is set to `20000525`, it's safe to say this data file was designed quite a while ago. 

OKS also provides tools which parse the XML and provide summaries of the contents of the databases. `config_dump`, part of the config package, is quite useful in this regard. Pass it `-h` to get a description of its abilities; if you just run `config_dump -d oksconfig:simple.data.xml` you'll get a summary of the classes used to defined the objects in the file. Running `config_dump -d oksconfig:simple.data.xml -o` will summarize the objects themselves. Play around with it. 

Note that as a database, it's possible to read in, modify, and save objects. E.g., you can do this once you've created `simple.schema.xml` and `simple.data.xml` and have entered an interactive Python environment:
```
import config
db = config.Configuration('oksconfig:simple.data.xml')
mick = db.get_dal('Person', 'mick')
```
and you can see that Mick's family situation is that he's unmarried:
```
print(mick.Family_Situation)   # Will print "Single"
```
Let's get Mick a spouse:
```
mick.Family_Situation = "Married"
db.update_dal(mick)
db.commit()
```
...and now, if you open up simple.data.xml and look up Mick, you'll see that he is, in fact, now married. Note that since `Family_Situation` is an enumeration, you can't give it an illegal value. See what happens if you do this:
```
mick.Family_Situation = "This is not an option"
```
Spoiler alert: you'll get an error. 

## A Realistic Example

The `simple.schema.xml` file and `simple.data.xml` files created by `oks_tutorial` are fairly easy to understand, and meant to be for educational purposes. To see the actual classes which are used on ATLAS, we need to go to the `dal` repo: `sourcecode/dal/schema/dal/core.schema.xml`. This file is quite large, and describes concepts which are actually used on DAQ systems like `ComputerProgram` and `Rack` and `Crate`. If you look in dal's `CMakeLists.txt` file you see the following:
```
daq_generate_dal(core.schema.xml
  NAMESPACE dunedaq::dal
  INCLUDE ${CMAKE_CODEGEN_BINARY_DIR}/include/${PROJECT_NAME}
  CPP ${CMAKE_CODEGEN_BINARY_DIR}/src
  CPP_OUTPUT  dal_cpp_srcs
  DUMP_OUTPUT cpp_dump_src)
  
  daq_add_library(algorithms.cpp disabled-components.cpp test_circular_dependency.cpp ${dal_cpp_srcs} DAL LINK_LIBRARIES config::config okssystem::okssystem logging::logging)
```
...where the things most important to notice at this time are that `core.schema.xml` gets fed into `daq_generate_dal` which proceeds to generate code off of the classes defined in `core.schema.xml`, storing the generated files in the `dal_cpp_srcs` variable and using those files as part of the build of the package's main library. You'll notice also that the classes in `core.schema.xml` contain not only Attributes and Relationships as in the tutorial example above, but also Methods. If you look at the `Partition` class (l. 415) and scroll down a bit, you'll see a `get_all_applications` Method declared, along with its accompanying C++ declaration (as well as Java declaration, but we ignore this). The implementation of `get_all_applications` needs to be done manually, however, and is accomplished on l. 1301 of `src/algorithms.cpp`. If you scroll to the top of that file you'll see a `#include "dal/Partition.hpp"` line. In the actual dal repo, there's no such include file. However, assuming you followed the build instructions at the top of this document, you'll find it in the `build/` area of your work area, as the header was in fact generated. 

To see the `get_all_applications` function in action, you can do the
following. Note the dummy environment variable settings are needed for
historical reasons and will be removed in the future:
```
export TDAQ_DB_DATA=DUMMY_TDAQ_ENV_VALUE ; export TDAQ_DB_PATH=DUMMY_TDAQ_ENV_VALUE ; export TDAQ_IPC_INIT_REF=DUMMY_TDAQ_ENV_VALUE
dal_dump_app_config -d oksconfig:install/dal/bin/dal_testing.data.xml -p ToyPartition -s ToyOnlineSegment
```

Likewise, you can see a Python script which serves the same function,
but via calling Python bindings to C++ functions. We of course want
the output to be identical:
```
dal_dump_app_config.py -d oksconfig:install/dal/bin/dal_testing.data.xml -p ToyPartition -s ToyOnlineSegment
```
You can play around with `dal_dump_apps/dal_dump_apps.py`, pass the
`-h` argument to see your options. 

## Looking Ahead

There are two major tasks, very much related, which we now face:
* Determining which classes from ATLAS we want to keep, and which to
throw out. Also, what classes DUNE needs added. Input from
collaborators will be required. 
* Determining how to best integrate these tools into our pre-existing
packages.



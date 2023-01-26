# An Introduction to OKS

## Overview

OKS (Object Kernel Support) is a suite of packages [originally
written](https://gitlab.cern.ch/atlas-tdaq-software/oks) for the ATLAS
data acquisition effort. Its features include:
* The ability to define object types in XML (known as OKS "classes"), off of which C++ and Python code can automatically be generated
* Support for class Attributes, Relationships, and Methods. Attributes and Relationships are automatically generated; Methods allow developers to add behavior to classes
* The ability to create instances of classes (known as OKS "objects"), modify them, read them into an XML file serving as a database and retrieve them from the database

As of this writing (Jan-18-2023) the packages have been refactored
mainly to the extent that they work within the DUNE DAQ framework
although further changes within the context of DUNE's needs could be
useful. Also, unfortunately most documentation for OKS is
access-protected and only available to ATLAS collaborators. However,
this document provides a taste of what OKS has to offer.

## Getting Started

_JCF, Jan-25-2023: this documentation is on a special branch of the dal repo; you should skip the manual setup of the repo below, get onto mu2edaq, and simply run `/home/jcfree/bin/set_up_oks_area.sh <new workarea name>`. Then proceed below with the sentence which begins with "With the packages built,"_

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

## A Look at OKS Databases: XML-Represented Classes and Objects

While ATLAS has various database implementations (Oracle-based, etc.), for the DUNE DAQ we only need their basic database format, which is an XML file. There are generally two types of database file: the kind that defines classes, which by convention have the `*.class.xml` extension, and the kind that define instances of those classes (i.e. objects) and have `*.data.xml` extensions. The class files are known from ATLAS as "schema files" and the object files are known from ATLAS as "data files". A good way to get a feel for these files is to start with the tutorial schema, which from the base of your workarea is `sourcecode/dal/schema/dal/tutorial.schema.xml`:

### Overview of `tutorial.schema.xml`

Let's start with a description of what `tutorial.schema.xml` contains before we even look at its contents. It describes via three classes needed for a (very) simple DAQ: `ReadoutApplication` for detector readout, and `RCApplication` for the Run Control in charge of `ReadoutApplication` instances, and a third class, `Application`, of which they're both subclasses. Open the file, and *unless you're purely curious, scroll past the lengthy header of the file which you'll never need to understand* until you see the following:
```
 <class name="Application" description="A software executable" is-abstract="yes">
  <attribute name="Name" description="Name of the executable, including full path" type="string" init-value="Unknown" is-not-null="yes"/>
 </class>
```
The `is-abstract` qualifier means that you can't have an object which is concretely of type `Application`, you need to subclass it. However, any class which is a subclass of `Application` will automatically contain a `Name` attribute, which here is intended to be the fully-qualified path of the executable in a running DAQ system. 

Next, we see the class for readout:
```
<class name="ReadoutApplication" description="An executable which reads out subdetectors">
  <superclass name="Application"/>
  <attribute name="SubDetector" description="An enum to describe what type of subdetector it can read out" type="enum" range="PMT,WireChamber" init-value="WireChamber"/>
 </class>
```
...where you can see that there's an OKS enumerated type, where here there are only two options, basically a photon detector or a TPC. 

Then, there's the run control application:
```
<class name="RCApplication" description="An executable which allows users to control datataking">
  <superclass name="Application"/>
  <attribute name="Timeout" description="Seconds to wait before giving up on a transition" type="u16" range="1..3600" init-value="20" is-not-null="yes"/>
  <relationship name="ApplicationsControlled" description="Applications RC is in charge of" class-type="Application" low-cc="one" high-cc="many"/>
 </class>
```
And here, we have two items of interest: 
* A `Timeout` Attribute representing the max number of seconds before giving up on a transition. This is capped at one hour, and defaults to 20 seconds. 
* An `ApplicationsControlled` Relationship, which refers to anywhere from one object subclassed from `Application` to "many", which is OKS-speak for "basically unlimited". 

OKS also provides tools which parse the XML and provide summaries of the contents of the database (XML file). `config_dump`, part of the config package, is quite useful in this regard. Pass it `-h` to get a description of its abilities; if you just run `config_dump -d oksconfig:simple.data.xml` you'll get a summary of the classes used to defined the objects in the file. Running `config_dump -d oksconfig:simple.data.xml -C` will give you much more detail. For a schema as simple as the one we're showing here, this tool isn't super-useful, but it can be powerful when schemas get bigger and more complex. 

### Overview of `tutorial.data.xml`

In this section, we're going to _make_ a data file using the classes from `tutorial.schema.xml`. It's extremely simple, just run this script:
```
tutorial.py 
```
...and it will produce `tutorial.data.xml`. We'll look at it in a moment, but two things to note first:
1. As you can see if you open up `tutorial.py`, a Python module is actually _generated_ off of `tutorial.schema.xml`. If we add Attributes, Relations, etc. to the classes, the Python code will automatically pick them up without any additional Python needing to be written. 
1. `config_dump -d oksconfig:tutorial.data.xml --list-objects --print-referenced-by` provides a nice summary of `tutorial.data.xml`'s contents

We can also see what `tutotial.py` made by opening up `tutorial.data.xml`. Again, please scroll past the extensive header. What we see is two types of readout application, one ID'd as `PhotonReadout` and the other ID'd as `TPCReadout`; these names, of course, are chosen to reflect the choice of the `SubDetector` enum. Then we also see an instance of `RCApplication` where the `ApplicationsControlled` relationship establishes that run control is in charge of the two readout applications:
```
<obj class="RCApplication" id="DummyRC">
 <attr name="Name" type="string" val="/full/pathname/of/RC/executable"/>
 <attr name="Timeout" type="u16" val="20"/>
 <rel name="ApplicationsControlled">
  <ref class="ReadoutApplication" id="PhotonReadout"/>
  <ref class="ReadoutApplication" id="TPCReadout"/>
 </rel>
</obj>
```
The run control timeout is set to its default of 20 seconds. Say we want to change this, and save the result. For such a small data file it would be easy to manually edit, but if you think of a full-blown DAQ system you'll want to automate a lot of things. Fortunately we can alter the value via Python. Go into an interactive Python environment and do the following:
```
import config
db = config.Configuration('oksconfig:tutorial.data.xml')
rc = db.get_dal("RCApplication", "DummyRC")  # i.e., first argument is name of the class, the second is the name of the object
print(rc.Timeout)
```
where in the last command, you see the timeout of 20 seconds.

For fun, let's try to set the timeout to an illegal value (i.e., a timeout greater than an hour):
```
rc.Timeout = 7200 # 2 hrs before run control gives up!
```
...you'll get a `ValueError`. 

Let's set it to a less-ridiculous 60 seconds, and save the result:
```
rc.Timeout = 60
db.update_dal(rc)
db.commit()
```
If we exit out of Python and look at `tutorial.data.xml`, we see the timeout is now 60 rather than 20. And if we read the data file back in, this update will be reflected. 


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



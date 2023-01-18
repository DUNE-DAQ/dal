# An Introduction to OKS

## Overview

OKS (Object Kernel Support) is a suite of packages originally written for ATLAS data acquisition. Its features include:
* The ability to define object types in XML (known as OKS "classes"), off of which C++ and Python code can automatically be generated
* Support for class Attributes, Relations, and Methods. Attributes and Relations are automatically generated; Methods allow developers to add behavior to classes
* The ability to create instances of classes, modify them, read them into a database and retrieve them from a database

Unfortunately most documentation for OKS is access-protected and only available to ATLAS collaborators. However, this document provides a taste of what OKS has to offer. 

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
function which generates code from the XML files. For the other repos, you want the `develop` branches which they default to upon cloning. As dependencies
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
Once you've made these additions to your work area, you can proceed to build everthing using the usual daq-buildtools commands. 

With the packages built, it's time to run some tests to make sure things are in working order. These include:
* `test_configuration.py`: From the config package. Tests that you can create objects, save them to a database, read them back, and remove them from a database
* `test_dal.py`: Also from the config package. Test that you can change the values of objects, and get expected errors if you assign out-of-range values. 
* `algorithm_tests.py: From the dal package. Test that Python bindings to class Methods written in C++ work as expected. 

If anything goes wrong during the tests, it will be self-evident. Make a note of what happened and contact John Freeman, `jcfree@fnal.gov`. 






tdaq_package()

set(cpp_srcs)
set(java_srcs)
set(cpp_dump_src)

tdaq_generate_dal(data/schema/core.schema.xml 
  NAMESPACE daq::core
  INCLUDE dal
  CPP_OUTPUT  dal_cpp_srcs
  JAVA_OUTPUT dal_java_srcs
  DUMP_OUTPUT cpp_dump_src)

tdaq_add_library(daq-core-dal DAL
  ${dal_cpp_srcs} 
  src/algorithms.cpp
  src/disabled-components.cpp
  src/test_circular_dependency.cpp
  LINK_LIBRARIES rdb is ipc config osw)

tdaq_add_library(dal_algo_helper python/src/helper.cpp 
  LINK_LIBRARIES daq-core-dal pyconfig Boost::python PythonLibs tdaq-common::ers
)

tdaq_add_library(dal_algo_tester python/tests/src/algorithm_test_bindings.cpp
  LINK_LIBRARIES daq-core-dal pyconfig Boost::python PythonLibs tdaq-common::ers
)

tdaq_generate_isinfo(dal_ISINFO data/is/oks-version.schema.xml
  SIMPLE NAMED PACKAGE dal JAVA_OUTPUT dal_is_java_srcs)

set(CMAKE_JAVA_INCLUDE_PATH ${CMAKE_CURRENT_BINARY_DIR}/tmp.java)
tdaq_add_jar(jdal 
  jsrc/dal/*.java
  ${dal_java_srcs}  
  ${dal_is_java_srcs}
  DAL
  INCLUDE_JARS config/config.jar rdb/rdb.jar is/is.jar ipc/ipc.jar Jers/ers.jar TDAQExtJars/external.jar
  OUTPUT_NAME dal)
  
tdaq_add_executable(dal_dump ${cpp_dump_src}                                              LINK_LIBRARIES daq-core-dal config ipc)
tdaq_add_executable(dal_print_hosts src/dal_print_hosts.cpp                               LINK_LIBRARIES daq-core-dal config Boost::program_options)
tdaq_add_executable(dal_test_rw examples/dal_test_rw.cpp                                  LINK_LIBRARIES daq-core-dal config Boost::program_options)
tdaq_add_executable(dal_test_timeouts examples/dal_test_timeouts.cpp                      LINK_LIBRARIES daq-core-dal config Boost::program_options)
tdaq_add_executable(dal_dump_apps examples/dal_dump_apps.cpp                              LINK_LIBRARIES daq-core-dal config Boost::program_options)
tdaq_add_executable(dal_dump_apps_mt examples/dal_dump_apps_mt.cpp                        LINK_LIBRARIES daq-core-dal config Boost::program_options pthread)
tdaq_add_executable(dal_dump_app_config examples/dal_dump_app_config.cpp                  LINK_LIBRARIES daq-core-dal config ipc Boost::program_options)
tdaq_add_executable(dal_dump_app_depends examples/dal_dump_app_depends.cpp                LINK_LIBRARIES daq-core-dal config ipc Boost::program_options)
tdaq_add_executable(dal_print_segments examples/dal_print_segments.cpp                    LINK_LIBRARIES daq-core-dal config Boost::program_options)
tdaq_add_executable(dal_get_app_env src/dal_get_app_env.cpp                               LINK_LIBRARIES daq-core-dal config Boost::program_options)
tdaq_add_executable(dal_get_igui_setup src/dal_get_igui_setup.cpp                         LINK_LIBRARIES daq-core-dal config ipc Boost::program_options)
tdaq_add_executable(dal_get_config_version src/dal_get_config_version.cpp                 LINK_LIBRARIES daq-core-dal config ipc Boost::program_options)
tdaq_add_executable(dal_set_config_version src/dal_set_config_version.cpp                 LINK_LIBRARIES daq-core-dal config ipc Boost::program_options)
tdaq_add_executable(dal_notify examples/dal_notify.cpp                                    LINK_LIBRARIES daq-core-dal config ipc Boost::program_options)
tdaq_add_executable(dal_control_view examples/dal_control_view.cpp                        LINK_LIBRARIES daq-core-dal config ipc Boost::program_options)
tdaq_add_executable(dal_test_disabled examples/dal_test_disabled.cpp                      LINK_LIBRARIES daq-core-dal config ipc Boost::program_options)
tdaq_add_executable(dal_load_is_info_files src/dal_load_is_info_files.cpp                 LINK_LIBRARIES daq-core-dal config Boost::program_options)
tdaq_add_executable(dal_create_db_connection_files src/dal_create_db_connection_files.cpp LINK_LIBRARIES daq-core-dal config ipc Boost::program_options stdc++fs)
tdaq_add_executable(dal_test_get_config examples/dal_test_get_config.cpp                  LINK_LIBRARIES daq-core-dal config ipc Boost::program_options NOINSTALL)

tdaq_add_python_package(dal_algo SOURCE python/src)
tdaq_add_python_files(python/src/dal_algo.py)
tdaq_add_scripts(scripts/*)
tdaq_add_scripts(bin/dal_dump_app_config python/scripts/dal_dump_app_config.py)
tdaq_add_scripts(bin/dal_dump_apps python/scripts/dal_dump_apps.py)
tdaq_add_scripts( python/tests/algorithm_tests.py)

tdaq_add_python_files(python/scripts/dal_dump_app_config.py)
tdaq_add_python_files(python/scripts/dal_dump_apps.py)

tdaq_add_schema(data/schema/core.schema.xml)
tdaq_add_is_schema(data/is/oks-version.schema.xml)

tdaq_add_data(data/gui/*.xml DESTINATION gui)
tdaq_add_data(data/gui/pixmaps/*.xpm DESTINATION gui/pixmaps)
tdaq_add_data(data/gui/bitmaps/*.xbm DESTINATION gui/bitmaps)

tdaq_add_docs(data/schema/views/* DESTINATION views)

tdaq_add_jar(jtest_oks   jexamples/TestOks.java  INCLUDE_JARS config/config.jar Jers/ers.jar TDAQExtJars/external.jar jdal  OUTPUT_NAME test_oks   NOINSTALL)
tdaq_add_jar(jtest_dump  jexamples/Dump.java     INCLUDE_JARS config/config.jar Jers/ers.jar TDAQExtJars/external.jar jdal  OUTPUT_NAME test_dump  NOINSTALL)
tdaq_add_jar(jtest_cb    jexamples/TestCB.java   INCLUDE_JARS config/config.jar Jers/ers.jar TDAQExtJars/external.jar jdal  OUTPUT_NAME test_cb    NOINSTALL)
tdaq_add_jar(jtest_rw    jexamples/TestRW.java   INCLUDE_JARS config/config.jar Jers/ers.jar TDAQExtJars/external.jar jdal  OUTPUT_NAME test_rw    NOINSTALL)

add_dependencies(jtest_oks 	JAR_dal_dal)
add_dependencies(jtest_dump 	JAR_dal_dal)
add_dependencies(jtest_cb 	JAR_dal_dal)
add_dependencies(jtest_rw 	JAR_dal_dal)

add_test(NAME dal_check  COMMAND ${TDAQ_RUNNER} ${CMAKE_CURRENT_SOURCE_DIR}/test/test.sh ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME dal_check_python_algorithms COMMAND ${TDAQ_RUNNER} tdaq_python ${CMAKE_CURRENT_SOURCE_DIR}/python/tests/algorithm_tests.py )

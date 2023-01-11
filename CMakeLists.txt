cmake_minimum_required(VERSION 3.12)
project(dal VERSION 0.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(system REQUIRED)
find_package(ers REQUIRED)
find_package(logging REQUIRED)
find_package(config REQUIRED)

set(cpp_srcs)
set(cpp_dump_src)

daq_generate_dal(data/schema/core.schema.xml 
  NAMESPACE daq::core
  INCLUDE ${CMAKE_CODEGEN_BINARY_DIR}/include/${PROJECT_NAME}
  CPP ${CMAKE_CODEGEN_BINARY_DIR}/src
  CPP_OUTPUT  dal_cpp_srcs
  DUMP_OUTPUT cpp_dump_src)

daq_add_library(algorithms.cpp disabled-components.cpp test_circular_dependency.cpp ${dal_cpp_srcs} DAL LINK_LIBRARIES config::config system::system logging::logging)

daq_add_python_bindings(*.cpp LINK_LIBRARIES dal)

daq_add_application(dal_dump ${cpp_dump_src}                                              LINK_LIBRARIES dal config::config)
daq_add_application(dal_print_hosts dal_print_hosts.cxx                               LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_test_rw dal_test_rw.cxx                                  LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_test_timeouts dal_test_timeouts.cxx                      LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_dump_apps dal_dump_apps.cxx                              LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_dump_apps_mt dal_dump_apps_mt.cxx                        LINK_LIBRARIES dal config::config Boost::program_options pthread)
daq_add_application(dal_dump_app_config dal_dump_app_config.cxx                 LINK_LIBRARIES dal config Boost::program_options)
daq_add_application(dal_dump_app_depends dal_dump_app_depends.cxx                LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_print_segments dal_print_segments.cxx                    LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_get_app_env dal_get_app_env.cxx                               LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_get_igui_setup dal_get_igui_setup.cxx                         LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_notify dal_notify.cxx                                    LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_control_view dal_control_view.cxx                        LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_test_disabled dal_test_disabled.cxx                      LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_load_is_info_files dal_load_is_info_files.cxx                 LINK_LIBRARIES dal config::config Boost::program_options)
daq_add_application(dal_create_db_connection_files dal_create_db_connection_files.cxx LINK_LIBRARIES dal config::config Boost::program_options stdc++fs)
daq_add_application(dal_test_get_config dal_test_get_config.cxx                  LINK_LIBRARIES dal config::config Boost::program_options)

daq_install()

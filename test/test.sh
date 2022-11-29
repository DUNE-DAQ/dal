#! /bin/sh

################################################################################

rdb_writer_pid=0
ipc_server_pid=0

Cleanup () {
  if [ $rdb_writer_pid -ne 0 ]
  then
    echo "kill rdb_writer process $rdb_writer_pid"
    kill $rdb_writer_pid
    sleep 0.25
  fi

  if [ $ipc_server_pid -ne 0 ]
  then
    echo "kill ipc_server process $ipc_server_pid"
    kill $ipc_server_pid
  fi
}

################################################################################

p_name='dal_test'
echo "generate partition ${p_name}"
pm_part_hlt.py -p ${p_name} || exit 1

################################################################################

db_file="`pwd`/${p_name}.data.xml"
new_file="test.$$.data.xml"
new_file2="test.$$.2.data.xml"
TDAQ_DB_DATA="$db_file" ; export TDAQ_DB_DATA
TDAQ_DB_VERSION='' ; export TDAQ_DB_VERSION

################################################################################

TDAQ_ERS_NO_SIGNAL_HANDLERS=1
export TDAQ_ERS_NO_SIGNAL_HANDLERS

################################################################################

# compare c++ and java output of applications dump using oksconfig
dal_dump_apps_cpp='dal_dump_apps_cpp.oksconfig.cpp.out'
dal_dump_apps_java='dal_dump_apps_cpp.oksconfig.java.out'

echo ''
echo 'TEST 1: run c++ and java applications parameters dump and compare output'
echo "(1/3) run: \"${1}/dal_dump_apps -d oksconfig:${db_file} -p ${p_name} -s > ${dal_dump_apps_cpp}\""
if ! ${1}/dal_dump_apps -d "oksconfig:${db_file}" -p ${p_name} -s > ${dal_dump_apps_cpp}
then 
  echo 'command failed, check FAILED'
  exit 1
fi

echo "(2/3) run: \"$TDAQ_JAVA_HOME/bin/java -classpath $1/dal.jar:\$CLASSPATH dal.DumpApps oksconfig:${db_file} ${p_name} > ${dal_dump_apps_java}\""
if ! $TDAQ_JAVA_HOME/bin/java -classpath $1/dal.jar:$CLASSPATH dal.DumpApps oksconfig:${db_file} ${p_name} > ${dal_dump_apps_java}
then 
  echo 'command failed, check FAILED'
  exit 1
fi

echo "(3/3) run: \"diff ${dal_dump_apps_cpp} ${dal_dump_apps_java}\""
if diff ${dal_dump_apps_cpp} ${dal_dump_apps_java}
then
  echo 'no differences, check PASSED'
  echo ''
  echo ''
else
  echo 'there are differences, check FAILED'
  exit 1
fi


################################################################################

# compare c++ and java output of applications config dump using oksconfig
dal_dump_app_config_cpp='dal_dump_app_config_cpp.oksconfig.cpp.out'
dal_dump_app_config_java='dal_dump_app_config_cpp.oksconfig.java.out'

echo ''
echo 'TEST 2: run c++ and java applications config dump and compare output'
echo "(1/3) run: \"${1}/dal_dump_app_config -d oksconfig:${db_file} -p ${p_name} -b > ${dal_dump_app_config_cpp} > ${dal_dump_app_config_cpp}\""
if ! ${1}/dal_dump_app_config -d "oksconfig:${db_file}" -p ${p_name} -b > ${dal_dump_app_config_cpp}
then 
  echo 'command failed, check FAILED'
  exit 1
fi

echo "(2/3) run: \"$TDAQ_JAVA_HOME/bin/java -classpath $1/dal.jar:\$CLASSPATH dal.DumpAppConfig -d oksconfig:${db_file} -p ${p_name} -b > ${dal_dump_app_config_java} > ${dal_dump_app_config_java}\""
if ! $TDAQ_JAVA_HOME/bin/java -classpath $1/dal.jar:$CLASSPATH dal.DumpAppConfig -d oksconfig:${db_file} -p ${p_name} -b > ${dal_dump_app_config_java}
then 
  echo 'command failed, check FAILED'
  exit 1
fi

echo "(3/3) run: \"diff ${dal_dump_app_config_cpp} ${dal_dump_app_config_java}\""
if diff ${dal_dump_app_config_cpp} ${dal_dump_app_config_java}
then
  echo 'no differences, check PASSED'
  echo ''
  echo ''
else
  echo 'there are differences, check FAILED'
  exit 1
fi

################################################################################

# compare c++ and java output of generated dump applications using oksconfig
dal_dump_cpp='dal_dump.oksconfig.cpp.out'
dal_dump_java='dal_dump.oksconfig.java.out'

echo ''
echo 'TEST 3: test generated dal cpp dump using oksconfig'
echo "(1/3) run: \"${1}/dal_dump -d oksconfig:${db_file} -c Partition | sed 's/daq::core /dal /g' > ${dal_dump_cpp}\""
if ! ${1}/dal_dump -d "oksconfig:${db_file}" -c Partition | sed 's/daq::core /dal /g' > ${dal_dump_cpp}
then
  echo 'command failed, check FAILED'
  exit 1
fi

echo "(2/3) run: \"$TDAQ_JAVA_HOME/bin/java -classpath $1/test_oks.jar:$1/dal.jar:\$CLASSPATH $ref_param dal_test.TestOks oksconfig:${db_file} > ${dal_dump_java}\""
if ! $TDAQ_JAVA_HOME/bin/java -classpath $1/test_oks.jar:$1/dal.jar:$CLASSPATH $ref_param dal_test.TestOks oksconfig:${db_file} > ${dal_dump_java}
then
  echo 'command failed, check FAILED'
  exit 1
fi

echo "(3/3) run: \"diff ${dal_dump_cpp} ${dal_dump_java}\""
if diff ${dal_dump_cpp} ${dal_dump_java}
then
  echo 'no differences, check PASSED'
  echo ''
  echo ''
else
  echo 'there are differences, check FAILED'
  exit 1
fi

################################################################################

echo ''
echo 'TEST 4: test generated cpp and java dal dump using rdbconfig'

ipc_file="/tmp/ipc_root.$$.ref"
TDAQ_IPC_INIT_REF="file:${ipc_file}"
export TDAQ_IPC_INIT_REF

db_name='dal'
startup_timeout='10'

trap 'echo "caught signal: destroy running ipc programs ..."; Cleanup; exit 1' 1 2 15


# run general ipc_server
echo "(1/6) run: \"ipc_server > /dev/null 2>&1 &\""
ipc_server > /dev/null 2>&1 &
ipc_server_pid=$!

count=0
result=1
while [ $count -lt ${startup_timeout} ] ; do
  sleep 1
  test_ipc_server
  result=$?
  if [ $result -eq 0 ] ; then break ; fi
  echo ' * waiting general ipc_server startup ...'
  count=`expr $count + 1`
done

if [ ! $result -eq 0 ] ; then
  echo "ERROR: failed to run general ipc_server (timeout after $startup_timeout seconds)"
#  exit 1
fi


# run rdb_server
echo "(2/6) run: \"rdb_writer -d ${db_name} -D ${db_file} > /dev/null 2>&1 &\""
rdb_writer -d ${db_name} -D ${db_file} > /dev/null 2>&1 &
rdb_writer_pid=$!

count=0
result=1
while [ $count -lt ${startup_timeout} ] ; do
  sleep 1
  test_rdb_writer -n ${db_name}
  result=$?
  if [ $result -eq 0 ] ; then break ; fi
  echo ' * waiting rdb_writer startup ...'
  count=`expr $count + 1`
done

if [ ! $result -eq 0 ] ; then
  echo "ERROR: failed to run rdb_writer (timeout after $startup_timeout seconds)"
fi


  # run check
# compare c++ and java output of generated dump applications using oksconfig
dal_dump_rdbconfig_cpp='dal_dump.rdbconfig.cpp.out'
dal_dump_rdbconfig_java='dal_dump.rdbconfig.java.out'

echo "(3/6) run: \"$1/dal_dump -d "rdbconfig:${db_name}" -c Partition\" | sed 's/daq::core /dal /g' > ${dal_dump_rdbconfig_cpp}\""
if ! $1/dal_dump -d "rdbconfig:${db_name}" -c Partition | sed 's/daq::core /dal /g' > ${dal_dump_rdbconfig_cpp}
then
  echo 'command failed, check FAILED'
  Cleanup
  exit 1
fi

echo "(4/6) run: \"diff ${dal_dump_cpp} ${dal_dump_rdbconfig_cpp}\""
if ! diff ${dal_dump_cpp} ${dal_dump_rdbconfig_cpp}
then
  echo 'there are differences, check FAILED'
  exit 1
fi


  # run java check

ref_param=''

if test $TDAQ_IPC_INIT_REF
then
  ref_param="-Dtdaq.ipc.init.ref=$TDAQ_IPC_INIT_REF"
fi

echo "(5/6) run: \"$TDAQ_JAVA_HOME/bin/java -classpath $1/test_dump.jar:$1/dal.jar::\$CLASSPATH dal_test.Dump -d rdbconfig:${db_name} -c Partition > ${dal_dump_rdbconfig_java}\""

if ! $TDAQ_JAVA_HOME/bin/java -classpath $1/test_dump.jar:$1/dal.jar::$CLASSPATH dal_test.Dump -d "rdbconfig:${db_name}" -c Partition > ${dal_dump_rdbconfig_java}
then
  echo 'command failed, check FAILED'
  Cleanup
  exit 1
fi

echo "(6/6) run: \"diff ${dal_dump_cpp} ${dal_dump_rdbconfig_java}\""
if diff ${dal_dump_cpp} ${dal_dump_rdbconfig_java}
then
  echo 'no differences, check PASSED'
  echo ''
  echo ''
else
  echo 'there are differences, check FAILED'
  exit 1
fi

################################################################################

test_java_cb_rdbconfig='java_cb.rdbconfig.java.out'
test_java_cb_oksconfig='java_cb.oksconfig.java.out'

echo ''
echo 'TEST 5: run java callback tests'

echo "(1/2) run: \"$TDAQ_JAVA_HOME/bin/java -classpath $1/test_cb.jar:$1/dal.jar:\$CLASSPATH $ref_param dal_test.TestCB rdbconfig:${db_name} 1000 > ${test_java_cb_rdbconfig}\""
if ! $TDAQ_JAVA_HOME/bin/java -classpath $1/test_cb.jar:$1/dal.jar:$CLASSPATH $ref_param dal_test.TestCB "rdbconfig:${db_name}" 1000 > ${test_java_cb_rdbconfig}
then
  echo 'command failed, check FAILED'
  Cleanup
  exit 1
fi

echo "(2/2) run: \"$TDAQ_JAVA_HOME/bin/java -classpath $1/test_cb.jar:$1/dal.jar:\$CLASSPATH dal_test.TestCB oksconfig:${db_file} > ${test_java_cb_oksconfig}\""
if $TDAQ_JAVA_HOME/bin/java -classpath $1/test_cb.jar:$1/dal.jar:$CLASSPATH dal_test.TestCB "oksconfig:${db_file}" > ${test_java_cb_oksconfig}
then
  echo 'check PASSED'
  echo ''
  echo ''
else
  echo 'command failed, check FAILED'
  exit 1
fi

################################################################################

echo ''
echo 'TEST 6: Run java dal read/write check'

echo "(1/2) run: \"$TDAQ_JAVA_HOME/bin/java -classpath $1/test_rw.jar:$1/dal.jar:\$CLASSPATH $ref_param dal_test.TestRW oksconfig ${new_file} ${new_file2}\""
if $TDAQ_JAVA_HOME/bin/java -classpath $1/test_rw.jar:$1/dal.jar:$CLASSPATH $ref_param dal_test.TestRW oksconfig ${new_file} ${new_file2}
then
  echo 'oksconfig check PASSED'
  echo '***** generated files are:'
  ls -la ${new_file} ${new_file2}
  rm -f ${new_file} ${new_file2}
  echo ''
  echo ''
else
  echo 'command failed, check FAILED'
  Cleanup
  exit 1
fi

echo "(2/2) run: \"$TDAQ_JAVA_HOME/bin/java -classpath $1/test_rw.jar:$1/dal.jar:\$CLASSPATH $ref_param dal_test.TestRW rdbconfig:${db_name} ${new_file} ${new_file2}\""
if $TDAQ_JAVA_HOME/bin/java -classpath $1/test_rw.jar:$1/dal.jar:$CLASSPATH $ref_param dal_test.TestRW "rdbconfig:${db_name}" ${new_file} ${new_file2}
then
  echo 'rdbconfig check PASSED'
  echo '***** generated files are:'
  ls -la ${new_file} ${new_file2}
  rm -f ${new_file} ${new_file2}
  echo ''
  echo ''
else
  echo 'command failed, check FAILED'
  Cleanup
  exit 1
fi

################################################################################

data_file="/tmp/test-dal-by-`whoami`.$$"
server_name='TestDAL'

echo ''
echo 'TEST 7: Run cpp read/write tests'

echo "(1/2) run: \"$1/dal_test_rw -d ${data_file} -s oksconfig\""
if $1/dal_test_rw -d ${data_file} -s oksconfig
then
  echo 'oksconfig check PASSED'
else
  echo 'command failed, check FAILED'
  Cleanup
  exit 1
fi

rm -rf ${data_file}*

echo "(2/2) run: \"$1/dal_test_rw -d ${data_file} -s rdbconfig:${db_name}\""
if $1/dal_test_rw -d ${data_file} -s "rdbconfig:${db_name}"
then
  echo 'rdbconfig check PASSED'
else
  echo 'command failed, check FAILED'
  Cleanup
  exit 1
fi

rm -rf ${data_file}*

################################################################################

echo ''
echo 'all tests finished, cleanup...'
echo ''

Cleanup

echo "rm ${ipc_file}"
rm -rf ${ipc_file}

################################################################################

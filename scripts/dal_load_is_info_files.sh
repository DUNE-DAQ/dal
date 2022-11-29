#!/bin/sh

partition=''
data=''
name='ISRepository'
rdb_server_options=''
report_duplicated_classes=0

while (test $# -gt 0)
do
  case "$1" in

    -d | --data)
      shift
      data=$1
      ;;

    -p | --partition)
      shift
      partition=$1
      ;;

    -n | --name)
      shift
      name=$1
      ;;

    -e | --report-ers)
      report_duplicated_classes=1
      ;;

    -h | --help)
      echo 'Usage: dal_load_is_info_files.sh [-d database] [-p partition] [-n name] [-h] ...'
      echo ''
      echo 'Arguments/Options:'
      echo '   -h | --help                  print this message'
      echo '   -d | --data                  name of the database (also defined by TDAQ_DB)'
      echo '   -p | --partition             name of partition (also defined by TDAQ_PARTITION)'
      echo '   -e | --report-ers            report duplicated classes to ERS'
      echo "   -n | --name                  name of RDB server for IS info (default: ${name})"
      echo '   *                            pass any other option to RDB server'
      echo ''
      echo 'Description:'
      echo '   Start-up IS info repository.'
      echo ''
      echo '   The utility reads configuration database to get information about IS info description files'
      echo "   used in the partition and runs RDB server [default name: \"${name}\"] with such files."
      echo ''
      exit 0
      ;;

    *)
      rdb_server_options="${rdb_server_options} $1"
      ;;

  esac
  shift
done

if [ -z "${data}" ]
then
  data="${TDAQ_DB}"
fi

if [ -z "${partition}" ]
then
  partition="${TDAQ_PARTITION}"
fi

files=`TDAQ_ERS_DEBUG_LEVEL=0 dal_load_is_info_files -d "${data}" -p "${partition}" | /bin/grep -v 'DEBUG_0'`
result="$?"
if [ ${result} -ne 0 ]
then
  exit ${result}
fi

echo "exec rdb_server ${rdb_server_options} -p ${partition} -d ${name} -s -D ${files}"
echo ''

if [ $report_duplicated_classes -eq 1 ]
then
  OKS_KERNEL_ERS_REPORT_DUPLICATED_CLASSES=1
  export OKS_KERNEL_ERS_REPORT_DUPLICATED_CLASSES
fi

echo 'unset TDAQ_DB_REPOSITORY'
unset TDAQ_DB_REPOSITORY

exec rdb_server ${rdb_server_options} -p "${partition}" -d "${name}" -s -D `echo "${files}"`

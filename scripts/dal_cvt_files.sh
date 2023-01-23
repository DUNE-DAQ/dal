#!/bin/sh

junk="/tmp/online2core.junk.$$"
nf="/tmp/online2core.new.$$"
df="/tmp/online2core.diff.$$"

######################################################
# Figure out how to write a line without a newline:

(echo 'hi there\c') > "$junk"
res=`cat $junk`
if test "$res" = 'hi there\c'; then
    e_n='-n'
    e_c=''
else
    e_n=''
    e_c='\c'
fi

rm "$junk"

######################################################

for f in `find . \( -name '*.c*' -o -name '*.h*' -o -name '*.xml' -o -name 'requirements' \) -print`
do
  echo "processing $f ..."

  cat $f | sed '
                s/\([^a-zA-Z0-9_]\)\(dal::\)\([a-zA-Z0-9_]*\)/\1dunedaq::dal::\3/g;
                s/online\/schema\/online.schema.xml/dal\/schema\/core.schema.xml/g;
                s/online\/schema\/test-repository.schema.xml/daq\/schema\/test-repository.schema.xml/g;
                s/online\/sw\/external.data.xml/DAQRelease\/sw\/external.data.xml/g;
                s/online\/sw\/repository.data.xml/DAQRelease\/sw\/repository.data.xml/g;
                s/online\/sw\/tags.data.xml/DAQRelease\/sw\/tags.data.xml/g;
                s/online\/sw\/common-resources.data.xml/daq\/sw\/common-resources.data.xml/g;
                s/online\/sw\/setup-environment.data.xml/daq\/sw\/setup-environment.data.xml/g;
                s/online\/sw\/test-repository-sw.data.xml/daq\/sw\/test-repository-sw.data.xml/g;
                s/online\/sw\/test-repository-v2.data.xml/daq\/sw\/test-repository-v2.data.xml/g;
                s/online\/hw\/workstations.data.xml/daq\/hw\/workstations.data.xml/g;
                s/online\/hw\/lxplus-cluster.data.xml/daq\/hw\/lxplus-cluster.data.xml/g;
                s/online\/segments\/common-environment.data.xml/daq\/segments\/common-environment.data.xml/g;
                s/online\/segments\/setup.data.xml/daq\/segments\/setup.data.xml/g;
                s/online\/segments\/lxplus_segments.data.xml/daq\/segments\/lxplus_segments.data.xml/g;
                s/online\/segments\/segments.data.xml/daq\/segments\/segments.data.xml/g;
                s/online\/partitions\/be_test.data.xml/daq\/partitions\/be_test.data.xml/g;
                s/online\/partitions\/onlsw_test_10_lxplus.data.xml/daq\/partitions\/onlsw_test_10_lxplus.data.xml/g;
                s/online\/partitions\/onlsw_test.data.xml/daq\/partitions\/onlsw_test.data.xml/g;
                s/NeedsEnvironment/ProcessEnvironment/g;
                s/\([^a-zA-Z0-9_]\)\(Environment\)\([^a-zA-Z0-9_]\)/\1Variable\3/g;
		s/<attr name="Variable"/<attr name="Name"/g;
                s/online-dal/daq-core-dal/g
	       ' > $nf

  diff $f $nf > $df

  if [ $? -eq 1 ]
  then
    echo 'Some possible modifications can be required, see diff result:'
    echo '*************************************************************'
    cat $df
    echo '*************************************************************'

    while true; do
      echo $e_n "Enter S to skip, C to copy, A to apply modifications: $e_c" 

      input=
      read input

      if [ ! -n "${input}" ] ; then
        continue
      elif [ "${input}" = "S" ] ; then
        echo 'skip changes'
        break
      elif [ "${input}" = "C" ] ; then
        cf="$f.modified"
        echo "copy changes to $cf"
	cp $nf $cf
        break
      elif [ "${input}" = "A" ] ; then
        echo "modify $f"
	mv $nf $f
        break
      fi

    done
  fi

  rm -f $junk $nf $df

done

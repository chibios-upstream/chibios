#!/bin/bash
if [ $# -eq 2 ]
  then
  if [ $1 = "rootpath" ]
  then
    find $2 -name "chconf.h" -exec bash update_chconf_rt.sh "{}" \;
  else
    echo "Usage: update_chconf_rt.sh [rootpath <root path>]"
  fi
elif [ $# -eq 1 ]
then
  declare conffile=$(<$1)
  if egrep -q "_CHIBIOS_RT_CONF_" <<< "$conffile"
  then
    echo Processing: $1
    # Preserve spaces inside single-line values, including port expressions.
    # Function-like and multiline hook definitions are not configuration values.
    awk '$1 == "#define" && $2 ~ /^[A-Za-z_][A-Za-z0-9_]*$/ &&
         NF > 2 && $3 != "\\" {
           name = $2;
           sub(/^[[:space:]]*#define[[:space:]]+[A-Za-z_][A-Za-z0-9_]*[[:space:]]+/, "");
           print name "=" $0;
         }' <<< "$conffile" > ./values.txt
    if ! fmpp -q -C conf.fmpp -S ../ftl/processors/conf/chconf_rt
    then
      echo
      echo "aborted"
      exit 1
    fi
    cp ./chconf.h $1
    rm ./chconf.h ./values.txt
  fi
else
 echo "Usage: update_chconf_rt.sh [rootpath <root path>]"
 echo "       update_chconf_rt.sh <configuration file>]"
fi

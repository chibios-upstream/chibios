#!/bin/bash
if [ $# -eq 2 ]
  then
  if [ $1 = "rootpath" ]
  then
    find $2 -name "mcuconf.h" -exec bash update_mcuconf_stm32c5xx.sh "{}" \;
  else
    echo "Usage: update_mcuconf_stm32c5xx.sh [rootpath <root path>]"
  fi
elif [ $# -eq 1 ]
then
  declare conffile=$(<$1)
  if egrep -q "STM32C5xx_MCUCONF" <<< "$conffile"
  then
    echo Processing: $1
    awk '
      /^#define[ \t]+[A-Za-z0-9_]+(\([^)]*\))?([ \t]+.*)?$/ {
        name = $2;
        sub(/\(.*/, "", name);
        if (NF >= 3) {
          sub(/^#define[ \t]+[A-Za-z0-9_]+(\([^)]*\))?[ \t]+/, "");
          print name "=" $0;
        }
        else {
          print name "=1";
        }
      }
    ' <<< "$conffile" > ./values.txt
    if ! fmpp -q -C conf.fmpp -S ../ftl/processors/conf/mcuconf_stm32c5xx
    then
      echo
      echo "aborted"
      exit 1
    fi
    cp ./mcuconf.h $1
    rm ./mcuconf.h ./values.txt
  fi
else
 echo "Usage: update_mcuconf_stm32c5xx.sh [rootpath <root path>]"
 echo "       update_mcuconf_stm32c5xx.sh <configuration file>]"
fi

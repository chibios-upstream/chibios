#!/bin/bash
if [ $# -eq 2 ]
  then
  if [ $1 = "rootpath" ]
  then
    find $2 -name "mcuconf.h" -exec bash update_mcuconf_stm32u575xx.sh "{}" \;
  else
    echo "Usage: update_mcuconf_stm32u575xx.sh [rootpath <root path>]"
  fi
elif [ $# -eq 1 ]
then
  declare conffile=$(<$1)
  if egrep -q "STM32U575_MCUCONF" <<< "$conffile" || egrep -q "STM32U585_MCUCONF" <<< "$conffile"
  then
    echo Processing: $1
    egrep -e "\#define\s+[a-zA-Z0-9_()]*\s+[^\s]" <<< "$conffile" | sed -r 's/\#define\s+([a-zA-Z0-9_]*)(\([^)]*\))?\s+/\1=/g' > ./values.txt
    if ! fmpp -q -C conf.fmpp -S ../ftl/processors/conf/mcuconf_stm32u575xx
    then
      echo
      echo "aborted"
      exit 1
    fi
    cp ./mcuconf.h $1
    rm ./mcuconf.h ./values.txt
  fi
else
 echo "Usage: update_mcuconf_stm32u575xx.sh [rootpath <root path>]"
 echo "       update_mcuconf_stm32u575xx.sh <configuration file>]"
fi

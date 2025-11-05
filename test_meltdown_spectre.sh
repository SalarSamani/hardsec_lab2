#!/bin/bash

set -e
make clean meltdown spectre >/dev/null

if [ "$N" = "" ]
then N=100
fi

MELTDOWN=meltdown
SPECTRE=spectre

function newsecret(){
    secret=`head -c 100 </dev/urandom | md5sum | awk '{ print $1 }'`
    if [ `echo -n $secret | wc -c` != 32 ]
    then
        echo Expected secret to be 32 bytes.
        exit 1
    fi
    echo "GENERATED SECRET: $secret" >&2
    echo $secret >/dev/wom
}

if [ "$1" = "batch" ]
then
    for bin in $MELTDOWN $SPECTRE
    do
      if [ ! -e $bin ]
      then    echo "No $bin found"
              continue
      fi
    
      echo "RELIABILITY TEST OVER $N RUNS OF $bin ..."
    
      newsecret
      rm -f log
      for x in `seq 1 $N`
      do
          rm -f out
          ./$bin >out 2>>errlog
          grep -q $secret out && echo -n "." || echo -n "?"
          cat out >>log
      done
      echo ''
      echo -n 'SUCCESS RATE: '
      grep -c $secret log  || true
      echo ''
    done

    rm -f log
    rm -f out
    rm -f errlog
    make clean >/dev/null
    exit 0
fi

for bin in $MELTDOWN $SPECTRE
do
    echo "**************** TESTING $bin ****************"
    newsecret
    ./$bin > out
    printf "\n======== YOUR BINARY OUTPUT ========\n"
    printf ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n"
    cat out
    printf "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n"
    printf "\nTEST RESULT: "
    grep -q $secret out && echo "PASS" || echo "FAIL"
    echo "**************************************************"
    echo ""
    rm -f out
done
make clean >/dev/null

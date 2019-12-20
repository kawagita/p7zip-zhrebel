#!/bin/sh

cd p7zip_16.02

if ! [ -f makefile ]; then
  echo '7za-zhrebel: No makefile found.'
  exit
fi

TARGET=$1
if ! [ -w makefile.$TARGET ]; then
  echo '7za-zhrebel: No target. Please select below.'
  ls makefile.* \
  | sed s/^makefile.// \
  | awk 'BEGIN {
           getline;
           printf $0;
         }
         ($0 != "common") && ($0 != "glb") && ($0 != "machine") && ($0 != "oldmake") {
           printf ", " $0;
         }'
  echo
  exit
fi

DIR=$2
if [ "$DIR" = "" ]; then
  DIR=/usr/local/bin
elif ! [ -d "$DIR" ]; then
  echo '7za-zhrebel: No install directory.'
  exit
fi

if ! mv makefile.machine makefile.machine.org > /dev/null 2>&1; then
  echo '7za-zhrebel: No build permission.'
  exit
fi

ln -s makefile.$TARGET makefile.machine
make zhrebel
rm makefile.machine
mv makefile.machine.org makefile.machine

if [ -x bin/7za-zhrebel ]; then
  cp bin/7za-zhrebel ${DIR}/7za-zhrebel
  chmod 755 ${DIR}/7za-zhrebel
  strip ${DIR}/7za-zhrebel
fi

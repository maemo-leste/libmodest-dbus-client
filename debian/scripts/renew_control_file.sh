#!/bin/sh

TMP=`dpkg -s libtinymail-1.0-0 | grep Version: | awk '{print $2}'`
echo $TMP
sed -i "s/-1.0-0/-1.0-0 (>= $TMP )/g" debian/modest/DEBIAN/control


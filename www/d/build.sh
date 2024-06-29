#!/bin/sh
echo $0 $*
date
D=`dirname $0`
cd "$D"
cat a.htm a.js aend.htm >a
ls -l a.htm a.js a
echo Done

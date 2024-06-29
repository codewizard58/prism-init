#!/bin/sh
NETDBM="/data/netdbm/www"
PRISMROOT="/data/www/games/prism"
if [ -f "config/prism.txt" ]; then
  tr -d '$' < "config/prism.txt" >"config/prism.info"
  . "config/prism.info"
fi

if [ "$1" != "" ]; then
  (cd "$1" ; tar cf - "$2" ) | tar xvf -
else
  ( cd "$NETDBM"; 	tar cf - d  ) | tar xvf -
  ( cd "$PRISMROOT"; 	tar cf - php  ) | tar xvf -
fi

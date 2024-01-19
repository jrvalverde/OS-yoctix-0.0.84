#!/usr/local/bin/bash
#  St� i yoctix-x.y.z/ n�r du k�r detta!

cd src
../../src2html*/functionfinder "/yoctix/yoctix-current/src/" `find \
	sys/`|sort|uniq|../../src2html*/sb_db_builder .functions

#  f�r att uppdatera sourcebrowser's funktionsdatabas.
#  Kopiera sedan src/sys/ till en extrakopia. "sys".

rm -fr ../../src2html*/sys
cp -R sys ../../src2html*/
rm -f ../../src2html*/.functions
mv -f .functions ../../src2html*/

#  K�r en:

cd ../../src2html*
find -H sys/ > a

#  K�r sedan f�ljande:

for a in `cat a`; do export QUERY_STRING="file=/$a"; echo $a; (./sourcebrowser > $a.html); done

cd sys
mv .html index.html

#  Tada!

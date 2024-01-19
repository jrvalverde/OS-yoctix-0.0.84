#!/usr/local/bin/bash
#  Stå i yoctix-x.y.z/ när du kör detta!

cd src
../../src2html*/functionfinder "/yoctix/yoctix-current/src/" `find \
	sys/`|sort|uniq|../../src2html*/sb_db_builder .functions

#  för att uppdatera sourcebrowser's funktionsdatabas.
#  Kopiera sedan src/sys/ till en extrakopia. "sys".

rm -fr ../../src2html*/sys
cp -R sys ../../src2html*/
rm -f ../../src2html*/.functions
mv -f .functions ../../src2html*/

#  Kör en:

cd ../../src2html*
find -H sys/ > a

#  Kör sedan följande:

for a in `cat a`; do export QUERY_STRING="file=/$a"; echo $a; (./sourcebrowser > $a.html); done

cd sys
mv .html index.html

#  Tada!

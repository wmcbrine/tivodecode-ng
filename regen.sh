#!/bin/sh
rm -f config.cache
aclocal -Im4
autoconf
autoheader
automake
exit

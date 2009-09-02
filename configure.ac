# Copyright (c) 2006,2007 Nokia Corporation
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# * Neither the name of the Nokia Corporation nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

AC_INIT([modest],[3.0],[http://maemo.org])
AC_CONFIG_HEADERS([config.h])

m4_define([modest_api_major_version], [1])
m4_define([modest_api_minor_version], [99])
m4_define([modest_api_micro_version], [0])
m4_define([modest_api_version], [modest_api_major_version.modest_api_minor_version.modest_api_micro_version])


AM_INIT_AUTOMAKE([dist-bzip2])

MODEST_API_MAJOR_VERSION=modest_api_major_version
MODEST_API_MINOR_VERSION=modest_api_minor_version
MODEST_API_MICRO_VERSION=modest_api_micro_version
MODEST_API_VERSION=modest_api_version
AC_SUBST(MODEST_API_MAJOR_VERSION)
AC_SUBST(MODEST_API_MINOR_VERSION)
AC_SUBST(MODEST_API_MICRO_VERSION)
AC_SUBST(MODEST_API_VERSION)

if test x$prefix = xNONE; then
   prefix=/usr/local
fi
AC_SUBST(prefix)

AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_LIBTOOL


# Option to enable debugging
AC_ARG_ENABLE(debug,
              [AC_HELP_STRING([--enable-debug],[Debugging (default=no)])],
              [with_debug=yes], [with_debug=no])

if test "x$with_debug" == "xyes" ; then
        CFLAGS="$CFLAGS -ggdb -O0 -DDEBUG -Wall"
else
        CFLAGS="$CFLAGS -O2 -Wall"
fi

# Option to enable debugging
AC_ARG_ENABLE(gcov,
              [AC_HELP_STRING([--enable-gcov],[Coverage reporting (default=no)])],
              [with_gcov=yes], [with_gcov=no])

if test "x$with_gcov" == "xyes" ; then
        CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
	LDFLAGS="$LDFLAGS -lgcov"
fi


PKG_CHECK_MODULES(MODEST_GSTUFF,glib-2.0 >= 2.6 libosso dbus-1 dbus-glib-1) 
AC_SUBST(MODEST_GSTUFF_CFLAGS)
AC_SUBST(MODEST_GSTUFF_LIBS)


#
# check for MCE
#
if test "x$have_mce" == "xtrue"; then
   mce="mce"	
   AC_DEFINE_UNQUOTED(MODEST_HAVE_MCE, 1, ["Whether mce is used."])
   AC_SUBST(MODEST_MCE_CFLAGS)
   AC_SUBST(MODEST_MCE_LIBS)
fi


# warnings are errors, but don't check format errors, because
# it breaks with logical ids
CFLAGS="$CFLAGS -Wno-format-extra-args -Werror"

serviceentrydir=`$PKG_CONFIG dbus-glib-1 --variable=prefix`/share/dbus-1/services

dbus_api="dbus_api"
AC_SUBST(dbus_api)

# Define as variables in Makefiles
AC_SUBST(serviceentrydir)


AC_OUTPUT([
Makefile
src/Makefile
src/libmodest-dbus-client-1.0.pc
])


echo
echo "modest configure results"
echo "-----------------------------------"
echo "Prefix           : ${prefix}"
echo "MCE              : $have_mce"
echo "CFLAGS           : $CFLAGS"
echo "Debug version    : $with_debug"

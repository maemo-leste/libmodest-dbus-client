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

INCLUDES=\
	$(MODEST_GSTUFF_CFLAGS) \
	$(MODEST_PLATFORM_CFLAGS) \
	-I$(top_srcdir)/src \
	-DPREFIX=\"@prefix@\"

LIBS=\
	$(MODEST_GSTUFF_LIBS)

lib_LTLIBRARIES = libmodest-dbus-client-1.0.la
libmodest_dbus_client_1_0_la_SOURCES = libmodest-dbus-api.h libmodest-dbus-client.h libmodest-dbus-client.c

library_includedir=$(includedir)/libmodest-dbus-client-1.0/libmodest-dbus-client
library_include_HEADERS = libmodest-dbus-api.h libmodest-dbus-client.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmodest-dbus-client-1.0.pc

DISTCLEANFILES = $(pkgconfig_DATA)

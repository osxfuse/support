#!/bin/bash

# Copyright (c) 2014 Benjamin Fleischer
# All rights reserved.
#
# Redistribution  and  use  in  source  and  binary  forms,  with   or   without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above  copyright  notice,
#    this list of conditions and the following disclaimer in  the  documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of osxfuse nor the names of its contributors may  be  used
#    to endorse or promote products derived from this software without  specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND  CONTRIBUTORS  "AS  IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT  NOT  LIMITED  TO,  THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS  FOR  A  PARTICULAR  PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE  COPYRIGHT  OWNER  OR  CONTRIBUTORS  BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,   SPECIAL,   EXEMPLARY,   OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT  LIMITED  TO,   PROCUREMENT   OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF  USE,  DATA,  OR  PROFITS;  OR  BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN
# CONTRACT, STRICT  LIABILITY,  OR  TORT  (INCLUDING  NEGLIGENCE  OR  OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN  IF  ADVISED  OF  THE
# POSSIBILITY OF SUCH DAMAGE.


function uninstall_osxfuse_2
{
    # Core

    /bin/rm -rf /Library/Filesystems/osxfusefs.fs
    /bin/rm -rf /Library/Frameworks/OSXFUSE.framework
    /bin/rm -rf /usr/local/include/osxfuse
    /bin/rm -f /usr/local/lib/libosxfuse.2.dylib
    /bin/rm -f /usr/local/lib/libosxfuse.dylib
    /bin/rm -f /usr/local/lib/libosxfuse.la
    /bin/rm -f /usr/local/lib/libosxfuse_i32.2.dylib
    /bin/rm -f /usr/local/lib/libosxfuse_i32.dylib
    /bin/rm -f /usr/local/lib/libosxfuse_i32.la
    /bin/rm -f /usr/local/lib/libosxfuse_i64.2.dylib
    /bin/rm -f /usr/local/lib/libosxfuse_i64.dylib
    /bin/rm -f /usr/local/lib/libosxfuse_i64.la
    /bin/rm -f /usr/local/lib/pkgconfig/fuse.pc
    /bin/rm -f /usr/local/lib/pkgconfig/osxfuse.pc
    /usr/sbin/pkgutil --forget com.github.osxfuse.pkg.Core

    # MacFUSE compatibilty layer

    if [[ -e /usr/local/lib/libmacfuse_i32.2.dylib ]]
    then
        /bin/rm -rf /Library/Frameworks/MacFUSE.framework
        /bin/rm -f /usr/local/lib/libfuse.0.dylib
        /bin/rm -f /usr/local/lib/libfuse.2.dylib
        /bin/rm -f /usr/local/lib/libfuse.dylib
        /bin/rm -f /usr/local/lib/libfuse.la
        /bin/rm -f /usr/local/lib/libfuse_ino64.2.dylib
        /bin/rm -f /usr/local/lib/libfuse_ino64.dylib
        /bin/rm -f /usr/local/lib/libfuse_ino64.la
        /bin/rm -f /usr/local/lib/libmacfuse_i32.2.dylib
        /bin/rm -f /usr/local/lib/libmacfuse_i32.dylib
        /bin/rm -f /usr/local/lib/libmacfuse_i64.2.dylib
        /bin/rm -f /usr/local/lib/libmacfuse_i64.dylib
        /usr/sbin/pkgutil --forget com.google.macfuse.core
    fi

    # Preference pane

    /bin/rm -rf /Library/PreferencePanes/OSXFUSE.prefPane
    /usr/sbin/pkgutil --forget com.github.osxfuse.pkg.PrefPane

    return 0
}

function uninstall_osxfuse_3
{
    # Core

    /bin/rm -rf /Library/Filesystems/osxfuse.fs
    /bin/rm -rf /Library/Frameworks/OSXFUSE.framework
    /bin/rm -rf /usr/local/include/osxfuse
    /bin/rm -f /usr/local/lib/libosxfuse.2.dylib
    /bin/rm -f /usr/local/lib/libosxfuse.dylib
    /bin/rm -f /usr/local/lib/libosxfuse.la
    /bin/rm -f /usr/local/lib/libosxfuse_i64.2.dylib
    /bin/rm -f /usr/local/lib/libosxfuse_i64.dylib
    /bin/rm -f /usr/local/lib/libosxfuse_i64.la
    /bin/rm -f /usr/local/lib/pkgconfig/fuse.pc
    /bin/rm -f /usr/local/lib/pkgconfig/osxfuse.pc
    /usr/sbin/pkgutil --forget com.github.osxfuse.pkg.Core

    # MacFUSE compatibilty layer

    if [[ -e /usr/local/lib/pkgconfig/macfuse.pc ]]
    then
        /bin/rm -rf /Library/Frameworks/MacFUSE.framework
        /bin/rm -rf /usr/local/include/fuse
        /bin/rm -f /usr/local/include/fuse.h
        /bin/rm -f /usr/local/lib/libfuse.0.dylib
        /bin/rm -f /usr/local/lib/libfuse.2.dylib
        /bin/rm -f /usr/local/lib/libfuse.dylib
        /bin/rm -f /usr/local/lib/libfuse.la
        /bin/rm -f /usr/local/lib/libfuse_ino64.2.dylib
        /bin/rm -f /usr/local/lib/libfuse_ino64.dylib
        /bin/rm -f /usr/local/lib/libfuse_ino64.la
        /bin/rm -f /usr/local/lib/pkgconfig/macfuse.pc
        /usr/sbin/pkgutil --forget com.github.osxfuse.pkg.MacFUSE
    fi

    # Preference pane

    /bin/rm -rf /Library/PreferencePanes/OSXFUSE.prefPane
    /usr/sbin/pkgutil --forget com.github.osxfuse.pkg.PrefPane

    return 0
}

function uninstaller_main
{
    if [[ -e "/Library/Filesystems/osxfuse.fs" ]]
    then
        uninstall_osxfuse_3
    fi

    if [[ -e "/Library/Filesystems/osxfusefs.fs" ]]
    then
        uninstall_osxfuse_2
    fi

    return 0
}

uninstaller_main "${@}"

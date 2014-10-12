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


function uninstaller_main
{
    # Source libraries

    local library_path=""
    for library_path in "${BASH_SOURCE[0]%/*}/lib"/*.sh
    do
        if [[ -f "${library_path}" ]]
        then
            source "${library_path}" || return 1
        fi
    done

    common_log_initialize
    common_signal_trap_initialize

    # Uninstall osxfuse

    if [[ -e "/Library/Filesystems/osxfuse.fs" ]]
    then
        osxfuse_uninstall_osxfuse_3_core
        osxfuse_uninstall_osxfuse_3_prefpane

        if [[ -e /usr/local/lib/pkgconfig/macfuse.pc ]]
        then
            osxfuse_uninstall_osxfuse_3_macfuse
        fi
    fi

    if [[ -e "/Library/Filesystems/osxfusefs.fs" ]]
    then
        osxfuse_uninstall_osxfuse_2_core
        osxfuse_uninstall_osxfuse_2_prefpane

        if [[ -e /usr/local/lib/libmacfuse_i32.2.dylib ]]
        then
            osxfuse_uninstall_osxfuse_2_macfuse
        fi
    fi

    return 0
}

uninstaller_main "${@}"

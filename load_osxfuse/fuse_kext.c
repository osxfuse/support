/*
 * Copyright (c) 2011—2016 Benjamin Fleischer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of osxfuse nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Portions copyright (c) 2007—2009 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Portions copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code as
 * defined in and that are subject to the Apple Public Source License Version
 * 2.0 (the 'License'). You may not use this file except in compliance with the
 * License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please see
 * the License for the specific language governing rights and limitations under
 * the License.
 */

#include "fuse_kext.h"

#include <Availability.h>
#include <errno.h>
#include <grp.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>

#include "fuse_param.h"
#include "fuse_version.h"

#define SYSTEM_VERSION_PATH "/System/Library/CoreServices/SystemVersion.plist"

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1060
    #include <libkern/OSReturn.h>

    extern OSReturn KextManagerLoadKextWithURL(
        CFURLRef kextURL,
        CFArrayRef dependencyKextAndFolderURLs
    ) __attribute__((weak_import));
#else
    #include <IOKit/kext/KextManager.h>
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
    #include <libkern/OSReturn.h>

    extern OSReturn KextManagerUnloadKextWithIdentifier(
        CFStringRef kextIdentifier
    ) __attribute__((weak_import));
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < 101100
    #define kOSKextReturnNotFound             -603947002
    #define kOSKextReturnInternalError        -603947007
    #define kOSKextReturnNoMemory             -603947006
    #define kOSKextReturnNoResources          -603947005
    #define kOSKextReturnNotPrivileged        -603947004
    #define kOSKextReturnInvalidArgument      -603947003
    #define kOSKextReturnNotFound             -603947002
    #define kOSKextReturnBadData              -603947001
    #define kOSKextReturnSerialization        -603947000
    #define kOSKextReturnUnsupported          -603946999
    #define kOSKextReturnDisabled             -603946998
    #define kOSKextReturnNotAKext             -603946997
    #define kOSKextReturnValidation           -603946996
    #define kOSKextReturnAuthentication       -603946995
    #define kOSKextReturnDependencies         -603946994
    #define kOSKextReturnArchNotFound         -603946993
    #define kOSKextReturnCache                -603946992
    #define kOSKextReturnDeferred             -603946991
    #define kOSKextReturnBootLevel            -603946990
    #define kOSKextReturnNotLoadable          -603946989
    #define kOSKextReturnLoadedVersionDiffers -603946988
    #define kOSKextReturnDependencyLoadError  -603946987
    #define kOSKextReturnLinkError            -603946986
    #define kOSKextReturnStartStopError       -603946985
    #define kOSKextReturnInUse                -603946984
    #define kOSKextReturnTimeout              -603946983
    #define kOSKextReturnStopping             -603946982
#else
    #include <libkern/OSKextLib.h>
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < 101300
    #define kOSKextReturnSystemPolicy         -603946981
#endif

static int
fuse_system_get_version(int *major, int *minor, int *bugfix)
{
    int ret = 0;

    CFStringRef plist_path = CFSTR(SYSTEM_VERSION_PATH);

    CFURLRef plist_url = CFURLCreateWithFileSystemPath(
            kCFAllocatorDefault, plist_path, kCFURLPOSIXPathStyle, false);
    CFReadStreamRef plist_stream = CFReadStreamCreateWithFile(
            kCFAllocatorDefault, plist_url);

    CFRelease(plist_url);
    if (!plist_stream) {
        return 1;
    }
    if (!CFReadStreamOpen(plist_stream)) {
        CFRelease(plist_stream);
        return 1;
    }

    CFPropertyListRef plist = NULL;

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
    // CFPropertyListCreateFromStream will be deprecated soon
    plist = CFPropertyListCreateFromStream(kCFAllocatorDefault, plist_stream, 0,
                                           kCFPropertyListImmutable,
                                           NULL, NULL);
#else
    plist = CFPropertyListCreateWithStream(kCFAllocatorDefault, plist_stream, 0,
                                           kCFPropertyListImmutable,
                                           NULL, NULL);
#endif

    CFReadStreamClose(plist_stream);
    CFRelease(plist_stream);
    if (!plist) {
        return 1;
    }

    CFStringRef version = CFDictionaryGetValue(plist, CFSTR("ProductVersion"));
    if (!version) {
        ret = 1;
        goto out_plist;
    }

    CFArrayRef components = CFStringCreateArrayBySeparatingStrings(
            kCFAllocatorDefault, version, CFSTR("."));
    if (!components) {
        ret = 1;
        goto out_plist;
    }

    CFIndex count = CFArrayGetCount(components);

#define component_get(components, count, index) \
        ((index) < (count) \
         ? CFStringGetIntValue(CFArrayGetValueAtIndex((components), (index))) \
         : 0)

    if (major) {
        *major = component_get(components, count, 0);
    }
    if (minor) {
        *minor = component_get(components, count, 1);
    }
    if (bugfix) {
        *bugfix = component_get(components, count, 2);
    }

#undef component_get

    CFRelease(components);

out_plist:
    CFRelease(plist);

    return ret;
}

int
fuse_kext_get_path(char **path)
{
    int ret = 0;

    int version_major = 0;
    int version_minor = 0;

    if (fuse_system_get_version(&version_major, &version_minor, NULL)) {
        /*
         * In order to load the correct kernel extension we need to determine
         * the version of macOS. Since we cannot figure out which version is
         * running return ENOENT.
         */
        return ENOENT;
    }

    ret = asprintf(path, "%s/%d.%d/%s", OSXFUSE_EXTENSIONS_PATH,
                   version_major, version_minor, OSXFUSE_KEXT_NAME);
    if (ret < 0) {
        ret = errno;
    } else {
        ret = 0;
    }

    return ret;
}

int
fuse_kext_check_version(void)
{
    int ret = 0;

    struct vfsconf vfc;
    ret = getvfsbyname(OSXFUSE_NAME, &vfc);
    if (ret) {
        // Kernel extension not loaded
        return ENOENT;
    }

    char version[32];
    size_t version_len = sizeof(version);
    ret = sysctlbyname(OSXFUSE_SYSCTL_VERSION_NUMBER, version, &version_len,
                       NULL, (size_t)0);
    if (ret) {
        // Kernel extension version not supported
        return EINVAL;
    }

    if (version_len != sizeof(OSXFUSE_VERSION)
        || strncmp(OSXFUSE_VERSION, version, version_len)) {
        // Kernel extension version not supported
        return EINVAL;
    }

    return ret;
}

int
fuse_kext_load(void)
{
    int ret = 0;

    char *path = NULL;
    ret = fuse_kext_get_path(&path);
    if (ret) {
        return ret;
    }

#if __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunreachable-code"
#endif

    if (&KextManagerLoadKextWithURL != NULL) {
        CFStringRef km_path = CFStringCreateWithCString(kCFAllocatorDefault,
                                                        path,
                                                        kCFStringEncodingUTF8);
        CFURLRef km_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                        km_path,
                                                        kCFURLPOSIXPathStyle,
                                                        true);

        ret = KextManagerLoadKextWithURL(km_url, NULL);

        CFRelease(km_path);
        CFRelease(km_url);

        if (ret == kOSReturnSuccess) {
            ret = 0;
        } else if (ret == kOSKextReturnNotFound) {
            ret = ENOENT;
        } else if (ret == kOSKextReturnSystemPolicy) {
            ret = EPERM;
        } else {
            ret = -1;
        }

    } else {
        /*
         * KextManager is not available on Mac OS X versions prior to 10.6. We
         * need to fall back to calling kextload directly.
         */

        int pid = fork();
        if (pid == -1) {
            ret = errno;
        } else if (pid == 0) {
            (void)execl(SYSTEM_KEXTLOAD, SYSTEM_KEXTLOAD, path, NULL);
            _exit(1);
        } else {
            int status;
            if (waitpid(pid, &status, 0) == pid && WIFEXITED(status)) {
                ret = WEXITSTATUS(status);
            } else {
                ret = -1;
            }
        }
    }

#if __clang__
    #pragma clang diagnostic pop
#endif

    free(path);
    if (ret) {
        return ret;
    }

    /*
     * Now do any kext-load-time settings that need to be done as root
     */

    struct group *admin_group = getgrnam(MACOSX_ADMIN_GROUP_NAME);
    if (admin_group) {
        int admin_gid = admin_group->gr_gid;
        (void)sysctlbyname(OSXFUSE_SYSCTL_TUNABLES_ADMIN, NULL, NULL,
                           &admin_gid, sizeof(admin_gid));
    }

    return ret;
}

int
fuse_kext_unload(void)
{
    int ret = 0;

#if __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunreachable-code"
#endif

    if (&KextManagerUnloadKextWithIdentifier != NULL) {
        ret = KextManagerUnloadKextWithIdentifier(
                CFSTR(OSXFUSE_BUNDLE_IDENTIFIER));
        if (ret == kOSReturnSuccess) {
            ret = 0;
        } else {
            ret = -1;
        }

    } else {
        /*
         * KextManager is not available on Mac OS X versions prior to 10.6. We
         * need to fall back to calling kextunload directly.
         */

        int pid = fork();
        if (pid == -1) {
            ret = errno;
        } else if (pid == 0) {
            (void)execl(SYSTEM_KEXTUNLOAD, SYSTEM_KEXTUNLOAD, "-b",
                        OSXFUSE_BUNDLE_IDENTIFIER, NULL);
            _exit(1);
        } else {
            int status;
            if (waitpid(pid, &status, 0) == pid && WIFEXITED(status)) {
                ret = WEXITSTATUS(status);
            } else {
                ret = -1;
            }
        }
    }

#if __clang__
    #pragma clang diagnostic pop
#endif

    if (ret != 0) {
        ret = EBUSY;
    }
    return ret;
}

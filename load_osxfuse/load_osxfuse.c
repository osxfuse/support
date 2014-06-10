/*
 * Copyright (c) 2006-2008 Amit Singh/Google Inc.
 * Copyright (c) 2011-2014 Benjamin Fleischer
 * All rights reserved.
 */

/*
 * Portions Copyright (c) 1999-2003 Apple Computer, Inc. All Rights Reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code as
 * defined in and that are subject to the Apple Public Source License Version
 * 2.0 (the 'License'). You may not use this file except in compliance with
 * the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. Please see
 * the License for the specific language governing rights and limitations
 * under the License.
 */

#include <fuse_param.h>
#include <fuse_version.h>

#include <grp.h>
#include <libkern/OSReturn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <AvailabilityMacros.h>
#include <CoreServices/CoreServices.h>

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
    #include <CoreFoundation/CoreFoundation.h>
    #include <IOKit/kext/KextManager.h>

#else /* MAC_OS_X_VERSION_MAX_ALLOWED < 1060 */
    // Declare prototypes introduced in OS X 10.6
    #include <libkern/OSReturn.h>
    extern OSReturn KextManagerLoadKextWithURL(CFURLRef kextURL, CFArrayRef dependencyKextAndFolderURLs) __attribute__((weak_import));
#endif /* MAC_OS_X_VERSION_MAX_ALLOWED >= 1060 */

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
    // Declare prototypes introduced in OS X 10.7
    #include <libkern/OSReturn.h>
    extern OSReturn KextManagerUnloadKextWithIdentifier(CFStringRef kextIdentifier) __attribute__((weak_import));
#endif /* MAC_OS_X_VERSION_MAX_ALLOWED < 1070 */

#if OSXFUSE_ENABLE_MACFUSE_MODE
    #define OSXFUSE_MACFUSE_MODE_ENV  "OSXFUSE_MACFUSE_MODE"
#endif

#define KLVersionComponentGetInt(components, count, index) \
    ((index) < (count) ? CFStringGetIntValue(CFArrayGetValueAtIndex((components), (index))) : 0)

static Boolean
GetSystemVersion(int *systemVersionMajor, int *systemVersionMinor, int *systemVersionBugfix)
{
    Boolean retval = true;

    CFURLRef fileURL;
    CFReadStreamRef fileStream;

    CFPropertyListFormat propertyListFormat;
    CFPropertyListRef propertyList = NULL;

    CFStringRef productVersion;
    CFArrayRef productVersionComponents = NULL;
    CFIndex count;

    fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                            CFSTR("/System/Library/CoreServices/SystemVersion.plist"),
                                            kCFURLPOSIXPathStyle, false);
    fileStream = CFReadStreamCreateWithFile(kCFAllocatorDefault, fileURL);
    CFRelease(fileURL);

    if (!fileStream) {
        goto out;
    }

    if (!CFReadStreamOpen(fileStream)) {
        CFRelease(fileStream);
        goto out;
    }

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
    // Note: This function will be deprecated soon.
    propertyList = CFPropertyListCreateFromStream(kCFAllocatorDefault,
                                                  fileStream,
                                                  0,
                                                  kCFPropertyListImmutable,
                                                  &propertyListFormat,
                                                  NULL);
#else
    propertyList = CFPropertyListCreateWithStream(kCFAllocatorDefault,
                                                  fileStream,
                                                  0,
                                                  kCFPropertyListImmutable,
                                                  &propertyListFormat,
                                                  NULL);
#endif

    CFReadStreamClose(fileStream);
    CFRelease(fileStream);

    if (!propertyList) {
        retval = false;
        goto out;
    }

    // Get value of property ProductVersion
    productVersion = CFDictionaryGetValue(propertyList, CFSTR("ProductVersion"));
    if (!productVersion) {
        retval = false;
        goto out;
    }

    productVersionComponents = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault,
                                                                      productVersion,
                                                                      CFSTR("."));
    if (!productVersionComponents) {
        retval = false;
        goto out;
    }
    count = CFArrayGetCount(productVersionComponents);

    if (systemVersionMajor) {
        *systemVersionMajor = KLVersionComponentGetInt(productVersionComponents, count, 0);
    }
    if (systemVersionMinor) {
        *systemVersionMinor = KLVersionComponentGetInt(productVersionComponents, count, 1);
    }
    if (systemVersionBugfix) {
        *systemVersionBugfix = KLVersionComponentGetInt(productVersionComponents, count, 2);
    }

out:
    if (propertyList) {
        CFRelease(propertyList);
    }
    if (productVersionComponents) {
        CFRelease(productVersionComponents);
    }

    return retval;
}

int
main(__unused int argc, __unused const char *argv[])
{
    int result = -1;
    int system_version_major = -1;
    int system_version_minor = -1;

    struct vfsconf vfc = { 0 };

    char   version[MAXHOSTNAMELEN + 1] = { 0 };
    size_t version_len = MAXHOSTNAMELEN;
    size_t version_len_desired = 0;

    int pid = -1;
    int status;

    char *kext_path = NULL;

    if (!GetSystemVersion(&system_version_major, &system_version_minor, NULL)) {
        /*
         * In order to load the correct kernel extension we need to determine
         * the version of Mac OS X. Since we cannot figure out which version is
         * running return ENOENT.
         */
        result = ENOENT;
        goto out;
    }

    result = getvfsbyname(OSXFUSE_FS_TYPE, &vfc);
    if (result) {
        // osxfuse kernel extension is not already loaded.
        goto load_kext;
    }

    // Some version of osxfusefs is already loaded. Let us check it out.

    result = sysctlbyname(SYSCTL_OSXFUSE_VERSION_NUMBER, version,
                          &version_len, NULL, (size_t)0);
    if (result) {
        result = -1;
        goto out;
    }

    // sysctlbyname includes the trailing '\0' in version_len
    version_len_desired = strlen(OSXFUSE_VERSION) + 1;

    if ((version_len == version_len_desired) &&
        !strncmp(OSXFUSE_VERSION, version, version_len)) {
        // Currently loaded kernel extension is good
        result = 0;
        goto kext_loaded;
    }

    /*
     * Version mismatch. We need to unload the currently loaded kernel
     * extension.
     */

    if (KextManagerUnloadKextWithIdentifier != NULL) {
        // Use KextManager to unload kernel extension
        result = KextManagerUnloadKextWithIdentifier(
                     CFSTR(OSXFUSE_BUNDLE_IDENTIFIER));
    } else {
        /*
         * KextManager is not available on Mac OS X versions prior to 10.6. We
         * need to fall back to calling kextunload directly.
         */
        pid = fork();
        if (pid == 0) {
            result = execl(SYSTEM_KEXTUNLOAD, SYSTEM_KEXTUNLOAD, "-b",
                           OSXFUSE_BUNDLE_IDENTIFIER, NULL);
            // We can only get here if the exec failed
            goto out;
        }

        if (pid == -1) {
            result = errno;
            goto out;
        }

        if (waitpid(pid, &status, 0) && WIFEXITED(status)) {
            result = WEXITSTATUS(status);
        } else {
            result = -1;
        }
    }

    if (result != 0) {
        // Unloading failed
        result = EBUSY;
        goto out;
    }

    // Unloading succeeded. Now load the on-disk version.

load_kext:
    result = asprintf(&kext_path, "%s/%ld.%ld/%s", OSXFUSE_EXTENSIONS_PATH,
                      (long) system_version_major, (long) system_version_minor,
                      OSXFUSE_KEXT_NAME);
    if (result == -1) {
        result = ENOENT;
        goto out;
    }

    if (KextManagerLoadKextWithURL != NULL) {
        /* Use KextManager to load kernel extension */
        CFStringRef km_path;
        CFURLRef km_url;

        km_path = CFStringCreateWithCString(kCFAllocatorDefault, kext_path,
                                            kCFStringEncodingUTF8);
        km_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, km_path,
                                               kCFURLPOSIXPathStyle, true);
        result = KextManagerLoadKextWithURL(km_url, NULL);

        CFRelease(km_path);
        CFRelease(km_url);
    } else {
        /*
         * KextManager is not available on Mac OS X versions prior to 10.6. We
         * need to fall back to calling kextload directly.
         */
        pid = fork();
        if (pid == 0) {
            result = execl(SYSTEM_KEXTLOAD, SYSTEM_KEXTLOAD, kext_path, NULL);

            // We can only get here if the exec failed
            goto out;
        }

        if (pid == -1) {
            result = errno;
            goto out;
        }

        if (waitpid(pid, &status, 0) && WIFEXITED(status)) {
            result = WEXITSTATUS(status);
        } else {
            result = -1;
        }
    }

    // Now do any kext-load-time settings we need to do as root

    if (result == 0) {
        int admin_gid = 0;
        struct group *admin_group = getgrnam(MACOSX_ADMIN_GROUP_NAME);
        if (!admin_group) {
            goto out;
        }
        admin_gid = admin_group->gr_gid;

        // If this fails, we don't care
        (void)sysctlbyname(SYSCTL_OSXFUSE_TUNABLES_ADMIN, NULL, NULL,
                           &admin_gid, sizeof(admin_gid));
    }

kext_loaded:
#if OSXFUSE_ENABLE_MACFUSE_MODE
    {
        char *env_value;
        env_value = getenv(OSXFUSE_MACFUSE_MODE_ENV);
        if (env_value != NULL && strcmp(env_value, "1") == 0) {
            // Enable MacFUSE mode of kernel extension
            int32_t enabled = 1;
            size_t  length = 4;

            (void)sysctlbyname(SYSCTL_OSXFUSE_MACFUSE_MODE, NULL, 0, &enabled,
                               length);
        }
    }
#endif /* OSXFUSE_ENABLE_MACFUSE_MODE */

out:
    if (kext_path) {
        free(kext_path);
    }

    _exit(result);
}

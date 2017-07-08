//
//  FPIPostInstallSection.m
//  PostInstall
//
//  Created by Benjamin Fleischer on 27.06.17.
//  Copyright © 2017 Benjamin Fleischer. All rights reserved.
//

#import "FPIPostInstallSection.h"

#import "FPIAllowSystemExtensionPane.h"

#define FPISystemVersionFilePath @"/System/Library/CoreServices/SystemVersion.plist"

@implementation FPIPostInstallSection

- (void)dealloc
{
    [_firstPane release];

    [super dealloc];
}

- (BOOL)shouldLoad
{
    NSDictionary *systemVersionDictionary = [NSDictionary dictionaryWithContentsOfFile:FPISystemVersionFilePath];
    if (!systemVersionDictionary) {
        return NO;
    }

    NSString *productVersion = [systemVersionDictionary objectForKey:@"ProductVersion"];
    if (!productVersion || ![productVersion isKindOfClass:[NSString class]]) {
        return NO;
    }

    NSArray *productVersionComponents = [productVersion componentsSeparatedByString:@"."];
    if (productVersionComponents.count < 2) {
        return NO;
    }

    NSInteger major = [[productVersionComponents objectAtIndex:0] integerValue];
    NSInteger minor = [[productVersionComponents objectAtIndex:1] integerValue];

    return major > 10 || (major == 10 && minor >= 13);
}

- (InstallerPane *)firstPane
{
    return _firstPane;
}

- (void)willLoadMainNib
{
    _firstPane = [[FPIAllowSystemExtensionPane alloc] initWithSection:self];
}

@end

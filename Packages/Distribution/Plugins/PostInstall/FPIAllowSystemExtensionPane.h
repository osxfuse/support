//
//  FPIAllowSystemExtensionPane.h
//  PostInstall
//
//  Created by Benjamin Fleischer on 27.06.17.
//  Copyright © 2017 Benjamin Fleischer. All rights reserved.
//

#import <InstallerPlugins/InstallerPlugins.h>

@interface FPIAllowSystemExtensionPane : InstallerPane {
    @private
        BOOL _systemExtensionAllowed;
        NSTimer *_loadSystemExtensionTimer;

        InstallerSectionDirection _paneEnterDirection;

        NSView *_view;
        NSTextField *_systemExtensionAllowedLabel;
        NSTextField *_systemExtensionBlockedLabel;
        NSButton *_systemPreferencesButton;
}

@end

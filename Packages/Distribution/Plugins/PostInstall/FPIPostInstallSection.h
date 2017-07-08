//
//  FPIPostInstallSection.h
//  PostInstall
//
//  Created by Benjamin Fleischer on 27.06.17.
//  Copyright © 2017 Benjamin Fleischer. All rights reserved.
//

#import <InstallerPlugins/InstallerPlugins.h>

@interface FPIPostInstallSection : InstallerSection {
    @private
        InstallerPane *_firstPane;
}

@end

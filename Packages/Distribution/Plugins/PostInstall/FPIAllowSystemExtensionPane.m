//
//  FPIAllowSystemExtensionPane.m
//  PostInstall
//
//  Created by Benjamin Fleischer on 27.06.17.
//  Copyright © 2017 Benjamin Fleischer. All rights reserved.
//

#import "FPIAllowSystemExtensionPane.h"

#define FPIKernelExtensionLoaderPath @"/Library/Filesystems/osxfuse.fs/Contents/Resources/load_osxfuse"
#define FPISecurityPreferencesPanePath @"/System/Library/PreferencePanes/Security.prefPane"

@interface FPIAllowSystemExtensionPane (Private)

- (NSString *)localizedStringForKey:(NSString *)key;
- (void)layout;

- (void)loadSystemExtension;
- (void)loadSystemExtensionTaskDidTerminate:(NSNotification *)notification;
- (void)setSystemExtensionAllowed:(BOOL)systemExtensionAllowed;
- (BOOL)isSystemExtensionAllowed;

- (void)openSystemPreferences;

@end

@implementation FPIAllowSystemExtensionPane

- (id)initWithSection:(id)parent
{
    self = [super initWithSection:parent];
    if (self) {
        NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0.0f, 0.0f, 400.0f, 300.0f)];

        NSTextField *systemExtensionAllowedLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16.0f, 184.0f, 368.0f, 100.0f)];
        [systemExtensionAllowedLabel setBezeled:NO];
        [systemExtensionAllowedLabel setDrawsBackground:NO];
        [systemExtensionAllowedLabel setEditable:NO];
        [systemExtensionAllowedLabel setStringValue:[self localizedStringForKey:@"SystemExtensionAllowed"]];

        [systemExtensionAllowedLabel setHidden:YES];
        [view addSubview:systemExtensionAllowedLabel];
        _systemExtensionAllowedLabel = systemExtensionAllowedLabel;

        NSTextField *systemExtensionBlockedLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(16.0f, 184.0f, 368.0f, 100.0f)];
        [systemExtensionBlockedLabel setBezeled:NO];
        [systemExtensionBlockedLabel setDrawsBackground:NO];
        [systemExtensionBlockedLabel setEditable:NO];
        [systemExtensionBlockedLabel setStringValue:[self localizedStringForKey:@"SystemExtensionBlocked"]];

        [systemExtensionBlockedLabel setHidden:YES];
        [view addSubview:systemExtensionBlockedLabel];
        _systemExtensionBlockedLabel = systemExtensionBlockedLabel;

        NSButton *systemPreferencesButton = [[NSButton alloc] initWithFrame:NSMakeRect(0.0f, 0.0f, 100.0f, 32.0f)];
        [systemPreferencesButton setButtonType:NSMomentaryPushInButton];
        [systemPreferencesButton setBezelStyle:NSRoundedBezelStyle];
        [systemPreferencesButton setTitle:[self localizedStringForKey:@"OpenSystemPreferences"]];
        [systemPreferencesButton sizeToFit];

        [systemPreferencesButton setHidden:YES];
        [systemPreferencesButton setTarget:self];
        [systemPreferencesButton setAction:@selector(openSystemPreferences)];


        [view addSubview:systemPreferencesButton];
        _systemPreferencesButton = systemPreferencesButton;

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(layout)
                                                     name:NSViewFrameDidChangeNotification
                                                   object:view];

        _view = view;
        [self layout];

        [self setNextEnabled:NO];
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [_loadSystemExtensionTimer invalidate];
    [_loadSystemExtensionTimer release];

    [_view release];
    [_systemExtensionAllowedLabel release];
    [_systemExtensionBlockedLabel release];
    [_systemPreferencesButton release];

    [super dealloc];
}

- (NSString *)localizedStringForKey:(NSString *)key
{
    return [self.section.bundle localizedStringForKey:key value:nil table:nil];
}

- (NSString *)title
{
    return [self localizedStringForKey:@"AllowSystemExtensionPaneTitle"];
}

- (NSView *)contentView
{
    return _view;
}

- (void)layout
{
    NSView *contentView = [self contentView];
    NSSize contentViewSize = contentView.frame.size;

    NSRect allowedLabelFrame = _systemExtensionAllowedLabel.frame;
    allowedLabelFrame.size.width = contentViewSize.width - 32.0f;
    allowedLabelFrame.size.height = 1000.0f;

    allowedLabelFrame.size = [[_systemExtensionAllowedLabel cell] cellSizeForBounds:allowedLabelFrame];

    allowedLabelFrame.origin.y = contentView.frame.size.height - allowedLabelFrame.size.height - 16.0f;
    _systemExtensionAllowedLabel.frame = allowedLabelFrame;

    NSRect blockedLabelFrame = _systemExtensionBlockedLabel.frame;
    blockedLabelFrame.size.width = contentViewSize.width - 32.0f;
    blockedLabelFrame.size.height = 1000.0f;

    blockedLabelFrame.size = [[_systemExtensionBlockedLabel cell] cellSizeForBounds:blockedLabelFrame];

    blockedLabelFrame.origin.y = contentView.frame.size.height - blockedLabelFrame.size.height - 16.0f;
    _systemExtensionBlockedLabel.frame = blockedLabelFrame;

    NSRect systemPreferencesButtonFrame = _systemPreferencesButton.frame;
    systemPreferencesButtonFrame.origin.x = (contentViewSize.width - systemPreferencesButtonFrame.size.width) / 2.0f;
    systemPreferencesButtonFrame.origin.y = blockedLabelFrame.origin.y - systemPreferencesButtonFrame.size.height - 16.0f;
    _systemPreferencesButton.frame = systemPreferencesButtonFrame;
}

- (void)willEnterPane:(InstallerSectionDirection)dir
{
    [super willEnterPane:dir];

    _paneEnterDirection = dir;

    if (![self isSystemExtensionAllowed]) {
        _loadSystemExtensionTimer = [[NSTimer scheduledTimerWithTimeInterval:1.0
                                                                      target:self
                                                                    selector:@selector(loadSystemExtension)
                                                                    userInfo:nil
                                                                     repeats:YES] retain];
        [self loadSystemExtension];
    }
}

- (void)loadSystemExtension
{
    if ([self isSystemExtensionAllowed]) {
        return;
    }

    NSTask *task = [[NSTask alloc] init];
    [task setLaunchPath:FPIKernelExtensionLoaderPath];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(loadSystemExtensionTaskDidTerminate:)
                                                 name:NSTaskDidTerminateNotification
                                               object:task];

    [task launch];
}

- (void)loadSystemExtensionTaskDidTerminate:(NSNotification *)notification
{
    NSTask *task = [notification object];

    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSTaskDidTerminateNotification object:task];

    int terminationStatus = [task terminationStatus];
    [task autorelease];

    [self setSystemExtensionAllowed:(terminationStatus != EPERM)];
}

- (void)setSystemExtensionAllowed:(BOOL)systemExtensionAllowed
{
    _systemExtensionAllowed = systemExtensionAllowed;

    [_systemExtensionAllowedLabel setHidden:!systemExtensionAllowed];
    [_systemExtensionBlockedLabel setHidden:systemExtensionAllowed];
    [_systemPreferencesButton setHidden:systemExtensionAllowed];

    [self setNextEnabled:systemExtensionAllowed];

    if (_systemExtensionAllowed) {
        [_loadSystemExtensionTimer invalidate];
        [_loadSystemExtensionTimer release];
        _loadSystemExtensionTimer = nil;

        if (_paneEnterDirection == InstallerDirectionForward) {
            [self gotoNextPane];
        }
    }
}

- (BOOL)isSystemExtensionAllowed
{
    return _systemExtensionAllowed;
}

- (void)openSystemPreferences
{
    NSURL *securityPreferencesPaneURL = [NSURL fileURLWithPath:FPISecurityPreferencesPanePath];

    NSWorkspace *workspace = [NSWorkspace sharedWorkspace];
    [workspace openURL:securityPreferencesPaneURL];
}

@end

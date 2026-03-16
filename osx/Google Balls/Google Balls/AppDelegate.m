#import "AppDelegate.h"
#import "BallsView.h"

@interface AppDelegate () {
    BallsView   *_ballsView;
    NSMenuItem  *_flatItem;
    NSMenuItem  *_aquaItem;
}
- (void)_buildStyleMenu;
- (void)_setStyleFlat:(id)sender;
- (void)_setStyleAqua:(id)sender;
@end

@implementation AppDelegate

@synthesize window = _window;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    /* Add BallsView to the window */
    NSRect contentRect = [[_window contentView] bounds];
    _ballsView = [[BallsView alloc] initWithFrame:contentRect];
    [_ballsView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [[_window contentView] addSubview:_ballsView];
    [_window makeFirstResponder:_ballsView];
    [_window setMinSize:NSMakeSize(400, 300)];

    /* Add "View" menu to the menu bar */
    [self _buildStyleMenu];
}

- (void)_buildStyleMenu
{
    NSMenu *menuBar = [NSApp mainMenu];

    /* --- View menu --- */
    NSMenuItem *viewMenuItem = [[NSMenuItem alloc] initWithTitle:@"View"
                                                          action:nil
                                                   keyEquivalent:@""];
    NSMenu *viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
    [viewMenuItem setSubmenu:viewMenu];
    [menuBar addItem:viewMenuItem];

    /* --- "Ball Style" heading (disabled, acts as label) --- */
    NSMenuItem *heading = [[NSMenuItem alloc] initWithTitle:@"Ball Style"
                                                     action:nil
                                              keyEquivalent:@""];
    [heading setEnabled:NO];
    [viewMenu addItem:heading];
    [heading release];

    [viewMenu addItem:[NSMenuItem separatorItem]];

    /* --- Flat --- */
    _flatItem = [[NSMenuItem alloc] initWithTitle:@"Flat"
                                           action:@selector(_setStyleFlat:)
                                    keyEquivalent:@"1"];
    [_flatItem setTarget:self];
    [_flatItem setState:NSOnState];   /* default */
    [viewMenu addItem:_flatItem];

    /* --- Aqua --- */
    _aquaItem = [[NSMenuItem alloc] initWithTitle:@"Aqua (Mac OS X)"
                                           action:@selector(_setStyleAqua:)
                                    keyEquivalent:@"2"];
    [_aquaItem setTarget:self];
    [_aquaItem setState:NSOffState];
    [viewMenu addItem:_aquaItem];

    [viewMenu release];
    [viewMenuItem release];
}

- (void)_setStyleFlat:(id)sender
{
    [_ballsView setBallStyle:BallStyleFlat];
    [_flatItem  setState:NSOnState];
    [_aquaItem  setState:NSOffState];
}

- (void)_setStyleAqua:(id)sender
{
    [_ballsView setBallStyle:BallStyleAqua];
    [_aquaItem  setState:NSOnState];
    [_flatItem  setState:NSOffState];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

- (void)dealloc
{
    [_ballsView release];
    [_flatItem  release];
    [_aquaItem  release];
    [super dealloc];
}

@end

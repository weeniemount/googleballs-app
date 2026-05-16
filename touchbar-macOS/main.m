#import <Cocoa/Cocoa.h>
#import <objc/message.h>
#import <dlfcn.h>
#import <math.h>

static NSTouchBarItemIdentifier const CustomTouchBarItemIdentifier = @"dev.codex.touchbar.fullscreen.item";
static NSTouchBarItemIdentifier const CustomTrayItemIdentifier = @"dev.codex.touchbar.fullscreen.tray";

typedef struct {
    CGFloat x;
    CGFloat y;
} GBVector;

typedef struct {
    CGFloat r;
    CGFloat g;
    CGFloat b;
} GBColor;

typedef struct {
    GBColor color;
    GBVector pos;
    GBVector originalPos;
    GBVector velocity;
    GBVector targetPos;
    CGFloat radius;
    CGFloat baseRadius;
} GBBall;

typedef struct {
    CGFloat x;
    CGFloat y;
    CGFloat radius;
    const char *hex;
} GBBallSeed;

static NSInteger GBHexValue(char value) {
    if (value >= '0' && value <= '9') { return value - '0'; }
    if (value >= 'a' && value <= 'f') { return value - 'a' + 10; }
    if (value >= 'A' && value <= 'F') { return value - 'A' + 10; }
    return 15;
}

static GBColor GBColorFromHex(const char *hex) {
    if (!hex || strlen(hex) < 7 || hex[0] != '#') {
        return (GBColor){1.0, 1.0, 1.0};
    }
    CGFloat r = (CGFloat)(GBHexValue(hex[1]) * 16 + GBHexValue(hex[2])) / 255.0;
    CGFloat g = (CGFloat)(GBHexValue(hex[3]) * 16 + GBHexValue(hex[4])) / 255.0;
    CGFloat b = (CGFloat)(GBHexValue(hex[5]) * 16 + GBHexValue(hex[6])) / 255.0;
    return (GBColor){r, g, b};
}

static GBBall GBBallMake(CGFloat x, CGFloat y, CGFloat radius, const char *hex) {
    GBVector point = {x, y};
    return (GBBall){
        .color = GBColorFromHex(hex),
        .pos = point,
        .originalPos = point,
        .velocity = {0.0, 0.0},
        .targetPos = point,
        .radius = radius,
        .baseRadius = radius,
    };
}

@interface FullWidthTouchBarView : NSView
@property(nonatomic) GBBall *balls;
@property(nonatomic) NSUInteger ballCount;
@property(nonatomic) CGFloat touchX;
@property(nonatomic) CGFloat touchY;
@property(nonatomic) BOOL touchActive;
@property(nonatomic) BOOL darkMode;
@property(nonatomic) CGFloat layoutWidth;
@property(nonatomic) CGFloat layoutHeight;
@property(nonatomic) CGFloat calibrationXOffset;
@end

@implementation FullWidthTouchBarView

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (!self) { return nil; }
    self.wantsLayer = YES;
    self.allowedTouchTypes = NSTouchTypeMaskDirect | NSTouchTypeMaskIndirect;
    self.wantsRestingTouches = YES;
    self.touchX = -1;
    self.touchY = -1;
    self.calibrationXOffset = -55.0;
    [self setFrameSize:NSMakeSize(2000, 30)];
    [NSTimer scheduledTimerWithTimeInterval:1.0 / 30.0
                                     target:self
                                   selector:@selector(tick:)
                                   userInfo:nil
                                    repeats:YES];
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (NSSize)intrinsicContentSize {
    return NSMakeSize(2000, 30);
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    return YES;
}

- (void)dealloc {
    free(self.balls);
}

- (void)setCalibrationXOffset:(CGFloat)calibrationXOffset {
    _calibrationXOffset = calibrationXOffset;
    self.layoutWidth = 0.0;
    self.layoutHeight = 0.0;
    self.needsDisplay = YES;
}

- (void)tick:(NSTimer *)timer {
    [self updateBalls];
    self.needsDisplay = YES;
}

- (void)mouseDown:(NSEvent *)event {
    [self handleMouseEvent:event togglesSwitch:YES];
}

- (void)mouseDragged:(NSEvent *)event {
    [self handleMouseEvent:event togglesSwitch:NO];
}

- (void)mouseUp:(NSEvent *)event {
    self.touchActive = NO;
    self.touchX = -1;
    self.touchY = -1;
    self.needsDisplay = YES;
}

- (void)touchesBeganWithEvent:(NSEvent *)event {
    [self handleTouches:[event touchesMatchingPhase:NSTouchPhaseAny inView:self] togglesSwitch:YES ended:NO];
}

- (void)touchesMovedWithEvent:(NSEvent *)event {
    [self handleTouches:[event touchesMatchingPhase:NSTouchPhaseAny inView:self] togglesSwitch:NO ended:NO];
}

- (void)touchesEndedWithEvent:(NSEvent *)event {
    [self handleTouches:[event touchesMatchingPhase:NSTouchPhaseAny inView:self] togglesSwitch:NO ended:YES];
}

- (void)touchesCancelledWithEvent:(NSEvent *)event {
    [self handleTouches:[event touchesMatchingPhase:NSTouchPhaseAny inView:self] togglesSwitch:NO ended:YES];
}

- (void)handleMouseEvent:(NSEvent *)event togglesSwitch:(BOOL)togglesSwitch {
    NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    [self handleInputAtPoint:point togglesSwitch:togglesSwitch ended:NO];
}

- (void)handleTouches:(NSSet<NSTouch *> *)touches togglesSwitch:(BOOL)togglesSwitch ended:(BOOL)ended {
    if (ended) {
        self.touchActive = NO;
        self.touchX = -1;
        self.touchY = -1;
        self.needsDisplay = YES;
        return;
    }

    NSTouch *touch = touches.anyObject;
    if (!touch) {
        return;
    }

    NSPoint point = touch.type == NSTouchTypeDirect
        ? [touch locationInView:self]
        : NSMakePoint(touch.normalizedPosition.x * NSWidth(self.bounds),
                      (1.0 - touch.normalizedPosition.y) * NSHeight(self.bounds));
    [self handleInputAtPoint:point togglesSwitch:togglesSwitch ended:NO];
}

- (void)handleInputAtPoint:(NSPoint)point togglesSwitch:(BOOL)togglesSwitch ended:(BOOL)ended {
    if (ended) {
        self.touchActive = NO;
        self.touchX = -1;
        self.touchY = -1;
        self.needsDisplay = YES;
        return;
    }

    self.touchX = MAX(0.0, MIN(NSWidth(self.bounds), point.x));
    self.touchY = MAX(0.0, MIN(NSHeight(self.bounds), point.y));
    self.touchActive = YES;
    if (togglesSwitch && self.touchX < [self switchWidth]) {
        self.darkMode = !self.darkMode;
    }
    self.needsDisplay = YES;
}

- (CGFloat)switchWidth {
    return 58.0;
}

- (CGFloat)layoutWidthForGoogleBalls {
    // NSTouchBar can report a very wide custom-item view while only part of it
    // is practically visible. Keep the port laid out in a stable visible band.
    return MIN(NSWidth(self.bounds), 1024.0);
}

- (void)ensureBallsForCurrentSize {
    NSRect bounds = self.bounds;
    CGFloat width = NSWidth(bounds);
    CGFloat height = NSHeight(bounds);
    if (width <= 1.0 || height <= 1.0) {
        return;
    }
    if (self.balls && fabs(self.layoutWidth - width) < 0.5 && fabs(self.layoutHeight - height) < 0.5) {
        return;
    }

    static const GBBallSeed seeds[] = {
        {202.0, 78.0, 9.0, "#ed9d33"}, {348.0, 83.0, 9.0, "#d44d61"},
        {256.0, 69.0, 9.0, "#4f7af2"}, {214.0, 59.0, 9.0, "#ef9a1e"},
        {265.0, 36.0, 9.0, "#4976f3"}, {300.0, 78.0, 9.0, "#269230"},
        {294.0, 59.0, 9.0, "#1f9e2c"}, {45.0, 88.0, 9.0, "#1c48dd"},
        {268.0, 52.0, 9.0, "#2a56ea"}, {73.0, 83.0, 9.0, "#3355d8"},
        {294.0, 6.0, 9.0, "#36b641"}, {235.0, 62.0, 9.0, "#2e5def"},
        {353.0, 42.0, 8.0, "#d53747"}, {336.0, 52.0, 8.0, "#eb676f"},
        {208.0, 41.0, 8.0, "#f9b125"}, {321.0, 70.0, 8.0, "#de3646"},
        {8.0, 60.0, 8.0, "#2a59f0"}, {180.0, 81.0, 8.0, "#eb9c31"},
        {146.0, 65.0, 8.0, "#c41731"}, {145.0, 49.0, 8.0, "#d82038"},
        {246.0, 34.0, 8.0, "#5f8af8"}, {169.0, 69.0, 8.0, "#efa11e"},
        {273.0, 99.0, 8.0, "#2e55e2"}, {248.0, 120.0, 8.0, "#4167e4"},
        {294.0, 41.0, 8.0, "#0b991a"}, {267.0, 114.0, 8.0, "#4869e3"},
        {78.0, 67.0, 8.0, "#3059e3"}, {294.0, 23.0, 8.0, "#10a11d"},
        {117.0, 83.0, 8.0, "#cf4055"}, {137.0, 80.0, 8.0, "#cd4359"},
        {14.0, 71.0, 8.0, "#2855ea"}, {331.0, 80.0, 8.0, "#ca273c"},
        {25.0, 82.0, 8.0, "#2650e1"}, {233.0, 46.0, 8.0, "#4a7bf9"},
        {73.0, 13.0, 8.0, "#3d65e7"}, {327.0, 35.0, 6.0, "#f47875"},
        {319.0, 46.0, 6.0, "#f36764"}, {256.0, 81.0, 6.0, "#1d4eeb"},
        {244.0, 88.0, 6.0, "#698bf1"}, {194.0, 32.0, 6.0, "#fac652"},
        {97.0, 56.0, 6.0, "#ee5257"}, {105.0, 75.0, 6.0, "#cf2a3f"},
        {42.0, 4.0, 6.0, "#5681f5"}, {10.0, 27.0, 6.0, "#4577f6"},
        {166.0, 55.0, 6.0, "#f7b326"}, {266.0, 88.0, 6.0, "#2b58e8"},
        {178.0, 34.0, 6.0, "#facb5e"}, {100.0, 65.0, 6.0, "#e02e3d"},
        {343.0, 32.0, 6.0, "#f16d6f"}, {59.0, 5.0, 6.0, "#507bf2"},
        {27.0, 9.0, 6.0, "#5683f7"}, {233.0, 116.0, 6.0, "#3158e2"},
        {123.0, 32.0, 6.0, "#f0696c"}, {6.0, 38.0, 6.0, "#3769f6"},
        {63.0, 62.0, 6.0, "#6084ef"}, {6.0, 49.0, 6.0, "#2a5cf4"},
        {108.0, 36.0, 6.0, "#f4716e"}, {169.0, 43.0, 6.0, "#f8c247"},
        {137.0, 37.0, 6.0, "#e74653"}, {318.0, 58.0, 6.0, "#ec4147"},
        {226.0, 100.0, 5.0, "#4876f1"}, {101.0, 46.0, 5.0, "#ef5c5c"},
        {226.0, 108.0, 5.0, "#2552ea"}, {17.0, 17.0, 5.0, "#4779f7"},
        {232.0, 93.0, 5.0, "#4b78f1"},
    };

    free(self.balls);
    self.ballCount = sizeof(seeds) / sizeof(seeds[0]);
    self.balls = calloc(self.ballCount, sizeof(GBBall));
    self.layoutWidth = width;
    self.layoutHeight = height;

    CGFloat originalWidth = 360.0;
    CGFloat originalHeight = 130.0;
    CGFloat scale = height / originalHeight;
    CGFloat clusterWidth = originalWidth * scale;
    CGFloat clusterHeight = originalHeight * scale;
    CGFloat visibleWidth = [self layoutWidthForGoogleBalls];
    CGFloat contentMinX = [self switchWidth];
    CGFloat contentWidth = MAX(1.0, visibleWidth - contentMinX);
    CGFloat offsetX = contentMinX + MAX(0.0, (contentWidth - clusterWidth) / 2.0) + self.calibrationXOffset;
    CGFloat offsetY = (height - clusterHeight) / 2.0;

    for (NSUInteger i = 0; i < self.ballCount; i++) {
        CGFloat x = offsetX + seeds[i].x * scale;
        CGFloat y = offsetY + seeds[i].y * scale;
        CGFloat radius = MAX(1.2, seeds[i].radius * scale);
        self.balls[i] = GBBallMake(x, y, radius, seeds[i].hex);
    }
}

- (void)updateBalls {
    [self ensureBallsForCurrentSize];
    const CGFloat interactionRadius = 56.0;
    for (NSUInteger i = 0; i < self.ballCount; i++) {
        GBBall *ball = &self.balls[i];
        if (self.touchActive && self.touchX >= [self switchWidth]) {
            CGFloat dx = self.touchX - ball->pos.x;
            CGFloat dy = self.touchY - ball->pos.y;
            CGFloat distance = sqrt(dx * dx + dy * dy);
            if (distance < interactionRadius) {
                ball->targetPos.x = ball->pos.x - dx;
                ball->targetPos.y = ball->pos.y - dy;
            } else {
                ball->targetPos = ball->originalPos;
            }
        } else {
            ball->targetPos = ball->originalPos;
        }

        CGFloat dx = ball->targetPos.x - ball->pos.x;
        ball->velocity.x += dx * 0.1;
        ball->velocity.x *= 0.8;
        ball->pos.x += ball->velocity.x;

        CGFloat dy = ball->targetPos.y - ball->pos.y;
        ball->velocity.y += dy * 0.1;
        ball->velocity.y *= 0.8;
        ball->pos.y += ball->velocity.y;

        CGFloat ox = ball->originalPos.x - ball->pos.x;
        CGFloat oy = ball->originalPos.y - ball->pos.y;
        CGFloat displaced = sqrt(ox * ox + oy * oy);
        ball->radius = MAX(1.0, ball->baseRadius * (displaced / 100.0 + 1.0));
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    [self ensureBallsForCurrentSize];

    NSRect bounds = self.bounds;
    NSColor *background = self.darkMode ? [NSColor colorWithCalibratedWhite:0.08 alpha:1.0] : NSColor.whiteColor;
    [background setFill];
    NSRectFill(bounds);

    [self drawSwitch];

    for (NSUInteger i = 0; i < self.ballCount; i++) {
        GBBall ball = self.balls[i];
        [[NSColor colorWithCalibratedRed:ball.color.r green:ball.color.g blue:ball.color.b alpha:1.0] setFill];
        NSRect circleRect = NSMakeRect(ball.pos.x - ball.radius, ball.pos.y - ball.radius, ball.radius * 2.0, ball.radius * 2.0);
        [[NSBezierPath bezierPathWithOvalInRect:circleRect] fill];
    }
}

- (void)drawSwitch {
    CGFloat width = 36.0;
    CGFloat height = 16.0;
    CGFloat x = ([self switchWidth] - width) / 2.0;
    CGFloat y = (NSHeight(self.bounds) - height) / 2.0;
    NSRect pillRect = NSMakeRect(x, y, width, height);

    NSColor *pillColor = self.darkMode ? [NSColor colorWithCalibratedWhite:0.30 alpha:1.0] : [NSColor colorWithCalibratedWhite:0.78 alpha:1.0];
    [pillColor setFill];
    [[NSBezierPath bezierPathWithRoundedRect:pillRect xRadius:height / 2.0 yRadius:height / 2.0] fill];

    CGFloat knobRadius = height - 5.0;
    CGFloat knobX = self.darkMode ? NSMaxX(pillRect) - knobRadius - 2.5 : NSMinX(pillRect) + 2.5;
    CGFloat knobY = NSMidY(pillRect) - knobRadius / 2.0;
    NSColor *knobColor = self.darkMode ? NSColor.whiteColor : [NSColor colorWithCalibratedWhite:0.08 alpha:1.0];
    [knobColor setFill];
    [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(knobX, knobY, knobRadius, knobRadius)] fill];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSTouchBarDelegate>
@property(nonatomic, strong) NSWindow *window;
@property(nonatomic, strong) NSTouchBar *bar;
@property(nonatomic, strong) FullWidthTouchBarView *touchView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [self buildWindow];
    [self buildTouchBar];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.35 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self tryPresentFullscreenTouchBar];
    });
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)buildWindow {
    NSRect frame = NSMakeRect(0, 0, 460, 160);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable)
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    self.window.title = @"Google Balls Touch Bar";
    [self.window center];

    NSTextField *label = [NSTextField labelWithString:@"Look down since I'm not putting controls here."];
    label.font = [NSFont systemFontOfSize:14 weight:NSFontWeightMedium];
    label.alignment = NSTextAlignmentCenter;
    label.translatesAutoresizingMaskIntoConstraints = NO;

    NSButton *quit = [NSButton buttonWithTitle:@"Quit" target:NSApp action:@selector(terminate:)];
    quit.translatesAutoresizingMaskIntoConstraints = NO;

    NSStackView *stack = [NSStackView stackViewWithViews:@[label, quit]];
    stack.orientation = NSUserInterfaceLayoutOrientationVertical;
    stack.alignment = NSLayoutAttributeCenterX;
    stack.spacing = 18;
    stack.translatesAutoresizingMaskIntoConstraints = NO;

    self.window.contentView = [[NSView alloc] initWithFrame:frame];
    [self.window.contentView addSubview:stack];
    [NSLayoutConstraint activateConstraints:@[
        [stack.centerXAnchor constraintEqualToAnchor:self.window.contentView.centerXAnchor],
        [stack.centerYAnchor constraintEqualToAnchor:self.window.contentView.centerYAnchor],
        [label.widthAnchor constraintLessThanOrEqualToAnchor:self.window.contentView.widthAnchor constant:-48],
    ]];
}

- (void)buildTouchBar {
    self.touchView = [[FullWidthTouchBarView alloc] initWithFrame:NSMakeRect(0, 0, 2000, 30)];
    self.bar = [[NSTouchBar alloc] init];
    self.bar.delegate = self;
    self.bar.defaultItemIdentifiers = @[CustomTouchBarItemIdentifier];
    self.bar.principalItemIdentifier = CustomTouchBarItemIdentifier;
    self.window.touchBar = self.bar;
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier {
    if (![identifier isEqualToString:CustomTouchBarItemIdentifier]) {
        return nil;
    }

    NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
    item.customizationLabel = @"Google Balls";
    item.visibilityPriority = NSTouchBarItemPriorityHigh;
    item.view = self.touchView;
    return item;
}

- (void)tryPresentFullscreenTouchBar {
    [self hideSystemModalCloseBoxIfAvailable];

    SEL selectorWithPlacement = NSSelectorFromString(@"presentSystemModalTouchBar:placement:systemTrayItemIdentifier:");
    SEL selectorWithoutPlacement = NSSelectorFromString(@"presentSystemModalTouchBar:systemTrayItemIdentifier:");
    SEL legacySelectorWithPlacement = NSSelectorFromString(@"presentSystemModalFunctionBar:placement:systemTrayItemIdentifier:");
    SEL privateLegacySelectorWithPlacement = NSSelectorFromString(@"_presentSystemModalFunctionBar:placement:systemTrayItemIdentifier:");

    if ([NSTouchBar respondsToSelector:selectorWithPlacement]) {
        void (*send)(id, SEL, NSTouchBar *, NSInteger, NSString *) = (void *)objc_msgSend;
        send(NSTouchBar.class, selectorWithPlacement, self.bar, 1, CustomTrayItemIdentifier);
        NSLog(@"Presented fullscreen Touch Bar using %@", NSStringFromSelector(selectorWithPlacement));
        return;
    }

    if ([NSTouchBar respondsToSelector:legacySelectorWithPlacement]) {
        void (*send)(id, SEL, NSTouchBar *, NSInteger, NSString *) = (void *)objc_msgSend;
        send(NSTouchBar.class, legacySelectorWithPlacement, self.bar, 1, CustomTrayItemIdentifier);
        NSLog(@"Presented fullscreen Touch Bar using %@", NSStringFromSelector(legacySelectorWithPlacement));
        return;
    }

    if ([NSTouchBar respondsToSelector:privateLegacySelectorWithPlacement]) {
        void (*send)(id, SEL, NSTouchBar *, NSInteger, NSString *) = (void *)objc_msgSend;
        send(NSTouchBar.class, privateLegacySelectorWithPlacement, self.bar, 1, CustomTrayItemIdentifier);
        NSLog(@"Presented fullscreen Touch Bar using %@", NSStringFromSelector(privateLegacySelectorWithPlacement));
        return;
    }

    if ([NSTouchBar respondsToSelector:selectorWithoutPlacement]) {
        void (*send)(id, SEL, NSTouchBar *, NSString *) = (void *)objc_msgSend;
        send(NSTouchBar.class, selectorWithoutPlacement, self.bar, CustomTrayItemIdentifier);
        NSLog(@"Presented fullscreen Touch Bar using %@", NSStringFromSelector(selectorWithoutPlacement));
        return;
    }

    NSLog(@"Fullscreen system modal Touch Bar selector is unavailable; using app-scoped Touch Bar.");
}

- (void)hideSystemModalCloseBoxIfAvailable {
    void *handle = dlopen("/System/Library/PrivateFrameworks/DFRFoundation.framework/DFRFoundation", RTLD_LAZY);
    if (!handle) {
        return;
    }

    void (*showsCloseBox)(BOOL) = dlsym(handle, "DFRSystemModalShowsCloseBoxWhenFrontMost");
    if (showsCloseBox) {
        showsCloseBox(NO);
    }
}

@end

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        NSApplication *application = NSApplication.sharedApplication;
        AppDelegate *delegate = [AppDelegate new];
        application.delegate = delegate;
        [application run];
    }
    return 0;
}

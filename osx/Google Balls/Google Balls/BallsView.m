#import "BallsView.h"
#import "BallPoint.h"

typedef struct { int x, y, size; const char *color; } RawPoint;

static const RawPoint kRawPoints[] = {
    {202,78,9,"#ed9d33"},{348,83,9,"#d44d61"},{256,69,9,"#4f7af2"},
    {214,59,9,"#ef9a1e"},{265,36,9,"#4976f3"},{300,78,9,"#269230"},
    {294,59,9,"#1f9e2c"},{45,88,9,"#1c48dd"},{268,52,9,"#2a56ea"},
    {73,83,9,"#3355d8"},{294,6,9,"#36b641"},{235,62,9,"#2e5def"},
    {353,42,8,"#d53747"},{336,52,8,"#eb676f"},{208,41,8,"#f9b125"},
    {321,70,8,"#de3646"},{8,60,8,"#2a59f0"},{180,81,8,"#eb9c31"},
    {146,65,8,"#c41731"},{145,49,8,"#d82038"},{246,34,8,"#5f8af8"},
    {169,69,8,"#efa11e"},{273,99,8,"#2e55e2"},{248,120,8,"#4167e4"},
    {294,41,8,"#0b991a"},{267,114,8,"#4869e3"},{78,67,8,"#3059e3"},
    {294,23,8,"#10a11d"},{117,83,8,"#cf4055"},{137,80,8,"#cd4359"},
    {14,71,8,"#2855ea"},{331,80,8,"#ca273c"},{25,82,8,"#2650e1"},
    {233,46,8,"#4a7bf9"},{73,13,8,"#3d65e7"},{327,35,6,"#f47875"},
    {319,46,6,"#f36764"},{256,81,6,"#1d4eeb"},{244,88,6,"#698bf1"},
    {194,32,6,"#fac652"},{97,56,6,"#ee5257"},{105,75,6,"#cf2a3f"},
    {42,4,6,"#5681f5"},{10,27,6,"#4577f6"},{166,55,6,"#f7b326"},
    {266,88,6,"#2b58e8"},{178,34,6,"#facb5e"},{100,65,6,"#e02e3d"},
    {343,32,6,"#f16d6f"},{59,5,6,"#507bf2"},{27,9,6,"#5683f7"},
    {233,116,6,"#3158e2"},{123,32,6,"#f0696c"},{6,38,6,"#3769f6"},
    {63,62,6,"#6084ef"},{6,49,6,"#2a5cf4"},{108,36,6,"#f4716e"},
    {169,43,6,"#f8c247"},{137,37,6,"#e74653"},{318,58,6,"#ec4147"},
    {226,100,5,"#4876f1"},{101,46,5,"#ef5c5c"},{226,108,5,"#2552ea"},
    {17,17,5,"#4779f7"},{232,93,5,"#4b78f1"}
};
static const int kRawPointCount = (int)(sizeof(kRawPoints) / sizeof(kRawPoints[0]));

@interface BallsView () {
    PointCollection *_collection;
    NSTimer         *_timer;
    NSTrackingArea  *_trackingArea;
}
@end

@implementation BallsView

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        _collection = [[PointCollection alloc] init];
        [self _buildPoints];

        _timer = [[NSTimer scheduledTimerWithTimeInterval:0.030
                                                   target:self
                                                 selector:@selector(_tick:)
                                                 userInfo:nil
                                                  repeats:YES] retain];
    }
    return self;
}

- (void)dealloc
{
    [_timer invalidate];
    [_timer release];
    [_collection release];
    [_trackingArea release];
    [super dealloc];
}

/* ---- style accessor --------------------------------------- */

- (void)setBallStyle:(BallStyle)style
{
    _collection->ballStyle = style;
    [self setNeedsDisplay:YES];
}

- (BallStyle)ballStyle
{
    return _collection->ballStyle;
}

/* ---- point construction ----------------------------------- */

static void computeBounds(double *outW, double *outH)
{
    int minX = 99999, maxX = -99999, minY = 99999, maxY = -99999;
    for (int i = 0; i < kRawPointCount; i++) {
        if (kRawPoints[i].x < minX) minX = kRawPoints[i].x;
        if (kRawPoints[i].x > maxX) maxX = kRawPoints[i].x;
        if (kRawPoints[i].y < minY) minY = kRawPoints[i].y;
        if (kRawPoints[i].y > maxY) maxY = kRawPoints[i].y;
    }
    *outW = (double)(maxX - minX);
    *outH = (double)(maxY - minY);
}

- (void)_buildPoints
{
    NSRect b = [self bounds];
    double logoW, logoH;
    computeBounds(&logoW, &logoH);

    double offsetX = (b.size.width  / 2.0) - (logoW / 2.0);
    double offsetY = (b.size.height / 2.0) + (logoH / 2.0);

    for (int i = 0; i < kRawPointCount; i++) {
        double x = offsetX + kRawPoints[i].x;
        double y = offsetY - kRawPoints[i].y;
        NSString *hex = [NSString stringWithUTF8String:kRawPoints[i].color];
        [_collection addPointWithX:x y:y z:0.0
                              size:(double)kRawPoints[i].size
                          colorHex:hex];
    }
}

- (void)_rebuildPoints
{
    [_collection->points removeAllObjects];
    [self _buildPoints];
}

/* ---- NSView overrides ------------------------------------- */

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)isFlipped             { return NO; }

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    if (_trackingArea) {
        [self removeTrackingArea:_trackingArea];
        [_trackingArea release];
        _trackingArea = nil;
    }
    NSTrackingAreaOptions opts =
        NSTrackingMouseMoved            |
        NSTrackingMouseEnteredAndExited |
        NSTrackingActiveInKeyWindow     |
        NSTrackingInVisibleRect;
    _trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                 options:opts
                                                   owner:self
                                                userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

- (void)viewDidMoveToWindow
{
    [super viewDidMoveToWindow];
    [self updateTrackingAreas];
}

- (void)setFrameSize:(NSSize)newSize
{
    [super setFrameSize:newSize];
    [self _rebuildPoints];
    [self updateTrackingAreas];
}

/* ---- mouse events ----------------------------------------- */

- (void)mouseMoved:(NSEvent *)event
{
    NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];
    _collection->mousePos = Vector3Make(p.x, p.y, 0.0);
}

- (void)mouseExited:(NSEvent *)event
{
    _collection->mousePos = Vector3Make(-9999.0, -9999.0, 0.0);
}

/* ---- drawing ---------------------------------------------- */

- (void)drawRect:(NSRect)dirtyRect
{
    CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
    CGContextSetRGBFillColor(ctx, 1.0f, 1.0f, 1.0f, 1.0f);
    CGContextFillRect(ctx, NSRectToCGRect([self bounds]));
    [_collection drawInContext:ctx];
}

/* ---- timer ------------------------------------------------- */

- (void)_tick:(NSTimer *)timer
{
    [_collection update];
    [self setNeedsDisplay:YES];
}

@end

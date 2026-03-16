#import "BallPoint.h"

@implementation BallPoint

- (id)initWithX:(double)x y:(double)y z:(double)z
           size:(double)sz
       colorHex:(NSString *)hex
{
    self = [super init];
    if (self) {
        curPos      = Vector3Make(x, y, z);
        originalPos = Vector3Make(x, y, z);
        targetPos   = Vector3Make(x, y, z);
        velocity    = Vector3Make(0, 0, 0);
        size            = sz;
        radius          = sz;
        friction        = 0.8;
        springStrength  = 0.1;
        color = RGBAColorFromHex(hex);
    }
    return self;
}

- (void)update
{
    /* --- X spring --- */
    double dx = targetPos.x - curPos.x;
    velocity.x += dx * springStrength;
    velocity.x *= friction;
    if (fabs(dx) < 0.1 && fabs(velocity.x) < 0.01) {
        curPos.x = targetPos.x; velocity.x = 0.0;
    } else {
        curPos.x += velocity.x;
    }

    /* --- Y spring --- */
    double dy = targetPos.y - curPos.y;
    velocity.y += dy * springStrength;
    velocity.y *= friction;
    if (fabs(dy) < 0.1 && fabs(velocity.y) < 0.01) {
        curPos.y = targetPos.y; velocity.y = 0.0;
    } else {
        curPos.y += velocity.y;
    }

    /* --- Z depth --- */
    double dox = originalPos.x - curPos.x;
    double doy = originalPos.y - curPos.y;
    double d   = sqrt(dox*dox + doy*doy);
    targetPos.z = d / 100.0 + 1.0;
    double dz = targetPos.z - curPos.z;
    velocity.z += dz * springStrength;
    velocity.z *= friction;
    if (fabs(dz) < 0.01 && fabs(velocity.z) < 0.001) {
        curPos.z = targetPos.z; velocity.z = 0.0;
    } else {
        curPos.z += velocity.z;
    }

    radius = size * curPos.z;
    if (radius < 1.0) radius = 1.0;
}

/* ---- flat style -------------------------------------------- */

static void drawFlat(CGContextRef ctx, CGFloat x, CGFloat y,
                     CGFloat r, RGBAColor c)
{
    CGContextSetRGBFillColor(ctx, c.r, c.g, c.b, c.a);
    CGContextFillEllipseInRect(ctx, CGRectMake(x - r, y - r, r*2, r*2));
}

/* ---- aqua style -------------------------------------------- */

static void drawAqua(CGContextRef ctx, CGFloat x, CGFloat y,
                     CGFloat r, RGBAColor c)
{
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();

    /* Inner: blend toward white for a specular highlight */
    CGFloat inner[4] = {
        c.r + (1.0f - c.r) * 0.55f,
        c.g + (1.0f - c.g) * 0.55f,
        c.b + (1.0f - c.b) * 0.55f,
        c.a
    };
    /* Outer: full original color, no darkening */
    CGFloat outer[4] = { c.r, c.g, c.b, c.a };

    CGColorRef innerColor = CGColorCreate(cs, inner);
    CGColorRef outerColor = CGColorCreate(cs, outer);

    CFMutableArrayRef colors =
        CFArrayCreateMutable(NULL, 2, &kCFTypeArrayCallBacks);
    CFArrayAppendValue(colors, innerColor);
    CFArrayAppendValue(colors, outerColor);

    CGFloat locations[2] = { 0.0f, 1.0f };
    CGGradientRef gradient = CGGradientCreateWithColors(cs, colors, locations);

    CGContextSaveGState(ctx);
    CGContextAddEllipseInRect(ctx, CGRectMake(x - r, y - r, r*2, r*2));
    CGContextClip(ctx);

    /* Highlight offset to upper-left */
    CGPoint centre = CGPointMake(x - r * 0.3f, y + r * 0.3f);
    CGContextDrawRadialGradient(ctx, gradient,
                                centre, 0.0f,
                                CGPointMake(x, y), r,
                                kCGGradientDrawsAfterEndLocation);
    CGContextRestoreGState(ctx);

    CGGradientRelease(gradient);
    CFRelease(colors);
    CGColorRelease(innerColor);
    CGColorRelease(outerColor);
    CGColorSpaceRelease(cs);
}

/* ---- dispatch --------------------------------------------- */

- (void)drawInContext:(CGContextRef)ctx style:(BallStyle)style
{
    CGFloat x = (CGFloat)curPos.x;
    CGFloat y = (CGFloat)curPos.y;
    CGFloat r = (CGFloat)radius;

    if (style == BallStyleAqua)
        drawAqua(ctx, x, y, r, color);
    else
        drawFlat(ctx, x, y, r, color);
}

@end


/* ===============================================================
   PointCollection
=============================================================== */

@implementation PointCollection

- (id)init
{
    self = [super init];
    if (self) {
        mousePos  = Vector3Make(-9999.0, -9999.0, 0.0);
        points    = [[NSMutableArray alloc] init];
        ballStyle = BallStyleFlat;
    }
    return self;
}

- (void)dealloc
{
    [points release];
    [super dealloc];
}

- (void)addPointWithX:(double)x y:(double)y z:(double)z
                 size:(double)sz
             colorHex:(NSString *)hex
{
    BallPoint *bp = [[BallPoint alloc] initWithX:x y:y z:z
                                            size:sz colorHex:hex];
    [points addObject:bp];
    [bp release];
}

- (void)update
{
    for (BallPoint *p in points) {
        double dx = mousePos.x - p->curPos.x;
        double dy = mousePos.y - p->curPos.y;
        double d  = sqrt(dx*dx + dy*dy);

        if (d < 150.0) {
            p->targetPos.x = p->curPos.x - dx;
            p->targetPos.y = p->curPos.y - dy;
        } else {
            p->targetPos.x = p->originalPos.x;
            p->targetPos.y = p->originalPos.y;
        }
        [p update];
    }
}

- (void)drawInContext:(CGContextRef)ctx
{
    for (BallPoint *p in points)
        [p drawInContext:ctx style:ballStyle];
}

@end

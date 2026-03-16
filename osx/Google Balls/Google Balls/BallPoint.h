#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

/* ---------------------------------------------------------------
   Ball render style
--------------------------------------------------------------- */
typedef enum {
    BallStyleFlat = 0,   /* plain filled circle – like the JS/SDL original */
    BallStyleAqua = 1    /* radial gradient    – Mac OS X Aqua look        */
} BallStyle;

/* ---------------------------------------------------------------
   Vector3
--------------------------------------------------------------- */
typedef struct {
    double x, y, z;
} Vector3;

static inline Vector3 Vector3Make(double x, double y, double z)
{
    Vector3 v; v.x = x; v.y = y; v.z = z; return v;
}

/* ---------------------------------------------------------------
   RGBAColor
--------------------------------------------------------------- */
typedef struct {
    CGFloat r, g, b, a;
} RGBAColor;

static inline RGBAColor RGBAColorFromHex(NSString *hex)
{
    NSString *s = [hex hasPrefix:@"#"] ? [hex substringFromIndex:1] : hex;
    unsigned int value = 0;
    [[NSScanner scannerWithString:s] scanHexInt:&value];
    RGBAColor c;
    c.r = ((value >> 16) & 0xFF) / 255.0;
    c.g = ((value >>  8) & 0xFF) / 255.0;
    c.b = ( value        & 0xFF) / 255.0;
    c.a = 1.0;
    return c;
}

/* ---------------------------------------------------------------
   BallPoint
--------------------------------------------------------------- */
@interface BallPoint : NSObject {
@public
    Vector3   curPos;
    Vector3   originalPos;
    Vector3   targetPos;
    Vector3   velocity;
    RGBAColor color;
    double    size;
    double    radius;
    double    friction;
    double    springStrength;
}

- (id)initWithX:(double)x y:(double)y z:(double)z
           size:(double)sz
       colorHex:(NSString *)hex;

- (void)update;
- (void)drawInContext:(CGContextRef)ctx style:(BallStyle)style;

@end

/* ---------------------------------------------------------------
   PointCollection
--------------------------------------------------------------- */
@interface PointCollection : NSObject {
@public
    Vector3         mousePos;
    NSMutableArray *points;
    BallStyle       ballStyle;
}

- (id)init;
- (void)addPointWithX:(double)x y:(double)y z:(double)z
                 size:(double)sz
             colorHex:(NSString *)hex;
- (void)update;
- (void)drawInContext:(CGContextRef)ctx;

@end

#import <Cocoa/Cocoa.h>
#import "BallPoint.h"

@interface BallsView : NSView

- (void)setBallStyle:(BallStyle)style;
- (BallStyle)ballStyle;

@end

/* Copyright (C) 2010, 2011  Chaoji Li */

#import "GamePadView.h"


#define CENTER_OF_RECT(r) CGPointMake(r.size.width/2,r.size.height/2)
#define DISTANCE_BETWEEN(a,b) sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y))

#define CHOP(x) \
if ((x) > 1.0) { \
	(x) = 1.0; \
} else if ((x) < -1.0) { \
	(x) = -1.0; \
} else {}

/** DPadView */
@implementation DPadView
@synthesize currentDirection;
@synthesize backgroundImage;
@synthesize images;
@synthesize keyInput;
@synthesize fourWay;

- (id)initWithFrame:(CGRect)frame {
	if ((self = [super initWithFrame:frame])) {
		self.backgroundColor = [UIColor clearColor];
		minDistance = MAX(10, frame.size.width/2 * 0.16);
		fourWay = NO;

#if SDL_VERSION_ATLEAST(2,0,13)
		int vjoy_index = SDL_JoystickAttachVirtual(
			SDL_JOYSTICK_TYPE_GAMECONTROLLER,
			SDL_CONTROLLER_AXIS_MAX,
			SDL_CONTROLLER_BUTTON_MAX,
			0
		);
		if (vjoy_index < 0) {
			printf("SDL_JoystickAttachVirtual failed: %s\n", SDL_GetError());
		} else {
			vjoy_controller = SDL_GameControllerOpen(vjoy_index);
			if (!vjoy_controller) {
				printf("SDL_GameControllerOpen failed for virtual joystick: %s\n", SDL_GetError());
				SDL_JoystickDetachVirtual(vjoy_index);
			}
		}
		// printf("VJOY INIT, controller=%p\n", vjoy_controller);
#endif
	}
	return self;
}

- (UIImage*)imageForDirection:(DPadDirection)dir
{
	switch (dir) {
	case DPadDirectionNone      : return [images objectAtIndex:0];
	case DPadDirectionRight     : return [images objectAtIndex:1];
	case DPadDirectionRightUp   : return [images objectAtIndex:2];
	case DPadDirectionUp        : return [images objectAtIndex:3];
	case DPadDirectionLeftUp    : return [images objectAtIndex:4];
	case DPadDirectionLeft      : return [images objectAtIndex:5];
	case DPadDirectionLeftDown  : return [images objectAtIndex:6];
	case DPadDirectionDown      : return [images objectAtIndex:7];
	case DPadDirectionRightDown : return [images objectAtIndex:8];
	default                     : return nil;
	}
}

- (void)drawRect:(CGRect)rect
{
	UIImage *image;
	if (backgroundImage)
		[backgroundImage drawInRect:rect];
	if ((image = [self imageForDirection:currentDirection]))
		[image drawInRect:rect];
}

- (void)dealloc {
#if SDL_VERSION_ATLEAST(2,0,13)
	if (vjoy_controller) {
		const SDL_JoystickID vjoy_controller_id = SDL_JoystickInstanceID(
			SDL_GameControllerGetJoystick(vjoy_controller)
		);
		SDL_GameControllerClose(vjoy_controller);
		for (int i = 0, n = SDL_NumJoysticks(); i < n; ++i) {
			const SDL_JoystickID current_id = SDL_JoystickGetDeviceInstanceID(i);
			if (current_id == vjoy_controller_id) {
				// printf("detach virtual at id:%d, index:%d\n", current_id, i);
				SDL_JoystickDetachVirtual(i);
				break;
			}
		}
	}
#endif

	[backgroundImage release];
	[images release];
	[super dealloc];
}

- (DPadDirection)directionOfPoint:(CGPoint)pt
{
	static DPadDirection eight_way_map[] = {
		DPadDirectionLeft,  DPadDirectionLeftDown,  DPadDirectionLeftDown,  DPadDirectionDown, 
		DPadDirectionDown,  DPadDirectionRightDown, DPadDirectionRightDown, DPadDirectionRight,
		DPadDirectionRight, DPadDirectionRightUp,   DPadDirectionRightUp,   DPadDirectionUp,
		DPadDirectionUp,    DPadDirectionLeftUp,    DPadDirectionLeftUp,    DPadDirectionLeft
	};
	static DPadDirection four_way_map[] = {
		DPadDirectionLeft,  DPadDirectionLeft,  DPadDirectionDown,  DPadDirectionDown, 
		DPadDirectionDown,  DPadDirectionDown,  DPadDirectionRight, DPadDirectionRight,
		DPadDirectionRight, DPadDirectionRight, DPadDirectionUp,    DPadDirectionUp,
		DPadDirectionUp,    DPadDirectionUp,    DPadDirectionLeft,  DPadDirectionLeft
	};

	CGPoint ptCenter = CGPointMake(self.bounds.size.width/2, self.bounds.size.height/2);
	double angle = atan2(ptCenter.y - pt.y, pt.x - ptCenter.x);
	int index = (unsigned)((angle+M_PI) / (M_PI/8)) % 16;
	return fourWay ? four_way_map[index] : eight_way_map[index];
}

- (void)sendKey:(int)code pressed:(BOOL)pressed
{
	if (keyInput) {
		if (pressed) {
			[keyInput keydown:code];
		} else {
			[keyInput keyup:code];
		}
	}
}

- (void)setCurrentDirection:(DPadDirection)dir
{
	if (dir != currentDirection) {
		switch (currentDirection)
		{
			case DPadDirectionLeft:
				[self sendKey:SDL_SCANCODE_LEFT  pressed:dir & 4];
				break;
			case DPadDirectionRight:
				[self sendKey:SDL_SCANCODE_RIGHT  pressed:dir & 1];
				break;
			case DPadDirectionUp:
				[self sendKey:SDL_SCANCODE_UP  pressed:dir & 2];
				break;
			case DPadDirectionDown:
				[self sendKey:SDL_SCANCODE_DOWN  pressed:dir & 8];
				break;
			case DPadDirectionLeftUp:
				[self sendKey:SDL_SCANCODE_KP_7  pressed:dir & 6];
				break;
			case DPadDirectionLeftDown:
				[self sendKey:SDL_SCANCODE_KP_1  pressed:dir & 12];
				break;
			case DPadDirectionRightUp:
				[self sendKey:SDL_SCANCODE_KP_9  pressed:dir & 3];
				break;
			case DPadDirectionRightDown:
				[self sendKey:SDL_SCANCODE_KP_3  pressed:dir & 9];
				break;
			default:
				break;
		}

		switch (dir)
		{
			case DPadDirectionLeft:
				[self sendKey:SDL_SCANCODE_LEFT  pressed:dir & 4];
				break;
			case DPadDirectionRight:
				[self sendKey:SDL_SCANCODE_RIGHT  pressed:dir & 1];
				break;
			case DPadDirectionUp:
				[self sendKey:SDL_SCANCODE_UP  pressed:dir & 2];
				break;
			case DPadDirectionDown:
				[self sendKey:SDL_SCANCODE_DOWN  pressed:dir & 8];
				break;
			case DPadDirectionLeftUp:
				[self sendKey:SDL_SCANCODE_KP_7  pressed:dir & 6];
				break;
			case DPadDirectionLeftDown:
				[self sendKey:SDL_SCANCODE_KP_1  pressed:dir & 12];
				break;
			case DPadDirectionRightUp:
				[self sendKey:SDL_SCANCODE_KP_9  pressed:dir & 3];
				break;
			case DPadDirectionRightDown:
				[self sendKey:SDL_SCANCODE_KP_3  pressed:dir & 9];
				break;
			default:
				break;
		}
		currentDirection = dir;
		[self setNeedsDisplay];
	}
}


- (void)updateCurrentDirectionFromPoint:(CGPoint)pt
{
	DPadDirection dir = DPadDirectionNone;
	CGPoint ptCenter = CENTER_OF_RECT(self.bounds);
	if (DISTANCE_BETWEEN(pt, ptCenter) > minDistance)
		dir = [self directionOfPoint:pt];
	if (dir != currentDirection)
		[self setCurrentDirection:dir];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	// UITouch *touch = [touches anyObject];
	// CGPoint pt = [touch locationInView:self];
	// [self updateCurrentDirectionFromPoint:pt];

	UITouch *touch = [touches anyObject];
	// printf("touchesBegan, %p\n", touch);

#if SDL_VERSION_ATLEAST(2,0,13)
	if (!vjoy_is_active && touch != nil) {
		vjoy_input_source = touch;
		vjoy_center = vjoy_current = [touch locationInView:self];
		vjoy_is_active = true;
		SDL_JoystickSetVirtualAxis(
			SDL_GameControllerGetJoystick(vjoy_controller),
			SDL_CONTROLLER_AXIS_LEFTX,
			0
		);
		SDL_JoystickSetVirtualAxis(
			SDL_GameControllerGetJoystick(vjoy_controller),
			SDL_CONTROLLER_AXIS_LEFTY,
			0
		);
		// printf("VJOY START\n");
	}
#endif
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	// UITouch *touch = [touches anyObject];
	// CGPoint pt = [touch locationInView:self];
	// [self updateCurrentDirectionFromPoint:pt];

	UITouch * touch = nil;
	for (UITouch * t in touches) {
		if (t == vjoy_input_source) {
			touch = t;
			break;
		}
	}

	// printf("touchesMoved, vjoy_input_source:%p, touch:%p\n", vjoy_input_source, touch);

#if SDL_VERSION_ATLEAST(2,0,13)
	if (vjoy_is_active && touch != nil) {
		vjoy_current = [touch locationInView:self];
		float dx = vjoy_current.x - vjoy_center.x;
		float dy = vjoy_current.y - vjoy_center.y;

		// Move the vjoy's center if it's outside of its radius
		float dlength = sqrt((dx * dx) + (dy * dy));
		if (dlength > vjoy_radius) {
			vjoy_center.x = vjoy_current.x - (dx * (vjoy_radius / dlength));
			vjoy_center.y = vjoy_current.y - (dy * (vjoy_radius / dlength));
			dx = vjoy_current.x - vjoy_center.x;
			dy = vjoy_current.y - vjoy_center.y;
		}

		// Update vjoy state
		const Sint16 joy_axis_x_raw = (Sint16)((dx / vjoy_radius) * SDL_JOYSTICK_AXIS_MAX);
		SDL_JoystickSetVirtualAxis(
			SDL_GameControllerGetJoystick(vjoy_controller),
			SDL_CONTROLLER_AXIS_LEFTX,
			joy_axis_x_raw
		);
		const Sint16 joy_axis_y_raw = (Sint16)((dy / vjoy_radius) * SDL_JOYSTICK_AXIS_MAX);
		SDL_JoystickSetVirtualAxis(
			SDL_GameControllerGetJoystick(vjoy_controller),
			SDL_CONTROLLER_AXIS_LEFTY,
			joy_axis_y_raw
		);
		// printf("VJOY MOVE: %d, %d\n", (int)joy_axis_x_raw, (int)joy_axis_y_raw);
	}
#endif
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	// [self setCurrentDirection:DPadDirectionNone];

	UITouch * touch = nil;
	for (UITouch * t in touches) {
		if (t == vjoy_input_source) {
			touch = t;
			break;
		}
	}

	// printf("touchesEnded, vjoy_input_source:%p, touch:%p\n", vjoy_input_source, touch);

#if SDL_VERSION_ATLEAST(2,0,13)
	if (vjoy_is_active && touch != nil) {
		// Reset vjoy position to zero
		SDL_JoystickSetVirtualAxis(
			SDL_GameControllerGetJoystick(vjoy_controller),
			SDL_CONTROLLER_AXIS_LEFTX,
			0
		);
		SDL_JoystickSetVirtualAxis(
			SDL_GameControllerGetJoystick(vjoy_controller),
			SDL_CONTROLLER_AXIS_LEFTY,
			0
		);

		// Mark vjoy as inactive
		vjoy_is_active = false;
		// printf("VJOY STOP\n");
	}
#endif
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
	CGPoint ptCenter = CENTER_OF_RECT(self.bounds);
	return DISTANCE_BETWEEN(point, ptCenter) < self.bounds.size.width/2;
}

@end

/** GamePadButton */
@implementation GamePadButton
@synthesize pressed;
@synthesize keyCodes;
@synthesize style;
@synthesize title;
@synthesize images;
@synthesize textColor;
@synthesize delegate;

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
	if (style == GamePadButtonStyleCircle) {
		CGPoint ptCenter = CENTER_OF_RECT(self.bounds);
		return DISTANCE_BETWEEN(point, ptCenter) < self.bounds.size.width/2;
	} else {
		return [super pointInside:point withEvent:event];
	}
}

- (void)setTitle:(NSString *)s
{
	if (title) [title release];
	title = s;
	[title retain];
	[self setNeedsDisplay];
}

- (void)setPressed:(BOOL)b
{
	if (pressed != b) {
		pressed = b;
		if (delegate) {
			if (b)
				[delegate buttonDown:self];
			else
				[delegate buttonUp:self];
		}
		[self setNeedsDisplay];
	}
}

- (void)setStyle:(GamePadButtonStyle)s
{
	style = s;
	[self setNeedsDisplay];
}

- (id)initWithFrame:(CGRect)frame {
	if ((self = [super initWithFrame:frame])) {
		self.backgroundColor = [UIColor clearColor];
		self.textColor       = [UIColor colorWithRed:0 green:0 blue:0 alpha:1];
		keyCodes = [[NSMutableArray alloc] initWithCapacity:2];
	}
	return self;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.pressed = YES;
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	self.pressed = NO;
//	[[self superview] touchesEnded:touches withEvent:event];
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
//	[[self superview] touchesMoved:touches withEvent:event];
}

- (void)drawRect:(CGRect)rect
{
	int bkgIndex;
	UIColor *color = nil;

	if (!pressed) {
		color = [textColor colorWithAlphaComponent:0.3];
		bkgIndex = 0;
	} else {
		color = [textColor colorWithAlphaComponent:0.8];
		bkgIndex = 1;
	}

	if ([images count] > bkgIndex) {
		UIImage *image = [images objectAtIndex:bkgIndex];
		[image drawInRect:rect];
	}

	if (title) {
		float fontSize = MIN(14, rect.size.height/4);
		UIFont *fnt = [UIFont systemFontOfSize:fontSize];
		CGSize size = [title sizeWithAttributes:@{NSFontAttributeName:fnt}];
		CGRect rc = CGRectMake((rect.size.width-size.width)/2,
					(rect.size.height-size.height)/2,
					size.width, size.height);
		[color setFill];
		[title drawInRect:rc withAttributes:@{NSFontAttributeName:fnt,
		                                      NSForegroundColorAttributeName: color}];
	}
}

- (void)dealloc {
	[images release];
	[title release];
	[textColor release];
	[keyCodes release];
	[super dealloc];
}

@end
/** End */

/*
 * Copyright (C) 2010, 2011, 2015  Chaoji Li
 * Copyright (C) 2016-2022 The Exult Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#import "GamePadView.h"

#include <objc/NSObjCRuntime.h>

#define CENTER_OF_RECT(r) CGPointMake(r.size.width / 2, r.size.height / 2)
#define DISTANCE_BETWEEN(a, b) \
	sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y))

#define CHOP(x)              \
	if ((x) > 1.0) {         \
		(x) = 1.0;           \
	} else if ((x) < -1.0) { \
		(x) = -1.0;          \
	} else {                 \
	}

const double vjoy_radius = 80.;    // max-radius of vjoy

/** DPadView */
@implementation DPadView
@synthesize     backgroundImage;
@synthesize     images;
@synthesize     vjoy_is_active;       // true when the vjoy is active
@synthesize     vjoy_center;          // center of the vjoy
@synthesize     vjoy_current;         // current position of the vjoy
@synthesize     vjoy_controller;      // the vjoy's SDL_GameController
@synthesize     vjoy_input_source;    // where vjoy input is coming from

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		self.backgroundColor = [UIColor clearColor];
		self.backgroundImage = [UIImage imageNamed:@"joythumb-glass.png"];

		int vjoy_index = SDL_JoystickAttachVirtual(
				SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
				SDL_CONTROLLER_BUTTON_MAX, 0);
		if (vjoy_index < 0) {
			printf("SDL_JoystickAttachVirtual failed: %s\n", SDL_GetError());
		} else {
			vjoy_controller = SDL_GameControllerOpen(vjoy_index);
			if (!vjoy_controller) {
				printf("SDL_GameControllerOpen failed for virtual joystick: "
					   "%s\n",
					   SDL_GetError());
				SDL_JoystickDetachVirtual(vjoy_index);
			}
		}
		[self reset];
		// printf("VJOY INIT, controller=%p\n", vjoy_controller);
	}
	return self;
}

- (void)drawRect:(CGRect)rect {
	if (self.backgroundImage) {
		[self.backgroundImage drawInRect:rect];
	}
}

- (void)dealloc {
	if (vjoy_controller) {
		const SDL_JoystickID vjoy_controller_id = SDL_JoystickInstanceID(
				SDL_GameControllerGetJoystick(vjoy_controller));
		SDL_GameControllerClose(vjoy_controller);
		for (int i = 0, n = SDL_NumJoysticks(); i < n; ++i) {
			const SDL_JoystickID current_id
					= SDL_JoystickGetDeviceInstanceID(i);
			if (current_id == vjoy_controller_id) {
				// printf("detach virtual at id:%d, index:%d\n", current_id, i);
				SDL_JoystickDetachVirtual(i);
				break;
			}
		}
	}

	[backgroundImage release];
	[images release];
	[super dealloc];
}

- (void)reset {
	self.vjoy_is_active = false;
	SDL_JoystickSetVirtualAxis(
			SDL_GameControllerGetJoystick(self.vjoy_controller),
			SDL_CONTROLLER_AXIS_LEFTX, 0);
	SDL_JoystickSetVirtualAxis(
			SDL_GameControllerGetJoystick(self.vjoy_controller),
			SDL_CONTROLLER_AXIS_LEFTY, 0);
	self.vjoy_center = self.vjoy_current = CGPointMake(0, 0);
	self.vjoy_input_source               = nil;
	[self updateViewTransform];
}

- (void)updateViewTransform {
	if (!self.vjoy_is_active) {
		self.transform = CGAffineTransformIdentity;
		//        printf("updateViewTransform: reset to identity\n");
	} else {
		// Calculate the position of vjoy_center within this view's
		// parent/super-view.  This requires first resetting this view's
		// UIKit 'transform' to an untransformed state, as UIView's
		// method, 'convertPoint:toView:', will apply any existing
		// transform.
		self.transform                     = CGAffineTransformIdentity;
		CGPoint vjoy_center_in_parent_view = [self convertPoint:self.vjoy_center
														 toView:self.superview];

		const CGPoint translation = CGPointMake(
				vjoy_center_in_parent_view.x - self.center.x,
				vjoy_center_in_parent_view.y - self.center.y);
		self.transform = CGAffineTransformMakeTranslation(
				translation.x, translation.y);
	}
	[self setNeedsDisplay];
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
	UITouch* touch = [touches anyObject];
	// printf("touchesBegan, %p\n", touch);

	if (!self.vjoy_is_active && touch != nil) {
		self.vjoy_input_source = touch;
		self.vjoy_center = self.vjoy_current = [touch locationInView:self];
		self.vjoy_is_active                  = true;
		SDL_JoystickSetVirtualAxis(
				SDL_GameControllerGetJoystick(self.vjoy_controller),
				SDL_CONTROLLER_AXIS_LEFTX, 0);
		SDL_JoystickSetVirtualAxis(
				SDL_GameControllerGetJoystick(self.vjoy_controller),
				SDL_CONTROLLER_AXIS_LEFTY, 0);
		[self updateViewTransform];
		// printf("VJOY START\n");
	}
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event {
	//    printf("touchesMoved, self.vjoy_input_source:%p, touch:%p\n",
	//    self.vjoy_input_source, touch);

	UITouch* __strong current_input_source = self.vjoy_input_source;
	if (self.vjoy_is_active && [touches containsObject:current_input_source]) {
		// Calculate new vjoy_current, but first, reset this view's
		// UIKit-transform, lest the call to UITouch's 'locationInView:'
		// method reports incorrect values (due to a previously-applied
		// transform).
		self.transform    = CGAffineTransformIdentity;
		self.vjoy_current = [current_input_source locationInView:self];

		double dx = self.vjoy_current.x - self.vjoy_center.x;
		double dy = self.vjoy_current.y - self.vjoy_center.y;

		// Move the vjoy's center if it's outside of its radius
		double dlength = sqrt((dx * dx) + (dy * dy));
		if (dlength > vjoy_radius) {
			const CGPoint point = CGPointMake(
					self.vjoy_current.x - (dx * (vjoy_radius / dlength)),
					self.vjoy_current.y - (dy * (vjoy_radius / dlength)));
			self.vjoy_center = point;
			dx               = self.vjoy_current.x - self.vjoy_center.x;
			dy               = self.vjoy_current.y - self.vjoy_center.y;
		}

		// Update vjoy state
		const Sint16 joy_axis_x_raw
				= (Sint16)((dx / vjoy_radius) * SDL_JOYSTICK_AXIS_MAX);
		SDL_JoystickSetVirtualAxis(
				SDL_GameControllerGetJoystick(self.vjoy_controller),
				SDL_CONTROLLER_AXIS_LEFTX, joy_axis_x_raw);
		const Sint16 joy_axis_y_raw
				= (Sint16)((dy / vjoy_radius) * SDL_JOYSTICK_AXIS_MAX);
		SDL_JoystickSetVirtualAxis(
				SDL_GameControllerGetJoystick(self.vjoy_controller),
				SDL_CONTROLLER_AXIS_LEFTY, joy_axis_y_raw);

		// Update visuals
		[self updateViewTransform];

		// printf("VJOY MOVE: %d, %d\n", (int)joy_axis_x_raw,
		// (int)joy_axis_y_raw);
	}
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
	// printf("touchesEnded, self.vjoy_input_source:%p, touch:%p\n",
	// self.vjoy_input_source, touch);

	if ([touches containsObject:self.vjoy_input_source]) {
		// Mark vjoy as inactive
		[self reset];
	}
}

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent*)event {
	CGPoint ptCenter = CENTER_OF_RECT(self.bounds);
	return DISTANCE_BETWEEN(point, ptCenter) < self.bounds.size.width / 2;
}

@end

/** GamePadButton */
@implementation GamePadButton
@synthesize     pressed;
@synthesize     keyCodes;
@synthesize     style;
@synthesize     title;
@synthesize     images;
@synthesize     textColor;
@synthesize     delegate;

- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent*)event {
	if (self.style == GamePadButtonStyleCircle) {
		CGPoint ptCenter = CENTER_OF_RECT(self.bounds);
		return DISTANCE_BETWEEN(point, ptCenter) < self.bounds.size.width / 2;
	}
	return [super pointInside:point withEvent:event];
}

- (void)setTitle:(NSString*)s {
	if (title) {
		[title release];
	}
	title = s;
	[title retain];
	[self setNeedsDisplay];
}

- (void)setPressed:(BOOL)b {
	if (pressed != b) {
		pressed = b;
		if (self.delegate) {
			if (b) {
				[self.delegate buttonDown:self];
			} else {
				[self.delegate buttonUp:self];
			}
		}
		[self setNeedsDisplay];
	}
}

- (void)setStyle:(GamePadButtonStyle)s {
	style = s;
	[self setNeedsDisplay];
}

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		self.backgroundColor = [UIColor clearColor];
		self.textColor       = [UIColor colorWithRed:0 green:0 blue:0 alpha:1];
		keyCodes             = [[NSMutableArray alloc] initWithCapacity:2];
	}
	return self;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
	self.pressed = YES;
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
	self.pressed = NO;
	//	[[self superview] touchesEnded:touches withEvent:event];
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event {
	//	[[self superview] touchesMoved:touches withEvent:event];
}

- (void)drawRect:(CGRect)rect {
	NSUInteger bkgIndex;
	UIColor*   color = nil;

	if (!self.pressed) {
		color    = [self.textColor colorWithAlphaComponent:0.3];
		bkgIndex = 0;
	} else {
		color    = [self.textColor colorWithAlphaComponent:0.8];
		bkgIndex = 1;
	}

	if ([self.images count] > bkgIndex) {
		UIImage* image = [self.images objectAtIndex:bkgIndex];
		[image drawInRect:rect];
	}

	if (self.title) {
		double  fontSize = MIN(14, rect.size.height / 4);
		UIFont* fnt      = [UIFont systemFontOfSize:fontSize];
		CGSize  size =
				[self.title sizeWithAttributes:@{NSFontAttributeName: fnt}];

		CGRect rc = CGRectMake(
				(rect.size.width - size.width) / 2,
				(rect.size.height - size.height) / 2, size.width, size.height);
		[color setFill];
		[self.title drawInRect:rc
				withAttributes:@{
					NSFontAttributeName: fnt,
					NSForegroundColorAttributeName: color
				}];
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

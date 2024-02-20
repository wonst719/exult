/*
 * Copyright (C) 2015 Chaoji Li
 * Copyright (C) 2020-2022 The Exult Team
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

#include "ios_utils.h"

#include "Configuration.h"
#import "GamePadView.h"
#include "ignore_unused_variable_warning.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include <cassert>

namespace {
	char docs_dir[512];
}

extern "C" int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);

@interface UIManager : NSObject <KeyInputDelegate, GamePadButtonDelegate>

@property(nonatomic, assign) TouchUI_iOS*   touchUI;
@property(nonatomic, retain) DPadView*      dpad;
@property(nonatomic, retain) GamePadButton* btn1;
@property(nonatomic, retain) GamePadButton* btn2;
@property(nonatomic, assign) SDL_Scancode   recurringKeycode;

- (void)promptForName:(NSString*)name;

@end

@implementation UIManager
@synthesize     touchUI;
@synthesize     dpad;
@synthesize     btn1;
@synthesize     btn2;
@synthesize     recurringKeycode;

- (void)sendRecurringKeycode {
	SDL_SendKeyboardKey(SDL_PRESSED, self.recurringKeycode);
	[self performSelector:@selector(sendRecurringKeycode)
			   withObject:nil
			   afterDelay:.5];
}

- (void)keydown:(SDL_Scancode)keycode {
	SDL_SendKeyboardKey(SDL_PRESSED, keycode);
	self.recurringKeycode = keycode;
	[NSObject cancelPreviousPerformRequestsWithTarget:self
											 selector:@selector
											 (sendRecurringKeycode)
											   object:nil];
	[self performSelector:@selector(sendRecurringKeycode)
			   withObject:nil
			   afterDelay:.5];
}

- (void)keyup:(SDL_Scancode)keycode {
	[NSObject cancelPreviousPerformRequestsWithTarget:self
											 selector:@selector
											 (sendRecurringKeycode)
											   object:nil];
	SDL_SendKeyboardKey(SDL_RELEASED, keycode);
}

- (void)buttonDown:(GamePadButton*)btn {
	NSNumber*  code     = btn.keyCodes[0];
	const auto scancode = static_cast<SDL_Scancode>([code integerValue]);
	SDL_SendKeyboardKey(SDL_PRESSED, scancode);
}

- (void)buttonUp:(GamePadButton*)btn {
	NSNumber*  code     = btn.keyCodes[0];
	const auto scancode = static_cast<SDL_Scancode>([code integerValue]);
	SDL_SendKeyboardKey(SDL_RELEASED, scancode);
}

- (void)promptForName:(NSString*)name {
	UIWindow* alertWindow =
			[[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
	alertWindow.windowLevel        = UIWindowLevelAlert;
	alertWindow.rootViewController = [UIViewController new];
	[alertWindow makeKeyAndVisible];

	UIAlertController* alert = [UIAlertController
			alertControllerWithTitle:@""
							 message:@""
					  preferredStyle:UIAlertControllerStyleAlert];

	UIAlertAction* ok = [UIAlertAction
			actionWithTitle:@"OK"
					  style:UIAlertActionStyleDefault
					handler:^(UIAlertAction* action) {
					  ignore_unused_variable_warning(action);
					  UITextField* textField = alert.textFields.firstObject;
					  TouchUI::onTextInput(textField.text.UTF8String);
					  alertWindow.hidden = YES;
					  [alert dismissViewControllerAnimated:YES completion:nil];
					}];

	UIAlertAction* cancel = [UIAlertAction
			actionWithTitle:@"Cancel"
					  style:UIAlertActionStyleDefault
					handler:^(UIAlertAction* action) {
					  ignore_unused_variable_warning(action);
					  alertWindow.hidden = YES;
					  [alert dismissViewControllerAnimated:YES completion:nil];
					}];
	[alert addAction:ok];
	[alert addAction:cancel];
	[alert addTextFieldWithConfigurationHandler:^(UITextField* textField) {
	  textField.placeholder = @"";
	  if (name != nullptr) {
		  [textField setText:name];
	  }
	}];

	[alertWindow.rootViewController presentViewController:alert
												 animated:YES
											   completion:nil];
}

- (CGRect)calcRectForDPad {
	UIWindow* window = [[[UIApplication sharedApplication] delegate] window];
	UIViewController* controller = window.rootViewController;
	CGRect            rcScreen   = controller.view.bounds;
	CGSize            sizeDpad   = CGSizeMake(100, 100);
	double            margin     = 100;

	std::string str;
	double      left = rcScreen.size.width - sizeDpad.width - margin;
	config->value("config/touch/dpad_location", str, "right");
	if (str == "no") {
		return CGRectZero;
	}
	if (str == "left") {
		left = 0;
	}
	CGRect rcDpad = CGRectMake(
			left, rcScreen.size.height - sizeDpad.height - margin,
			sizeDpad.width, sizeDpad.height);
	return rcDpad;
}

- (void)onDpadLocationChanged {
	self.dpad.frame = [self calcRectForDPad];
}

- (GamePadButton*)createButton:(NSString*)title
					   keycode:(int)keycode
						  rect:(CGRect)rect {
	GamePadButton* btn = [[GamePadButton alloc] initWithFrame:rect];
	btn.keyCodes       = @[@(keycode)];

	auto* button        = [UIImage imageNamed:@"btn.png"];
	auto* buttonPressed = [UIImage imageNamed:@"btnpressed.png"];
	NSAssert(button != nil, @"Failed to load image 'btn.png'");
	NSAssert(buttonPressed != nil, @"Failed to load image 'btnpressed.png'");

	btn.images = @[
		button,
		buttonPressed,
	];
	btn.textColor = [UIColor whiteColor];
	btn.title     = title;
	btn.delegate  = self;
	return btn;
}

- (void)showGameControls {
	if (self.dpad == nil) {
		self.dpad = [[DPadView alloc] initWithFrame:CGRectZero];
	}
	UIWindow* window = [[[UIApplication sharedApplication] delegate] window];
	UIViewController* controller = window.rootViewController;

	self.dpad.frame = [self calcRectForDPad];
	[controller.view addSubview:self.dpad];

	self.dpad.alpha = 1;
}

- (void)hideGameControls {
	self.dpad.alpha = 0;
}

- (void)showButtonControls {
	if (self.btn1 == nil) {
		self.btn1 = [self createButton:@"ESC"
							   keycode:static_cast<int>(SDL_SCANCODE_ESCAPE)
								  rect:CGRectZero];
	}

	UIWindow* window = [[[UIApplication sharedApplication] delegate] window];
	UIViewController* controller = window.rootViewController;

	CGRect rcScreen   = controller.view.bounds;
	CGSize sizeButton = CGSizeMake(60, 30);

	CGRect rcButton = CGRectMake(
			10, rcScreen.size.height - sizeButton.height, sizeButton.width,
			sizeButton.height);
	self.btn1.frame = rcButton;
	[controller.view addSubview:self.btn1];

	self.btn1.alpha = 1;
}

- (void)hideButtonControls {
	self.btn1.alpha = 0;
}

@end

namespace {
	UIManager* DefaultManager;
}

/* ---------------------------------------------------------------------- */
#pragma mark TouchUI iOS

TouchUI_iOS::TouchUI_iOS() {
	if (DefaultManager == nil) {
		DefaultManager = [[UIManager alloc] init];
	}
}

void TouchUI_iOS::promptForName(const char* name) {
	if (name == nullptr) {
		[DefaultManager promptForName:nil];
	} else {
		[DefaultManager promptForName:[NSString stringWithUTF8String:name]];
	}
}

void TouchUI_iOS::showGameControls() {
	[DefaultManager showGameControls];
}

void TouchUI_iOS::hideGameControls() {
	[DefaultManager hideGameControls];
}

void TouchUI_iOS::showButtonControls() {
	[DefaultManager showButtonControls];
}

void TouchUI_iOS::hideButtonControls() {
	[DefaultManager hideButtonControls];
}

void TouchUI_iOS::onDpadLocationChanged() {
	[DefaultManager onDpadLocationChanged];
}

/* ---------------------------------------------------------------------- */

const char* ios_get_documents_dir() {
	if (docs_dir[0] == 0) {
		NSArray* paths = NSSearchPathForDirectoriesInDomains(
				NSDocumentDirectory, NSUserDomainMask, YES);
		NSString* docDirectory = [paths objectAtIndex:0];
		strcpy(docs_dir, docDirectory.UTF8String);
		printf("Documents: %s\n", docs_dir);
		//	chdir(docs_dir);
		//		*strncpy(docs_dir, , sizeof(docs_dir)-1) = 0;
	}
	return docs_dir;
}

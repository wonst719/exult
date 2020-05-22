#import <UIKit/UIKit.h>


#include <SDL.h>

@protocol KeyInputDelegate

- (void)keydown:(SDL_Scancode)keycode;
- (void)keyup:(SDL_Scancode)keycode;

@end


const float vjoy_radius = 80.f;  // max-radius of vjoy

@interface DPadView : UIView
{
	UIImage *backgroundImage;
	bool vjoy_is_active; 		            // true when the vjoy is active
	CGPoint vjoy_center;     	            // center of the vjoy
	CGPoint vjoy_current;     	            // current position of the vjoy
#if SDL_VERSION_ATLEAST(2,0,13)
	SDL_GameController *vjoy_controller;    // the vjoy's SDL_GameController
#endif
	UITouch * __weak vjoy_input_source;	    // where vjoy input is actively coming from
}

@property (nonatomic, retain) NSArray *images;
@property (nonatomic, retain) UIImage *backgroundImage;

@end


typedef enum
{
    GamePadButtonStyleRoundedRectangle,
    GamePadButtonStyleCircle,
} GamePadButtonStyle;

@class GamePadButton;

@protocol GamePadButtonDelegate
- (void)buttonDown:(GamePadButton*)btn;
- (void)buttonUp:(GamePadButton*)btn;
@end

@interface GamePadButton : UIView
{
	/* Bindings */
	NSArray *keyCodes;

	NSString *title;
	BOOL pressed;
	GamePadButtonStyle style;

	NSArray *images;
	UIColor *textColor;
	id<GamePadButtonDelegate> delegate;
}

@property (nonatomic, assign) id<GamePadButtonDelegate> delegate;
@property (nonatomic, retain) UIColor *textColor;
@property (nonatomic, retain) NSArray *images;
@property (nonatomic, retain) NSString *title;
@property (nonatomic, assign) BOOL pressed;
@property (nonatomic, retain) NSArray *keyCodes;
@property (nonatomic, assign) GamePadButtonStyle style;

@end

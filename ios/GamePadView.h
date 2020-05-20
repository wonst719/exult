#import <UIKit/UIKit.h>


#include <SDL.h>

@protocol KeyInputDelegate

- (void)keydown:(SDL_Scancode)keycode;
- (void)keyup:(SDL_Scancode)keycode;

@end

/*
 * A direction code has 4 bits. Each bit indicates a direction:
 *
 *    3   2    1    0
 *  Down Left Up Right
 *
 * In this way we can infer key changes with a XOR between current
 * direction and previous direction.
 *
 *          0010
 *   0110     |    0011
 *            |
 * 0100 ------+------- 0001
 *            |
 *    1100    |   1001
 *          1000
 */
typedef enum
{
	DPadDirectionNone      = 0,
	DPadDirectionRight     = 1,
	DPadDirectionRightUp   = 3,
	DPadDirectionUp        = 2,
	DPadDirectionLeftUp    = 6,
	DPadDirectionLeft      = 4,
	DPadDirectionLeftDown  = 12,
	DPadDirectionDown      = 8,
	DPadDirectionRightDown = 9
} DPadDirection;


const float vjoy_radius = 80.f;  // max-radius of vjoy

@interface DPadView : UIView
{
	DPadDirection currentDirection;
	float minDistance;
	UIImage *backgroundImage;
	NSArray *images;
	BOOL fourWay;
	id<KeyInputDelegate> keyInput;

	bool vjoy_is_active; 		// true when the vjoy is active
	CGPoint vjoy_center;     	// center of the vjoy
	CGPoint vjoy_current;     	// current position of the vjoy
#if SDL_VERSION_ATLEAST(2,0,13)
	SDL_GameController * vjoy_controller; // the vjoy's SDL_GameController
#endif
	UITouch * __weak vjoy_input_source;	// where vjoy input is actively coming from
}

@property (nonatomic, retain) NSArray *images;
@property (nonatomic, retain) UIImage *backgroundImage;
@property (nonatomic, assign) DPadDirection currentDirection;
@property (nonatomic, assign) id<KeyInputDelegate> keyInput;
@property (nonatomic, assign) BOOL fourWay;

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

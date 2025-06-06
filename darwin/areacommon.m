#import "uipriv_darwin.h"

// 10.8 fixups
#define NSEventModifierFlags NSUInteger

// incomplete type only with needed members, same layout for uiArea and uiOpenGLArea
struct uiArea {
	uiDarwinControl c;
	NSView *view;
	uiAreaHandler *ah;
	NSEvent *dragevent;
	BOOL scrolling;
	// ...
};

@implementation uiprivAreaCommonView {
	uiArea *area;
	NSTrackingArea *libui_ta;
}

- (void)setArea:(uiArea *)a
{
	self->area = a;
}

// capture on drag is done automatically on OS X
- (void)doMouseEvent:(NSEvent *)e
{
	uiArea *a = self->area;
	uiAreaMouseEvent me;
	NSPoint point;
	unsigned int buttonNumber;
	NSUInteger pmb;
	unsigned int i, max;

	// this will convert point to drawing space
	// thanks swillits in irc.freenode.net/#macdev
	point = [self convertPoint:[e locationInWindow] fromView:nil];
	me.X = point.x;
	me.Y = point.y;

	me.AreaWidth = 0;
	me.AreaHeight = 0;
	if (!a->scrolling) {
		me.AreaWidth = [self frame].size.width;
		me.AreaHeight = [self frame].size.height;
	}

	buttonNumber = [e buttonNumber] + 1;
	// swap button numbers 2 and 3 (right and middle)
	if (buttonNumber == 2)
		buttonNumber = 3;
	else if (buttonNumber == 3)
		buttonNumber = 2;

	me.Down = 0;
	me.Up = 0;
	me.Count = 0;
	switch ([e type]) {
	case NSLeftMouseDown:
	case NSRightMouseDown:
	case NSOtherMouseDown:
		me.Down = buttonNumber;
		me.Count = [e clickCount];
		break;
	case NSLeftMouseUp:
	case NSRightMouseUp:
	case NSOtherMouseUp:
		me.Up = buttonNumber;
		break;
	case NSLeftMouseDragged:
	case NSRightMouseDragged:
	case NSOtherMouseDragged:
		// we include the button that triggered the dragged event in the Held fields
		buttonNumber = 0;
		break;
	}

	me.Modifiers = [self parseModifiers:e];

	pmb = [NSEvent pressedMouseButtons];
	me.Held1To64 = 0;
	if (buttonNumber != 1 && (pmb & 1) != 0)
		me.Held1To64 |= 1;
	if (buttonNumber != 2 && (pmb & 4) != 0)
		me.Held1To64 |= 2;
	if (buttonNumber != 3 && (pmb & 2) != 0)
		me.Held1To64 |= 4;
	// buttons 4..32
	// https://developer.apple.com/library/mac/documentation/Carbon/Reference/QuartzEventServicesRef/index.html#//apple_ref/c/tdef/CGMouseButton says Quartz only supports up to 32 buttons
	max = 32;
	for (i = 4; i <= max; i++) {
		uint64_t j;

		if (buttonNumber == i)
			continue;
		j = 1 << (i - 1);
		if ((pmb & j) != 0)
			me.Held1To64 |= j;
	}

	if (self->libui_enabled) {
		// and allow dragging here
		a->dragevent = e;
		(*(a->ah->MouseEvent))(a->ah, a, &me);
		a->dragevent = nil;
	}
}

#define mouseEvent(name) \
	- (void)name:(NSEvent *)e \
	{ \
		[self doMouseEvent:e]; \
	}
mouseEvent(mouseMoved)
mouseEvent(mouseDragged)
mouseEvent(rightMouseDragged)
mouseEvent(otherMouseDragged)
mouseEvent(mouseDown)
mouseEvent(rightMouseDown)
mouseEvent(otherMouseDown)
mouseEvent(mouseUp)
mouseEvent(rightMouseUp)
mouseEvent(otherMouseUp)

- (void)mouseEntered:(NSEvent *)e
{
	uiArea *a = self->area;

	if (self->libui_enabled)
		(*(a->ah->MouseCrossed))(a->ah, a, 0);
}

- (void)mouseExited:(NSEvent *)e
{
	uiArea *a = self->area;

	if (self->libui_enabled)
		(*(a->ah->MouseCrossed))(a->ah, a, 1);
}

- (int)sendKeyEvent:(uiAreaKeyEvent *)ke
{
	uiArea *a = self->area;

	return (*(a->ah->KeyEvent))(a->ah, a, ke);
}

- (int)doFlagsChanged:(NSEvent *)e
{
	uiAreaKeyEvent ke;
	uiModifiers whichmod;

	ke.Key = 0;
	ke.ExtKey = 0;

	// Mac OS X sends this event on both key up and key down.
	// Fortunately -[e keyCode] IS valid here, so we can simply map from key code to Modifiers, get the value of [e modifierFlags], and check if the respective bit is set or not — that will give us the up/down state
	if (!uiprivKeycodeModifier([e keyCode], &whichmod))
		return 0;
	ke.Modifier = whichmod;
	ke.Modifiers = [self parseModifiers:e];
	ke.Up = (ke.Modifiers & ke.Modifier) == 0;
	// and then drop the current modifier from Modifiers
	ke.Modifiers &= ~ke.Modifier;
	return [self sendKeyEvent:&ke];
}

- (uiModifiers)parseModifiers:(NSEvent *)e
{
	NSEventModifierFlags mods;
	uiModifiers m;

	m = 0;
	mods = [e modifierFlags];
	if ((mods & NSControlKeyMask) != 0)
		m |= uiModifierCtrl;
	if ((mods & NSAlternateKeyMask) != 0)
		m |= uiModifierAlt;
	if ((mods & NSShiftKeyMask) != 0)
		m |= uiModifierShift;
	if ((mods & NSCommandKeyMask) != 0)
		m |= uiModifierSuper;
	return m;
}

// note: there is no equivalent to WM_CAPTURECHANGED on Mac OS X; there literally is no way to break a grab like that
// even if I invoke the task switcher and switch processes, the mouse grab will still be held until I let go of all buttons
// therefore, no DragBroken()

- (int)doKeyDownUp:(NSEvent *)e up:(int)up
{
	uiAreaKeyEvent ke;

	ke.Key = 0;
	ke.ExtKey = 0;
	ke.Modifier = 0;

	ke.Modifiers = [self parseModifiers:e];

	ke.Up = up;

	if (!uiprivFromKeycode([e keyCode], &ke))
		return 0;
	return [self sendKeyEvent:&ke];
}

- (int)doKeyDown:(NSEvent *)e
{
	return [self doKeyDownUp:e up:0];
}

- (int)doKeyUp:(NSEvent *)e
{
	return [self doKeyDownUp:e up:1];
}



- (void)setupNewTrackingArea
{
	self->libui_ta = [[NSTrackingArea alloc] initWithRect:[self bounds]
		options:(NSTrackingMouseEnteredAndExited |
			NSTrackingMouseMoved |
			NSTrackingActiveAlways |
			NSTrackingInVisibleRect |
			NSTrackingEnabledDuringMouseDrag)
		owner:self
		userInfo:nil];
	[self addTrackingArea:self->libui_ta];
}

- (void)updateTrackingAreas
{
	[self removeTrackingArea:self->libui_ta];
	[self->libui_ta release];
	[self setupNewTrackingArea];
}

- (BOOL)becomeFirstResponder
{
	return [self isEnabled];
}

- (BOOL)isEnabled
{
	return self->libui_enabled;
}

- (void)setEnabled:(BOOL)e
{
	self->libui_enabled = e;
	if (!self->libui_enabled && [self window] != nil)
		if ([[self window] firstResponder] == self)
			[[self window] makeFirstResponder:nil];
}

- (BOOL)isFlipped
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (void)setFrameSize:(NSSize)size
{
	uiArea *a = self->area;

	[super setFrameSize:size];
	if (!a->scrolling)
		// we must redraw everything on resize because Windows requires it
		[self setNeedsDisplay:YES];
}

@end

// wengwengweng

#ifndef D_APP_H
#define D_APP_H

#include <stdbool.h>
#include "math.h"

typedef enum {
	D_KEY_NONE,
	D_KEY_Q,
	D_KEY_W,
	D_KEY_E,
	D_KEY_R,
	D_KEY_T,
	D_KEY_Y,
	D_KEY_U,
	D_KEY_I,
	D_KEY_O,
	D_KEY_P,
	D_KEY_A,
	D_KEY_S,
	D_KEY_D,
	D_KEY_F,
	D_KEY_G,
	D_KEY_H,
	D_KEY_J,
	D_KEY_K,
	D_KEY_L,
	D_KEY_Z,
	D_KEY_X,
	D_KEY_C,
	D_KEY_V,
	D_KEY_B,
	D_KEY_N,
	D_KEY_M,
	D_KEY_1,
	D_KEY_2,
	D_KEY_3,
	D_KEY_4,
	D_KEY_5,
	D_KEY_6,
	D_KEY_7,
	D_KEY_8,
	D_KEY_9,
	D_KEY_0,
	D_KEY_F1,
	D_KEY_F2,
	D_KEY_F3,
	D_KEY_F4,
	D_KEY_F5,
	D_KEY_F6,
	D_KEY_F7,
	D_KEY_F8,
	D_KEY_F9,
	D_KEY_F10,
	D_KEY_F11,
	D_KEY_F12,
	D_KEY_MINUS,
	D_KEY_EQUAL,
	D_KEY_COMMA,
	D_KEY_PERIOD,
	D_KEY_BACKQUOTE,
	D_KEY_SLASH,
	D_KEY_BACKSLASH,
	D_KEY_SEMICOLON,
	D_KEY_QUOTE,
	D_KEY_UP,
	D_KEY_DOWN,
	D_KEY_LEFT,
	D_KEY_RIGHT,
	D_KEY_ESC,
	D_KEY_TAB,
	D_KEY_SPACE,
	D_KEY_BACKSPACE,
	D_KEY_ENTER,
	D_KEY_LBRACKET,
	D_KEY_RBRACKET,
	D_KEY_LSHIFT,
	D_KEY_RSHIFT,
	D_KEY_LALT,
	D_KEY_RALT,
	D_KEY_LMETA,
	D_KEY_RMETA,
	D_KEY_LCTRL,
	D_KEY_RCTRL,
	_D_NUM_KEYS,
} d_key;

typedef enum {
	D_MOUSE_NONE,
	D_MOUSE_LEFT,
	D_MOUSE_RIGHT,
	D_MOUSE_MIDDLE,
	_D_NUM_MOUSE,
} d_mouse;

typedef enum {
	D_CURSOR_ARROW,
	D_CURSOR_EDIT,
	D_CURSOR_WAIT,
	D_CURSOR_HAND,
	D_CURSOR_CROSSHAIR,
	D_CURSOR_SIZEALL,
	D_CURSOR_SIZENWSE,
	D_CURSOR_SIZENESW,
	D_CURSOR_SIZEWE,
	D_CURSOR_SIZENS,
	_D_NUM_CURSORS,
} d_cursor;

typedef enum {
	D_NOVSYNC = (1 << 0),
	D_FULLSCREEN = (1 << 1),
	D_RESIZABLE = (1 << 2),
} d_wflag;

// lifecycle

// init app
void d_init(const char *title, int width, int height);
// run main loop
void d_run(void (*frame)());
// quit app
void d_quit();
// quit with error message
void d_fail(const char *fmt, ...);

// settings / query

// set window vsync
void d_set_vsync(bool b);
// get / set window fullscreen state
bool d_fullscreen();
void d_set_fullscreen(bool b);
// get / set mouse relative state
bool d_mouse_relative();
void d_set_mouse_relative(bool b);
// get / set mouse hidden state
bool d_mouse_hidden();
void d_set_mouse_hidden(bool b);
// get / set window title
const char* d_title();
void d_set_title(const char *title);
// set cursor icon
void d_set_cursor(d_cursor icon);
// get window width / height
int d_width();
int d_height();

// time

// get total run time
float d_time();
// get delta time since last frame
float d_dt();

// input

// check if a key was pressed last frame
bool d_key_pressed(d_key k);
// check if a key was pressed on repeat last frame
bool d_key_rpressed(d_key k);
// check if a key was released last frame
bool d_key_released(d_key k);
// check if a key button is being pressed down
bool d_key_down(d_key k);
// check if a mouse button was pressed last frame
bool d_mouse_pressed(d_mouse m);
// check if a mouse button was released last frame
bool d_mouse_released(d_mouse m);
// check if a mouse button is being pressed down
bool d_mouse_down(d_mouse m);
// check if mouse moved last frame
bool d_mouse_moved();
// get current mouse position
vec2 d_mouse_pos();
// get mouse delta position last frame
vec2 d_mouse_dpos();
// check if scroll wheeled last frame
bool d_scrolled();
// get the scroll wheel value last frame
vec2 d_wheel();
// check if window resized last frame
bool d_resized();
// get text input from last frame
const char* d_tinput();

#endif

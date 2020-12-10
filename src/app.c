// wengwengweng

// TODO: scale strategies

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <dirty/dirty.h>

#if !defined(D_CPU) && !defined(D_GL) && !defined(D_METAL) && !defined(D_WGPU) && !defined(D_D3D11)
#error "must define a blit method (D_CPU, D_GL, D_METAL, D_WGPU, D_D3D11)"
#endif

#if defined(D_METAL) && !defined(D_MACOS) && !defined(D_IOS)
#error "D_METAL is only on macOS or iOS"
#elif defined(D_WGPU) && !defined(D_WEB)
#error "D_WGPU is only on web"
#elif defined(D_D3D11) && !defined(D_WINDOWS)
#error "D_D3D11 is only on windows"
#endif

#if !defined(D_COCOA) && !defined(D_UIKIT) && !defined(D_X11) && !defined(D_CANVAS)
#if defined(D_MACOS)
	#define D_COCOA
#elif defined(D_IOS)
	#define D_UIKIT
#elif defined(D_LINUX)
	#define D_X11
#elif defined(D_WEB)
	#define D_CANVAS
#endif
#endif

#if defined(D_GL)

#define GL_SILENCE_DEPRECATION
#define GLES_SILENCE_DEPRECATION

#if defined(D_MACOS)
	#include <OpenGL/gl.h>
#elif defined(D_IOS)
	#define D_GLES
	#include <OpenGLES/ES2/gl.h>
#elif defined(D_LINUX)
	#include <GL/gl.h>
#elif defined(D_ANDROID)
	#define D_GLES
	#include <GLES2/gl2.h>
#elif defined(D_WINDOWS)
	#include <GL/gl.h>
#elif defined(D_WEB)
	#define D_GLES
	#include <GLES2/gl2.h>
#endif

#endif // D_GL

#if defined(D_METAL)
	#import <Metal/Metal.h>
	#import <MetalKit/MetalKit.h>
#endif

#if defined(D_COCOA)
	#import <Cocoa/Cocoa.h>
#elif defined(D_UIKIT)
	#import <UIKit/UIKit.h>
#elif defined(D_CANVAS)
	#include <emscripten/emscripten.h>
	#include <emscripten/html5.h>
#elif defined(D_X11)
	#include <X11/Xlib.h>
#elif defined(D_WAYLAND)
	#include <wayland-client.h>
#endif

#define D_MAX_TOUCHES 8

void d_gfx_init(const d_desc *desc);
void d_audio_init(const d_desc *desc);
void d_fs_init(const d_desc *desc);
void d_gfx_frame_end();

#if defined(D_COCOA)

@interface DAppDelegate : NSObject<NSApplicationDelegate>
	-(void)loop:(NSTimer*) timer;
@end

@interface DWindowDelegate : NSObject<NSWindowDelegate>
@end

#if defined(D_GL)
@interface DView : NSOpenGLView
#elif defined(D_METAL)
@interface DView : MTKView
#elif defined(D_CPU)
@interface DView : NSView
#endif
@end

#elif defined(D_UIKIT)

#if defined(D_CPU)
@interface DView : UIView
#elif defined(D_METAL)
@interface DView : MTKView
#endif
@end

@interface DAppDelegate : NSObject<UIApplicationDelegate>
	-(void)loop:(NSTimer*) timer;
@end

#endif

typedef enum {
	D_BTN_IDLE,
	D_BTN_PRESSED,
	D_BTN_RPRESSED,
	D_BTN_RELEASED,
	D_BTN_DOWN,
} d_btn;

typedef struct {
	uintptr_t id;
	vec2 pos;
	vec2 dpos;
	d_btn state;
} d_touch_state;

typedef struct {

	d_desc desc;
	color *buf;
	struct timeval start_time;
	float time;
	float dt;
	int width;
	int height;
	int win_width;
	int win_height;
	vec2 mouse_pos;
	vec2 mouse_dpos;
	vec2 wheel;
	d_btn key_states[_D_NUM_KEYS];
	d_btn mouse_states[_D_NUM_MOUSE];
	d_touch_state touches[D_MAX_TOUCHES];
	int num_touches;
	bool resized;
	char char_input;
	float fps_timer;
	int fps;
	bool quit;

#if defined(D_COCOA)
	NSWindow *window;
	DView *view;
#elif defined(D_UIKIT)
	UIWindow *window;
	DView *view;
#elif defined(D_X11)
	Display *display;
	Window window;
#endif

#if defined(D_GL)
	GLuint gl_tex;
#endif

} d_app_ctx;

static d_app_ctx d_app;

void d_present(color *canvas) {
	d_app.buf = canvas;
}

static void d_process_btn(d_btn *b) {
	if (*b == D_BTN_PRESSED || *b == D_BTN_RPRESSED) {
		*b = D_BTN_DOWN;
	} else if (*b == D_BTN_RELEASED) {
		*b = D_BTN_IDLE;
	}
}

static void d_app_init() {

	d_gfx_init(&d_app.desc);
	d_audio_init(&d_app.desc);
	d_fs_init(&d_app.desc);
	gettimeofday(&d_app.start_time, NULL);

	if (d_app.desc.init) {
		d_app.desc.init();
	}

}

static void d_app_frame() {

	if (d_app.desc.frame) {
		d_app.desc.frame();
	}

	d_gfx_frame_end();

	// time
	struct timeval time;
	gettimeofday(&time, NULL);
	float t = (float)(time.tv_sec - d_app.start_time.tv_sec) + (float)(time.tv_usec - d_app.start_time.tv_usec) / 1000000.0;
	d_app.dt = t - d_app.time;
	d_app.time = t;
	d_app.fps_timer += d_app.dt;

	if (d_app.fps_timer >= 1.0) {
		d_app.fps_timer = 0.0;
		d_app.fps = (int)(1.0 / d_app.dt);
	}

	// reset input states
	for (int i = 0; i < _D_NUM_KEYS; i++) {
		d_process_btn(&d_app.key_states[i]);
	}

	for (int i = 0; i < _D_NUM_MOUSE; i++) {
		d_process_btn(&d_app.mouse_states[i]);
	}

	for (int i = 0; i < d_app.num_touches; i++) {
		d_process_btn(&d_app.touches[i].state);
		if (d_app.touches[i].state == D_BTN_IDLE) {
			d_app.touches[i] = d_app.touches[d_app.num_touches - 1];
			d_app.num_touches--;
			i--;
		}
	}

	d_app.wheel.x = 0.0;
	d_app.wheel.y = 0.0;
	d_app.resized = false;
	d_app.mouse_dpos = vec2f(0.0, 0.0);
	d_app.char_input = 0;

}

// -------------------------------------------------------------
// OpenGL
#if defined(D_GL)

static void d_gl_init() {

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &d_app.gl_tex);
	glBindTexture(GL_TEXTURE_2D, d_app.gl_tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		d_width(),
		d_height(),
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		NULL
	);

	glBindTexture(GL_TEXTURE_2D, 0);

}

static void d_gl_blit() {

	glViewport(0, 0, d_win_width(), d_win_height());
	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, d_app.gl_tex);

	glTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0,
		0,
		d_width(),
		d_height(),
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		d_app.buf
	);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(-1, 1);
	glTexCoord2f(1, 0); glVertex2f(1, 1);
	glTexCoord2f(1, 1); glVertex2f(1, -1);
	glTexCoord2f(0, 1); glVertex2f(-1, -1);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glFlush();

}

#endif // D_GL

// -------------------------------------------------------------
// macOS
#if defined(D_COCOA)

static d_key d_cocoa_key(unsigned short k) {
	switch (k) {
		case 0x1D: return D_KEY_0;
		case 0x12: return D_KEY_1;
		case 0x13: return D_KEY_2;
		case 0x14: return D_KEY_3;
		case 0x15: return D_KEY_4;
		case 0x17: return D_KEY_5;
		case 0x16: return D_KEY_6;
		case 0x1A: return D_KEY_7;
		case 0x1C: return D_KEY_8;
		case 0x19: return D_KEY_9;
		case 0x00: return D_KEY_A;
		case 0x0B: return D_KEY_B;
		case 0x08: return D_KEY_C;
		case 0x02: return D_KEY_D;
		case 0x0E: return D_KEY_E;
		case 0x03: return D_KEY_F;
		case 0x05: return D_KEY_G;
		case 0x04: return D_KEY_H;
		case 0x22: return D_KEY_I;
		case 0x26: return D_KEY_J;
		case 0x28: return D_KEY_K;
		case 0x25: return D_KEY_L;
		case 0x2E: return D_KEY_M;
		case 0x2D: return D_KEY_N;
		case 0x1F: return D_KEY_O;
		case 0x23: return D_KEY_P;
		case 0x0C: return D_KEY_Q;
		case 0x0F: return D_KEY_R;
		case 0x01: return D_KEY_S;
		case 0x11: return D_KEY_T;
		case 0x20: return D_KEY_U;
		case 0x09: return D_KEY_V;
		case 0x0D: return D_KEY_W;
		case 0x07: return D_KEY_X;
		case 0x10: return D_KEY_Y;
		case 0x06: return D_KEY_Z;
		case 0x27: return D_KEY_QUOTE;
		case 0x2A: return D_KEY_BACKSLASH;
		case 0x2B: return D_KEY_COMMA;
		case 0x18: return D_KEY_EQUAL;
		case 0x32: return D_KEY_BACKQUOTE;
		case 0x21: return D_KEY_LBRACKET;
		case 0x1B: return D_KEY_MINUS;
		case 0x2F: return D_KEY_PERIOD;
		case 0x1E: return D_KEY_RBRACKET;
		case 0x29: return D_KEY_SEMICOLON;
		case 0x2C: return D_KEY_SLASH;
		case 0x33: return D_KEY_BACKSPACE;
		case 0x7D: return D_KEY_DOWN;
		case 0x24: return D_KEY_ENTER;
		case 0x35: return D_KEY_ESC;
		case 0x7A: return D_KEY_F1;
		case 0x78: return D_KEY_F2;
		case 0x63: return D_KEY_F3;
		case 0x76: return D_KEY_F4;
		case 0x60: return D_KEY_F5;
		case 0x61: return D_KEY_F6;
		case 0x62: return D_KEY_F7;
		case 0x64: return D_KEY_F8;
		case 0x65: return D_KEY_F9;
		case 0x6D: return D_KEY_F10;
		case 0x67: return D_KEY_F11;
		case 0x6F: return D_KEY_F12;
		case 0x7B: return D_KEY_LEFT;
		case 0x3A: return D_KEY_LALT;
		case 0x3B: return D_KEY_LCTRL;
		case 0x38: return D_KEY_LSHIFT;
		case 0x37: return D_KEY_LMETA;
		case 0x7C: return D_KEY_RIGHT;
		case 0x3D: return D_KEY_RALT;
		case 0x3E: return D_KEY_RCTRL;
		case 0x3C: return D_KEY_RSHIFT;
		case 0x36: return D_KEY_RMETA;
		case 0x31: return D_KEY_SPACE;
		case 0x30: return D_KEY_TAB;
		case 0x7E: return D_KEY_UP;
	}
	return D_KEY_NONE;
}

@implementation DAppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)noti {

	NSWindow *window = [[NSWindow alloc]
		initWithContentRect: NSMakeRect(0, 0, d_app.win_width, d_app.win_height)
		styleMask: 0
			| NSWindowStyleMaskTitled
			| NSWindowStyleMaskClosable
			| NSWindowStyleMaskResizable
			| NSWindowStyleMaskMiniaturizable
		backing: NSBackingStoreBuffered
		defer: NO
	];

	d_app.window = window;

	if (d_app.desc.title) {
		[window setTitle:[NSString stringWithUTF8String:d_app.desc.title]];
	}

	[window setAcceptsMouseMovedEvents:YES];
	[window center];
	[window setDelegate:[[DWindowDelegate alloc] init]];
	DView *view = [[DView alloc] init];
	d_app.view = view;
	[window setContentView:view];
	[window makeKeyAndOrderFront:nil];
	[window makeFirstResponder:view];

#if defined(D_GL)

	NSOpenGLContext* ctx = [view openGLContext];

	if (d_app.desc.hidpi) {
		[view setWantsBestResolutionOpenGLSurface:YES];
	}

	[ctx setValues:(int*)&d_app.desc.vsync forParameter:NSOpenGLContextParameterSwapInterval];
	[ctx makeCurrentContext];

	d_gl_init();

#elif defined(D_METAL)
	// TODO
#endif // D_METAL

	d_app_init();

	[NSTimer
		scheduledTimerWithTimeInterval:0.001
		target:self
		selector:@selector(loop:)
		userInfo:nil
		repeats:YES
	];

}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
	return YES;
}
-(void)loop:(NSTimer*)timer {
	[d_app.view setNeedsDisplay:YES];
}
@end

@implementation DWindowDelegate
- (void)windowDidResize:(NSNotification*)noti {
	NSSize size = [d_app.view frame].size;
	d_app.win_width = size.width;
	d_app.win_height = size.height;
	d_app.resized = true;
}
@end

@implementation DView
// - (BOOL)isOpaque {
// 	return YES;
// }
- (BOOL)canBecomeKeyView {
	return YES;
}
- (BOOL)acceptsFirstResponder {
	return YES;
}
- (void)keyDown:(NSEvent*)event {
	d_key k = d_cocoa_key(event.keyCode);
	if (k) {
		if (event.ARepeat) {
			d_app.key_states[k] = D_BTN_RPRESSED;
		} else {
			d_app.key_states[k] = D_BTN_PRESSED;
		}
	}
}
- (void)keyUp:(NSEvent*)event {
	d_key k = d_cocoa_key(event.keyCode);
	if (k) {
		d_app.key_states[k] = D_BTN_RELEASED;
	}
}
- (void)mouseDown:(NSEvent*)event {
	d_app.mouse_states[D_MOUSE_LEFT] = D_BTN_PRESSED;
}
- (void)mouseUp:(NSEvent*)event {
	d_app.mouse_states[D_MOUSE_LEFT] = D_BTN_RELEASED;
}
- (void)rightMouseDown:(NSEvent*)event {
	d_app.mouse_states[D_MOUSE_RIGHT] = D_BTN_PRESSED;
}
- (void)rightMouseUp:(NSEvent*)event {
	d_app.mouse_states[D_MOUSE_RIGHT] = D_BTN_RELEASED;
}
- (void)scrollWheel:(NSEvent*)event {
	d_app.wheel = vec2f(event.scrollingDeltaX, event.scrollingDeltaY);
}
- (void)drawRect:(NSRect)rect {

	NSPoint ompos = [d_app.window mouseLocationOutsideOfEventStream];
	vec2 mpos = vec2f(
		ompos.x * d_app.width / d_app.win_width,
		d_app.height - ompos.y * d_app.height / d_app.win_height
	);
	d_app.mouse_dpos = vec2_sub(mpos, d_app.mouse_pos);
	d_app.mouse_pos = mpos;

	d_app_frame();

#if defined(D_GL)
	d_gl_blit();
#elif defined(D_METAL)
	// TODO
#elif defined(D_CPU)

	int w = d_width();
	int h = d_height();

	CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
	CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
	CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, d_app.buf, w * h * 4, NULL);

	CGImageRef img = CGImageCreate(
		w,
		h,
		8,
		32,
		4 * w,
		rgb,
		kCGBitmapByteOrderDefault | kCGImageAlphaLast,
		provider,
		NULL,
		false,
		kCGRenderingIntentDefault
	);

	CGContextDrawImage(ctx, rect, img);

	CGColorSpaceRelease(rgb);
	CGDataProviderRelease(provider);
	CGImageRelease(img);

#endif // D_CPU

	if (d_app.quit) {
		[NSApp terminate:nil];
	}

}
@end

static void d_cocoa_run(const d_desc *desc) {
	[NSApplication sharedApplication];
	[NSApp setDelegate:[[DAppDelegate alloc] init]];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	[NSApp activateIgnoringOtherApps:YES];
	[NSApp run];
}

#endif // D_COCOA

// -------------------------------------------------------------
// iOS
#if defined(D_UIKIT)

static void d_uikit_touch(d_btn state, NSSet<UITouch*> *tset, UIEvent *event) {

	NSArray<UITouch*> *touches = [tset allObjects];

	if (d_app.desc.touch_is_mouse) {
		if ([touches count] == 1) {
			UITouch *t = touches[0];
			CGPoint pos = [t locationInView:[t view]];
			d_app.mouse_states[D_MOUSE_LEFT] = state;
			d_app.mouse_pos = vec2f(
				pos.x * d_app.width / d_app.win_width,
				pos.y * d_app.height / d_app.win_height
			);
		}
	}

	for (UITouch *touch in touches) {
		uintptr_t id = (uintptr_t)touch;
		CGPoint cpos = [touch locationInView:[touch view]];
		vec2 pos = vec2f(cpos.x, cpos.y);
		switch (state) {
			case D_BTN_PRESSED:
				if (d_app.num_touches < D_MAX_TOUCHES) {
					d_app.touches[d_app.num_touches++] = (d_touch_state) {
						.id = id,
						.pos = pos,
						.dpos = vec2f(0.0, 0.0),
						.state = D_BTN_PRESSED,
					};
				}
				break;
			case D_BTN_DOWN:
				for (int i = 0; i < d_app.num_touches; i++) {
					d_touch_state *t = &d_app.touches[i];
					if (t->id == id) {
						t->dpos = vec2_sub(pos, t->pos);
						t->pos = pos;
					}
				}
				break;
			case D_BTN_RELEASED:
				for (int i = 0; i < d_app.num_touches; i++) {
					d_touch_state *t = &d_app.touches[i];
					if (t->id == id) {
						t->state = D_BTN_RELEASED;
					}
				}
				break;
			default:
				break;
		}
	}

}

@implementation DView
- (void)drawRect:(CGRect)rect {

	d_app_frame();

#if defined(D_GL)
	d_gl_blit();
#elif defined(D_METAL)
	// TODO
#elif defined(D_CPU)

	int w = d_width();
	int h = d_height();

	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
	CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, d_app.buf, w * h * 4, NULL);

	CGImageRef img = CGImageCreate(
		w,
		h,
		8,
		32,
		4 * w,
		rgb,
		kCGBitmapByteOrderDefault | kCGImageAlphaLast,
		provider,
		NULL,
		false,
		kCGRenderingIntentDefault
	);

	// TODO: why is it up side down
	CGContextTranslateCTM(ctx, 0, rect.size.height);
	CGContextScaleCTM(ctx, 1.0, -1.0);
	CGContextDrawImage(ctx, rect, img);

	CGColorSpaceRelease(rgb);
	CGDataProviderRelease(provider);
	CGImageRelease(img);

#endif // D_CPU

}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
	d_uikit_touch(D_BTN_PRESSED, touches, event);
}
- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
	d_uikit_touch(D_BTN_DOWN, touches, event);
}
- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
	d_uikit_touch(D_BTN_RELEASED, touches, event);
}
- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
	d_uikit_touch(D_BTN_RELEASED, touches, event);
}
@end

@implementation DAppDelegate
- (BOOL)application:(UIApplication*)app didFinishLaunchingWithOptions:(NSDictionary*)opt {

	CGRect screen_rect = [[UIScreen mainScreen] bounds];
	UIWindow *window = [[UIWindow alloc] initWithFrame:screen_rect];
	d_app.window = window;
	d_app.win_width = screen_rect.size.width;
	d_app.win_height = screen_rect.size.height;

	UIViewController *view_ctrl = [[UIViewController alloc] init];
	DView *view = [[DView alloc] init];
	d_app.view = view;
	view_ctrl.view = view;
	window.rootViewController = view_ctrl;
	[window makeKeyAndVisible];

	d_app_init();

	[NSTimer
		scheduledTimerWithTimeInterval:0.001
		target:self
		selector:@selector(loop:)
		userInfo:nil
		repeats:YES
	];

	return YES;

}
-(void)loop:(NSTimer*)timer {
	[d_app.view setNeedsDisplay];
}
@end

static void d_uikit_run(const d_desc *desc) {
	UIApplicationMain(0, nil, nil, NSStringFromClass([DAppDelegate class]));
}

#endif // D_UIKIT

// -------------------------------------------------------------
// Linux
#if defined(D_X11)

static void d_x11_run(const d_desc *desc) {

	Display *display = XOpenDisplay(NULL);
	int screen = DefaultScreen(display);
	Visual *visual = DefaultVisual(display, screen);
	GC gc = DefaultGC(display,screen);
	unsigned int depth = DefaultDepth(display, screen);
	d_app.display = display;

	Window window = XCreateSimpleWindow(
		display,
		RootWindow(display, screen),
		0,
		0,
		d_app.win_width,
		d_app.win_height,
		0,
		BlackPixel(display, screen),
		WhitePixel(display, screen)
	);

	Atom del_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(display, window, &del_window, 1);

	XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask);
	XMapWindow(display, window);

	XEvent event;

	d_app_init();

	while (!d_app.quit) {

		XNextEvent(display, &event);

		switch (event.type) {
			case KeyPress:
				break;
			case ClientMessage:
				d_app.quit = true;
				break;
			case Expose: {
				d_app_frame();
				XImage *img = XCreateImage(
					display,
					visual,
					depth,
					ZPixmap,
					0,
					d_app.buf,
					d_app.width,
					d_app.height,
					32,
					0
				);
				// TODO: 0.5x scaled, hidpi?
				// TODO: it's drawing in BGRA
				XPutImage(display, window, gc, img, 0, 0, 0, 0, d_app.win_width, d_app.win_height);
			}
		}

	}

	XDestroyWindow(display, window);
	XCloseDisplay(display);

}

#endif // D_LINUX

// -------------------------------------------------------------
// Web
#if defined(D_CANVAS)

EM_JS(void, d_web_init, (const d_desc *desc), {
	// TODO
});

void d_web_run(const d_desc *desc) {
// 	emscripten_set_main_loop(func, 60, true);
}

#endif // D_CANVAS

void d_run(d_desc desc) {

	d_app.desc = desc;
	float scale = desc.scale ? desc.scale : 1.0;
	d_app.width = desc.width ? desc.width : 640;
	d_app.height = desc.height ? desc.height : 480;
	d_app.win_width = d_app.width * scale;
	d_app.win_height = d_app.height * scale;

#if defined(D_COCOA)
	d_cocoa_run(&desc);
#elif defined(D_UIKIT)
	d_uikit_run(&desc);
#elif defined(D_X11)
	d_x11_run(&desc);
#elif defined(D_CANVAS)
	d_canvas_run(&desc);
#endif

}

void d_quit() {
	d_app.quit = true;
}

void d_fail(const char *fmt, ...) {
	d_quit();
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fflush(stdout);
	fflush(stderr);
	exit(EXIT_FAILURE);
}

void d_assert(bool test, const char *fmt, ...) {
	if (!test) {
		d_quit();
		va_list args;
		va_start(args, fmt);
		vfprintf(stderr, fmt, args);
		va_end(args);
		fflush(stdout);
		fflush(stderr);
		exit(EXIT_FAILURE);
	}
}

float d_time() {
	return d_app.time;
}

float d_dt() {
	return d_app.dt;
}

int d_fps() {
	return d_app.fps;
}

void d_set_fullscreen(bool b) {
#if defined(D_COCOA)
	if (b != d_fullscreen()) {
		[d_app.window toggleFullScreen:nil];
	}
#endif
}

bool d_fullscreen() {
#if defined(D_COCOA)
	return [d_app.window styleMask] & NSWindowStyleMaskFullScreen;
#elif defined(D_UIKIT)
	return true;
#endif
	return false;
}

// TODO
void d_lock_mouse(bool b) {
}

// TODO
bool d_mouse_locked() {
	return false;
}

// TODO
void d_hide_mouse(bool b) {
}

// TODO
bool d_mouse_hidden() {
	return false;
}

void d_set_title(const char *title) {
#if defined(D_COCOA)
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	[d_app.window setTitle:[NSString stringWithUTF8String:title]];
	[pool drain];
#endif
}

bool d_key_pressed(d_key k) {
	return
		d_app.key_states[k] == D_BTN_PRESSED
		;
}

bool d_key_rpressed(d_key k) {
	return
		d_app.key_states[k] == D_BTN_PRESSED
		|| d_app.key_states[k] == D_BTN_RPRESSED
		;
}

bool d_key_down(d_key k) {
	return
		d_app.key_states[k] == D_BTN_PRESSED
		|| d_app.key_states[k] == D_BTN_RPRESSED
		|| d_app.key_states[k] == D_BTN_DOWN
		;
}

bool d_key_released(d_key k) {
	return d_app.key_states[k] == D_BTN_RELEASED;
}

bool d_key_mod(d_kmod kmod) {
	switch (kmod) {
		case D_KMOD_ALT: return d_key_down(D_KEY_LALT) || d_key_down(D_KEY_RALT);
		case D_KMOD_META: return d_key_down(D_KEY_LMETA) || d_key_down(D_KEY_RMETA);
		case D_KMOD_CTRL: return d_key_down(D_KEY_LCTRL) || d_key_down(D_KEY_RCTRL);
		case D_KMOD_SHIFT: return d_key_down(D_KEY_LSHIFT) || d_key_down(D_KEY_RSHIFT);
		default: return false;
	}
	return false;
}

bool d_mouse_pressed(d_mouse k) {
	return d_app.mouse_states[k] == D_BTN_PRESSED;
}

bool d_mouse_released(d_mouse k) {
	return d_app.mouse_states[k] == D_BTN_RELEASED;
}

bool d_mouse_down(d_mouse k) {
	return d_app.mouse_states[k] == D_BTN_DOWN || d_app.mouse_states[k] == D_BTN_PRESSED;
}

int d_width() {
	return d_app.width;
}

int d_height() {
	return d_app.height;
}

int d_win_width() {
	return d_app.win_width;
}

int d_win_height() {
	return d_app.win_height;
}

vec2 d_mouse_pos() {
	return d_app.mouse_pos;
}

vec2 d_mouse_dpos() {
	return d_app.mouse_dpos;
}

bool d_mouse_moved() {
	return d_app.mouse_dpos.x != 0.0 || d_app.mouse_dpos.y != 0.0;
}

bool d_touch_pressed(d_touch t) {
	d_assert(t < D_MAX_TOUCHES, "touch not found: %d\n", t);
	return d_app.touches[t].state == D_BTN_PRESSED;
}

bool d_touch_released(d_touch t) {
	d_assert(t < D_MAX_TOUCHES, "touch not found: %d\n", t);
	return d_app.touches[t].state == D_BTN_RELEASED;
}

bool d_touch_moved(d_touch t) {
	d_assert(t < D_MAX_TOUCHES, "touch not found: %d\n", t);
	return d_app.touches[t].dpos.x != 0.0 || d_app.touches[t].dpos.x != 0.0;
}

vec2 d_touch_pos(d_touch t) {
	d_assert(t < D_MAX_TOUCHES, "touch not found: %d\n", t);
	return d_app.touches[t].pos;
}

vec2 d_touch_dpos(d_touch t) {
	d_assert(t < D_MAX_TOUCHES, "touch not found: %d\n", t);
	return d_app.touches[t].dpos;
}

bool d_resized() {
	return d_app.resized;
}

bool d_scrolled() {
	return d_app.wheel.x != 0.0 || d_app.wheel.y != 0.0;
}

vec2 d_wheel() {
	return d_app.wheel;
}

char d_input() {
	return d_app.char_input;
}

bool d_active() {
#if defined(D_COCOA)
	return [d_app.window isMainWindow];
#endif
	return true;
}


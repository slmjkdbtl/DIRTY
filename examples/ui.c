// wengwengweng

#include <dirty/dirty.h>

static void frame() {

	if (d_key_pressed(D_KEY_ESC)) {
		d_quit();
	}

	d_ui_window_start("hi", vec2f(-100.0, 100.0), 160.0, 240.0);

	d_ui_sliderf("age", 0.0, 0.0, 10.0);

	d_ui_window_end();

}

int main() {
	d_init("ui", 640, 480);
	d_run(frame);
}


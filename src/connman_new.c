/**********************************************************************\
* CONNEW.C - module for BLUEPLATE
*
* Author: Jesse McCure
* License: GPL3
\**********************************************************************/

/* Note: this is a work-in-progress rewrite of connman.c to address concerns
 * noted in the comments of that file */

#include "blueplate.h"
#include "config.h"

#include <dbus/dbus.h>

static const char *rule = "type='signal',interface='net.connman.Manager'";
static Window win;
static DBusWatch *watch;
static int watch_fd;

static int xlib_init() {
	if (!(dpy=XOpenDisplay(0x0))) return 1;
	root = DefaultRootWindow(dpy);
	int scr = DefaultScreen(dpy);
	init_atoms();
	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.background_pixel = background;
	wa.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask;
	win = XCreateWindow(dpy, root, 0, 0, 22, 22, 0, DefaultDepth(dpy,scr),
			InputOutput, DefaultVisual(dpy, scr),
			CWBackPixel | CWBackingStore | CWEventMask, &wa);
	XGCValues val;
	val.background = background;
	int i;
	for (i = 0; i < StateLAST; ++i) {
		val.foreground = conn_color[i];
		gc[i] = XCreateGC(dpy, bars, GCForeground | GCBackground, &val);
	}
}

static int xlib_free() {
	int i;
	for (i = 0; i < StateLAST; ++i)
		XFreeGC(dpy, gc[i]);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

static dbus_bool_t add_watch(DBusWatch *_watch, void *data) {
	if (dbus_watch_get_flags(watch) & DBUS_WATCH_READABLE) {
		watch = _watch;
		watch_fd = dbus_watch_get_fd(watch);
	}
	return TRUE;
}

static void remove_watch(DBusWatch *_watch, void *data) {
	int i, found = 0;
	if (watch != _watch) return;
	watch = NULL;
	watch_fd = 0;
}

DBusConnection *get_connection() {
	DBusConnection *conn;
	conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (!conn) return 1;
	if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch,
			NULL, NULL, NULL)) return 1;
	dbus_bus_add_match(conn, rule, NULL);

	// TODO finish dbus watch setup
	if (dbus_watch_get_enabled(watch))
		;// ...
	//dbus_watch_handle(watch, 0)
	return conn;
}

int get_msg() {
	// TODO:
	// - Parse message
	// - if changed:
	// 	* create pixmap
	// 	* redraw bars
	// 	* set pixmap to win background
	// 	* xflush
	return 0;
}

int connman() {
	DBusConnection *conn = get_connection();
	int state = get_state(conn);
	xlib_init();
	embed_window(win);
	int n, ret;
	int xfd = ConnectionNumber(dpy);
	int nfd = (watch_fd > xfd ? watch_fd : xfd) + 1;
	XEvent ev;
	while (running) {
		FD_ZERO(&fds);
		FD_SET(xfd, &fds);
		FD_SET(watch_fd, &fds);
		select(nfd, &fds, 0, 0, NULL);
		if (watch_fd && FD_ISSET(watcg_fd, &fds))
			get_msg();
		if (FD_ISSET(xfd, &fds)) while (XPending(dpy)) {
			XNextEvent(dpy, &ev);
			if (ev.type == ButtonPress && connman_click[0])
				execvp(connman_click[0], (char * const *) connman_click);
		}
	}
	xlib_free();
	free_connection(&conn);
	return 0;
}


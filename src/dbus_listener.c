/**********************************************************************\
* DBUS_LISTENER.C - module for BLUEPLATE
*
* Author: Jesse McCure
* License: GPL3
\**********************************************************************/

/* Note: this is a work-in-progress rewrite of connman.c to address concerns
 * noted in the comments of that file */

#include "blueplate.h"
#include "config.h"
#include <dbus/dbus.h>

typedef struct Listener {
	const char *name;
	Window win;
	DBusWatch *watch;
	int fd;
	const char *rule;
	const char *signal[];
	const char *icon[];
	const unsigned long int color[];
	const char *click[];
} Listener;

/* config example */

static const char icon_blank[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static Listener listener[] {
	{ "connman", "type='signal',interface='net.connman.Manager'", 0, NULL, 0,
		{ "offline", "idle", "ready", "online", NULL },
		{ icon_blank, icon_blank, icon_blank, icon_blank },
		{ 0x000000, 0x888888, 0xFFDD0E, 0x88FF88 },
		{ "cmst", "--disable-tray-icon", NULL } },
	{ NULL, NULL, 0, NULL, 0, { NULL }, { NULL }, { 0x000000 }, { NULL } },
};

/******************/


static int xlib_init() {
	if (!(dpy=XOpenDisplay(0x0))) return 1;
	root = DefaultRootWindow(dpy);
	int i, scr = DefaultScreen(dpy);
	init_atoms();
	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.background_pixel = background;
	Listener *lstn;
	wa.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask;
	for (lstn = &listener[i=0]; lstn->name; lstn = &listender[++i]) {
		lstn->win = XCreateWindow(dpy, root, 0, 0, 22, 22, 0,
				DefaultDepth(dpy,scr), InputOutput, DefaultVisual(dpy, scr),
				CWBackPixel | CWBackingStore | CWEventMask, &wa);
	}
}

static int xlib_free() {
	int i;
	for (lstn = &listener[i=0]; lstn->name; lstn = &listender[++i])
		XDestroyWindow(dpy, lstn->win);
	XCloseDisplay(dpy);
}

static dbus_bool_t add_watch(DBusWatch *_watch, void *data) {
	Listener *lstn = data;
	if (dbus_watch_get_flags(watch) & DBUS_WATCH_READABLE) {
		lstn->watch = _watch;
		lstn->fd = dbus_watch_get_fd(_watch);
	}
	return TRUE;
}

static void remove_watch(DBusWatch *_watch, void *data) {
	Listener *lstn = data;
	if (lstn->watch != _watch) return;
	lstn->watch = NULL;
	lstn->fd = 0;
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

int dbus_listener() {
	DBusConnection *conn = get_connection();
	int state = get_state(conn);
	xlib_init();
	embed_window(win);
	int n, ret;
	int xfd = ConnectionNumber(dpy);
// TODO loop through listeners
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


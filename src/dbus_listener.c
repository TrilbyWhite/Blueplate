/**********************************************************************\
* DBUS_LISTENER.C - module for BLUEPLATE
*
* Author: Jesse McCure
* License: GPL3
\**********************************************************************/

/* Note: this is a work-in-progress rewrite of connman.c to address concerns
 * noted in the comments of that file */

// COMPILE as standalone test:
// gcc -o dbus_listener dbus_listener.c $(pkg-config dbus-1 x11)

#include "blueplate.h"
#include "config.h"
#include <dbus/dbus.h>





/* excerpt from blueplate.c for testing */
int embed_window(Window embed) {
	while (!(tray=XGetSelectionOwner(dpy, atom[TrayWin]))) sleep(1);
	XEvent ev; memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage; ev.xclient.window = tray;
	ev.xclient.message_type = atom[TrayCmd]; ev.xclient.format = 32;
	ev.xclient.data.l[0] = CurrentTime; ev.xclient.data.l[2] = embed;
	XSendEvent(dpy, tray, False, NoEventMask, &ev);
	XSync(dpy, False);
/*** not used yet:
	Window igw;
	int igd;
	XGetGeometry(dpy, embed, &igw, &igd, &igd, &width, &height, &igd, &igd);
*/
	return 0;
}

int init_atoms() {
	const char *atom_name[AtomNum] = {
		[WinList] = "_NET_CLIENT_LIST",
		[NoDesks] = "_NET_NUMBER_OF_DESKTOPS",
		[CurDesk] = "_NET_CURRENT_DESKTOP",
		[WinDesk] = "_NET_WM_DESKTOP",
		[IconGeo] = "_NET_WM_ICON_GEOMETRY",
		[TrayWin] = "_NET_SYSTEM_TRAY_S0",
		[TrayCmd] = "_NET_SYSTEM_TRAY_OPCODE",
	};
	int i;
	for (i = 0; i < AtomNum; ++i) atom[i] = XInternAtom(dpy, atom_name[i], False);
	return 0;
}
/* end excerpt from blueplate.c for testing */


static int wfd = 0;
static Window win;
static DBusWatch *w;
//const char *rule = "eavesdrop=true,type='signal'";
const char *rule = "eavesdrop=true";
//const char *rule = "eavesdrop=true,type='signal',interface='net.connman.Manager'";

static int xlib_init() {
	if (!(dpy=XOpenDisplay(0x0))) return 1;
	root = DefaultRootWindow(dpy);
	int i, scr = DefaultScreen(dpy);
	init_atoms();
	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.background_pixel = background;
	wa.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask;
	win = XCreateWindow(dpy, root, 0, 0, 22, 22, 0,
			DefaultDepth(dpy,scr), InputOutput, DefaultVisual(dpy, scr),
			CWBackPixel | CWBackingStore | CWEventMask, &wa);
}

static int xlib_free() {
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

static int handle(DBusMessage *msg) {
	if (dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL) {
		printf("sender=%s\n", dbus_message_get_sender(msg));
		printf("\tserial=%u\n", dbus_message_get_serial(msg));
		printf("\tpath=%s\n", dbus_message_get_path(msg));
		printf("\tinterface=%s\n", dbus_message_get_interface(msg));
		printf("\tmember=%s\n", dbus_message_get_member(msg));
	}
	dbus_message_unref(msg);
	return 0;
}

static dbus_bool_t w_add(DBusWatch *watch, void *data) {
	w = watch;
	wfd = dbus_watch_get_unix_fd(w);
	return TRUE;
}

static dbus_bool_t w_del(DBusWatch *watch, void *data) {
	w = NULL;
	wfd = 0;
	return TRUE;
}

//int dbus_listener() {
int main() { // for testing only
	/* dbus setup */
	DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, NULL);
//	DBusConnection *conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (!conn) return 1;
	dbus_bus_add_match(conn, rule, NULL);
	dbus_connection_set_watch_functions(conn,
			(DBusAddWatchFunction) w_add,
			(DBusRemoveWatchFunction) w_del,
			NULL, NULL, NULL);
	DBusMessage *msg = NULL;
if (!wfd) {
	fprintf(stderr,"no watch yet\n");
	return 2;
}
	/* xlib setup */
	xlib_init();
	embed_window(win);
	/* main loop */
	fd_set fds;
	int xfd = ConnectionNumber(dpy);
	int nfd;
	int ret;
	XEvent ev;
int running = 1; // TODO
	while (running) {
		FD_ZERO(&fds);
		FD_SET(xfd, &fds);
		if (wfd) FD_SET(wfd, &fds);
		nfd = (wfd > xfd ? wfd : xfd) + 1;
		ret = select(nfd, &fds, 0, 0, NULL);
		if (wfd && FD_ISSET(wfd, &fds)) {
			dbus_watch_handle(w, DBUS_WATCH_READABLE);
			while ( (msg=dbus_connection_pop_message(conn)) )
				handle(msg);
		}
		if (FD_ISSET(xfd, &fds)) while (XPending(dpy)) {
			XNextEvent(dpy, &ev);
			//if (ev.type == ButtonPress && connman_click[0])
			//	execvp(connman_click[0], (char * const *) connman_click);
		}
	}
	xlib_free();
	// TODO clean up after dbus?
	return 0;
}


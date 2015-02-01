/**********************************************************************\
* CONNMAN.C - module for BLUEPLATE
*
* Author: Andrew J. Bibb, copyright 2015
* License: GPL3
\**********************************************************************/

/*
 *  Suggested improvements (from J. McClure AKA "Trilby"): (see connman_new.c)
 *
 *  1. Replace the nonblocking read with dbus_connection_read_write_dispatch or
 *     similar function to avoid the "sleep-loop" syndrome. (done ajb, 31 jan 15)
 *  2. Remove the pthread dependence by using DBusWatch and a select loop.(done ajb 31 Jan 15)
 *  3. Use array for gcs and color configs.  Initialization can then be done in
 *     a loop through all possible conditionals allowing easier additions in the
 *     future.
 *  4. Consider drawing to pixmap and setting as window background rather than
 *     drawing directly to the window.  This way the expose event can be ignored
 *     as backingstore is already set.
 *  5. Clean up main loop (done: Trilby, 19 Jan 2015)
 */

#include "blueplate.h"
#include "config.h"

#include <dbus/dbus.h>
#include <stdbool.h>

enum {
	// how to draw the rectangles
	Hollow = 0x01,
	Filled = 0x02,
	
	// global connection states
	Undefined = 0x11,
	Offline 	= 0x12,
	Idle			= 0x13,
	Ready			= 0x14,
	Online		= 0x15,
	
};	// enum

// Constants
static const short bar_width = 3;		// width of the rectangle (px)
static const short bar_gap = 2;			// gap between rectangles (px)
static const short bar_height = 13;	// height of the rectangles (px)

// Variables
static Window bars;
GC gc_undefined, gc_offline, gc_idle, gc_ready, gc_online;
int connstate = Undefined;
static DBusConnection* conn;
static DBusError err;
static bool b_havenewstate = FALSE;
static int wfd = 0;	// watch file descriptor
static DBusWatch *w;
const char *rule = "eavesdrop=true,type='signal',interface='net.connman.Manager'";

// Function to draw a single bar
static int draw_bar (int start, GC* gc, int type)
{
	switch (type) {
		case Hollow: 
			XDrawRectangle(dpy, bars, *gc, start * (bar_width + bar_gap), 0, bar_width - 1, bar_height - 1);
			break;
		case Filled:
			XFillRectangle(dpy, bars, *gc, start * (bar_width + bar_gap), 0, bar_width, bar_height);
			break;
		default:
			return 1;
		}	// switch
	
	return 0;
} 

// Function to draw the window we embed in the system tray
static int redraw() {
	// clear the window
	XClearWindow(dpy, bars);
	
	// draw over with new stuff
	switch (connstate) {
		case Offline:
			draw_bar(0, &gc_offline, Hollow);
			draw_bar(1, &gc_offline, Hollow);
			draw_bar(2, &gc_offline, Hollow);
			break;
		case Idle:
			draw_bar(0, &gc_idle, Hollow);
			draw_bar(1, &gc_idle, Filled);
			draw_bar(2, &gc_idle, Hollow);
			break;
		case Ready:
			draw_bar(0, &gc_ready, Filled);
			draw_bar(1, &gc_ready, Hollow);
			draw_bar(2, &gc_ready, Filled);
			break;
		case Online:
			draw_bar(0, &gc_online, Filled);
			draw_bar(1, &gc_online, Filled);
			draw_bar(2, &gc_online, Filled);
			break;
		default:
			draw_bar(0, &gc_undefined, Hollow);
			draw_bar(2, &gc_undefined, Hollow);
			break;
		}	// switch				
	
	// Send our bars onto the XServer
	XFlush(dpy);
	
	return 0;
}

//
static int xlib_init() {
	if (!(dpy=XOpenDisplay(0x0))) return 1;
	root = DefaultRootWindow(dpy);
	int scr = DefaultScreen(dpy);
	init_atoms();
	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.background_pixel = background;
	wa.event_mask = StructureNotifyMask | ExposureMask;
	bars = XCreateWindow(dpy, root, 0, 0, 22, 22, 0, DefaultDepth(dpy,scr),
			InputOutput, DefaultVisual(dpy, scr),
			CWBackPixel | CWBackingStore | CWEventMask, &wa);
	
	XGCValues val;
	val.background = background;
	
	val.foreground = conn_undefined;
	gc_undefined = XCreateGC(dpy, bars, GCForeground | GCBackground, &val);
	
	val.foreground = conn_offline;
	gc_offline = XCreateGC(dpy, bars, GCForeground | GCBackground, &val);	
	
	val.foreground = conn_idle;
	gc_idle = XCreateGC(dpy, bars, GCForeground | GCBackground, &val);
	
	val.foreground = conn_ready;
	gc_ready = XCreateGC(dpy, bars, GCForeground | GCBackground, &val);
	
	val.foreground = conn_online;
	gc_online = XCreateGC(dpy, bars, GCForeground | GCBackground, &val);	
	
	XSelectInput (dpy, bars, ExposureMask | ButtonPressMask); 
}

// DBus watch functions
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

//
static int xlib_free() {
	XFreeGC(dpy, gc_undefined);
	XFreeGC(dpy, gc_offline);
	XFreeGC(dpy, gc_idle);
	XFreeGC(dpy, gc_ready);
	XFreeGC(dpy, gc_online);
	XDestroyWindow(dpy, bars);
	XCloseDisplay(dpy);
}

// Function that receives a dbus message iterator and will drill down
// recursively to retrieve the connman State value.  Based loosely on
// the print_iter function in dbus-print-message.c  In his function we
// only deal with types that can be returned by the getProperties() method
// call or by checkSignaling for the PropertyChanged() signal.
void findState(DBusMessageIter* iter)
{
	do {
		int type = dbus_message_iter_get_arg_type(iter);
		if (type == DBUS_TYPE_INVALID) break;
	
		switch (type) {
			case DBUS_TYPE_BOOLEAN: {	
				break;
			}	// case
			
			case DBUS_TYPE_VARIANT: {	
				DBusMessageIter subiter;
				dbus_message_iter_recurse(iter, &subiter);
				findState(&subiter);
				break;
			}	// case
			
			case DBUS_TYPE_ARRAY: { 
				DBusMessageIter subiter;
				dbus_message_iter_recurse (iter, &subiter);
				findState(&subiter);
				break;
			}	// case
			
			case DBUS_TYPE_DICT_ENTRY: {
				DBusMessageIter subiter;
				dbus_message_iter_recurse (iter, &subiter);
				findState(&subiter);
				break;
			}	// case 
			
			case DBUS_TYPE_STRING: {
				char* val;
				dbus_message_iter_get_basic(iter, &val);
				if (val != NULL) {
					if (strncmp(val, "online", 6) == 0) {connstate = Online; b_havenewstate = TRUE;}
					else if (strncmp(val, "idle", 4) == 0) {connstate = Idle; b_havenewstate = TRUE;}
					else if (strncmp(val, "ready", 5) == 0) {connstate = Ready; b_havenewstate = TRUE;}
					else if (strncmp(val, "offline", 7) == 0) {connstate = Offline; b_havenewstate = TRUE;}
				}	// if we could get_basic and the string is not NULL
				break;
			}	// case
				 
			default:
			break;
		}	// switch
	
	}	while (dbus_message_iter_next (iter));
}

// Called when we dispatch a message from the main loop
static DBusHandlerResult monitor_filter_func (DBusConnection* connection, DBusMessage* msg, void *user_data)
{
	b_havenewstate = FALSE;	// set or unset in findState()
	DBusMessageIter iter;
	
	if (dbus_message_is_signal(msg, "net.connman.Manager", "PropertyChanged")) {
		// read the parameters
		dbus_message_iter_init(msg, &iter);
		findState(&iter);
		
		// redraw if new message
		if (b_havenewstate) redraw();
		
		return DBUS_HANDLER_RESULT_HANDLED;
		}	// if
	
	else
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;	
}


// Function to query connman.Manager to get the current connection state.
// Only called once when we are initializing.  After that monitor the
// PropertiesChanged() signal in a separate thread.
void getState()
{
	DBusMessage* msg;
	DBusMessageIter iter;
	DBusPendingCall* pending;
	
	// create a new method call for getProperties and check for errors
	msg = dbus_message_new_method_call("net.connman", // target for the method call
																		"/", // object to call on
																		"net.connman.Manager", // interface to call on
																		"GetProperties"); // method name
	
	// make sure we could make a connection
	if (NULL == msg) {
		fprintf(stderr, "Unable to make a connection to the Connman net.connman.Manager interface.\n");
		exit(1);
	}
	
	// append arguments (actually there are none to this method)
	dbus_message_iter_init_append(msg, &iter);
	
	// send message and get a handle for a reply
	if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) { // -1 is default timeout
		fprintf(stderr, "Out Of Memory!\n");
		dbus_message_unref(msg);
		exit(1);
	}
	if (NULL == pending) {
		fprintf(stderr, "Pending Call Null\n");
		dbus_message_unref(msg);
		exit(1);
	}
	dbus_connection_flush(conn);

	// free method message
	dbus_message_unref(msg);
	
	// block until we recieve a reply
	dbus_pending_call_block(pending);
	
	// get the reply message
	msg = dbus_pending_call_steal_reply(pending);
	if (NULL == msg) {
		fprintf(stderr, "Reply Null\n");
		exit(1);
	}
	
	// free the pending message handle
	dbus_pending_call_unref(pending);
	
	// get the current connection state
	dbus_message_iter_init(msg, &iter);
	findState(&iter);
	
	// free the reply message
	dbus_message_unref(msg);
}

//
// Main function once we've forked from the Blueplate parent
int connman() {
	xlib_init();
	XEvent ev;
	embed_window(bars);
	fd_set fds;
	int xfd = ConnectionNumber(dpy);	// file descriptor for xlib
	int nfd;													// number of file descriptors
	int ret;													// return value
	
	// initialise the dbus errors
	dbus_error_init(&err);

	// Connect to the bus and check for errors
	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "Error Connecting to DBus (%s)\n", err.message);
		dbus_error_free(&err);
	}
	if (NULL == conn) {
		exit(1);
	}
	
	// Add match rule and set up watch (file descriptor)
	dbus_bus_add_match(conn, rule, NULL);
	dbus_connection_set_watch_functions(conn,
			(DBusAddWatchFunction) w_add,
			(DBusRemoveWatchFunction) w_del,
			NULL, NULL, NULL);
	DBusMessage *msg = NULL;
	if (!wfd) {
		fprintf(stderr,"no watch yet\n");
		exit(2);
	}

	// Add message handler for dbus_connection_dispatch() function
	DBusHandleMessageFunction filter_func = monitor_filter_func;
	if (! dbus_connection_add_filter (conn, filter_func, NULL, NULL)) {
		fprintf (stderr, "Couldn't add filter\n");
		exit (3);
	}
		
	// Get the current connection state
	getState();	
		
	// Main loop
	while (running) {
		// Initialize the file descriptor set, must be done after every select()
		FD_ZERO(&fds);
		FD_SET(xfd, &fds);
		if (wfd) FD_SET(wfd, &fds);
		nfd = (wfd > xfd ? wfd : xfd) + 1;
		redraw();
		
		// select() blocks until one of the file descriptors needs attention
		ret = select(nfd, &fds, 0, 0, NULL);
		
		// if dbus watch needs attention
		if (wfd && FD_ISSET(wfd, &fds)) {
			dbus_watch_handle(w, DBUS_WATCH_READABLE);
			while (dbus_connection_dispatch(conn) != DBUS_DISPATCH_COMPLETE) { 
			}	// while dispatch not complete
		}	// if wfd
		
		// if xevents need attention
		if (FD_ISSET(xfd, &fds)) while (XPending(dpy)) {
			XNextEvent(dpy, &ev);
			// left button open connman_click[0], any other button close
			if (ev.type == ButtonPress) {
				XButtonEvent* xbv = (XButtonEvent*) &ev;
				if (xbv->button == 1 && connman_click[0]) {
					// use a double fork to creating a zombie process
					pid_t pid1;
					pid_t pid2;
					int status;
					if ( (pid1 = fork()) < 0 )
						fprintf (stderr, "Couldn't fork the a child process to execute %s\n", connman_click[0]);
					else {
						if (pid1 > 0)
							waitpid(pid1, &status, NULL);	// parent process, wait for child to exit
						else {													// child process, try to fork again
							if ( (pid2 = fork()) < 0 )
								fprintf (stderr, "Couldn't fork the grandchild to execute %s\n", connman_click[0]);
							else {
								if (pid2 > 0)	
									exit(0);										// kill the child to orphan the grandchild - init takes ownership
								else													// grandchild process		 
									execvp(connman_click[0], (char * const *) connman_click);	
							}	// else fork/pid2
						}	// else child process
					}	// else fork/pid1	
				}	// if button 1
				else running = FALSE;
			}	// if button press			
		}	// if xfd
	}	// whle running
	
	xlib_free();
	return 0;
}


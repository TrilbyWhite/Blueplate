/**********************************************************************\
* CONNMAN.C - module for BLUEPLATE
*
* Author: Andrew J. Bibb, copyright 2015
* License: GPL3
\**********************************************************************/

/*
 *  Suggested improvements (from J. McClure AKA "Trilby"):
 *
 *  1. Replace the nonblocking read with dbus_connection_read_write_dispatch or
 *     similar function to avoid the "sleep-loop" syndrome.
 *  2. Remove the pthread dependence by using DBusWatch and a select loop.
 *  3. Use array for gcs and color configs.  Initialization can then be done in
 *     a loop through all possible conditionals allowing easier additions in the
 *     future.
 *  4. Clean up main loop (done: Trilby, 19 Jan 2015)
 */

#include "blueplate.h"
#include "config.h"

#include <dbus/dbus.h>
#include <stdbool.h>
#include  <pthread.h>

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
Window bars;
GC gc_undefined, gc_offline, gc_idle, gc_ready, gc_online;
int connstate = Undefined;
DBusConnection* conn;
DBusError err;
bool b_havenewstate = FALSE;

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
	
	//XSelectInput(dpy, root, PropertyChangeMask);
	XSelectInput (dpy, bars, ExposureMask | ButtonPressMask); 
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

// Function to check the PropertyChanged signal.  This function is run 
// in a separate thread.  
void* checkSignal(void* arg)
{
	DBusMessage* msg;
	DBusMessageIter iter;
	b_havenewstate = FALSE;	// set or unset in findState()
	
	while (running) {
		// non blocking read of the next available message
		dbus_connection_read_write(conn, 0);
		msg = dbus_connection_pop_message(conn);
		
		// if no message sleep for a while before we look again	
		if (NULL == msg) {
			usleep(5000);
			continue;
		} 
			
		// check if the message is a signal from the correct interface and with the correct name
		if (dbus_message_is_signal(msg, "net.connman.Manager", "PropertyChanged")) {
			// read the parameters
			dbus_message_iter_init(msg, &iter);
			findState(&iter);
		}	// if
	
		// free the message
		dbus_message_unref(msg);
		
		// redraw if new message
		if (b_havenewstate) redraw();
	
	}	// checkSignal loop
	
	return NULL;
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
		exit(1);
	}
	if (NULL == pending) {
		fprintf(stderr, "Pending Call Null\n");
		exit(1);
	}
	dbus_connection_flush(conn);

	// free message
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
	
	// free the message
	dbus_message_unref(msg);
}

//
// Main function once we've forked from the Blueplate parent
int connman() {
	xlib_init();
	XEvent ev;
	embed_window(bars);

	// initialise the dbus errors
	dbus_error_init(&err);

	// connect to the bus and check for errors
	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "Error Connecting to DBus (%s)\n", err.message);
		dbus_error_free(&err);
	}
	if (NULL == conn) {
		exit(1);
	}

	// Add a rule for the dbus signal we want to see
	dbus_bus_add_match(conn, "type='signal',interface='net.connman.Manager'", &err); // see signals from the given interface
	dbus_connection_flush(conn);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "Match Error (%s)\n", err.message);
		exit(1);
	}
		
	// initialize the bars
	getState();
	
	// start up the thread to monitor DBus
	int thread_err;
	pthread_t tid;
	thread_err = pthread_create(&tid, NULL, &checkSignal, NULL);
	if (thread_err != 0) {
		fprintf(stderr, "Error Creating the getState Thread: (%s)\n", strerror(thread_err) );
		exit(1);
	}

	// start event loop
	while (running && !XNextEvent(dpy, &ev)) {
		if (ev.type == Expose) redraw();
		else if (ev.type == ButtonPress && connman_click[0] && fork() == 0)
			execvp(connman_click[0], (char * const *) connman_click);
	}
	xlib_free();
	return 0;
}


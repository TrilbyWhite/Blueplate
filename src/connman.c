/**********************************************************************\
* CONNMAN.C - module for BLUEPLATE
*
* Author: Andrew J. Bibb, copyright 2015
* License: GPL3
\**********************************************************************/

/*
 *  This is at variance with the master branch as follows:
 * 	cmst branch uses poll() in lieu of select.
 */

#include "blueplate.h"
#include "config.h"

#include <dbus/dbus.h>
#include <stdbool.h>
#include <poll.h>

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



// Variables
static Window bars;
GC gc_undefined, gc_offline, gc_idle, gc_ready, gc_online;
int connstate = Undefined;
int offlinemode = -1;
static DBusConnection* cnxn_system;
static DBusConnection* cnxn_session;
static DBusError err;
static bool b_havenewstate = FALSE;
static int wfd = 0;	// watch file descriptor
static DBusWatch *w;
const char *rule = "eavesdrop=true,type='signal',interface='net.connman.Manager'";

// Function to draw a single bar
static int draw_bar (int start, GC* gc, int type)
{
	const short bar_height = 3 * bar_width + 2 * bar_gap;
	
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
	wa.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask;
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
	
	XSelectInput (dpy, root, PropertyChangeMask);
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


// Function to send a notification to the notification daemon
// Currently only notify on change of OfflineMode property, called from toggleOfflineMode()
static void sendNotification()
{
	// constants and variables for use here
	const short appiconsize = 25;
	const short summarysize = 25;
	const short bodysize = 67;
	
	// make sure we can send a notification
	if (cnxn_session == NULL) return;
	
	// constants and variables for display
	const char* app_name = "Blueplate";	
	static dbus_uint32_t notify_id = 0;	
	char* app_icon = malloc (appiconsize * sizeof(char));
	char* summary = malloc (summarysize * sizeof(char));
	char* body = malloc (bodysize * sizeof(char));														
	const dbus_int32_t expire = -1;
	
	// hints
	const char* key00 = "urgency";
	unsigned char val00= 1;
	const char* key01 = "category";
	const char* val01 = "device";	

	// create the summary, app_icon, body text and urgency level
	if (offlinemode) {
		snprintf(app_icon, 16, "network-offline");
		snprintf(summary, 22, "OfflineMode Engaged\n");
		snprintf(body, 59, "All network devices are powered off, now in Airplane mode.");
	}
	else {
		snprintf(app_icon, 25, "network-transmit-receive");
		snprintf(summary, 25, "OfflineMode Disengaged\n");
		snprintf(body, 67, "Power has been restored to all previously powered network devices.");
	}	 
		
	// create a new method call for Notify 
	DBusMessage* msg;
	DBusMessageIter iter00, iter01, iter02, iter03;
	msg = dbus_message_new_method_call("org.freedesktop.Notifications", // target for the method call
																		"/org/freedesktop/Notifications", // object to call on
																		"org.freedesktop.Notifications", // interface to call on
																		"Notify"); // method name																															
	
	// assemble the arguments for the method call	
	dbus_message_iter_init_append(msg, &iter00);
	dbus_message_iter_append_basic(&iter00, DBUS_TYPE_STRING, &app_name);
	dbus_message_iter_append_basic(&iter00, DBUS_TYPE_UINT32, &notify_id);
	dbus_message_iter_append_basic(&iter00, DBUS_TYPE_STRING, &app_icon);
	dbus_message_iter_append_basic(&iter00, DBUS_TYPE_STRING, &summary);
	dbus_message_iter_append_basic(&iter00, DBUS_TYPE_STRING, &body);
	
	dbus_message_iter_open_container(&iter00, DBUS_TYPE_ARRAY, "s", &iter01);
	dbus_message_iter_close_container(&iter00, &iter01);
	
	char sig[5];
	sig[0] = DBUS_DICT_ENTRY_BEGIN_CHAR;
	sig[1] = DBUS_TYPE_STRING;
	sig[2] = DBUS_TYPE_VARIANT;
	sig[3] = DBUS_DICT_ENTRY_END_CHAR;
	sig[4] = '\0';
	dbus_message_iter_open_container(&iter00, DBUS_TYPE_ARRAY, sig, &iter01);	
		dbus_message_iter_open_container(&iter01, DBUS_TYPE_DICT_ENTRY, NULL, &iter02);
		dbus_message_iter_append_basic(&iter02, DBUS_TYPE_STRING, &key00);
			dbus_message_iter_open_container(&iter02, DBUS_TYPE_VARIANT, "y", &iter03);
			dbus_message_iter_append_basic(&iter03, DBUS_TYPE_BYTE, &val00);
			dbus_message_iter_close_container(&iter02, &iter03);
		dbus_message_iter_close_container(&iter01, &iter02);
	
		dbus_message_iter_open_container(&iter01, DBUS_TYPE_DICT_ENTRY, NULL, &iter02);
		dbus_message_iter_append_basic(&iter02, DBUS_TYPE_STRING, &key01);
			dbus_message_iter_open_container(&iter02, DBUS_TYPE_VARIANT, "s", &iter03);
			dbus_message_iter_append_basic(&iter03, DBUS_TYPE_STRING, &val01);
			dbus_message_iter_close_container(&iter02, &iter03);
		dbus_message_iter_close_container(&iter01, &iter02);
	dbus_message_iter_close_container(&iter00, &iter01);
	
	dbus_message_iter_append_basic(&iter00, DBUS_TYPE_INT32, &expire);
	
	// send the message, create a reply and use to find the notify_id number
	DBusPendingCall* pending;
	if (! dbus_connection_send_with_reply (cnxn_session, msg, &pending, -1)) {
		fprintf(stderr, "Out of memory for reply message.\n");	
		dbus_message_unref(msg);
	}
	else {
		dbus_message_unref(msg);
		dbus_pending_call_block(pending);
		msg = dbus_pending_call_steal_reply(pending);
		dbus_pending_call_unref(pending);
		DBusMessageIter args;
		if (dbus_message_iter_init(msg, &args)) {
			if (DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type(&args) ) {
				dbus_message_iter_get_basic(&args, &notify_id);
			}	// if uint32
		}	// if iter has args
		dbus_message_unref(msg);
	}	// else
	
	free(body);
	free(summary);
	free(app_icon);
	return;
}

// Function to toggle the offline (airplane) mode property  
static void toggleOfflineMode()
{
	DBusMessage* msg;
	DBusMessageIter iter00, iter01;
	const char* keyname = "OfflineMode";	
	
	// create a new method call to SetProperty and check for errors
	msg = dbus_message_new_method_call("net.connman", // target for the method call
																		"/", // object to call on
																		"net.connman.Manager", // interface to call on
																		"SetProperty"); // method name
	
	// make sure we could make a connection
	if (NULL == msg) {
		fprintf(stderr, "Unable to make a connection to the Connman net.connman.Manager interface for SetProperty.\n");
		return;
	}
	
	// append arguments
	dbus_message_iter_init_append(msg, &iter00);
	dbus_message_iter_append_basic(&iter00, DBUS_TYPE_STRING, &keyname);
		dbus_message_iter_open_container(&iter00, DBUS_TYPE_VARIANT, "b", &iter01);
		dbus_bool_t newmode = offlinemode ? FALSE : TRUE;	
		dbus_message_iter_append_basic(&iter01, DBUS_TYPE_BOOLEAN, &newmode);
		dbus_message_iter_close_container(&iter00, &iter01);
	
	// send message 
	if (!dbus_connection_send (cnxn_system, msg, NULL) ) { 
		fprintf(stderr, "dbus connection send error!\n");
		dbus_message_unref(msg);
		return;
	}
	dbus_connection_flush(cnxn_system);
		
	// free method message
	dbus_message_unref(msg);		
}

// Function that receives a dbus message iterator and will drill down
// recursively to retrieve the connman State value.  Based loosely on
// the print_iter function in dbus-print-message.c  In his function we
// only deal with types that can be returned by the getProperties() method
// call or by checkSignaling for the PropertyChanged() signal.
static void findState(DBusMessageIter* iter)
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
					else if (strncmp(val, "OfflineMode", 11) == 0) {
						int oldmode = offlinemode;
						if (dbus_message_iter_next(iter) ) {
							DBusMessageIter subiter;
							dbus_message_iter_recurse (iter, &subiter);
							dbus_message_iter_get_basic(&subiter, &offlinemode);
							if (oldmode >= 0 && oldmode != offlinemode ) sendNotification();
						}	// if iter has next
					}	// if OfflineMode
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
static void getState()
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
	if (!dbus_connection_send_with_reply (cnxn_system, msg, &pending, -1)) { // -1 is default timeout
		fprintf(stderr, "Out Of Memory!\n");
		dbus_message_unref(msg);
		exit(1);
	}
	if (NULL == pending) {
		fprintf(stderr, "Pending Call Null\n");
		dbus_message_unref(msg);
		exit(1);
	}
	dbus_connection_flush(cnxn_system);

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
	fd_set fds;
	int xfd = ConnectionNumber(dpy);	// file descriptor for xlib
	struct pollfd pfd[2];						// poll fd structure
	const uint statemask = (1 << 2);	// (shift << 0, ctrl << 2, alt << 3)  
	
	// initialise the dbus errors
	dbus_error_init(&err);

	// Connect to the system bus and check for errors (communicate with Connman)
	cnxn_system = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "Error Connecting to System DBus (%s)\n", err.message);
		dbus_error_free(&err);
	}
	if (NULL == cnxn_system) {
		exit(1);
	}
	
	// Connect to the session bus and check for errors (for notifications)
	cnxn_session = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "Error Connecting to Session DBus (%s)\n", err.message);
		dbus_error_free(&err);
	}
	
	// Add match rule and set up watch (file descriptor)
	dbus_bus_add_match(cnxn_system, rule, NULL);
	dbus_connection_set_watch_functions(cnxn_system,
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
	if (! dbus_connection_add_filter (cnxn_system, filter_func, NULL, NULL)) {
		fprintf (stderr, "Couldn't add filter\n");
		exit (3);
	}
	
	// Poll structures 
	pfd[0].fd = xfd;
	pfd[0].events = POLLIN;
	if (wfd) {
		pfd[1].fd = wfd;
		pfd[1].events = POLLIN;
	}
	// Get the current connection state
	getState();	
		
	// Main loop
	embed_window(bars);
	redraw();
	while (running && poll(pfd, sizeof(pfd) / sizeof(pfd[0]), -1) ) {
	
		// if dbus watch needs attention
		if (pfd[1].revents & POLLIN) {
			dbus_watch_handle(w, DBUS_WATCH_READABLE);
			while (dbus_connection_dispatch(cnxn_system) != DBUS_DISPATCH_COMPLETE) { 
			}	// while dispatch not complete
		}	// if wfd
		
		// if xevents need attention
		if (pfd[0].revents & POLLIN) while (XPending(dpy)) {
			XNextEvent(dpy, &ev);
			if (ev.xany.window == bars && ev.type == UnmapNotify) embed_window(bars);
			// process button press
			else if (ev.type == ButtonPress) {
				XButtonEvent* xbv = (XButtonEvent*) &ev;
				
				if (xbv->button == 1 && (xbv->state & statemask) )
					running = FALSE;
				
				else if (xbv->button == 1 ) {
					if (connman_click[0]) {
						pid_t pid1;
						if ( (pid1 = fork()) < 0 )
							fprintf (stderr, "Couldn't fork the a child process to execute %s\n", connman_click[0]);
						else {
							if (pid1 == 0)
								execvp(connman_click[0], (char * const *) connman_click);
						}	// else we could fork
					}	// else if connman_click[0]
				}	// if button 1 pressed
				
				else if (xbv->button == 3 ) toggleOfflineMode();
				 
			}	// else if button press	
		}	// while xpending
	
		redraw();
	}	// while running
	xlib_free();
	return 0;
}


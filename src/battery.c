/**********************************************************************\
* BATTERY.C - module for BLUEPLATE
r*
* Author: Andrew Bibb, copyright 2014-2015
* License: GPLv3
\**********************************************************************/

#include "blueplate.h"
#include "config.h"

#include <poll.h>
#include <libudev.h>

enum {
	// states
	Discharging = 0x01,
	Charging = 0x02,
	Full = 0x03,	
	
	// show health
	Health_No = 0x00,
	Health_Yes = 0x01,
};	// enum

static int xlib_init() {
	if (!(dpy=XOpenDisplay(0x0))) return 1;
	root = DefaultRootWindow(dpy);
	int scr = DefaultScreen(dpy);
	init_atoms();
	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.event_mask = StructureNotifyMask;
	
	int i;
	for (i = 0; i < sizeof(bat)/sizeof(bat[0]); ++i) {
		bat[i].win = XCreateWindow(dpy, root, 0, 0, 32, 32, 0,
			DefaultDepth(dpy,scr), InputOutput, DefaultVisual(dpy, scr),
			CWBackingStore | CWEventMask, &wa);
	
	XGCValues val;
	val.foreground = bat[i].fill_color;
	val.background = background;
	val.fill_style = FillSolid;
	bat[i].gc = XCreateGC(dpy, bat[i].win, GCForeground | GCBackground | GCFillStyle, &val);	
	
	XSelectInput (dpy, bat[i].win, ExposureMask | ButtonPressMask); 
	}

return 0;
}

static int xlib_free() {
	int i;
	for (i = 0; i < sizeof(bat)/sizeof(bat[0]); ++i) 
		XDestroyWindow(dpy, bat[i].win);	
	XCloseDisplay(dpy);
}

static int rescan() {
	static unsigned int show = 0;
	unsigned int prev = show;
	short batt, i;
	short n_bat; // number of batteries discovered during scan, not the number of battery icons defined in config.h
	struct udev* udev;
	struct udev_device* dev;	
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_enumerate *enumerate;	
	
	// Create a udev object 
	udev = udev_new();
	if (!udev) {
		fprintf(stderr, "Can't create udev object in function rescan\n");
		exit(1);
	}	
	
	// Get all devices in power_supply (may include things other than batteries)
	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "power_supply");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate); 	

	// Run through the device list counting batteries
	n_bat = 0;
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char* path;
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		if (strncmp (udev_device_get_sysname(dev), "BAT", 3) == 0 ) 
			++n_bat;		
		udev_device_unref(dev);
	}	// foreach	
	// Make sure there are enough icons to cover the number of batteries found
	if (n_bat > sizeof(bat)/sizeof(bat[0])) {
		fprintf(stderr, "Error: found %i batteries, but only %i icons are defined\n",n_bat, sizeof(bat)/sizeof(bat[0]));
		udev_unref(udev);
		xlib_free();
		exit(1);
	}

	// Run through each battery icon and try to get data about it
	for (i = 0; i < sizeof(bat)/sizeof(bat[0]); ++i) {	
		batt = 0;
		int e_state = Discharging;
		float e_design = 0.0;
		float e_full = 0.0;
		float e_now = 0.0;
		const char* pchar;
		
		// Match batteries in device list with icons
		if (i < n_bat) {	
			udev_list_entry_foreach(dev_list_entry, devices) {
				int j = 0;
				const char* path;
				path = udev_list_entry_get_name(dev_list_entry);
				dev = udev_device_new_from_syspath(udev, path);
				if (strncmp (udev_device_get_sysname(dev), "BAT", 3) == 0 && i == j) {
					// read the values
					e_design = atof(udev_device_get_sysattr_value(dev, "energy_full_design"));
					e_full = atof(udev_device_get_sysattr_value(dev, "energy_full"));
					e_now = atof(udev_device_get_sysattr_value(dev, "energy_now"));
					pchar = udev_device_get_sysattr_value(dev, "status");
					if (strncmp (pchar, "Charging", 8) == 0 ) e_state = Charging;
					else if (strncmp (pchar, "Full", 4) == 0) e_state = Full;
					++j;
					if (e_design == 0.0) fprintf(stderr, "Error: could not determine the design capacity of battery %i\n", i);
					else if (e_full == 0.0 ) fprintf(stderr, "Error: could not determine the full capacity of battery %i\n", i);
					else batt = 1;
				}	//	if BAT && i == j 
			udev_device_unref(dev);
			}	// foreach dev_list_entry in devices
		}	// if (j < n_bat)
					
		// prepare the icons				
		if (batt) {
			show |= (1<<i);			
			int scr = DefaultScreen(dpy);
			
			// In this module draw everything on a pixmap and then put the pixmap on the window
			Pixmap pix;
			
			if (bat[i].health == Health_Yes && e_design != 0.0 ) {	
				// create a blank pixmap
				char bg[battery_icon_size.x * battery_icon_size.y];
				int j;
				for (j = 0; j < sizeof(bg); ++j) 
					bg[j] = 0x00;
				pix = XCreatePixmapFromBitmapData(dpy, root, bg, battery_charging_size.x, battery_charging_size.y,
					bat[i].outline_color, background, DefaultDepth(dpy,scr));		
					
				XFillArc(dpy, pix, bat[i].gc, 0, 0, battery_icon_size.x, battery_icon_size.y, 90 * 64, (int)(360.0 * 64.0 * e_full / e_design) );
			} 
				
			else if (e_state == Charging) {
				pix = XCreatePixmapFromBitmapData(dpy, root,
					(char *) battery_charging_data, battery_charging_size.x, battery_charging_size.y,
					bat[i].outline_color, background, DefaultDepth(dpy,scr));
			}	// if charging
			
			else if (e_state == Full) {
				pix = XCreatePixmapFromBitmapData(dpy, root,
					(char *) battery_full_data, battery_charging_size.x, battery_charging_size.y,
					bat[i].fill_color, background, DefaultDepth(dpy,scr));
			}	// else full
			
			else if (e_full != 0.0) {
				pix = XCreatePixmapFromBitmapData(dpy, root,
					(char *) battery_icon_data, battery_icon_size.x, battery_icon_size.y,
					bat[i].outline_color, background, DefaultDepth(dpy,scr));			
				int fillheight = (int) (battery_fill_rect.height * e_now / e_full + 0.5f);
				XFillRectangle(dpy, pix, bat[i].gc, battery_fill_rect.x, battery_fill_rect.y + (battery_fill_rect.height - fillheight), battery_fill_rect.width, fillheight);
			}	// else discharging	
				
			XClearWindow(dpy, bat[i].win);
			XSetWindowBackgroundPixmap(dpy, bat[i].win, pix);
			XFreePixmap(dpy, pix);
			embed_window(bat[i].win);
		}	// if batt
		
		else {							
			show &= ~(1<<i);
			XUnmapWindow(dpy, bat[i].win);
			XReparentWindow(dpy, bat[i].win, root, 0, 0);
		}	// else
		
	}	// for each icon
	
	// Cleanup, refresh icons if necessary and return
	udev_enumerate_unref(enumerate);
	udev_unref(udev);
	if (show != prev) XFlush(dpy);
	return 0;		
}

int battery() {	
	struct udev* udev;
	struct udev_monitor* mon;
	struct udev_device* dev;		
	struct pollfd pfd[2];
	int mfd;		// monitor file descriptor	
	const int timeout = 5 * 60 * 1000;	// interval to check battery if nothing else has occured (milliseconds)
		
	// Initialize xlib
	xlib_init();
	XEvent ev;
	int xfd = ConnectionNumber(dpy);	// file descriptor for xlib

	// Create a udev object 
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev object in main\n");
		xlib_free();
		exit(1);
	}
	
	// Setup power_supply monitoring	
	mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply", NULL);
	udev_monitor_enable_receiving(mon);
	mfd = udev_monitor_get_fd(mon);
	
	// Find batteries and get the data about each
	rescan();
		
	// Poll structures 
	pfd[0].fd = mfd;
	pfd[0].events = POLLIN;
	pfd[1].fd = xfd;
	pfd[1].events = POLLIN;

	// Event loop
	while (running) {
		// process timeout
		if (poll(pfd, sizeof(pfd) / sizeof(pfd[0]), timeout) == 0) {
			rescan();
		}  		
			
		// process udev changes
		else if (pfd[0].revents & POLLIN) {
			dev = udev_monitor_receive_device(mon);
			if (strncmp (udev_device_get_sysname(dev), "BAT", 3) == 0 ) 
				rescan();
			udev_device_unref(dev);	
		}	// if pfd[0]
		
		// process x event
		else if (pfd[1].revents & POLLIN) {
			while (XPending(dpy)) {
				XNextEvent(dpy, &ev);
				if (ev.type == ButtonPress) {
					XButtonEvent* xbv = (XButtonEvent*) &ev;
					int i;
					for (i = 0; i < sizeof(bat)/sizeof(bat[0]); ++i) {
						if (xbv->window == bat[i].win ) {
							bat[i].health = (bat[i].health == Health_No ? Health_Yes : Health_No);
						}	// if
					}	// for each bat[i]
					rescan();
				}	// if ButtonPress
			}	// while XPending
		}	// if pfd[1]

	}	// while
	
	// Cleanup
	udev_unref(udev);
	xlib_free();
	return 0;
}	

/**********************************************************************\
* BATTERY.C - module for BLUEPLATE
*
* Author: Andrew Bibb, copyright 2014-2015
* License: GPLv3
\**********************************************************************/

#include "blueplate.h"
#include "config.h"

#include <poll.h>

enum {
	// states
	Discharging = 0x01,
	Charging = 0x02,
	Full = 0x03,	
	
	// show health
	Health_No = 0x00,
	Health_Yes = 0x01,
};	// enum

static int rescan() {
	static unsigned int show = 0;
	unsigned int prev = show;
	
	FILE *fp;	
	int i, batt;
	for (i = 0; bat[i].battpath; ++i) {
		batt = 0;
		float e_design = 0.0;
		float e_full = 0.0;
		float e_now = 0.0;
		int e_state = Discharging;
		
		char* uevt = malloc(strlen(bat[i].battpath) + 8);
		strcpy(uevt, bat[i].battpath);
		strcat(uevt, "/uevent");
		
		if( (fp = fopen(uevt, "r")) ) {
			// Read the file and extract the data we want
			char line [512];
			batt = 1;
			const char delim[2] = "=";
			char* pchar;
			while(fgets (line, sizeof (line), fp) ) {
				pchar = strtok(line, delim);
				while (pchar != NULL) {
					if (strcmp(pchar, "POWER_SUPPLY_ENERGY_FULL_DESIGN") == 0 ) {
						pchar = strtok(NULL, delim);
						e_design = atof(pchar);
					}	// if
					else if (strcmp(pchar, "POWER_SUPPLY_ENERGY_FULL") == 0 ) {
						pchar = strtok(NULL, delim);
						e_full = atof(pchar);
					}	// else if
					else if (strcmp(pchar, "POWER_SUPPLY_ENERGY_NOW") == 0 ) {
						pchar = strtok(NULL, delim);
						e_now = atof(pchar);
					}	// else if
					else if (strcmp(pchar, "POWER_SUPPLY_STATUS") == 0 ) {
						pchar = strtok(NULL, delim);
						if (strncmp (pchar, "Charging", 8) == 0 ) e_state = Charging;
						else if (strncmp (pchar, "Full", 4) == 0) e_state = Full;
					}	// else if
					break;
				}	// pchar while
			}	// fgets while	
			
			fclose(fp);
			free(uevt);
		} // if file open
		
		if (batt) {
			show |= (1<<i);			
			int scr = DefaultScreen(dpy);
			// In this module draw everything on a pixmap and then put the pixmap on the window
			Pixmap pix;
			
			if (bat[i].health == Health_Yes ) {	
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
			
			else {
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
	}	// for bat[]

	if (show != prev) XFlush(dpy);
	return 0;		
}

static int xlib_init() {
	if (!(dpy=XOpenDisplay(0x0))) return 1;
	root = DefaultRootWindow(dpy);
	int scr = DefaultScreen(dpy);
	init_atoms();
	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.event_mask = StructureNotifyMask;
	
	int i;
	for (i = 0; bat[i].battpath; ++i) {
	bat[i].win = XCreateWindow(dpy, root, 0, 0, 32, 32, 0,
			DefaultDepth(dpy,scr), InputOutput, DefaultVisual(dpy, scr),
			CWBackPixmap | CWBackingStore | CWEventMask, &wa);
	
	XGCValues val;
	val.foreground = bat[i].fill_color;
	val.background = background;
	val.fill_style = FillSolid;
	bat[i].gc = XCreateGC(dpy, bat[i].win, GCForeground | GCBackground | GCFillStyle, &val);	
	
	XSelectInput (dpy, bat[i].win, ExposureMask | ButtonPressMask); 
	}
}

static int xlib_free() {
	int i;
	for (i = 0; bat[i].battpath; ++i)
		XDestroyWindow(dpy, bat[i].win);
	XCloseDisplay(dpy);
}

int battery() {
	struct pollfd pfd[2];
	int i, fd;
	char buf[4096];

	// Initialize xlib
	xlib_init();
	XEvent ev;
	int xfd = ConnectionNumber(dpy);	// file descriptor for xlib
	
	// Add inotify watches
	fd = inotify_init();
	for (i = 0; bat[i].battpath; ++i)
		inotify_add_watch(fd, bat[i].battpath, INOTIFY_FLAGS);
	rescan();
	
	// Poll structures 
	pfd[0].fd = fd;
	pfd[0].events = POLLIN;
	pfd[1].fd = xfd;
	pfd[1].events = POLLIN;
	
	while (running && poll(pfd, sizeof(pfd) / sizeof(pfd[0]), -1) ) {		
		if (pfd[0].revents & POLLIN) {
			read (pfd[0].fd, buf, sizeof(buf) );	
			rescan();
		}	// if
		
		if (pfd[1].revents & POLLIN) {
			while (XPending(dpy)) {
				XNextEvent(dpy, &ev);
				if (ev.type == ButtonPress) {
					XButtonEvent* xbv = (XButtonEvent*) &ev;
					for (i = 0; bat[i].battpath; ++i) {
						if (xbv->window == bat[i].win ) {
							bat[i].health = (bat[i].health == Health_No ? Health_Yes : Health_No);
						}	// if
					}	// for each bat[i]
					rescan();
				}	// if ButtonPress
			}	// while XPending
		}	// if pfd[1]
	
	}	// while
	
	close (pfd[0].fd);
	xlib_free();
	return 0;
}	

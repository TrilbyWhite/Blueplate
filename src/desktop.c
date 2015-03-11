/**********************************************************************\
* DESKTOP.C - module for BLUEPLATE
*
* Author: Jesse McClure, copyright 2014-2015
* License: GPLv3
\**********************************************************************/

#include "blueplate.h"
#include "config.h"

static Window desks;
static GC cgc, ogc, dgc;

static int redraw() {
	int i; long n;
	unsigned long count, ul;
	unsigned char *d1 = NULL;
	Atom aa; GC *gc = NULL;
	if ((XGetWindowProperty(dpy, root, atom[CurDesk], 0L, 32, False,
			XA_CARDINAL, &aa, &i, &ul, &ul, &d1) == Success && d1)) {
		n = *(long *) d1;
		XFree(d1);
	}
	for (i = 0; i < sizeof(desk)/sizeof(desk[0]); i++) {
		if (i == n) gc = &cgc;
		else if (desk[i].status) gc = &ogc;
		else gc = &dgc;
		XFillRectangle(dpy, desks, *gc, desk[i].x, desk[i].y, desk[i].w, desk[i].h);
	}
	return 0;
}

static int rehash() {
	int i, j; long n;
	unsigned long count, ul;
	unsigned char *d1 = NULL, *d2 = NULL;
	Atom aa;
	if (!(XGetWindowProperty(dpy, root, atom[WinList], 0L, sizeof(aa),
			False, XA_WINDOW, &aa, &i, &count, &ul, &d1) == Success && d1))
		return 1;
	Window *win = (Window *) d1;
	for (i = 0; i < sizeof(desk)/sizeof(desk[0]); ++i) desk[i].status = 0;
	for (j = 0; j < count; ++j) {
		if ((XGetWindowProperty(dpy, win[j], atom[WinDesk], 0L, 32, False,
				XA_CARDINAL, &aa, &i, &ul, &ul, &d2) == Success && d2)) {
			n = *(long *) d2;
			XSelectInput(dpy, win[j], PropertyChangeMask);
			if (n > sizeof(desk)/sizeof(desk[0])) continue;
			desk[n].status = 1;
			XFree(d2);
		}
		// TODO urgent flag?
	}
	XFree(d1);
	redraw();
	return 0;
}

static int xlib_init() {
	if (!(dpy=XOpenDisplay(0x0))) return 1;
	root = DefaultRootWindow(dpy);
	int scr = DefaultScreen(dpy);
	init_atoms();
	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.background_pixel = background;
	wa.event_mask = StructureNotifyMask | ExposureMask;
	desks = XCreateWindow(dpy, root, 0, 0, 32, 32, 0, DefaultDepth(dpy,scr),
			InputOutput, DefaultVisual(dpy, scr),
			CWBackPixel | CWBackingStore | CWEventMask, &wa);
	XGCValues val;
	val.background = background;
	val.foreground = space_norm;
	dgc = XCreateGC(dpy, desks, GCForeground | GCBackground, &val);
	val.foreground = space_used;
	ogc = XCreateGC(dpy, desks, GCForeground | GCBackground, &val);
	val.foreground = space_curr;
	cgc = XCreateGC(dpy, desks, GCForeground | GCBackground, &val);
	XSelectInput(dpy, root, PropertyChangeMask);
}

static int xlib_free() {
	XFreeGC(dpy, dgc);
	XFreeGC(dpy, ogc);
	XFreeGC(dpy, cgc);
	XDestroyWindow(dpy, desks);
	XCloseDisplay(dpy);
}

int desktop() {
	xlib_init();
	XEvent ev;
	while (running) {
		printf("starting embed\n");
		rehash(); redraw();
		embed_window(desks);
		while (running && !XNextEvent(dpy, &ev)) {
			if (ev.xany.window == desks && ev.type == UnmapNotify){
				printf("going to break\n");
				 break;
			 }
			else if (ev.xany.window == desks && ev.type == Expose) {
				printf("expose event\n");
				 redraw();
			 }
			else if (!(ev.xany.window == root && ev.type == PropertyNotify)) {
				printf("continue\n");
				 continue;
			 }
			else if (ev.xproperty.atom == atom[CurDesk]) {
				printf("cur desk\n");
				 redraw();
				 }
			else if (ev.xproperty.atom == atom[IconGeo]) {
				printf("icon geo\n");
				redraw();
			}
			else if (ev.xproperty.atom == atom[WinList]) {
				printf("winlist\n");
				 rehash();
			 }
			else if (ev.xproperty.atom == atom[WinDesk]) {
				printf("windesk\n");
				 rehash();
			 }
		}
	}
	xlib_free();
	return 0;
}


/**********************************************************************\
* MAIL.C - module for BLUEPLATE
*
* Author: Jesse McClure, copyright 2014-2015
* License: GPLv3
\**********************************************************************/

#include "blueplate.h"
#include "config.h"

static int rescan() {
	static unsigned int show = 0;
	unsigned int prev = show;
	DIR *dir;
	struct dirent *de;
	int i, mail;
	for (i = 0; box[i].path; ++i) {
		mail = 0;
		if ( !(dir=opendir(box[i].path)) ) continue;
		while ( (de=readdir(dir)) )
			if (de->d_name[0] != '.') mail = 1;
		closedir(dir);
		if (mail) {
			show |= (1<<i);
			embed_window(box[i].win);
		}
		else {
			show &= ~(1<<i);
			XUnmapWindow(dpy, box[i].win);
			XReparentWindow(dpy, box[i].win, root, 0, 0);
		}
	}
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
	Pixmap pix;
	for (i = 0; box[i].path; ++i) {
	pix = XCreatePixmapFromBitmapData(dpy, root,
			(char *) mail_icon_data, mail_icon_size.x, mail_icon_size.y,
			box[i].color, background, DefaultDepth(dpy,scr));
	wa.background_pixmap = pix;
	box[i].win = XCreateWindow(dpy, root, 0, 0, 32, 32, 0,
			DefaultDepth(dpy,scr), InputOutput, DefaultVisual(dpy, scr),
			CWBackPixmap | CWBackingStore | CWEventMask, &wa);
	XFreePixmap(dpy, pix);
	}
}

static int xlib_free() {
	int i;
	for (i = 0; box[i].path; ++i)
		XDestroyWindow(dpy, box[i].win);
	XCloseDisplay(dpy);
}

int mail() {
	xlib_init();
	char buf[4096];
	int i, fd;
	fd = inotify_init();
	for (i = 0; box[i].path; ++i)
		inotify_add_watch(fd, box[i].path, INOTIFY_FLAGS);
	rescan();
	while (running && (read(fd, buf, sizeof(buf)) > 0) )
		rescan();
	close(fd);
	xlib_free();
	return 0;
}



#include "blueplate.h"

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

void sighandler(int sig) {
	if (sig == SIGTERM) running = False;
}

int main(int argc, const char **argv) {
	running = True;
	int i, pid, (*child)();
	struct sigaction sa;
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	for (i = 0; i < argc; ++i) {
		child = NULL;
		if (strncmp(argv[i], "desk", 4) == 0) child = desktop;
		else if (strncmp(argv[i], "mail", 4) == 0) child = mail;
		else {
			// TODO Show help
		}
		if (child && (fork() == 0) ) {
			setsid();
			sigaction(SIGTERM, &sa, NULL);
			return child();
		}
	}
	return 0;
}


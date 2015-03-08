/**********************************************************************\
* BLUEPLATE - System tray clients
*
* Author: Jesse McClure, copyright 2014
* License: GPLv3
*
*    This program is free software: you can redistribute it and/or
*    modify it under the terms of the GNU General Public License as
*    published by the Free Software Foundation, either version 3 of the
*    License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see
*    <http://www.gnu.org/licenses/>.
*
\**********************************************************************/

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
	
	switch (sig) {
		case SIGTERM: {
			running = False;
			break;
		}
		case SIGCHLD: {
			// we don't use pid or status for anything, but they are here if we want them
			pid_t pid;
			int status;
			
			pid = waitpid(-1, &status, NULL);
			break;
		}	
		default: 
			break;
	}	// switch
	
}

void help() {
printf("Coming soon...\n");
}

int main(int argc, const char **argv) {
	running = True;
	int i, pid, (*child)();
	struct sigaction sa;
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	for (i = 1; i < argc; ++i) {
		child = NULL;
		if (strncmp(argv[i], "help", 4) == 0) help();
#ifdef module_desktop
		else if (strncmp(argv[i], "desk", 4) == 0) child = desktop;
#endif
#ifdef module_mail
		else if (strncmp(argv[i], "mail", 4) == 0) child = mail;
#endif
#ifdef module_connman
		else if (strncmp(argv[i], "conn", 4) == 0) child = connman;
#endif		
#ifdef module_battery
		else if (strncmp(argv[i], "batt", 4) == 0) child = battery;
#endif
		else fprintf(stderr,"%s: %s: no such module\n", argv[0], argv[i]);
		if (child && (fork() == 0) ) {
			setsid();
			sigaction(SIGTERM, &sa, NULL);
			sigaction(SIGCHLD, &sa, NULL);
			return child();
		}
	}
	return 0;
}


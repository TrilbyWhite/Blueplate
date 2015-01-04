
BluePlate
=========
With all the fixins


Description
-----------
Blueplate is a system tray client program with a modular approach.  Each module
can provide one or more informational icons for a system tray.  Blueplate does
not itself provide the tray application but will work with trays including
tint2.

Blueplate modules will remain as small and simple as possible.  The modules are
not designed to provide interaction (no menus, etc) but serve as indicators.
Currently two modules exist: desktops and mail.

### Current Modules

- **Desktop:** Shows a desktop/workspace indicator on window managers that
  provide the necessary EWMH hints.
  - Desktop indicator layout, size, and colors are defined in config.h.  By
	 default, a 2x2 grid of spaces is used with colors as defined in the
	 config.h.
- **Mail:** Shows a colored mail icon when there is mail in any number of
  maildir folders.  Each folder is configured with a color used to indicate mail
  in that folder.  The default config.h provides a template for two maildir
  accounts in ~/mail/ - these must be edited to match the actual paths.

### Potential Upcoming Modules

**Note:** By using Xlib events for property changes for the desktop monitor, and
using inotify events for maildir changes, the current modules avoid any tradeoff
between resolution of timing and resource use.  Changes in the maildir can be
detected immediately without having a rapid infinite loop.  A goal for future
modules will be to find designs that allow for a similar efficiency, though this
may not be practical for all modules.

- **Weather:** A current conditions indicator, potentially using
  [shaman](https://github.com/HalosGhost/shaman)
- **CPU Load:**

Usage
-----
Blueplate forks a process for each module specified on the command line.  There
is no need to background blueplate:

```
$ blueplate <module> [ <module> ... ]
```

Packaging
---------

Blueplate includes a Makefile that accepts PREFIX, DESTDIR, and other standard
compiler/linker flags

```bash
git clone ...
cd blueplate
make
sudo make install
```

A PKGBUILD for archlinux will be coming soon

Known Bugs & Todo List
----------------------

None - bugs all currently reside in the 'unknown' category.  Let me know when
you find them.



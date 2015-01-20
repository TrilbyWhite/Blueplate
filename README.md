
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


- **DBusListener:** Conceptually based on the Connman module submitted by
  Andrew-Bibb.  The Connman module - while functional - doesn't (yet) meet the
  standards indicated above.  But development with the above goals in mind has
  lead to the possibility of a more generic dbus-listener.  The progress can be
  seen in dbus-listener.c.  Anyone fluent in dbus programming is encouraged to
  send pull requests with a working DBusWatch select loop (the only currently
  missing puzzle piece).
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

### Bugs

None - bugs all currently reside in the 'unknown' category.  Let me know when
you find them.

### Todo

1. Test conditional compiling of modules
1. Write command line help function
1. Write man page

See Also
--------

[Archlinux.org forum thread](https://bbs.archlinux.org/viewtopic.php?id=191842)

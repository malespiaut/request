-------------------------------------------
  The "Official" new DOS/Win95 Quest 2.4

                    by
Alexander Malmberg <alexander@malmberg.org>

            original version by
     Chris Carollo and Trey Harrison

Distributed under the GNU General Public
License. See legal.txt for more information.
------------------------------------------


Quick install
-------------
 If you want to get started quickly, here's what to do:

a.  Unzip Quest in a directory (any will do). Make sure you preserve
   directories (-d switch if you're using pkzip).

b.  Start your favorite text editor and open quest.cfg . Read through the
   whole file and make sure all the settings match your directories.
   quest.cfg has instructions.

 And that's it. It is still highly recommended that you read this file and
changes.txt for information on how to use Quest, though.


0. Important files
------------------
readme.txt     This file. Important! Read!
legal.txt      Legal information you must read. Important!
changes.txt    Information on changes and how to use them.
menu.hlp       Help on the menus.
keys.hlp       Keyboard quick-reference.
csg.txt        Information on CSG operations.

Note: If you want to help with writing documentation or tutorials, contact
Alexander Malmberg <alexander@malmberg.org>.

If you want the new Quest manual, you can browse it online or download it
at the Quest site.


1. Welcome
----------
Welcome to Quest version 2.4. Quest is a 3d map editor for Quake, Quake 2,
Quake 3, and several other Quake-like games. There's a Quest site at:

  http://www.frag.com/quest/

From this site you can download Quest and its source, reach the bug
tracking system, subscribe to the mailing lists, and get the latest new about
it. There's also a SourceForge project page at:

  http://sourceforge.net/project/?group_id=3684


This version is a new, improved version of the old Quest (which was
maintained by Chris Carollo and Trey Harrison). The Quest site has an About
page with more information about Quest, but in short, Quest has:

* Full Quake, Quake 2, Quake 3, Hexen 2, Heretic 2, Halflife, and Sin
  support.

* 2d/3d wireframe views, textured views, flat shaded views, and lighted
  views (including ray-traced shadows).

* Powerful editing capabilities for all areas of map editing (3d geometry,
  entities, texture alignment, etc.).

* Easy-to-use GUI.

* Lots of other stuff that makes it a really great editor.

Quset should handle all your map editing needs.

If you've used Quest before, you should read 'changes.txt' for a list of
changes and additions as well as instructions for using them. If you haven't,
you'd better read all the documentation.

This file mainly covers the DOS/Win* version of Quest, although a lot of
information applies to both versions. Information specific to the Linux
version can be found in the Linux version's INSTALL file.


2. Contact information
----------------------
Quest is maintained by Alexander Malmberg, who can be reached at
<alexander@malmberg.org>.

Bug reports should be submitted to the bug tracking system at
http://sourceforge.net/bugs/?group_id=3684 . If you have general questions
about Quest or using Quest to edit maps, you should consider posting them to
the Quest users' mailing list (go to the SourceForge project page to learn
how to subscribe to the mailing list).

If it doesn't fit any of the above categories, then questions, comments,
suggestions, bug reports, etc. should be sent to Alexander Malmberg
<alexander@malmberg.org> or <martin.malmberg@helsingborg.mail.telia.com>.


Chris Carollo and Trey Harrison no longer work with Quest. They don't have
anything to do with this version of Quest, and they shouldn't be bothered
about it. Id software doesn't have anything to do with this, either.


3. Bugs
-------
We attempt to keep Quest as stable and bug-free as possible, but it is still
possible that there are bugs. If you do encounter bugs, please report them to
the bug tracking system at http://sourceforge.net/bugs/?group_id=3684 .
Remember to include information on how to recreate the bug, what game you're
editing, operating system, and any other information that could be important.
We will attempt to fix all bugs as fast as we can.

If an unrecoverable error should happen, Quest will attempt to save your
map to q_backup.map. This should save your work most of the time, although
there have been bugs with Quest overwriting memory and corrupting the map
data. Moral: Save often and many copies in many places.

If Quest crashes and dumps out a lot of info that starts like this:

Exiting due to signal SIG???

you should write down all this and send it to me. If you don't want to write
everything down, the most important parts are the 'SIG???' and the numbers
after 'Call frame traceback EIPs:'.


4. Operating systems
--------------------
The DOS version of Quest runs in DOS, Windows 95 and Windows 98. It should
work in Windows 2000 too, but I haven't tested that myself.

Note: Quest won't run under Windows NT. I'm working on this. (This is
because NT doesn't let programs access the VESA VBE interface. A Win32
port of Quest probably wouldn't be difficult to do using mingw32, so if
you want to volunteer for this job, contact Alexander Malmberg.)

Quest has also been tested on Linux-i386-{x11,svgalib}, and will probably run
on other i386 based Unix-like systems with X-Windows, although that hasn't
been tested. You can get the Linux version at the Quest site 
(http://www.frag.com/quest/).


5. Documentation
----------------
The new Quest manual can be found at the Quest site.

For up-to-date information, read changes.txt and keys.txt. changes.txt is
the only file that is guaranteed to have up-to-date information on all
features and changes.

The first time you use Quest, you should probably read all the text files.


6. Automatic map building
-------------------------
Information about Quest map building system can be found in changes.txt
and the build*.cfg files.


7. Force exit
-------------
If Quest seems to have crashed or gotten stuck in an infinite loop, you
can press Ctrl-Break. This will attempt to kill Quest, no questions asked.
You won't be able to save your work. If you can recreate a situation where
this happens, it's a bug and it should be reported.


8. Legal
--------
Quest is distributed under the GNU General Public License, a copy of which
can be found in legal.txt.


9. Development
--------------
Quest development is hosted at SourceForge. From the main project site
at http://sourceforge.net/project/?group_id=3684 you can join the
developers' mailing list and get instructions for getting the latest
source version using CVS.



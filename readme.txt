-------------------------------------------
  The "Official" new DOS/Win95 Quest 2.4

                    by
Alexander Malmberg <alexander@malmberg.org>

            original version by
     Chris Carollo and Trey Harrison

Distributed under the GNU General Public
License. See legal.txt for more information.
------------------------------------------

0. Important files
------------------
readme.txt     This file. Important! Read!
legal.txt      Legal information you must read. Important!
changes.txt    Information on changes and how to use them.
menu.hlp       Help on the menus.
keys.hlp       Keyboard quick-reference.
csg.txt        Information on CSG operations.

Note: If you want to help with writing docs or tutorials, contact me.

If you want the new Quest manual, you can browse it online or download it
at the Quest site.

1. Welcome
----------
Welcome to the "Official" new DOS/Win95 Quest version by Alexander
Malmberg, a continuation of the original Quest by Chris Carollo and
Trey Harrison. There's a Quest site at:

   http://www.frag.com/quest/

From this site you can download Quest and its source, and it'll contain the
latest new about it.

This is a new, improved version of the old Quest. In many ways, it is still
similar to the old version, but several areas have changed a lot. Among the 
features are:
* Full Quake, Quake 2, Quake 3, Hexen 2, Heretic 2, Halflife, and Sin support.
* Three viewports, with 2d and 3d wireframe/poly/BSP/textured modes.
* Powerful geometry system, allowing you to move vertices freely and
  shape the brushes the way you want to, with face merging and splitting.
  Also has brush scaling and rotating.
* Face editor to edit individual faces' properties.
* Ability to parse the game source to find classnames and information
  about them.
* Customizable keyboard configuration.
* Entity editor that allows editing several entities at once, and
  displays help for them.
* Support for VBE 2.0 for fast rendering.
* Four CSG operations: intersect, subtract, make hollow, and make room.
* Freelook mode.
* Fullscreen texture picker.
* Easy-to-use GUI.
* Ability to compile maps and run a game from within editor.
* Grouping system, which allows you to edit large maps one part at a time.
* Lots of other stuff that makes it a really great editor.

In my opinion, this is a really great editor, and although I've tested many
other editors, including WorldCraft, Thred, Qoole, BSP, QuakeEd4, and some
more, I still think this is the best one. Hopefully, you'll find it as
useful as I have. It should handle all your map editing needs.

If you've used Quest before, you should read 'changes.txt' for a list of
changes and additions as well as instructions for using them. If you haven't,
you'd better read all the documentation.

This file mainly covers the DOS/Win* version of Quest. Information specific
to the Linux version can be found in the Linux version's INSTALL file.

2. Contact information
----------------------
Quest is maintained by Alexander Malmberg.
Questions, comments, suggestions, bug reports, etc. should be sent to
Alexander Malmberg <alexander@malmberg.org> or
<martin.malmberg@helsingborg.mail.telia.com>.

Chris Carollo and Trey Harrison don't have anything to do with this version
of Quest, and shouldn't be bothered about it. Id software doesn't have
anything to do with this, either.

3. Bugs
-------
I attempt to keep Quest as stable and bug-free as possible, but it is quite
possible that there will be bugs. If you do encounter bugs, please report them
to Alexander Malmberg. Remember to include information on how to recreate the
bug, what game you're editing, operating system, computer information, and
anything else important. I will attempt to fix all bugs as fast as I can.

If a unrecoverable error should happen, Quest will first attempt to save
your current work to q_backup.map. This should save your work most of the
time, although there have been bugs with Quest overwriting memory and
corrupting the map data. Moral: Save often and many copies in many places.
If Quest crashes and dumps out a lot of info that starts like this:

Exiting due to signal SIG???

you should write down all this and send it to me. If you don't want to write
everything down, the most important parts are the 'SIG???' and the numbers
after 'Call frame traceback EIPs:'.

4. Operating systems
--------------------
The DOS version of Quest runs in DOS, Windows 95 and Windows 98. It should
work in Windows 2000 too, but I haven't tested that myself.

Note: Quest won't run under Windows NT. I'm working on this.

Quest has also been tested in Linux-i386-{x11,svgalib}, and will probably run
in other i386 based Unix-like systems with X-Windows, although that hasn't
been tested. You can get the Linux version at the Quest site 
(http://www.frag.com/quest/).

5. Documentation
----------------
The new Quest manual can be found at the Quest site.

For up-to-date information, read changes.txt and keys.txt. changes.txt is
the only file that is guaranteed to be up-to-date.

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


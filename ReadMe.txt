== Introduction ==

DXGL is a project to create a DirectDraw/Direct3D version 1 to 7 implementation that runs on OpenGL 2.0. It is intended to alleviate some of the graphics glitches inherent with using legacy DirectX interfaces on modern video cards. The API will be 100% binary compatible with the system ddraw.dll file.

DXGL is currently in a pre-alpha stage and very little works at this point.

== System Requirements ==

* Windows operating system (currently XP or above)
* OpenGL 2.0 or higher compatible video card, with hardware accelerated non-power-of-two size textures

== Build Requirements ==
* Visual Studio 2010 or Visual C++ 2010 Express
* Latest version of Windows SDK

== Build instructions ==
These instructions assume that you do not have any of the required software installed.  If you already have any or all of this software installed and set up, skip those steps.
* Install Visual C++ 2010 Express at http://www.microsoft.com/express/downloads/#2010-Visual-CPP
* Install the Windows SDK at:
http://www.microsoft.com/downloads/en/details.aspx?familyid=6b6c21d2-2006-4afa-9702-529fa782d63b 
Warning:  If you have installed Visual Studio 2010 SP1, install the Windows SDK update at:
http://www.microsoft.com/downloads/en/details.aspx?FamilyID=689655B4-C55D-4F9B-9665-2C547E637B70 
* Open the dxgl.sln file, select your build configuration (Debug or Release) in the toolbar, and press F5.

== Progress ==
What works:
* DirectDraw object creation and destruction
* Display mode enumeration and switching (with emulated mode switching)
* Fullscreen and windowed modes.
* Basic Blt() functionality

What partially works:
* SetCooperativeLevel (not always on top for debugging purposes.)
* 8-bit color

What doesn't work:
* Most functions are stubbed out and return an error
* No 3D graphics support

== Roadmap ==
These are goals to be set for future releases.

- Version 0.0.4 Pre-Alpha
* Add support for DirectDraw versions 2 to 7
* Sort video modes (optional, set in dxglcfg)
* Support uncommon color depths (optional, set in dxglcfg)
* Support low resolution modes (optional, set in dxglcfg)
* Fix GCC build

- Version 0.0.5 Pre-Alpha
* Fully implement scaling and filtering options
* Add more test patterns to dxgltest (increase back buffer count to see them)
* Support surface access via Windows GDI

== Installation ==

Extract ddraw.dll to the folder where your application is installed.
To uninstall, delete ddraw.dll from the application's folder.
To test DXGL, run dxgltest.exe

== SVN ==

SVN readonly access is available at:
https://www.williamfeely.info/svn/dxgl

There is a web interface at:
http://www.williamfeely.info/websvn/listing.php?repname=dxgl

== Bug reports ==

Bug reports are managed by a Bugzilla system available at:
https://www.williamfeely.info/bugzilla

A user account needs to be created at this site to post bug reports.

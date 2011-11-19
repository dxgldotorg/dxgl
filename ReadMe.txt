== Introduction ==

DXGL is a project to create a DirectDraw/Direct3D version 1 to 7 implementation that runs on OpenGL 2.0. It is intended to alleviate some of the graphics glitches inherent with using legacy DirectX interfaces on modern video cards. The API will be 100% binary compatible with the system ddraw.dll file.

DXGL is currently in a pre-alpha stage and very little works at this point.

== System Requirements ==

* Windows operating system (currently XP or above)
* OpenGL 2.0 or higher compatible video card, with hardware accelerated non-power-of-two size textures

== Build Requirements ==
* Visual Studio 2010 or Visual C++ 2010 Express
* Latest DirectX SDK, with DXSDK_DIR environment variable set to the installation path
* wxWidgets 2.9.1, with WXWIN environment variable set to the installation path (dxgltest only)
* Latest version of Windows SDK may be required

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

Items marked (done) have been completed in a SVN build.

- Version 0.0.3 Pre-Alpha
* Sort video modes (optional, set in dxglcfg)
* Support uncommon color depths (optional, set in dxglcfg)
* Support low resolution modes (optional, set in dxglcfg)
* Support surface access via Windows GDI

- Version 0.0.4 Pre-Alpha
* Fully implement scaling and filtering options
* Add more test patterns to dxgltest (increase back buffer count to see them)

- Version 0.0.5 Pre-Alpha
* Fully implement color palettes for primary surface

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

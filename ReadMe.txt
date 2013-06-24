DXGL 0.5.1
http://www.williamfeely.info/wiki/DXGL

== Introduction ==

DXGL is a project to create a DirectDraw/Direct3D version 1 to 7 implementation that runs on OpenGL 2.x. It is intended to alleviate some of the graphics glitches inherent with using legacy DirectX interfaces on modern video cards. The API will be 100% binary compatible with the system ddraw.dll file.

DXGL is currently in an alpha stage, but several applications and games already work.

== System Requirements ==

* Windows XP, Vista, 7, or 8 (May work under recent builds of Wine)
* OpenGL 2.0 or higher compatible video card, with hardware accelerated non-power-of-two size textures
* Visual C++ 2010 x86 runtime, available at http://www.microsoft.com/download/en/details.aspx?id=5555 (will be installed if not present)

== Build Requirements ==
* Visual Studio 2010 or Visual C++ 2010 Express
* The following components are optional.  The build process will ask for these if they do not exist:
** TortoiseSVN (to fill in revision on SVN builds)
** HTML Help Workshop (to build help)
** NSIS (to build installer, requires TortoiseSVN and HTML Help Workshop to succeed)

== Build Instructions ==
These instructions assume that you do not have any of the required software installed. If you already have any or all of this software installed and set up, skip those steps.
* Install Visual C++ 2010 Express at http://www.microsoft.com/express/downloads/#2010-Visual-CPP .
* Open the dxgl.sln file, select your build configuration (Debug or Release) in the toolbar, and press F7.

== Progress ==
For detailed progress information, please check http://www.williamfeely.info/wiki/DXGL_Features
What works:
* DirectDraw object creation and destruction (versions 1 to 7)
* Display mode enumeration and switching (with emulated mode switching)
* Fullscreen and windowed modes.
* Basic Blt() functionality
* 8-bit color emulated with GLSL shader

What partially works:
* 3D graphics are only partially supported.

What doesn't work:
* Many functions are stubbed out and return an error

== Installation ==

Run the installer.  When the installer completes, open DXGL Config and add your program files to the config program.
To uninstall, go to the Add/Remove Programs or Programs and Features control panel and uninstall.

== SVN ==

SVN readonly access is available at:
svn://www.williamfeely.info/dxgl

There is a Mediawiki-based SVN log at:
http://www.williamfeely.info/wiki/Special:Code/DXGL

== AppDB ==

An AppDB system (similar to that on winehq.org) is now available at:
https://www.williamfeely.info/appdb/

This requires a user account separate from the other services.

== Discussion boards ==

You may discuss DXGL at:
https://www.williamfeely.info/phpBB3

You must create a forum account to post content.


== Bug reports ==

Bug reports are managed by a Bugzilla system available at:
https://www.williamfeely.info/bugzilla

A user account needs to be created at this site to post bug reports.

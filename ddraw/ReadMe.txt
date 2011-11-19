== Introduction ==

DXGL is a project to create a DirectDraw/Direct3D version 1 to 7 implementation that runs on OpenGL 2.0.  It is intended to alleviate some of the graphics glitches inherent with using legacy DirectX interfaces on modern video cards.  The API will be 100% binary compatible with the system ddraw.dll file.

DXGL is currently in a pre-alpha stage and very little works at this point.

== System Requirements ==

* Windows operating system (currently XP or above)
* OpenGL 2.0 or higher compatible video card, with hardware accelerated non-power-of-two size textures

== Progress ==
What works:
* DirectDraw object creation and destruction
* Display mode enumeration and switching (with emulated mode switching)
* OpenGL context creation and destruction

What partially works:
* Fullscreen modes
* Primary surface creation and display.

What doesn't work:
* Most functions are stubbed out and return an error
* No offscreen surfaces
* No 3D graphics support

== Installation ==

Extract ddraw.dll to the folder where your application is installed.
To uninstall, delete ddraw.dll from the application's folder.
To test DXGL, run dxgltest.exe
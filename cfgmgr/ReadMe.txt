Struct DXGLCFG
Member scaler
REG_DWORD HKCU\DXGL\<app>\ScalingMode
Determines the method of scaling full screen modes.
Valid settings:
0 - Change display mode to match requested mode.
1 - Stretch output to desktop
2 - Stretch output to desktop, preserving aspect ratio
3 - Center output on desktop
4 - Change display mode, stretch if matching mode not found
5 - Change display mode, aspect corrected stretch if matching mode not found
6 - Change display mode, center if matching mode not found

Member colormode
REG_DWORD HKCU\DXGL\<app>\ChangeColorDepth
If nonzero, switches screen color depth if requested by the application.
Recommended setting is off.  DXGL handles color depth conversion internally.

Member scalingfilter
REG_DWORD HKCU\DXGL\<app>\ScalingFilter
Filter to use for stretched 2D blits.
Valid settings:
0 - Nearest-neighbor stretching
1 - Bilinear interpolation
2 - Use pixel shader defined by shaderfile
3 - Use pixel shader defined by shaderfile, only on primary surface.

Member highres
REG_DWORD HKCU\DXGL\<app>\AdjustPrimaryResolution
Changes primary resolution to match desktop resolution.
May cause glitches
Valid settings:
0 - Use native primary surface size.  More compatible.
1 - Adjust primary surface size.

Member texfilter
REG_DWORD HKCU\DXGL\<app>\TextureFilter
Filter for 3D textured polygons
Valid settings:
0 - Use application settings
1 - GL_NEAREST (Nearest-neighbor filtering)
2 - GL_LINEAR (Bilinear filtering)
3 - GL_NEAREST_MIPMAP_NEAREST (Nearest-neighbor with mipmap)
4 - GL_NEAREST_MIPMAP_LINEAR (Nearest-neighbor with linear mipmap)
5 - GL_LINEAR_MIPMAP_NEAREST (Bilinear with mipmap)
6 - GL_LINEAR_MIPMAP_LINEAR (Trilinear filtering)

Member anisotropic
REG_DWORD HKCU\DXGL\<app>\AnisotropicFiltering
Anisotropic filter level, may decrese performance on slower GPUs.
Valid settings:
0 - Application default
1 - off.
Larger numbers enable anisotropic filtering, maximum determined by GPU driver.

member msaa
REG_DWORD HKCU\DXGL\<app>\Antialiasing
Level of Full Screen Antialiasing.  May considerably decrease performance on
slower GPUs.
Valid settings:
0 - Application default
1 - Disabled.
Larger numbers enable FSAA by GPU implementation.
For GPUs that support CSAA in OpenGL, it is stored in the format
Coverage sample count + 4096 × Color sample count
Example:  16 coverage, 8 color = 0x8010

Member aspect
REG_DWORD HKCU\DXGL\<app>\AdjustAspectRatio
If the scaler is 1 or 4, adjust 3D projection to correct aspect ratio.  Does
not affect 2D blits or similar operations.
Valid settings:
0 - No correction
1 - Expand viewable area.  May have glitches on edge of screen.
2 - Crop to viewable area. May cause graphics to get cut off.

Member shaderfile
REG_SZ HKCU\DXGL\<app>\ShaderFile
Full path to file containing a custom pixel shader for 2D blits.

Member SortModes
REG_DWORD HKCU\DXGL\<app>\SortModes
Sort display modes
Valid settings:
0 - As reported by system
1 - Group by depth
2 - Group by resolution

Member AllColorDepths
REG_DWORD HKCU\DXGL\<app>\AllColorDepths
Enable all color depths, even if unsupported by the system
Valid settings:
0 - Off
1 - On

Member LowResModes
REG_DWORD HKCU\DXGL\<app>\LowResModes
Enable low resolution video modes, even if unsupported by the system
Valid settings:
0 - Off
1 - On

Member vsync
REG_DWORD HKCU\DXGL\<app>\VSync
Vertical retrace control
Valid settings:
0 - Determined by application
1 - Off
2 - On

Member TextureFormat
REG_DWORD HKCU\DXGL\<app>\TextureFormat
Texture format
Valid settings:
0 - Automatic

Member TexUpload
REG_DWORD HKCU\DXGL\<app>\TexUpload
Method used to upload textures
Valid settings:
0 - Automatic
1 - OpenGL Standard
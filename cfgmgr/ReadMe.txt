Struct DXGLCFG
Member scaler
REG_DWORD HKCU\DXGL\Profiles\<app>\ScalingMode
Determines the method of scaling full screen modes.
Valid settings:
0 - Change display mode to match requested mode.
1 - Stretch output to desktop
2 - Stretch output to desktop, preserving aspect ratio
3 - Center output on desktop
4 - Change display mode, stretch if matching mode not found
5 - Change display mode, aspect corrected stretch if matching mode not found
6 - Change display mode, center if matching mode not found
7 - Crop output to desktop, preserving aspect ratio

Member fullmode
REG_DWORD HKCU\DXGL\Profiles\<app>\FullscreenWindowMode
Determines how to handle fullscreen modes.
Valid settings:
0 - Use exclusive fullscreen
1 - Use non-exclusive fullscreen, not quite borderless windowed mode
2 - Use a non-resizable window
3 - Use a resizable window, uses scaler mode, preferably 1, 2, 3, or 7
4 - Use a borderless, non-resizable window, called windowed borderless in industry

Member colormode
REG_DWORD HKCU\DXGL\Profiles\<app>\ChangeColorDepth
If nonzero, switches screen color depth if requested by the application.
Recommended setting is off.  DXGL handles color depth conversion internally.

Member firstscalefilter
REG_DWORD HKCU\DXGL\Profiles\<app>\FirstScaleFilter
Filter to use for the optional first pass display scaling.
Valid settings:
0 - Nearest-neighbor stretching
1 - Bilinear interpolation

Member firstscalex
REG_DWORD HKCU\DXGL\Profiles\<app>\FirstScaleX
Amount to stretch the display in the X direction for the first pass.
If either X or Y is set to 0 or less than 0.25, automatically choose
2x or 2x1 scaling for certain low resolutions.
Stored as a 32-bit float encoded as a DWORD.

Member firstscaley
REG_DWORD HKCU\DXGL\Profiles\<app>\FirstScaley
Amount to stretch the display in the Y direction for the first pass.
If either X or Y is set to 0 or less than 0.25, automatically choose
2x or 2x1 scaling for certain low resolutions.
Stored as a 32-bit float encoded as a DWORD.

Member scalingfilter
REG_DWORD HKCU\DXGL\Profiles\<app>\ScalingFilter
Filter to use for stretched 2D blits and the final display scaling.
Valid settings:
0 - Nearest-neighbor stretching
1 - Bilinear interpolation

Member primaryscale
REG_DWORD HKCU\DXGL\Profiles\<app>\AdjustPrimaryResolution
Changes primary resolution to match desktop resolution.
May cause glitches
Valid settings:
0 - Use native primary surface size.  Most compatible.
1 - Adjust primary surface size to match display.
2 - Adjust primary surface to nearest integer multiple of native.
3 - Use exact 1.5x scale.
4 - Use exact 2x scale.
5 - Use exact 2.5x scale.
6 - Use exact 3x scale.
7 - Use exact 4x scale.
8 - Use custom scale.

Member primaryscalex
REG_DWORD HKCU\DXGL\Profiles\<app>\PrimaryScaleX
Custom X scale for primary scaling.
Stored as a 32-bit float encoded as a DWORD.
If zero, negative, or an invalid value, set to 1.

Member primaryscaley
REG_DWORD HKCU\DXGL\Profiles\<app>\PrimaryScaleY
Custom X scale for primary scaling.
Stored as a 32-bit float encoded as a DWORD.
If zero, negative, or an invalid value, set to 1.

Member texfilter
REG_DWORD HKCU\DXGL\Profiles\<app>\TextureFilter
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
REG_DWORD HKCU\DXGL\Profiles\<app>\AnisotropicFiltering
Anisotropic filter level, may decrese performance on slower GPUs.
Valid settings:
0 - Application default
1 - off.
Larger numbers enable anisotropic filtering, maximum determined by GPU driver.

member msaa
REG_DWORD HKCU\DXGL\Profiles\<app>\Antialiasing
Level of Full Screen Antialiasing.  May considerably decrease performance on
slower GPUs.
Valid settings:
0 - Application default
1 - Disabled.
Larger numbers enable FSAA by GPU implementation.
For GPUs that support CSAA in OpenGL, it is stored in the format
Coverage sample count + 4096 × Color sample count
Example:  16 coverage, 8 color = 0x8010

Member aspect3d
REG_DWORD HKCU\DXGL\Profiles\<app>\AdjustAspectRatio
If the scaler is 1 or 4, adjust 3D projection to correct aspect ratio.  Does
not affect 2D blits or similar operations.
Valid settings:
0 - No correction
1 - Expand viewable area.  May have glitches on edge of screen.
2 - Crop to viewable area. May cause graphics to get cut off.

Member EnableShader
REG_DWORD HKCU\DXGL\Profiles\<app>\EnableShader
If nonzero, enables post-process shaders.

Member shaderfile
REG_SZ HKCU\DXGL\Profiles\<app>\ShaderFile
Full path to file containing a post-process shader script.

Member SortModes
REG_DWORD HKCU\DXGL\Profiles\<app>\SortModes
Sort display modes
Valid settings:
0 - As reported by system
1 - Group by depth
2 - Group by resolution

Member AllColorDepths
REG_DWORD HKCU\DXGL\Profiles\<app>\AllColorDepths
[DEPRECATED]Enable all color depths, even if unsupported by the system
Valid settings:
0 - Off
1 - On

Member AddColorDepths
REG_DWORD HKCU\DXGL\Profiles\<app>\AddColorDepths
Adds color depths, even if unsupported by the system
Bit-mapped variable
Valid settings, OR'ed to combine settings:
0 - None
1 - Add 8-bit modes
2 - Add 15-bit modes
4 - Add 16-bit modes
8 - Add 24-bit modes
16 - Add 32-bit modes

Member ExtraModes
REG_DWORD HKCU\DXGL\Profiles\<app>\ExtraModes
[DEPRECATED]Enable extra video modes, even if unsupported by the system
Valid settings:
0 - Off
1 - On

member AddModes
REG_DWORD HKCU\DXGL\Profiles\<app>\AddModes
Adds additional video modes, even if unsupported by the system
Bit-mapped variable
Valid settings, OR'ed to combine settings:
0 - None
1 - Add common low-resolution modes
2 - Add less common low-resolution modes
4 - Add higher resolution modes

Member vsync
REG_DWORD HKCU\DXGL\Profiles\<app>\VSync
Vertical retrace control
Valid settings:
0 - Determined by application

Member TextureFormat
REG_DWORD HKCU\DXGL\Profiles\<app>\TextureFormat
Texture format
Valid settings:
0 - Automatic

Member TexUpload
REG_DWORD HKCU\DXGL\Profiles\<app>\TexUpload
Method used to upload textures
Valid settings:
0 - Automatic
1 - OpenGL Standard

Member DPIScale
REG_DWORD HKCU\DXGL\Profiles\<app>\DPIScale
Enable DPI scaling fix
The program may be restarted if the Windows AppCompat method is enabled or
disabled.
Valid settings:
0 - Disabled
1 - Enabled
2 - Use Windows AppCompat

Member aspect
REG_DWORD HKCU\DXGL\Profiles\<app>\ScreenAspect
Screen aspect ratio to simulate, if the scaler is 1 or 5.
Stored as a 32-bit float encoded as a DWORD.
If zero, negative, or an invalid value, use automatic ratio based on square pixels.
Positive values indicate a specific screen aspect ratio.

Member Windows8Detected
REG_DWORD HKCU\DXGL\Global\Windows8Detected
Nonzero if Windows 8 (or later) is detected.
If zero or undefined and Windows 8 (or later) is detected, AllColorDepths in
Global key is set to 1 and Windows8Detected is also set to 1.
If nonzero, AllColorDepths is not affected.
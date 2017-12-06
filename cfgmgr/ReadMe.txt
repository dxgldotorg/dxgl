Struct DXGLCFG

member NoWriteRegistry
INI Entry NoWriteRegistry
INI Group system
Does not have a registry value.
If nonzero, DXGL will not write a profile to the registry.

member OverrideDefaults
INI Entry OverrideDefaults
INI Group system
Does not have a registry value.
If nonzero, ignores registry settings from Global section of registry.

Member scaler
INI Entry ScalingMode
INI Group display
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
8 - Center output, multiply by custom values
9 - Set display to custom resolution and refresh
10 - Center output, scale to custom size

Member fullmode
INI Entry FullscreenWindowMode
INI Group display
REG_DWORD HKCU\DXGL\Profiles\<app>\FullscreenWindowMode
Determines how to handle fullscreen modes.
Valid settings:
0 - Use exclusive fullscreen
1 - Use non-exclusive fullscreen, not quite borderless windowed mode
2 - Use a non-resizable window
3 - Use a resizable window, uses scaler mode, preferably 1, 2, 3, or 7
4 - Use a borderless, non-resizable window, called windowed borderless in industry

Member colormode
INI Entry ChangeColorDepth
INI Group display
REG_DWORD HKCU\DXGL\Profiles\<app>\ChangeColorDepth
If nonzero, switches screen color depth if requested by the application.
Recommended setting is off.  DXGL handles color depth conversion internally.

Member AddColorDepths
INI Entry AllColorDepths
INI Group display
REG_DWORD HKCU\DXGL\Profiles\<app>\AllColorDepths
[DEPRECATED FOR DXGLCFG2]Enable all color depths, even if unsupported by the system
Valid settings:
0 - Off
1 - On

Member AddColorDepths
INI Entry AddColorDepths
INI Group display
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

Member AddModes
INI Entry ExtraModes
INI Group display
REG_DWORD HKCU\DXGL\Profiles\<app>\ExtraModes
[DEPRECATED FOR DXGLCFG2]Enable extra video modes, even if unsupported by the system
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
4 - Add uncommon standard definition modes
8 - Add high definition modes
16 - Add QHD to UHD modes.
32 - Add over-4K UHD modes.  Check GPU specifications before enabling.
64 - Add very uncommon resolutions of all dimensions.

Member SortModes
REG_DWORD HKCU\DXGL\Profiles\<app>\SortModes
Sort display modes
Valid settings:
0 - As reported by system
1 - Group by depth
2 - Group by resolution

Member CustomResolutionX
REG_DWORD HKCU\DXGL\Profiles\<app>\CustomResolutionX
INI Entry CustomResolutionX
INI Group display
Width of the custom resolution for the display output for modes 9 and 10.
Default is 640

Member CustomResolutionY
REG_DWORD HKCU\DXGL\Profiles\<app>\CustomResolutionY
INI Entry CustomResolutionY
INI Group display
Height of the custom resolution for the display output for modes 9 and 10.
Default is 480

Member CustomRefresh
REG_DWORD HKCU\DXGL\Profiles\<app>\CustomRefresh
INI Entry CustomRefresh
INI Group display
Refresh rate for the display output for modes 9 and 10.
Default is 60

Member DisplayMultiplierX
REG_DWORD HKCU\DXGL\Profiles\<app>\
INI Entry DisplayMultiplierX
INI Group display
Multiplier for the pixel width for display mode 8.
Default is 1.0

Member DisplayMultiplierY
REG_DWORD HKCU\DXGL\Profiles\<app>\
INI Entry DisplayMultiplierY
INI Group display
Multiplier for the pixel height for display mode 8.
Default is 1.0

Member scalingfilter
INI Entry ScalingFilter
INI Group scaling
REG_DWORD HKCU\DXGL\Profiles\<app>\ScalingFilter
Filter to use for display scaling.
Valid settings:
0 - Nearest-neighbor stretching
1 - Bilinear interpolation

Member BltScale
INI Entry BltScale
INI Group scaling
REG_DWORD HKCU\DXGL\Profiles\<app>\BltScale
Filter to use for Blt scaling.
Valid settings:
0 - Nearest-neighbor stretching
1 - Bilinear interpolation
2 - Bilinear interpolation, sharp color key
3 - Bilinear interpolation, soft color key

Member BltThreshold
INI Entry BltThreshold
INI Group scaling
REG_DWORD HKCU\DXGL\Profiles\<app>\BltThreshold
Threshold point for sharp color key scaling.
Default is 127.
Valid settings are 0 to 254, 255 will be accepted but disable color keying.

Member primaryscale
INI Entry AdjustPrimaryResolution
INI Group scaling
REG_DWORD HKCU\DXGL\Profiles\<app>\AdjustPrimaryResolution
Changes primary resolution to match desktop resolution.
May cause glitches
Valid settings:
0 - Use native primary surface size.  Most compatible.
1 - Adjust primary surface size to match display.
(future)
2 - Adjust primary surface to nearest integer multiple of native.
3 - Use exact 1.5x scale.
4 - Use exact 2x scale.
5 - Use exact 2.5x scale.
6 - Use exact 3x scale.
7 - Use exact 4x scale.
8 - Use custom scale.

(future)
Member primaryscalex
INI Entry PrimaryScaleX
INI Group scaling
REG_DWORD HKCU\DXGL\Profiles\<app>\PrimaryScaleX
Custom X scale for primary scaling.
Stored as a 32-bit float encoded as a DWORD.
If zero, negative, or an invalid value, set to 1.

(future)
Member primaryscaley
INI Entry PrimaryScaleY
INI Group scaling
REG_DWORD HKCU\DXGL\Profiles\<app>\PrimaryScaleY
Custom X scale for primary scaling.
Stored as a 32-bit float encoded as a DWORD.
If zero, negative, or an invalid value, set to 1.

Member aspect
INI Entry ScreenAspect
INI Group scaling
REG_DWORD HKCU\DXGL\Profiles\<app>\ScreenAspect
Screen aspect ratio to simulate, if the scaler is 1 or 5.
Stored as a 32-bit float encoded as a DWORD.
If zero, negative, or an invalid value, use automatic ratio based on square pixels.
Positive values indicate a specific screen aspect ratio.

Member DPIScale
INI Entry DPIScale
INI Group scaling
REG_DWORD HKCU\DXGL\Profiles\<app>\DPIScale
Enable DPI scaling fix
The program may be restarted if the Windows AppCompat method is enabled or
disabled.
Valid settings:
0 - Disabled
1 - Enabled
2 - Use Windows AppCompat

Member postfilter
INI Entry PostprocessFilter
INI Group postprocess
REG_DWORD HKCU\DXGL\Profiles\<app>\PostprocessFilter
Alternately reads from REG_DWORD HKCU\DXGL\Profiles\<app>\FirstScaleFilter
Filter to use for the optional postprocess display scaling.
Valid settings:
0 - Nearest-neighbor stretching
1 - Bilinear interpolation

Member postsizex
INI Entry PostprocessScaleX
INI Group postprocess
REG_DWORD HKCU\DXGL\Profiles\<app>\PostprocessScaleX
Alternately reads from REG_DWORD HKCU\DXGL\Profiles\<app>\FirstScaleX
Amount to stretch the display in the X direction for the postprocess pass.
If either X or Y is set to 0 or less than 0.25, automatically choose
2x, 2x1, or 1x2 scaling for certain low resolutions.
Stored as a 32-bit float encoded as a DWORD.

Member postsizey
INI Entry PostprocessScaleY
INI Group postprocess
REG_DWORD HKCU\DXGL\Profiles\<app>\PostprocessScaleY
Alternately reads from REG_DWORD HKCU\DXGL\Profiles\<app>\FirstScaleY
Amount to stretch the display in the Y direction for the postprocess pass.
If either X or Y is set to 0 or less than 0.25, automatically choose
2x, 2x1, or 1x2 scaling for certain low resolutions.
Stored as a 32-bit float encoded as a DWORD.

Member EnableShader
INI Entry EnableShader
INI Group postprocess
REG_DWORD HKCU\DXGL\Profiles\<app>\EnableShader
If nonzero, enables post-process shaders.

Member shaderfile
INI Entry ShaderFile
INI Group postprocess
REG_SZ HKCU\DXGL\Profiles\<app>\ShaderFile
Full path to file containing a post-process shader script.

Member texfilter
INI Entry TextureFilter
INI Group d3d
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
INI Entry AnisotropicFiltering
INI Group d3d
REG_DWORD HKCU\DXGL\Profiles\<app>\AnisotropicFiltering
Anisotropic filter level, may decrese performance on slower GPUs.
Valid settings:
0 - Application default
1 - off.
Larger numbers enable anisotropic filtering, maximum determined by GPU driver.

member msaa
INI Entry Antialiasing
INI Group d3d
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
INI Entry D3DAspect
INI Group d3d
REG_DWORD HKCU\DXGL\Profiles\<app>\D3DAspect
Altrenately reads from REG_DWORD HKCU\DXGL\Profiles\<app>\AdjustAspectRatio
If the scaler is 1 or 4, adjust 3D projection to correct aspect ratio.  Does
not affect 2D blits or similar operations.
Valid settings:
0 - No correction
1 - Expand viewable area.  May have glitches on edge of screen.
2 - Crop to viewable area. May cause graphics to get cut off.

Member vsync
INI Entry VSync
INI Group advanced
REG_DWORD HKCU\DXGL\Profiles\<app>\VSync
Vertical retrace control
Valid settings:
0 - Determined by application

Member TextureFormat
INI Entry TextureFormat
INI Group advanced
REG_DWORD HKCU\DXGL\Profiles\<app>\TextureFormat
Texture format
Valid settings:
0 - Automatic

Member TexUpload
INI Entry TexUpload
INI Group advanced
REG_DWORD HKCU\DXGL\Profiles\<app>\TexUpload
Method used to upload textures
Valid settings:
0 - Automatic

Member SingleBufferDevice
INI Entry SingleBufferDevice
INI Group advanced
REG_DWORD HKCU\DXGL\Profiles\<app>\SingleBufferDevice
If true, do not use double buffering in OpenGL.

Default for all Debug variables is 0 or FALSE.

Member DebugNoExtFramebuffer
INI Entry DebugNoExtFramebuffer
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugNoExtFramebuffer
If nonzero, disables use of the EXT_framebuffer_object OpenGL extension.
If both EXT_framebuffer_object and ARB_framebuffer_object are disabled or
unavailable DXGL will fail to initialize.

Member DebugNoArbFramebuffer
INI Entry DebugNoArbFramebuffer
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugNoArbFramebuffer
If nonzero, disables use of the ARB_framebuffer_object OpenGL extension.
If both EXT_framebuffer_object and ARB_framebuffer_object are disabled or
unavailable DXGL will fail to initialize.

Member DebugNoES2Compatibility
INI Entry DebugNoES2Compatibility
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugNoES2Compatibility
If nonzero, disables use of the ARB_ES2_compatibility OpenGL extension.
This will disable use of the 16-bit GL_RGB565 texture format which is part
of OpenGL ES2 but not the desktop OpenGL standard.

Member DebugNoExtDirectStateAccess
INI Entry DebugNoExtDirectStateAccess
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugNoExtDirectStateAccess
If nonzero, disables use of the EXT_direct_state_access OpenGL extension which
simplifies manipulation of certain types of OpenGL objects.

Member DebugNoArbDirectStateAccess
INI Entry DebugNoArbDirectStateAccess
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugNoArbDirectStateAccess
If nonzero, disables use of the ARB_direct_state_access OpenGL extension which
simplifies manipulation of certain types of OpenGL objects.

Member DebugNoSamplerObjects
INI Entry DebugNoSamplerObjects
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugNoSamplerObjects
If nonzero, disables use of sampler objects, either via GL_ARB_sampler_objects
or OpenGL version 3.3+.  Disabling sampler objects reduces the accuracy of
Direct3D content in some situations.

Member DebugNoGpuShader4
INI Entry DebugNoGpuShader4
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugNoGpuShader4
If nonzero, disables use of the EXT_gpu_shader4 extension on OpenGL 2.x cards.
This will disable most raster operations in DirectDraw.  This has no effect on
OpenGL 3.0 or higher because the functionality is in core, unless DebugNoGLSL130
is also enabled.

Member DebugNoGLSL130
INI Entry DebugNoGLSL130
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugNoGLSL130
If nonzero, disables use of GLSL version 1.30.  When combined with
DebugNoGpuShader4 this will disable raster operations in DirectDraw.

Member DebugMaxGLVersionMajor
INI Entry DebugMaxGLVersionMajor
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugMaxGLVersionMajor
If nonzero, sets the maximum OpenGL major version that DXGL will use,
and uses the value from DebugMaxGLVersionMinor.

Member DebugMaxGLVersionMinor
INI Entry DebugMaxGLVersionMinor
INI Group debug
REG_DWORD HKCU\DXGL\Profiles\<app>\DebugMaxGLVersionMinor
If DebugMaxGLVersionMajor is nonzero, sets the maximum OpenGL minor version
that DXGL will use, unless the actual major version is less than
DebugMaxGLVersionMajor.

Member Windows8Detected
Not in INI file
REG_DWORD HKCU\DXGL\Global\Windows8Detected
Nonzero if Windows 8 (or later) is detected.
If zero or undefined and Windows 8 (or later) is detected, AllColorDepths in
Global key is set to 1 and Windows8Detected is also set to 1.
If nonzero, AllColorDepths is not affected.

Member PasrsedAddColorDepths
Not in INI file
Not in registry
Nonzero if the INI parser has read the AddColorDepths entry.
This prevents the AddColorDepths variable from being overridden by
AllColorDepths.  If zero, the AllColorDepths INI entry will overwrite the
AddColorDepths configuration variable.

Member ParsedAddModes
Not in INI file
Not in registry
Nonzero if the INI parser has read the AddModes entry.
This prevents the AddModes variable from being overridden by ExtraModes.
If zero, the ExtraModes INI entry will overwrite the AddModes configuration
variable.
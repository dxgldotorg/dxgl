// DXGL
// Copyright (C) 2011-2018 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Dialogs
#define IDD_DXGLCFG                             101
#define IDD_LOADING                             102
#define IDD_TEXSHADER                           103
#define IDD_VERTEXSHADER                        104
#define IDD_MODELIST                            105

// Tabs
#define IDD_DISPLAY                             201
#define IDD_EFFECTS                             202
#define IDD_3DGRAPHICS                          203
#define IDD_ADVANCED                            204
#define IDD_DEBUG                               205
#define IDD_HACKS                               206
#define IDD_TESTGFX                             207
#define IDD_ABOUT                               208

// Icons
#define IDI_DXGL                                301
#define IDI_DXGLSM                              302
#define IDI_STAR                                303
#define IDI_X16                                 304

// Controls - DXGL Config Dialog
#define IDC_APPLY                               1000
#define IDC_APPS                                1001
#define IDC_TABS                                1002
#define IDC_ADD                                 1003
#define IDC_REMOVE                              1004

// Bitmaps
#define IDB_DXGLINV                             311
#define IDB_DXGLINV64                           312

// Controls - Progress Dialog
//#define IDC_PROGRESS                            1101

// Controls - Display Tab
#define IDC_VIDMODE                             2001
#define IDC_COLORDEPTH                          2002
#define IDC_SCALE                               2003
#define IDC_EXTRAMODES                          2004
#define IDC_ASPECT                              2005
#define IDC_SORTMODES                           2006
#define IDC_DPISCALE                            2007
#define IDC_VSYNC                               2008
#define IDC_FULLMODE                            2009
#define IDC_FIXEDSCALELABEL                     2010
#define IDC_FIXEDSCALELABELX                    2011
#define IDC_FIXEDSCALEX                         2012
#define IDC_FIXEDSCALELABELY                    2013
#define IDC_FIXEDSCALEY                         2014
#define IDC_CUSTOMMODELABEL                     2015
#define IDC_CUSTOMMODE                          2016
#define IDC_SETMODE                             2017
#define IDC_COLOR                               2018
#define IDC_SINGLEBUFFER                        2019

// Controls - Effects Tab
#define IDC_POSTSCALE                           2101
#define IDC_POSTSCALESIZE                       2102
#define IDC_USESHADER                           2103
#define IDC_SHADER                              2104
#define IDC_BROWSESHADER                        2105
#define IDC_PRIMARYSCALE                        2106
#define IDC_BLTFILTER                           2107
#define IDC_CUSTOMSCALELABEL                    2108
#define IDC_CUSTOMSCALELABELX                   2109
#define IDC_CUSTOMSCALEX                        2110
#define IDC_CUSTOMSCALELABELY                   2111
#define IDC_CUSTOMSCALEY                        2112
// Removed for DXGL 0.5.13 release
// #define IDC_BLTTHRESHOLDSLIDER                  2113
// #define IDC_BLTTHRESHOLD                        2114

// Controls - 3D Graphics Tab
#define IDC_TEXFILTER                           2201
#define IDC_ANISO                               2202
#define IDC_MSAA                                2203
#define IDC_ASPECT3D                            2204
#define IDC_DITHERING                           2205

// Controls - Advanced Tab
#define IDC_TEXTUREFORMAT                       2301
#define IDC_TEXUPLOAD                           2302
#define IDC_DISPLAYNAME                         2303
#define IDC_LOWCOLORRENDER                      2304
#define IDC_WINDOWPOS                           2305
#define IDC_REMEMBERWINDOWPOS                   2306
#define IDC_REMEMBERWINDOWSIZE                  2307
#define IDC_WINDOWX                             2308
#define IDC_WINDOWY                             2309
#define IDC_WINDOWWIDTH                         2310
#define IDC_WINDOWHEIGHT                        2311
#define IDC_WINDOWMAXIMIZED                     2312
#define IDC_NOAUTOSIZE                          2313
#define IDC_PATHLABEL                           2314
#define IDC_PROFILEPATH                         2315
#define IDC_WRITEINI                            2316

// Controls - Debug Tab
#define IDC_DEBUGLIST                           2401
#define IDC_GLVERSION                           2402

// Controls - Hacks Tab
#define IDC_HACKSLIST                           2501

// Controls - Graphics Tests Tab
#define IDC_TESTLIST                            2601
#define IDC_VIDMODES                            2602
#define IDC_WINDOWED                            2603
#define IDC_FULLSCREEN                          2604
#define IDC_RESIZABLE                           2605
#define IDC_TESTVSYNC                           2606
#define IDC_APIVER                              2607
#define IDC_SPINAPI                             2608
#define IDC_BUFFERS                             2609
#define IDC_SPINBACK                            2610
#define IDC_FRAMERATE                           2611
#define IDC_SPINFRAME                           2612
#define IDC_FILTERLABEL                         2613
#define IDC_FILTER                              2614
#define IDC_FSAALABEL                           2615
#define IDC_FSAA                                2616
#define IDC_TEST                                2617

// Controls - About Tab
#define IDC_ABOUTTEXT                           2701
#define IDC_DDTYPE                              2702
#define IDC_DDVER                               2703
#define IDC_DXDIAG                              2704

// Controls - Shader Test Dialog Common
#define IDC_DISPLAY                             2801
#define IDC_TEXTURE                             2802
#define IDC_TEXTUREFILE                         2803
#define IDC_TEXTUREBROWSE                       2804
#define IDC_VERTEXFOGMODE                       2805
#define IDC_PIXELFOGMODE                        2806
#define IDC_FOGSTART                            2807
#define IDC_FOGEND                              2808
#define IDC_FOGDENSITY                          2809
#define IDC_RANGEBASEDFOG                       2810
#define IDC_FOGENABLE                           2811
#define IDC_DIFFUSE                             2812
#define IDC_DIFFUSESELECT                       2813
#define IDC_SPECULAR                            2814
#define IDC_SPECULARSELECT                      2815
#define IDC_FACTOR                              2816
#define IDC_FACTORSELECT                        2817
#define IDC_FOGCOLOR                            2818
#define IDC_FOGCOLORSELECT                      2819
#define IDC_BGCOLOR                             2820
#define IDC_BGCOLORSELECT                       2821


// Controls - Texture Shader Test Dialog
#define IDC_TEXSTAGE                            2901
#define IDC_SPINSTAGE                           2902
#define IDC_TEXCOLORKEY                         2903
#define IDC_SETTEXCOLORKEY                      2904
#define IDC_CARG1                               2905
#define IDC_CARG1INV                            2906
#define IDC_CARG1A                              2907
#define IDC_CARG2                               2908
#define IDC_CARG2INV                            2909
#define IDC_CARG2A                              2910
#define IDC_COLOROP                             2911
#define IDC_AARG1                               2912
#define IDC_AARG1INV                            2913
#define IDC_AARG1A                              2914
#define IDC_AARG2                               2915
#define IDC_AARG2INV                            2916
#define IDC_AARG2A                              2917
#define IDC_ALPHAOP                             2918
#define IDC_TEXTUREPREVIEW                      2919
#define IDC_ALPHABLEND                          2920
#define IDC_SRCBLEND                            2921
#define IDC_DESTBLEND                           2922
#define IDC_ALPHAREF                            2923
#define IDC_SPINALPHAREF                        2924
#define IDC_ALPHAFUNC                           2925
#define IDC_ALPHASTIPPLE                        2926
#define IDC_ALPHATEST                           2927
#define IDC_COLORKEY                            2928
#define IDC_COLORKEYBLEND                       2929
#define IDC_LINESTIPPLEPATTERN                  2930
#define IDC_LINESTIPPLEREPEAT                   2931
#define IDC_SPINLINEREPEAT                      2932
#define IDC_FILLSTIPPLELLLABEL                  2933
#define IDC_FILLSTIPPLETYPE                     2934
#define IDC_FILLSTIPPLEFILE                     2935
#define IDC_FILLSTIPPLEBROWSE                   2936
#define IDC_FILLSTIPPLEPREVIEW                  2937

// Controls - Vertex Test Dialog
#define IDC_AMBIENT                             3001
#define IDC_AMBIENTSELECT                       3002
#define IDC_EMISSIVE                            3003
#define IDC_EMISSIVESELECT                      3004
#define IDC_MATAMBIENT                          3005
#define IDC_MATAMBIENTSELECT                    3006
#define IDC_MATDIFFUSE                          3007
#define IDC_MATDIFFUSESELECT                    3008
#define IDC_MATSPECULAR                         3009
#define IDC_MATSPECULARSELECT                   3010
#define IDC_FILLMODE                            3011
#define IDC_SHADEMODE                           3012
#define IDC_CULLMODE                            3013
#define IDC_ENABLELIGHT                         3014
#define IDC_ENABLESPECULAR                      3015
#define IDC_VERTEXCOLOR                         3016
#define IDC_LOCALVIEWER                         3017
#define IDC_DETAIL                              3018
#define IDC_SPINDETAIL                          3019
#define IDC_DIFFUSESOURCE                       3020
#define IDC_AMBIENTSOURCE                       3021
#define IDC_SPECULARSOURCE                      3022
#define IDC_EMISSIVESOURCE                      3023
#define IDC_LIGHTNUMBER                         3024
#define IDC_SPINLIGHT                           3025
#define IDC_LIGHTDIFFUSE                        3026
#define IDC_LIGHTDIFFUSESELECT                  3027
#define IDC_LIGHTAMBIENT                        3028
#define IDC_LIGHTAMBIENTSELECT                  3029
#define IDC_LIGHTTYPE                           3030
#define IDC_LIGHTSPECULAR                       3031
#define IDC_LIGHTSPECULARSELECT                 3032
#define IDC_LIGHTRANGE                          3033
#define IDC_LIGHTENABLED                        3034
#define IDC_POWER                               3035
#define IDC_LIGHTFALLOFF                        3036
#define IDC_LIGHTATTEN0                         3037
#define IDC_LIGHTATTEN1                         3038
#define IDC_LIGHTATTEN2                         3039
#define IDC_LIGHTTHETA                          3040
#define IDC_LIGHTPHI                            3041

// Controls - Mode list dialog
#define IDC_MODELIST                            3101

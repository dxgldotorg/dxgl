// DXGL
// Copyright (C) 2011 William Feely

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
#define IDD_TEXSHADER                           103
#define IDD_VERTEXSHADER                        104
#define IDD_DXGLTEST                            199

// Tabs
#define IDD_TESTGFX                             206
#define IDD_SYSINFO                             207

// Icons
#define IDI_DXGL                                301
#define IDI_DXGLSM                              302
#define IDI_X16                                 304

// Bitmaps
#define IDB_DXGLINV                             311
#define IDB_DXGLINV64                           312

// Controls - DXGL Test Dialog
#define IDC_TABS                                1002

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

// Controls - System Information Tab
#define IDC_DDTYPE                              2701
#define IDC_DDVER                               2702
#define IDC_DXDIAG                              2703

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

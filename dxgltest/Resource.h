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
#define IDC_TESTLIST                            2501
#define IDC_VIDMODES                            2502
#define IDC_WINDOWED                            2503
#define IDC_FULLSCREEN                          2504
#define IDC_RESIZABLE                           2505
#define IDC_TESTVSYNC                           2506
#define IDC_APIVER                              2507
#define IDC_SPINAPI                             2508
#define IDC_BUFFERS                             2509
#define IDC_SPINBACK                            2510
#define IDC_FRAMERATE                           2511
#define IDC_SPINFRAME                           2512
#define IDC_FILTERLABEL                         2513
#define IDC_FILTER                              2514
#define IDC_FSAALABEL                           2515
#define IDC_FSAA                                2516
#define IDC_TEST                                2517

// Controls - System Information Tab
#define IDC_DDTYPE                              2601
#define IDC_DDVER                               2602
#define IDC_DXDIAG                              2603

// Controls - Shader Test Dialog Common
#define IDC_DISPLAY                             2701
#define IDC_TEXTURE                             2702
#define IDC_TEXTUREFILE                         2703
#define IDC_TEXTUREBROWSE                       2704
#define IDC_VERTEXFOGMODE                       2705
#define IDC_PIXELFOGMODE                        2706
#define IDC_FOGSTART                            2707
#define IDC_FOGEND                              2708
#define IDC_FOGDENSITY                          2709
#define IDC_RANGEBASEDFOG                       2710
#define IDC_FOGENABLE                           2711
#define IDC_DIFFUSE                             2712
#define IDC_DIFFUSESELECT                       2713
#define IDC_SPECULAR                            2714
#define IDC_SPECULARSELECT                      2715
#define IDC_FACTOR                              2716
#define IDC_FACTORSELECT                        2717
#define IDC_FOGCOLOR                            2718
#define IDC_FOGCOLORSELECT                      2719
#define IDC_BGCOLOR                             2720
#define IDC_BGCOLORSELECT                       2721


// Controls - Texture Shader Test Dialog
#define IDC_TEXSTAGE                            2801
#define IDC_SPINSTAGE                           2802
#define IDC_TEXCOLORKEY                         2803
#define IDC_SETTEXCOLORKEY                      2804
#define IDC_CARG1                               2805
#define IDC_CARG1INV                            2806
#define IDC_CARG1A                              2807
#define IDC_CARG2                               2808
#define IDC_CARG2INV                            2809
#define IDC_CARG2A                              2810
#define IDC_COLOROP                             2811
#define IDC_AARG1                               2812
#define IDC_AARG1INV                            2813
#define IDC_AARG1A                              2814
#define IDC_AARG2                               2815
#define IDC_AARG2INV                            2816
#define IDC_AARG2A                              2817
#define IDC_ALPHAOP                             2818
#define IDC_TEXTUREPREVIEW                      2819
#define IDC_ALPHABLEND                          2820
#define IDC_SRCBLEND                            2821
#define IDC_DESTBLEND                           2822
#define IDC_ALPHAREF                            2823
#define IDC_SPINALPHAREF                        2824
#define IDC_ALPHAFUNC                           2825
#define IDC_ALPHASTIPPLE                        2826
#define IDC_ALPHATEST                           2827
#define IDC_COLORKEY                            2828
#define IDC_COLORKEYBLEND                       2829
#define IDC_LINESTIPPLEPATTERN                  2830
#define IDC_LINESTIPPLEREPEAT                   2831
#define IDC_SPINLINEREPEAT                      2832
#define IDC_FILLSTIPPLELLLABEL                  2833
#define IDC_FILLSTIPPLETYPE                     2834
#define IDC_FILLSTIPPLEFILE                     2835
#define IDC_FILLSTIPPLEBROWSE                   2836
#define IDC_FILLSTIPPLEPREVIEW                  2837

// Controls - Vertex Test Dialog
#define IDC_AMBIENT                             2901
#define IDC_AMBIENTSELECT                       2902
#define IDC_EMISSIVE                            2903
#define IDC_EMISSIVESELECT                      2904
#define IDC_MATAMBIENT                          2905
#define IDC_MATAMBIENTSELECT                    2906
#define IDC_MATDIFFUSE                          2907
#define IDC_MATDIFFUSESELECT                    2908
#define IDC_MATSPECULAR                         2909
#define IDC_MATSPECULARSELECT                   2910
#define IDC_FILLMODE                            2911
#define IDC_SHADEMODE                           2912
#define IDC_CULLMODE                            2913
#define IDC_ENABLELIGHT                         2914
#define IDC_ENABLESPECULAR                      2915
#define IDC_VERTEXCOLOR                         2916
#define IDC_LOCALVIEWER                         2917
#define IDC_DETAIL                              2918
#define IDC_SPINDETAIL                          2919
#define IDC_DIFFUSESOURCE                       2920
#define IDC_AMBIENTSOURCE                       2921
#define IDC_SPECULARSOURCE                      2922
#define IDC_EMISSIVESOURCE                      2923
#define IDC_LIGHTNUMBER                         2924
#define IDC_SPINLIGHT                           2925
#define IDC_LIGHTDIFFUSE                        2926
#define IDC_LIGHTDIFFUSESELECT                  2927
#define IDC_LIGHTAMBIENT                        2928
#define IDC_LIGHTAMBIENTSELECT                  2929
#define IDC_LIGHTTYPE                           2930
#define IDC_LIGHTSPECULAR                       2931
#define IDC_LIGHTSPECULARSELECT                 2932
#define IDC_LIGHTRANGE                          2933
#define IDC_LIGHTENABLED                        2934
#define IDC_POWER                               2935
#define IDC_LIGHTFALLOFF                        2936
#define IDC_LIGHTATTEN0                         2937
#define IDC_LIGHTATTEN1                         2938
#define IDC_LIGHTATTEN2                         2939
#define IDC_LIGHTTHETA                          2940
#define IDC_LIGHTPHI                            2941

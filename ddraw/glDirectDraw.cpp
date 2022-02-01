// DXGL
// Copyright (C) 2011-2021 William Feely

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

#include "common.h"
#include "util.h"
#include <string>
using namespace std;
#include "shadergen2d.h"
#include "ddraw.h"
#include "timer.h"
#include "glDirect3D.h"
#include "glDirectDraw.h"
#include "glTexture.h"
#include "glUtil.h"
#include "glDirectDrawClipper.h"
#include "dxglDirectDrawSurface.h"
#include "glDirectDrawPalette.h"
#include "glRenderer.h"
#include "../common/version.h"
#include "hooks.h"
#include "fourcc.h"
#ifndef SDC_APPLY
#include "include/mingw-setdisplayconfig.h"
#endif

#ifndef DISP_CHANGE_BADDUALVIEW
#define DISP_CHANGE_BADDUALVIEW -6
#endif

extern "C" {

BOOL NoSetDisplayConfig = FALSE;
LONG (WINAPI *_GetDisplayConfigBufferSizes)(UINT32 flags, UINT32 *numPathArrayElements,
	UINT32 *numModeInfoArrayElements) = NULL;
LONG (WINAPI *_QueryDisplayConfig)(UINT32 flags, UINT32 *numPathArrayElements,
	DISPLAYCONFIG_PATH_INFO *pathArray, UINT32 *numModeInfoArrayElements,
	DISPLAYCONFIG_MODE_INFO *modeInfoArray, DISPLAYCONFIG_TOPOLOGY_ID *currentTopologyId) = NULL;
LONG (WINAPI *_SetDisplayConfig)(UINT32 numPathArrayElements, DISPLAYCONFIG_PATH_INFO *pathArray,
	UINT32 numModeInfoArrayElements, DISPLAYCONFIG_MODE_INFO *modeInfoArray, UINT32 flags) = NULL;

LONG SetVidMode(LPCTSTR devname, DEVMODE *mode, DWORD flags)
{
	DEVMODE currmode;
	HMODULE hUser32;
	UINT32 numPathArrayElements = 0;
	UINT32 numModeInfoArrayElements = 0;
	DISPLAYCONFIG_PATH_INFO *pathArray;
	DISPLAYCONFIG_MODE_INFO *modeInfoArray;
	LONG error;
	int i;
	if (dxglcfg.UseSetDisplayConfig)
	{
		currmode.dmSize = sizeof(DEVMODE);
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &currmode);
		if (mode->dmDisplayFrequency &&
			(mode->dmDisplayFrequency != currmode.dmDisplayFrequency)) // Mismatched refresh
			return ChangeDisplaySettingsEx(devname, mode, NULL, flags, NULL);
		if(NoSetDisplayConfig) return ChangeDisplaySettingsEx(devname, mode, NULL, flags, NULL);
		else
		{
			hUser32 = GetModuleHandle(_T("user32.dll"));
			if (!hUser32) NoSetDisplayConfig = TRUE;
			if (!NoSetDisplayConfig)
			{
				_GetDisplayConfigBufferSizes = (LONG(WINAPI*)(UINT32, UINT32*, UINT32*))
					GetProcAddress(hUser32, "GetDisplayConfigBufferSizes");
				if (!_GetDisplayConfigBufferSizes) NoSetDisplayConfig = TRUE;
			}
			if (!NoSetDisplayConfig)
			{
				_QueryDisplayConfig = (LONG(WINAPI*)(UINT32, UINT32*, DISPLAYCONFIG_PATH_INFO*,
					UINT32*, DISPLAYCONFIG_MODE_INFO*, DISPLAYCONFIG_TOPOLOGY_ID*))
					GetProcAddress(hUser32, "QueryDisplayConfig");
				if (!_QueryDisplayConfig) NoSetDisplayConfig = TRUE;
			}
			if (!NoSetDisplayConfig)
			{
				_SetDisplayConfig = (LONG(WINAPI*)(UINT32, DISPLAYCONFIG_PATH_INFO*, UINT32,
					DISPLAYCONFIG_MODE_INFO*, UINT32))
					GetProcAddress(hUser32, "SetDisplayConfig");
				if (!_SetDisplayConfig) NoSetDisplayConfig = TRUE;
			}
			if (!NoSetDisplayConfig)
			{
				error = _GetDisplayConfigBufferSizes(QDC_ALL_PATHS, &numPathArrayElements, &numModeInfoArrayElements);
				if (error != ERROR_SUCCESS) NoSetDisplayConfig = TRUE;
				if (!NoSetDisplayConfig)
				{
					pathArray = (DISPLAYCONFIG_PATH_INFO*)malloc(numPathArrayElements * sizeof(DISPLAYCONFIG_PATH_INFO));
					if(!pathArray) NoSetDisplayConfig = TRUE;
					if (!NoSetDisplayConfig)
					{
						modeInfoArray = (DISPLAYCONFIG_MODE_INFO *)malloc(numModeInfoArrayElements * sizeof(DISPLAYCONFIG_MODE_INFO));
						if (!modeInfoArray)
						{
							free(pathArray);
							NoSetDisplayConfig = TRUE;
						}
						if (!NoSetDisplayConfig)
						{
							ZeroMemory(pathArray, numPathArrayElements * sizeof(DISPLAYCONFIG_PATH_INFO));
							ZeroMemory(modeInfoArray, numModeInfoArrayElements * sizeof(DISPLAYCONFIG_MODE_INFO));
							_QueryDisplayConfig(QDC_ALL_PATHS, &numPathArrayElements, pathArray, &numModeInfoArrayElements, modeInfoArray, NULL);
							i = pathArray[0].sourceInfo.modeInfoIdx;
							modeInfoArray[i].sourceMode.width = mode->dmPelsWidth;
							modeInfoArray[i].sourceMode.height = mode->dmPelsHeight;
							switch (mode->dmBitsPerPel)
							{
							case 32:
							default:
								modeInfoArray[i].sourceMode.pixelFormat = DISPLAYCONFIG_PIXELFORMAT_32BPP;
								break;
							case 24:
								modeInfoArray[i].sourceMode.pixelFormat = DISPLAYCONFIG_PIXELFORMAT_24BPP;
								break;
							case 16:
							case 15:
								modeInfoArray[i].sourceMode.pixelFormat = DISPLAYCONFIG_PIXELFORMAT_16BPP;
								break;
							case 8:
								modeInfoArray[i].sourceMode.pixelFormat = DISPLAYCONFIG_PIXELFORMAT_8BPP;
								break;
							}
							error = _SetDisplayConfig(numPathArrayElements, pathArray, numModeInfoArrayElements, modeInfoArray,
								SDC_APPLY | SDC_USE_SUPPLIED_DISPLAY_CONFIG | SDC_ALLOW_CHANGES | SDC_SAVE_TO_DATABASE);
							free(pathArray);
							free(modeInfoArray);
							if (error == ERROR_SUCCESS) return DISP_CHANGE_SUCCESSFUL;
							else return DISP_CHANGE_BADMODE;
						}
					}
				}
			}
			else return ChangeDisplaySettingsEx(devname, mode, NULL, flags, NULL);
			if(NoSetDisplayConfig) return ChangeDisplaySettingsEx(devname, mode, NULL, flags, NULL);
		}
	}
	else return ChangeDisplaySettingsEx(devname, mode, NULL, flags, NULL);
}

const DDDEVICEIDENTIFIER2 devid_default = {
	"ddraw.dll",
	"DXGL DDraw Wrapper",
	0,
	0,0,0,0,
	0,0};

const D3DDevice d3ddevices[3] =
{
	{
		"Simulated RGB Rasterizer",
		"DXGL RGB Rasterizer",
	},
	{
		"DXGL Hardware Accelerator",
		"DXGL D3D HAL",
	},
	{
		"DXGL Hardware Accelerator with Transform and Lighting",
		"DXGL D3D T&L HAL",
	}
};

void DiscardDuplicateModes(DEVMODE **array, DWORD *count)
{
	DEVMODE *newarray = (DEVMODE *)malloc(sizeof(DEVMODE)*(*count));
	if(!newarray) return;
	DWORD newcount = 0;
	bool match;
	for(DWORD x = 0; x < (*count); x++)
	{
		match = false;
		memcpy(&newarray[newcount],&(*array)[x],sizeof(DEVMODE));
		for(int y = newcount; y > 0; y--)
		{
			if((*array)[x].dmBitsPerPel == newarray[y-1].dmBitsPerPel &&
				(*array)[x].dmDisplayFrequency == newarray[y-1].dmDisplayFrequency &&
				(*array)[x].dmPelsWidth == newarray[y-1].dmPelsWidth &&
				(*array)[x].dmPelsHeight == newarray[y-1].dmPelsHeight)
			{
				match = true;
				break;
			}
		}
		if(!match) newcount++;
	}
	DEVMODE *newarray2 = (DEVMODE*)realloc(newarray,sizeof(DEVMODE)*newcount);
	if(newarray2) newarray = newarray2;
	free(*array);
	*array = newarray;
	*count = newcount;
}

bool ScanModeList(DEVMODE *array, DEVMODE comp, DWORD count)
{
	for(DWORD i = 0; i < count; i++)
	{
		if(!memcmp(&array[i],&comp,sizeof(DEVMODE))) return true;
	}
	return false;
}

const int START_LOWRESMODES = __LINE__;
const int LowResModes[][3] =
{
	{ 320,200,70 },
	{ 320,240,60 },
	{ 320,400,70 },
	{ 320,480,60 },
	{ 640,400,70 }
};
const int END_LOWRESMODES = __LINE__ - 4;
const int NumLowResModes = END_LOWRESMODES - START_LOWRESMODES;

const int START_UNCOMMONLOWRESMODES = __LINE__;
const int UncommonLowResModes[][3] =
{
	{ 360,200,70 },
	{ 360,240,60 },
	{ 360,400,70 },
	{ 360,480,60 },
	{ 400,300,60 }
};
const int END_UNCOMMONLOWRESMODES = __LINE__ - 4;
const int NumUncommonLowResModes = END_UNCOMMONLOWRESMODES - START_UNCOMMONLOWRESMODES;

const int START_UNCOMMONSDMODES = __LINE__;
const int UncommonSDModes[][3] =
{
	{ 512,384,60 },
	{ 640,350,70 },
	{ 640,360,60 },
	{ 720,400,70 },
	{ 720,480,60 },
	{ 960,540,60 },
	{ 960,600,60 },
	{ 960,720,60 },
	{ 1024,600,60 }
};
const int END_UNCOMMONSDMODES = __LINE__ - 4;
const int NumUncommonSDModes = END_UNCOMMONSDMODES - START_UNCOMMONSDMODES;

const int START_HDMODES = __LINE__;
const int HDModes[][3] =
{
	{ 1024,800,60 },
	{ 1280,720,60 },
	{ 1280,800,60 },
	{ 1360,768,60 },
	{ 1366,768,60 },
	{ 1400,1050,60 },
	{ 1440,900,60 },
	{ 1600,800,60 },
	{ 1680,1050,60 },
	{ 1920,1080,60 },
	{ 1920,1200,60 },
	{ 2048,1080,60 }
};
const int END_HDMODES = __LINE__ - 4;
const int NumHDModes = END_HDMODES - START_HDMODES;

const int START_UHDMODES = __LINE__;
const int UHDModes[][3] =
{
	{ 2560,1080,60 },
	{ 2560,1440,60 },
	{ 2560,1600,60 },
	{ 2560,1920,60 },
	{ 2560,2048,60 },
	{ 2800,2100,60 },
	{ 3200,1800,60 },
	{ 3200,2048,60 },
	{ 3840,2160,60 },
	{ 3840,2400,60 },
	{ 4096,2304,60 },
	{ 4096,3072,60 }
};
const int END_UHDMODES = __LINE__ - 4;
const int NumUHDModes = END_UHDMODES - START_UHDMODES;

const int START_UHD2MODES = __LINE__;
const int UHD2Modes[][3] =
{
	{ 5120,2880,60 },
	{ 5120,3200,60 },
	{ 5120,4096,60 },
	{ 6400,4800,60 },
	{ 7680,4320,60 },
	{ 7680,4800,60 },
	{ 8192,4608,60 }
};
const int END_UHD2MODES = __LINE__ - 4;
const int NumUHD2Modes = END_UHD2MODES - START_UHD2MODES;

const int START_UNCOMMONMODES = __LINE__;
const int UncommonModes[][3] =
{
	{ 240,160,60 },
	{ 256,224,60 },
	{ 256,240,60 },
	{ 320,175,60 },
	{ 320,224,60 },
	{ 400,240,60 },
	{ 416,312,75 },
	{ 512,448,60 },
	{ 512,480,60 },
	{ 576,432,60 },
	{ 640,200,60 },
	{ 640,512,60 },
	{ 700,525,60 },
	{ 720,350,70 },
	{ 720,450,60 },
	{ 800,512,60 },
	{ 832,624,75 },
	{ 840,525,60 },
	{ 896,672,60 },
	{ 928,696,60 },
};
const int END_UNCOMMONMODES = __LINE__ - 4;
const int NumUncommonModes = END_UNCOMMONMODES - START_UNCOMMONMODES;

const int START_COMMONSVGAMODES = __LINE__;
const int CommonSVGAModes[][3] =
{
	{ 640,480,60 },
	{ 800,600,56 },
	{ 800,600,60 },
	{ 1024,768,60 },
	{ 1152,864,60 },
	{ 1280,960,60 },
	{ 1280,1024,60 },
	{ 1600,1200,60 },
	{ 1920,1440,60 },
	{ 2048,1536,60 }
};
const int END_COMMONSVGAMODES = __LINE__ - 4;
const int NumCommonSVGAModes = END_COMMONSVGAMODES - START_COMMONSVGAMODES;

const int START_DOUBLEDMODESCOUNT = __LINE__;
const int DoubledModes[] [5] = 
{
	{320,175,70,640,350},
	{320,200,70,640,400},
	{320,240,60,640,480},
	{320,400,70,640,400},
	{320,480,60,640,480},
	{640,200,70,640,400},
	{360,200,70,720,400},
	{360,240,60,720,480},
	{360,400,70,720,400},
	{360,480,60,720,480},
	{400,300,56,800,600},
	{400,300,60,800,600}
};
const int END_DOUBLEDMODESCOUNT = __LINE__ - 4;
const int numdoubledmodes = END_DOUBLEDMODESCOUNT - START_DOUBLEDMODESCOUNT;

bool ScanColorMode(DEVMODE *array, DWORD count, int bpp)
{
	for(DWORD i = 0; i < count; i++)
	{
		if(array[i].dmBitsPerPel == bpp) return true;
	}
	return false;
}

bool ScanModeListNoRefresh(DEVMODE *array, DEVMODE comp, DWORD count)
{
	for(DWORD i = 0; i < count; i++)
	{
		if((array[i].dmBitsPerPel == comp.dmBitsPerPel) &&
			(array[i].dmPelsWidth == comp.dmPelsWidth) &&
			(array[i].dmPelsHeight == comp.dmPelsHeight)) return true;
	}
	return false;
} 

void AddExtraResolutions(DEVMODE **array, DWORD *count, const int (*modelist)[3], const int nummodes)
{
	DEVMODE *array2 = (DEVMODE *)malloc(sizeof(DEVMODE) * 5 * nummodes);
	DEVMODE compmode = *array[0];
	DWORD newcount = 0;
	int i;
	if (ScanColorMode(*array, *count, 8))
	{
		compmode.dmBitsPerPel = 8;
		for (i = 0; i < nummodes; i++)
		{
			compmode.dmPelsWidth = modelist[i][0];
			compmode.dmPelsHeight = modelist[i][1];
			compmode.dmDisplayFrequency = modelist[i][2];
			if (!ScanModeListNoRefresh(*array, compmode, *count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if (ScanColorMode(*array, *count, 15))
	{
		compmode.dmBitsPerPel = 15;
		for (i = 0; i < nummodes; i++)
		{
			compmode.dmPelsWidth = modelist[i][0];
			compmode.dmPelsHeight = modelist[i][1];
			compmode.dmDisplayFrequency = modelist[i][2];
			if (!ScanModeListNoRefresh(*array, compmode, *count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if (ScanColorMode(*array, *count, 16))
	{
		compmode.dmBitsPerPel = 16;
		for (i = 0; i < nummodes; i++)
		{
			compmode.dmPelsWidth = modelist[i][0];
			compmode.dmPelsHeight = modelist[i][1];
			compmode.dmDisplayFrequency = modelist[i][2];
			if (!ScanModeListNoRefresh(*array, compmode, *count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if (ScanColorMode(*array, *count, 24))
	{
		compmode.dmBitsPerPel = 24;
		for (i = 0; i < nummodes; i++)
		{
			compmode.dmPelsWidth = modelist[i][0];
			compmode.dmPelsHeight = modelist[i][1];
			compmode.dmDisplayFrequency = modelist[i][2];
			if (!ScanModeListNoRefresh(*array, compmode, *count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if (ScanColorMode(*array, *count, 32))
	{
		compmode.dmBitsPerPel = 32;
		for (i = 0; i < nummodes; i++)
		{
			compmode.dmPelsWidth = modelist[i][0];
			compmode.dmPelsHeight = modelist[i][1];
			compmode.dmDisplayFrequency = modelist[i][2];
			if (!ScanModeListNoRefresh(*array, compmode, *count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	*array = (DEVMODE *)realloc(*array, (*count + newcount) * sizeof(DEVMODE));
	memcpy(&(*array)[*count - 1], array2, newcount * sizeof(DEVMODE));
	free(array2);
	*count += newcount;
}

void AddDoubledResolutions(DEVMODE **array, DWORD *count)
{
	DEVMODE *array2 = (DEVMODE *)malloc(sizeof(DEVMODE) * 5 * numdoubledmodes);
	DEVMODE compmode = *array[0];
	DWORD newcount = 0;
	int i;
	if (ScanColorMode(*array, *count, 8))
	{
		compmode.dmBitsPerPel = 8;
		for (i = 0; i < numdoubledmodes; i++)
		{
			compmode.dmPelsWidth = DoubledModes[i][3];
			compmode.dmPelsHeight = DoubledModes[i][4];
			compmode.dmDisplayFrequency = DoubledModes[i][2];
			if (ScanModeListNoRefresh(*array, compmode, *count))
			{
				compmode.dmPelsWidth = DoubledModes[i][0];
				compmode.dmPelsHeight = DoubledModes[i][1];
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if (ScanColorMode(*array, *count, 15))
	{
		compmode.dmBitsPerPel = 15;
		for (i = 0; i < numdoubledmodes; i++)
		{
			compmode.dmPelsWidth = DoubledModes[i][3];
			compmode.dmPelsHeight = DoubledModes[i][4];
			compmode.dmDisplayFrequency = DoubledModes[i][2];
			if (ScanModeListNoRefresh(*array, compmode, *count))
			{
				compmode.dmPelsWidth = DoubledModes[i][0];
				compmode.dmPelsHeight = DoubledModes[i][1];
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if (ScanColorMode(*array, *count, 16))
	{
		compmode.dmBitsPerPel = 16;
		for (i = 0; i < numdoubledmodes; i++)
		{
			compmode.dmPelsWidth = DoubledModes[i][3];
			compmode.dmPelsHeight = DoubledModes[i][4];
			compmode.dmDisplayFrequency = DoubledModes[i][2];
			if (ScanModeListNoRefresh(*array, compmode, *count))
			{
				compmode.dmPelsWidth = DoubledModes[i][0];
				compmode.dmPelsHeight = DoubledModes[i][1];
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if (ScanColorMode(*array, *count, 24))
	{
		compmode.dmBitsPerPel = 24;
		for (i = 0; i < numdoubledmodes; i++)
		{
			compmode.dmPelsWidth = DoubledModes[i][3];
			compmode.dmPelsHeight = DoubledModes[i][4];
			compmode.dmDisplayFrequency = DoubledModes[i][2];
			if (ScanModeListNoRefresh(*array, compmode, *count))
			{
				compmode.dmPelsWidth = DoubledModes[i][0];
				compmode.dmPelsHeight = DoubledModes[i][1];
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if (ScanColorMode(*array, *count, 32))
	{
		compmode.dmBitsPerPel = 32;
		for (i = 0; i < numdoubledmodes; i++)
		{
			compmode.dmPelsWidth = DoubledModes[i][3];
			compmode.dmPelsHeight = DoubledModes[i][4];
			compmode.dmDisplayFrequency = DoubledModes[i][2];
			if (ScanModeListNoRefresh(*array, compmode, *count))
			{
				compmode.dmPelsWidth = DoubledModes[i][0];
				compmode.dmPelsHeight = DoubledModes[i][1];
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	*array = (DEVMODE *)realloc(*array, (*count + newcount) * sizeof(DEVMODE));
	memcpy(&(*array)[*count - 1], array2, newcount * sizeof(DEVMODE));
	free(array2);
	*count += newcount;
}

void RemoveTVAspectModes(DEVMODE **array, DWORD count)
{
	for (DWORD i = 0; i < count; i++)
	{
		if ((*array)[i].dmDisplayFrequency == 23)
			(*array)[i].dmDisplayFrequency = 24;
		else if ((*array)[i].dmDisplayFrequency == 29)
			(*array)[i].dmDisplayFrequency = 30;
		else if ((*array)[i].dmDisplayFrequency == 47)
			(*array)[i].dmDisplayFrequency = 48;
		else if ((*array)[i].dmDisplayFrequency == 59)
			(*array)[i].dmDisplayFrequency = 60;
		else if ((*array)[i].dmDisplayFrequency == 71)
			(*array)[i].dmDisplayFrequency = 72;
		else if ((*array)[i].dmDisplayFrequency == 119)
			(*array)[i].dmDisplayFrequency = 120;
	}
}


void AddExtraColorModes(DEVMODE **array, DWORD *count)
{
	DEVMODE *array2 = (DEVMODE *)malloc(sizeof(DEVMODE)*(7*(*count)));
	DEVMODE compmode;
	DWORD count2 = 0;
	for(DWORD i = 0; i < *count; i++)
	{
		switch((*array)[i].dmBitsPerPel)
		{
		case 15:
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 16;
			if(!ScanModeList(*array,compmode,*count) && (dxglcfg.AddColorDepths & 4))
			{
				array2[count2] = compmode;
				count2++;
			}
			break;
		case 16:
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 15;
			if(!ScanModeList(*array,compmode,*count) && (dxglcfg.AddColorDepths & 2))
			{
				array2[count2] = compmode;
				count2++;
			}
			break;
		case 24:
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 32;
			if(!ScanModeList(*array,compmode,*count) && (dxglcfg.AddColorDepths & 16))
			{
				array2[count2] = compmode;
				count2++;
			}
			break;
		case 32:
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 24;
			if(!ScanModeList(*array,compmode,*count) && (dxglcfg.AddColorDepths & 8))
			{
				array2[count2] = compmode;
				count2++;
			}
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 16;
			if(!ScanModeList(*array,compmode,*count) && (dxglcfg.AddColorDepths & 4))
			{
				array2[count2] = compmode;
				count2++;
			}
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 15;
			if(!ScanModeList(*array,compmode,*count) && (dxglcfg.AddColorDepths & 2))
			{
				array2[count2] = compmode;
				count2++;
			}
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 8;
			if(!ScanModeList(*array,compmode,*count) && (dxglcfg.AddColorDepths & 1))
			{
				array2[count2] = compmode;
				count2++;
			}
			break;
		default:
			break;
		}
	}
	*array = (DEVMODE *)realloc(*array,(*count+count2)*sizeof(DEVMODE));
	memcpy(&(*array)[*count-1],array2,count2*sizeof(DEVMODE));
	free(array2);
	*count += count2;
	if ((dxglcfg.AddColorDepths & 2) && !(dxglcfg.AddColorDepths & 4))
	{
		for (DWORD x = 0; x < (*count); x++)
		{
			if ((*array)[x].dmBitsPerPel == 15) (*array)[x].dmBitsPerPel = 16;
		}
	}
}

int SortDepth(const DEVMODE *mode1, const DEVMODE *mode2)
{
	unsigned __int64 mode1num = 
		(mode1->dmDisplayFrequency & 0xFFFF) |
		((unsigned __int64)(mode1->dmPelsWidth & 0xFFFF)<< 16) |
		((unsigned __int64)(mode1->dmPelsHeight & 0xFFFF)<< 32) |
		((unsigned __int64)(mode1->dmBitsPerPel & 0xFFFF)<< 48);
	unsigned __int64 mode2num = 
		(mode2->dmDisplayFrequency & 0xFFFF) |
		((unsigned __int64)(mode2->dmPelsWidth & 0xFFFF)<< 16) |
		((unsigned __int64)(mode2->dmPelsHeight & 0xFFFF)<< 32) |
		((unsigned __int64)(mode2->dmBitsPerPel & 0xFFFF)<< 48);
	if(mode2num > mode1num) return -1;
	else if(mode2num < mode1num) return  1;
	else return 0;
}
int SortRes(const DEVMODE *mode1, const DEVMODE *mode2)
{
	unsigned __int64 mode1num = 
		(mode1->dmDisplayFrequency & 0xFFFF) |
		((unsigned __int64)(mode1->dmPelsWidth & 0xFFFF)<< 32) |
		((unsigned __int64)(mode1->dmPelsHeight & 0xFFFF)<< 48) |
		((unsigned __int64)(mode1->dmBitsPerPel & 0xFFFF)<< 16);
	unsigned __int64 mode2num = 
		(mode2->dmDisplayFrequency & 0xFFFF) |
		((unsigned __int64)(mode2->dmPelsWidth & 0xFFFF)<< 32) |
		((unsigned __int64)(mode2->dmPelsHeight & 0xFFFF)<< 48) |
		((unsigned __int64)(mode2->dmBitsPerPel & 0xFFFF)<< 16);
	if(mode2num > mode1num) return -1;
	else if(mode2num < mode1num) return  1;
	else return 0;
}

HRESULT EnumDisplayModes1(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	if(!lpEnumModesCallback) return DDERR_INVALIDPARAMS;
	if (dwFlags & 0xFFFFFFFC) return DDERR_INVALIDPARAMS;
	BOOL match;
	BOOL scalemodes;
	DWORD modenum = 0;
	DWORD modemax = 128;
	DEVMODE mode;
	ZeroMemory(&mode,sizeof(DEVMODE));
	mode.dmSize = sizeof(DEVMODE);
	DEVMODE *modes = (DEVMODE*)malloc(128*sizeof(DEVMODE));
	DEVMODE *tmp;
	if(!modes) ERR(DDERR_OUTOFMEMORY);
	DDSURFACEDESC ddmode;
	ZeroMemory(&ddmode,sizeof(DDSURFACEDESC));
	ddmode.dwSize = sizeof(DDSURFACEDESC);
	ddmode.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
	ddmode.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	if (!_isnan(dxglcfg.postsizex) && !_isnan(dxglcfg.postsizey) &&
		(dxglcfg.postsizex > 0.25f) && (dxglcfg.postsizey > 0.25f) &&
		(dxglcfg.postsizex != 1.0f) && (dxglcfg.postsizey != 1.0f) &&
		((dxglcfg.scaler == 0) || ((dxglcfg.scaler >= 4) && (dxglcfg.scaler <= 6))) &&
		(!dxglcfg.primaryscale))
		scalemodes = TRUE;
	else scalemodes = FALSE;
	while(EnumDisplaySettings(NULL,modenum++,&mode))
	{
		if (scalemodes)
		{
			mode.dmPelsWidth = (DWORD)((float)mode.dmPelsWidth / dxglcfg.postsizex);
			mode.dmPelsHeight = (DWORD)((float)mode.dmPelsHeight / dxglcfg.postsizey);
		}
		modes[modenum-1] = mode;
		if(modenum >= modemax)
		{
			modemax += 128;
			tmp = (DEVMODE*)realloc(modes,modemax*sizeof(DEVMODE));
			if(tmp == NULL)
			{
				free(modes);
				ERR(DDERR_OUTOFMEMORY);
			}
			modes = tmp;
		}
	}
	DiscardDuplicateModes(&modes,&modenum);
	if(dxglcfg.AddColorDepths) AddExtraColorModes(&modes,&modenum);
	if (dxglcfg.scaler != 0)
	{
		if (dxglcfg.AddModes & 1) //Common low resolutions and doubled modes
			AddExtraResolutions(&modes, &modenum, LowResModes, NumLowResModes);
		if (dxglcfg.AddModes & 2) //Uncommon low resolutions
			AddExtraResolutions(&modes, &modenum, UncommonLowResModes, NumUncommonLowResModes);
		if (dxglcfg.AddModes & 4) //Uncommon SD reosolutions
			AddExtraResolutions(&modes, &modenum, UncommonSDModes, NumUncommonSDModes);
		if (dxglcfg.AddModes & 8) //High definition resolutions
			AddExtraResolutions(&modes, &modenum, HDModes, NumHDModes);
		if (dxglcfg.AddModes & 16) //Ultra-HD resolutions
			AddExtraResolutions(&modes, &modenum, UHDModes, NumUHDModes);
		if (dxglcfg.AddModes & 32) //Ultra-HD resolutions above 4k
			AddExtraResolutions(&modes, &modenum, UHD2Modes, NumUHD2Modes);
		if (dxglcfg.AddModes & 64) //Very uncommon resolutions
			AddExtraResolutions(&modes, &modenum, UncommonModes, NumUncommonModes);
		if (dxglcfg.AddModes & 128) //Common SVGA modes
			AddExtraResolutions(&modes, &modenum, CommonSVGAModes, NumCommonSVGAModes);
	}
	if (dxglcfg.AddModes && (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
		(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f)))
	{
		if (dxglcfg.AddModes & 1) AddDoubledResolutions(&modes, &modenum);
	}
	if (dxglcfg.HackNoTVRefresh) RemoveTVAspectModes(&modes, modenum);
	DiscardDuplicateModes(&modes, &modenum);
	modenum--;
	switch(dxglcfg.SortModes)
	{
	case 0:
	default:
		break;
	case 1:
		qsort(modes,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortDepth);
		break;
	case 2:
		qsort(modes,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortRes);
		break;
	}
	for(DWORD i = 0; i < modenum; i++)
	{
		match = TRUE;
		if(dwFlags & DDEDM_REFRESHRATES) ddmode.dwRefreshRate = modes[i].dmDisplayFrequency;
		else
		{
			ddmode.dwRefreshRate = 0;
			for(DWORD x = 0; x < i; x++)
				if((modes[x].dmBitsPerPel == modes[i].dmBitsPerPel) &&
					(modes[x].dmPelsWidth == modes[i].dmPelsWidth) &&
					(modes[x].dmPelsHeight == modes[i].dmPelsHeight)) match = FALSE;
		}
		if(lpDDSurfaceDesc)
		{
			if(lpDDSurfaceDesc->dwFlags & DDSD_WIDTH)
				if(lpDDSurfaceDesc->dwWidth != modes[i].dmPelsWidth) match = FALSE;
			if(lpDDSurfaceDesc->dwFlags & DDSD_HEIGHT)
				if(lpDDSurfaceDesc->dwHeight != modes[i].dmPelsHeight) match = FALSE;
			if(lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
				if(lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount != modes[i].dmBitsPerPel) match = FALSE;
			if(lpDDSurfaceDesc->dwFlags & DDSD_REFRESHRATE)
				if(lpDDSurfaceDesc->dwRefreshRate != modes[i].dmDisplayFrequency) match = FALSE;
		}
		if(match)
		{
			if(modes[i].dmBitsPerPel == 8) ddmode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
			else if(modes[i].dmBitsPerPel == 4) ddmode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED4;
			else ddmode.ddpfPixelFormat.dwFlags = DDPF_RGB;
			if(modes[i].dmBitsPerPel == 8)
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0;
				ddmode.ddpfPixelFormat.dwGBitMask = 0;
				ddmode.ddpfPixelFormat.dwBBitMask = 0;
			}
			else if(modes[i].dmBitsPerPel == 15)
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0x7C00;
				ddmode.ddpfPixelFormat.dwGBitMask = 0x3E0;
				ddmode.ddpfPixelFormat.dwRBitMask = 0x1F;
			}
			else if(modes[i].dmBitsPerPel == 16)
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0xF800;
				ddmode.ddpfPixelFormat.dwGBitMask = 0x7E0;
				ddmode.ddpfPixelFormat.dwBBitMask = 0x1F;
			}
			else
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddmode.ddpfPixelFormat.dwRBitMask = 0xFF00;
				ddmode.ddpfPixelFormat.dwRBitMask = 0xFF;
			}
			ddmode.ddpfPixelFormat.dwRGBBitCount = modes[i].dmBitsPerPel;
			ddmode.dwWidth = modes[i].dmPelsWidth;
			ddmode.dwHeight = modes[i].dmPelsHeight;
			if(modes[i].dmBitsPerPel == 15) ddmode.lPitch = modes[i].dmPelsWidth * 2;
			else if(modes[i].dmBitsPerPel == 4) ddmode.lPitch = modes[i].dmPelsWidth / 2;
			else ddmode.lPitch = modes[i].dmPelsWidth * (modes[i].dmBitsPerPel / 8);
			if(lpEnumModesCallback(&ddmode,lpContext) == DDENUMRET_CANCEL) return DD_OK;
		}
	}
	free(modes);
	return DD_OK;
}
HRESULT EnumDisplayModes2(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
	BOOL match;
	BOOL scalemodes;
	DWORD modenum = 0;
	DWORD modemax = 128;
	DEVMODE mode;
	ZeroMemory(&mode,sizeof(DEVMODE));
	mode.dmSize = sizeof(DEVMODE);
	DEVMODE *modes = (DEVMODE*)malloc(128*sizeof(DEVMODE));
	DEVMODE *tmp;
	if(!modes) ERR(DDERR_OUTOFMEMORY);
	DDSURFACEDESC2 ddmode;
	ZeroMemory(&ddmode,sizeof(DDSURFACEDESC2));
	ddmode.dwSize = sizeof(DDSURFACEDESC2);
	ddmode.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
	ddmode.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	if (!_isnan(dxglcfg.postsizex) && !_isnan(dxglcfg.postsizey) &&
		(dxglcfg.postsizex > 0.25f) && (dxglcfg.postsizey > 0.25f) &&
		(dxglcfg.postsizex != 1.0f) && (dxglcfg.postsizey != 1.0f) &&
		((dxglcfg.scaler == 0) || ((dxglcfg.scaler >= 4) && (dxglcfg.scaler <= 6))) &&
		(!dxglcfg.primaryscale))
		scalemodes = TRUE;
	else scalemodes = FALSE;
	while(EnumDisplaySettings(NULL,modenum++,&mode))
	{
		if (scalemodes)
		{
			mode.dmPelsWidth = (DWORD)((float)mode.dmPelsWidth / dxglcfg.postsizex);
			mode.dmPelsHeight = (DWORD)((float)mode.dmPelsHeight / dxglcfg.postsizey);
		}
		modes[modenum-1] = mode;
		if(modenum >= modemax)
		{
			modemax += 128;
			tmp = (DEVMODE*)realloc(modes,modemax*sizeof(DEVMODE));
			if(tmp == NULL)
			{
				free(modes);
				ERR(DDERR_OUTOFMEMORY);
			}
			modes = tmp;
		}
	}
	DiscardDuplicateModes(&modes,&modenum);
	if(dxglcfg.AddColorDepths) AddExtraColorModes(&modes,&modenum);
	if (dxglcfg.scaler != 0)
	{
		if (dxglcfg.AddModes & 1) //Common low resolutions and doubled modes
			AddExtraResolutions(&modes, &modenum, LowResModes, NumLowResModes);
		if (dxglcfg.AddModes & 2) //Uncommon low resolutions
			AddExtraResolutions(&modes, &modenum, UncommonLowResModes, NumUncommonLowResModes);
		if (dxglcfg.AddModes & 4) //Uncommon SD reosolutions
			AddExtraResolutions(&modes, &modenum, UncommonSDModes, NumUncommonSDModes);
		if (dxglcfg.AddModes & 8) //High definition resolutions
			AddExtraResolutions(&modes, &modenum, HDModes, NumHDModes);
		if (dxglcfg.AddModes & 16) //Ultra-HD resolutions
			AddExtraResolutions(&modes, &modenum, UHDModes, NumUHDModes);
		if (dxglcfg.AddModes & 32) //Ultra-HD resolutions above 4k
			AddExtraResolutions(&modes, &modenum, UHD2Modes, NumUHD2Modes);
		if (dxglcfg.AddModes & 64) //Very uncommon resolutions
			AddExtraResolutions(&modes, &modenum, UncommonModes, NumUncommonModes);
	}
	if (dxglcfg.AddModes && (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
		(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f)))
	{
		if (dxglcfg.AddModes & 1) AddDoubledResolutions(&modes, &modenum);
	}
	if (dxglcfg.HackNoTVRefresh) RemoveTVAspectModes(&modes, modenum);
	DiscardDuplicateModes(&modes, &modenum);
	modenum--;
	switch(dxglcfg.SortModes)
	{
	case 0:
	default:
		break;
	case 1:
		qsort(modes,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortDepth);
		break;
	case 2:
		qsort(modes,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortRes);
		break;
	}
	for(DWORD i = 0; i < modenum; i++)
	{
		match = true;
		if(dwFlags & DDEDM_REFRESHRATES) ddmode.dwRefreshRate = modes[i].dmDisplayFrequency;
		else
		{
			ddmode.dwRefreshRate = 0;
			for(DWORD x = 0; x < i; x++)
				if((modes[x].dmBitsPerPel == modes[i].dmBitsPerPel) &&
					(modes[x].dmPelsWidth == modes[i].dmPelsWidth) &&
					(modes[x].dmPelsHeight == modes[i].dmPelsHeight)) match = false;
		}
		if(lpDDSurfaceDesc)
		{
			if(lpDDSurfaceDesc->dwFlags & DDSD_WIDTH)
				if(lpDDSurfaceDesc->dwWidth != modes[i].dmPelsWidth) match = false;
			if(lpDDSurfaceDesc->dwFlags & DDSD_HEIGHT)
				if(lpDDSurfaceDesc->dwHeight != modes[i].dmPelsHeight) match = false;
			if(lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
				if(lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount != modes[i].dmBitsPerPel) match = false;
			if(lpDDSurfaceDesc->dwFlags & DDSD_REFRESHRATE)
				if(lpDDSurfaceDesc->dwRefreshRate != modes[i].dmDisplayFrequency) match = false;
		}
		if(match)
		{
			if(modes[i].dmBitsPerPel == 8) ddmode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
			else if(modes[i].dmBitsPerPel == 4) ddmode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED4;
			else ddmode.ddpfPixelFormat.dwFlags = DDPF_RGB;
			if(modes[i].dmBitsPerPel == 8)
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0;
				ddmode.ddpfPixelFormat.dwGBitMask = 0;
				ddmode.ddpfPixelFormat.dwBBitMask = 0;
			}
			else if(modes[i].dmBitsPerPel == 15)
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0x7C00;
				ddmode.ddpfPixelFormat.dwGBitMask = 0x3E0;
				ddmode.ddpfPixelFormat.dwRBitMask = 0x1F;
			}
			else if(modes[i].dmBitsPerPel == 16)
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0xF800;
				ddmode.ddpfPixelFormat.dwGBitMask = 0x7E0;
				ddmode.ddpfPixelFormat.dwBBitMask = 0x1F;
			}
			else
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddmode.ddpfPixelFormat.dwRBitMask = 0xFF00;
				ddmode.ddpfPixelFormat.dwRBitMask = 0xFF;
			}
			ddmode.ddpfPixelFormat.dwRGBBitCount = modes[i].dmBitsPerPel;
			ddmode.dwWidth = modes[i].dmPelsWidth;
			ddmode.dwHeight = modes[i].dmPelsHeight;
			if(modes[i].dmBitsPerPel == 15) ddmode.lPitch = modes[i].dmPelsWidth * 2;
			else if(modes[i].dmBitsPerPel == 4) ddmode.lPitch = modes[i].dmPelsWidth / 2;
			else ddmode.lPitch = modes[i].dmPelsWidth * (modes[i].dmBitsPerPel / 8);
			if(lpEnumModesCallback(&ddmode,lpContext) == DDENUMRET_CANCEL) return DD_OK;
		}
	}
	free(modes);
	return DD_OK;
}

// DDRAW7/common routines

glDirectDraw7Vtbl glDirectDraw7_impl =
{
	glDirectDraw7_QueryInterface,
	glDirectDraw7_AddRef,
	glDirectDraw7_Release,
	glDirectDraw7_Compact,
	glDirectDraw7_CreateClipper,
	glDirectDraw7_CreatePalette,
	glDirectDraw7_CreateSurface,
	glDirectDraw7_DuplicateSurface,
	glDirectDraw7_EnumDisplayModes,
	glDirectDraw7_EnumSurfaces,
	glDirectDraw7_FlipToGDISurface,
	glDirectDraw7_GetCaps,
	glDirectDraw7_GetDisplayMode,
	glDirectDraw7_GetFourCCCodes,
	glDirectDraw7_GetGDISurface,
	glDirectDraw7_GetMonitorFrequency,
	glDirectDraw7_GetScanLine,
	glDirectDraw7_GetVerticalBlankStatus,
	glDirectDraw7_Initialize,
	glDirectDraw7_RestoreDisplayMode,
	glDirectDraw7_SetCooperativeLevel,
	glDirectDraw7_SetDisplayMode,
	glDirectDraw7_WaitForVerticalBlank,
	glDirectDraw7_GetAvailableVidMem,
	glDirectDraw7_GetSurfaceFromDC,
	glDirectDraw7_RestoreAllSurfaces,
	glDirectDraw7_TestCooperativeLevel,
	glDirectDraw7_GetDeviceIdentifier,
	glDirectDraw7_StartModeTest,
	glDirectDraw7_EvaluateMode
};

HRESULT glDirectDraw7_Create(glDirectDraw7 **glDD7)
{
	TRACE_ENTER(1,14,glDD7);
	glDirectDraw7 *This = (glDirectDraw7*)malloc(sizeof(glDirectDraw7));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	ZeroMemory(This, sizeof(glDirectDraw7));
	This->lpVtbl = &glDirectDraw7_impl;
	glDirectDraw1_Create(This, &This->glDD1);
	glDirectDraw2_Create(This, &This->glDD2);
	glDirectDraw4_Create(This, &This->glDD4);
	glDirect3D7_Create(This, &This->glD3D7);
	glDirect3D3_Create(This->glD3D7, &This->glD3D3);
	glDirect3D2_Create(This->glD3D7, &This->glD3D2);
	glDirect3D1_Create(This->glD3D7, &This->glD3D1);
	This->clippers = NULL;
	This->surfaces = NULL;
	This->initialized = false;
	This->devid.liDriverVersion.QuadPart = DXGLVERQWORD;
	This->refcount7 = 1;
	This->refcount4 = 0;
	This->refcount2 = 0;
	This->refcount1 = 0;
	This->renderer = NULL;
	*glDD7 = This;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT glDirectDraw7_CreateAndInitialize(GUID FAR* lpGUID, IUnknown FAR* pUnkOuter, glDirectDraw7 **glDD7)
{
	TRACE_ENTER(3,24,lpGUID,14,pUnkOuter,14,glDD7);
	glDirectDraw7 *This = (glDirectDraw7*)malloc(sizeof(glDirectDraw7));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	ZeroMemory(This, sizeof(glDirectDraw7));
	This->lpVtbl = &glDirectDraw7_impl;
	This->initialized = false;
	if (((ULONG_PTR)lpGUID > 2) && !IsReadablePointer(lpGUID, sizeof(GUID)))
	{
		free(This);
		TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	}
	GUID guid;
	if((ULONG_PTR)lpGUID > 2)
	{
		guid = *lpGUID;
		guid.Data1 &= 0xFFFFFF00;
		if(guid != device_template)
		{
			free(This);
			TRACE_RET(HRESULT,23,DDERR_INVALIDDIRECTDRAWGUID);
		}
	}
	if(pUnkOuter)
	{
		free(This);
		TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	}
	glDirectDraw1_Create(This, &This->glDD1);
	glDirectDraw2_Create(This, &This->glDD2);
	glDirectDraw4_Create(This, &This->glDD4);
	glDirect3D7_Create(This, &This->glD3D7);
	glDirect3D3_Create(This->glD3D7, &This->glD3D3);
	glDirect3D2_Create(This->glD3D7, &This->glD3D2);
	glDirect3D1_Create(This->glD3D7, &This->glD3D1);
	This->devid.liDriverVersion.QuadPart = DXGLVERQWORD;
	This->renderer = NULL;
	This->error = glDirectDraw7_Initialize(This, lpGUID);
	if (FAILED(This->error))
	{
		if (This->glDD1) free(This->glDD1);
		if (This->glDD2) free(This->glDD2);
		if (This->glDD4) free(This->glDD4);
		if (This->glD3D7) free(This->glD3D7);
		if (This->glD3D3) free(This->glD3D3);
		if (This->glD3D2) free(This->glD3D2);
		if (This->glD3D1) free(This->glD3D1);
		free(This);
		TRACE_RET(HRESULT, 23, This->error);
	}
	This->refcount7 = 1;
	This->refcount4 = 0;
	This->refcount2 = 0;
	This->refcount1 = 0;
	*glDD7 = This;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

void glDirectDraw7_Delete(glDirectDraw7 *This)
{
	TRACE_ENTER(1,14,This);
	if(This->initialized)
	{
		UninstallDXGLHook(This->hWnd);
		glDirectDraw7_RestoreDisplayMode(This);
		if (This->clippers)
		{
			for (int i = 0; i < This->clippercount; i++)
			{
				if (This->clippers[i]) glDirectDrawClipper_Release(This->clippers[i]);
				This->clippers[i] = NULL;
			}
			free(This->clippers);
		}
		if(This->surfaces)
		{
			for(int i = 0; i < This->surfacecount; i++)
			{
				if(This->surfaces[i]) dxglDirectDrawSurface7_Delete(This->surfaces[i]);
				This->surfaces[i] = NULL;
			}
			free(This->surfaces);
		}
		if(This->renderer)
		{
			glRenderer_Delete(This->renderer);
			free(This->renderer);
		}
		This->renderer = NULL;
	}
	if (This->glDD1) free(This->glDD1);
	if (This->glDD2) free(This->glDD2);
	if (This->glDD4) free(This->glDD4);
	if (This->glD3D7) free(This->glD3D7);
	if (This->glD3D3) free(This->glD3D3);
	if (This->glD3D2) free(This->glD3D2);
	if (This->glD3D1) free(This->glD3D1);
	DeleteDirectDraw();
	free(This);
	TRACE_EXIT(-1,0);
}

HRESULT WINAPI glDirectDraw7_QueryInterface(glDirectDraw7 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if((riid == IID_IUnknown) || (riid == IID_IDirectDraw7))
	{
		glDirectDraw7_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDraw)
	{
		glDirectDraw7_AddRef1(This);
		*ppvObj = This->glDD1;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDraw2)
	{
		glDirectDraw7_AddRef2(This);
		*ppvObj = This->glDD2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDraw4)
	{
		glDirectDraw7_AddRef4(This);
		*ppvObj = This->glDD4;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3D)
	{
		glDirectDraw7_AddRef1(This);
		*ppvObj = This->glD3D1;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3D2)
	{
		glDirectDraw7_AddRef1(This);
		*ppvObj = This->glD3D2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3D3)
	{
		glDirectDraw7_AddRef1(This);
		*ppvObj = This->glD3D3;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3D7)
	{
		glDirectDraw7_AddRef(This);
		*ppvObj = This->glD3D7;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	/*if(riid == IID_IDDVideoPortContainer)
	{
		ERR(DDERR_GENERIC);
	}*/
	*ppvObj = NULL;
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}
ULONG WINAPI glDirectDraw7_AddRef(glDirectDraw7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	InterlockedIncrement((LONG*)&This->refcount7);
	TRACE_EXIT(8,This->refcount7);
	return This->refcount7;
}
ULONG WINAPI glDirectDraw7_Release(glDirectDraw7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	if (This->refcount7 == 0) TRACE_RET(ULONG,8,0);
	InterlockedDecrement((LONG*)&This->refcount7);
	ret = This->refcount7;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) &&
		(This->refcount2 == 0) && (This->refcount1 == 0))
		glDirectDraw7_Delete(This);
	TRACE_EXIT(8,ret);
	return ret;
}

ULONG WINAPI glDirectDraw7_AddRef4(glDirectDraw7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount4);
	TRACE_EXIT(8, This->refcount4);
	return This->refcount4;
}
ULONG WINAPI glDirectDraw7_Release4(glDirectDraw7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount4 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount4);
	ret = This->refcount4;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) &&
		(This->refcount2 == 0) && (This->refcount1 == 0))
		glDirectDraw7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}

ULONG WINAPI glDirectDraw7_AddRef2(glDirectDraw7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount2);
	TRACE_EXIT(8, This->refcount2);
	return This->refcount2;
}
ULONG WINAPI glDirectDraw7_Release2(glDirectDraw7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount2 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount2);
	ret = This->refcount2;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) &&
		(This->refcount2 == 0) && (This->refcount1 == 0))
		glDirectDraw7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}

ULONG WINAPI glDirectDraw7_AddRef1(glDirectDraw7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount1);
	TRACE_EXIT(8, This->refcount1);
	return This->refcount1;
}
ULONG WINAPI glDirectDraw7_Release1(glDirectDraw7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount1 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount1);
	ret = This->refcount1;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) &&
		(This->refcount2 == 0) && (This->refcount1 == 0))
		glDirectDraw7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}

HRESULT WINAPI glDirectDraw7_Compact(glDirectDraw7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!This->fullscreen) TRACE_RET(HRESULT, 23, DDERR_NOEXCLUSIVEMODE);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_CreateClipper(glDirectDraw7 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4, 14, This, 9, dwFlags, 14, lplpDDClipper, 14, pUnkOuter);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDraw7_CreateClipper2(This, dwFlags, lplpDDClipper, pUnkOuter);
	if (ret == DD_OK)
	{
		glDirectDraw7_AddRef(This);
		((glDirectDrawClipper*)*lplpDDClipper)->creator = (IUnknown*)This;
	}
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT glDirectDraw7_CreateClipper2(glDirectDraw7 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lplpDDClipper,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDDClipper) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(pUnkOuter) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	This->clippercount++;
	if (This->clippercount > This->clippercountmax)
	{
		glDirectDrawClipper **clippers2;
		clippers2 = (glDirectDrawClipper **)realloc(This->clippers, (This->clippercountmax + 1024)*sizeof(glDirectDrawClipper *));
		if (!clippers2)	TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
		This->clippers = clippers2;
		ZeroMemory(&This->clippers[This->clippercountmax], 1024 * sizeof(glDirectDrawClipper *));
		This->clippercountmax += 1024;
	}
	TRACE_RET(HRESULT,23,glDirectDrawClipper_Create(dwFlags, This, lplpDDClipper));
}
HRESULT WINAPI glDirectDraw7_CreatePalette(glDirectDraw7 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(5, 14, This, 9, dwFlags, 14, lpDDColorArray, 14, lplpDDPalette, 14, pUnkOuter);
	if (!IsReadablePointer(This,sizeof(glDirectDraw7))) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (pUnkOuter) TRACE_RET(HRESULT, 23, CLASS_E_NOAGGREGATION);
	HRESULT ret = glDirectDraw7_CreatePalette2(This, dwFlags, lpDDColorArray, lplpDDPalette, pUnkOuter);
	if (ret == DD_OK)
	{
		glDirectDraw7_AddRef(This);
		((glDirectDrawPalette*)*lplpDDPalette)->creator = (IUnknown*)This;
	}
	TRACE_EXIT(23, ret);
	return ret;
}

HRESULT glDirectDraw7_CreatePalette2(glDirectDraw7 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(5,14,This,9,dwFlags,14,lpDDColorArray,14,lplpDDPalette,14,pUnkOuter);
	if (!IsReadablePointer(This, sizeof(glDirectDraw7))) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDDPalette) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(pUnkOuter) TRACE_RET(HRESULT,23,CLASS_E_NOAGGREGATION);
	if (!This->cooplevel) TRACE_RET(HRESULT, 23, DDERR_NOCOOPERATIVELEVELSET);
	HRESULT ret = glDirectDrawPalette_Create(dwFlags, lpDDColorArray, lplpDDPalette);
	TRACE_VAR("*lplpDDPalette",14,*lplpDDPalette);
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirectDraw7_CreateSurface(glDirectDraw7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4,14,This,14,lpDDSurfaceDesc2,14,lplpDDSurface,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSurfaceDesc2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(pUnkOuter) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpDDSurfaceDesc2->dwSize < sizeof(DDSURFACEDESC2)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT ret = glDirectDraw7_CreateSurface2(This,lpDDSurfaceDesc2,lplpDDSurface,pUnkOuter,TRUE,7);
	if (ret == DD_OK)
	{
		glDirectDraw7_AddRef(This);
		((dxglDirectDrawSurface7*)*lplpDDSurface)->creator = (IUnknown*)This;
	}
	TRACE_EXIT(23, ret);
	return ret;
}


HRESULT glDirectDraw7_CreateSurface2(glDirectDraw7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter, BOOL RecordSurface, int version)
{
	HRESULT error;
	DWORD mipcount;
	DWORD complexcount = 1;
	size_t surfacesize;
	TRACE_ENTER(5, 14, This, 14, lpDDSurfaceDesc2, 14, lplpDDSurface, 14, pUnkOuter, 22, RecordSurface);
	// Validate interface pointer pointer
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	// Validate DDSURFACEDESC2 pointer
	if (!lpDDSurfaceDesc2) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	// API doesn't support COM aggregation
	if (pUnkOuter) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	// Check if cooperative level is set
	if (!This->renderer) TRACE_RET(HRESULT, 23, DDERR_NOCOOPERATIVELEVELSET);
	// Check if primary exists if creating primary
	if (This->primary && (lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
	{
		if (This->primarylost)
		{
			dxglDirectDrawSurface7_Restore(This->primary);
			*lplpDDSurface = (LPDIRECTDRAWSURFACE7)This->primary;
			This->primarylost = false;
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT, 23, DDERR_PRIMARYSURFACEALREADYEXISTS);
	}
	// Mipmap textures need a width and height
	if ((lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_MIPMAP) && 
		(!(lpDDSurfaceDesc2->dwFlags & DDSD_WIDTH) || !(lpDDSurfaceDesc2->dwFlags & DDSD_HEIGHT)))
		TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	// Check if mipmap count doesn't go past dimensions of 1
	if (lpDDSurfaceDesc2->dwFlags & DDSD_MIPMAPCOUNT)
	{
		if (!(lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_MIPMAP))
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (lpDDSurfaceDesc2->dwMipMapCount < 1) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		mipcount = CalculateMipLevels(lpDDSurfaceDesc2->dwWidth, lpDDSurfaceDesc2->dwHeight);
		if (mipcount < lpDDSurfaceDesc2->dwMipMapCount) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	}
	// Get complex surface size
	// Validate backbuffer count and multiply by number of buffers
	if (lpDDSurfaceDesc2->dwFlags & DDSD_BACKBUFFERCOUNT)
	{
		if (lpDDSurfaceDesc2->dwBackBufferCount < 1) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		complexcount *= lpDDSurfaceDesc2->dwBackBufferCount + 1;
	}
	// Multiply by number of mipmaps
	if (lpDDSurfaceDesc2->dwFlags & DDSD_MIPMAPCOUNT)
		complexcount * lpDDSurfaceDesc2->dwMipMapCount;
	// Calculate surface size
	surfacesize = sizeof(dxglDirectDrawSurface7) * complexcount;
	if (lpDDSurfaceDesc2->dwFlags & DDSD_BACKBUFFERCOUNT)
		surfacesize += sizeof(glTexture) * (lpDDSurfaceDesc2->dwBackBufferCount + 1);
	else surfacesize += sizeof(glTexture);

	// Create recorded surface
	if (RecordSurface)
	{
		This->surfacecount++;
		if (This->surfacecount > This->surfacecountmax)
		{
			dxglDirectDrawSurface7 **surfaces2;
			surfaces2 = (dxglDirectDrawSurface7 **)realloc(This->surfaces, (This->surfacecountmax + 1024)*sizeof(dxglDirectDrawSurface7 *));
			if (!surfaces2) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
			This->surfaces = surfaces2;
			ZeroMemory(&This->surfaces[This->surfacecountmax], 1024 * sizeof(dxglDirectDrawSurface7 *));
			This->surfacecountmax += 1024;
		}
		This->surfaces[This->surfacecount - 1] = (dxglDirectDrawSurface7*)malloc(surfacesize);
		if (!This->surfaces[This->surfacecount - 1])
		{
			This->surfacecount--;
			TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
		}
		error = dxglDirectDrawSurface7_Create((LPDIRECTDRAW7)This, lpDDSurfaceDesc2, NULL, NULL, version, This->surfaces[This->surfacecount - 1]);
		if (lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			This->primary = This->surfaces[This->surfacecount - 1];
			This->primarylost = false;
		}
		*lplpDDSurface = (LPDIRECTDRAWSURFACE7)This->surfaces[This->surfacecount - 1];
	}
	else
	{
		if (lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		*lplpDDSurface = (LPDIRECTDRAWSURFACE7)malloc(surfacesize);
		if (!*lplpDDSurface) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
		error = dxglDirectDrawSurface7_Create((LPDIRECTDRAW7)This, lpDDSurfaceDesc2, NULL, NULL, version, (dxglDirectDrawSurface7 *)*lplpDDSurface);
	}
	// Delete surface if creation failed
	if (error != DD_OK)
	{
		dxglDirectDrawSurface7_Delete((dxglDirectDrawSurface7*)*lplpDDSurface);
		*lplpDDSurface = NULL;
	}
	TRACE_VAR("*lplpDDSurface",14,*lplpDDSurface);
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDraw7_DuplicateSurface(glDirectDraw7 *This, LPDIRECTDRAWSURFACE7 lpDDSurface, LPDIRECTDRAWSURFACE7 FAR *lplpDupDDSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSurface,14,lplpDupDDSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lplpDupDDSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	HRESULT ret;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	lpDDSurface->GetSurfaceDesc(&ddsd);
	ret = glDirectDraw7_CreateSurface(This, &ddsd, lplpDupDDSurface, NULL);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ret = (*lplpDupDDSurface)->Blt(NULL, lpDDSurface, NULL, 0, NULL);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw7_EnumDisplayModes(glDirectDraw7 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
	TRACE_ENTER(5,14,This,9,dwFlags,14,lpDDSurfaceDesc2,14,lpContext,14,lpEnumModesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpEnumModesCallback) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT ret = EnumDisplayModes2(dwFlags,lpDDSurfaceDesc2,lpContext,lpEnumModesCallback);
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI EnumSurfacesCallback1(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext)
{
	LPVOID *lplpContext = (LPVOID*)lpContext;
	LPDDENUMSURFACESCALLBACK callback = (LPDDENUMSURFACESCALLBACK)lplpContext[0];
	LPVOID context = (LPVOID)lplpContext[1];
	LPDIRECTDRAWSURFACE dds1;
	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	lpDDSurface->QueryInterface(IID_IDirectDrawSurface, (void**)&dds1);
	lpDDSurface->Release();
	dds1->GetSurfaceDesc(&ddsd);
	return callback(dds1, &ddsd, context);
}
HRESULT WINAPI EnumSurfacesCallback2(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext)
{
	LPVOID *lplpContext = (LPVOID*)lpContext;
	LPDDENUMSURFACESCALLBACK2 callback = (LPDDENUMSURFACESCALLBACK2)lplpContext[0];
	LPVOID context = (LPVOID)lplpContext[1];
	LPDIRECTDRAWSURFACE4 dds4;
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	lpDDSurface->QueryInterface(IID_IDirectDrawSurface4, (void**)&dds4);
	lpDDSurface->Release();
	dds4->GetSurfaceDesc(&ddsd);
	return callback(dds4, &ddsd, context);
}

HRESULT WINAPI glDirectDraw7_EnumSurfaces(glDirectDraw7 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
	int i;
	BOOL match;
	HRESULT ret;
	LPDIRECTDRAWSURFACE7 surface;
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	TRACE_ENTER(5,14,This,9,dwFlags,14,lpDDSD2,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSD2 && !(dwFlags & DDENUMSURFACES_ALL)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpEnumSurfacesCallback) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwFlags & 0xFFFFFFE0) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if ((dwFlags & DDENUMSURFACES_CANBECREATED) && (dwFlags & DDENUMSURFACES_DOESEXIST)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!(dwFlags & DDENUMSURFACES_CANBECREATED) && !(dwFlags & DDENUMSURFACES_DOESEXIST)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDENUMSURFACES_CANBECREATED) && !(dwFlags & DDENUMSURFACES_MATCH)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDENUMSURFACES_ALL) && (dwFlags & DDENUMSURFACES_MATCH)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDENUMSURFACES_ALL) && (dwFlags & DDENUMSURFACES_NOMATCH)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDENUMSURFACES_MATCH) && (dwFlags & DDENUMSURFACES_NOMATCH)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (dwFlags & DDENUMSURFACES_CANBECREATED)
	{
		// create surface
		ret = glDirectDraw7_CreateSurface(This, lpDDSD2, &surface, NULL);
		if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
		surface->GetSurfaceDesc(&ddsd);
		lpEnumSurfacesCallback(surface, &ddsd, lpContext);
		TRACE_EXIT(23, DD_OK);
		return DD_OK;
	}
	else
	{
		for (i = 0; i < This->surfacecount; i++)
		{
			if (This->surfaces[i])
			{
				if (dwFlags & DDENUMSURFACES_ALL) match = TRUE;
				if (dwFlags & DDENUMSURFACES_MATCH)
				{
					if (!memcmp(&This->surfaces[i]->ddsd, lpDDSD2, sizeof(DDSURFACEDESC2))) match = TRUE;
					else match = FALSE;
				}
				if (dwFlags & DDENUMSURFACES_NOMATCH)
				{
					if (memcmp(&This->surfaces[i]->ddsd, lpDDSD2, sizeof(DDSURFACEDESC2))) match = TRUE;
					else match = FALSE;
				}
				if (match)
				{
					dxglDirectDrawSurface7_AddRef(This->surfaces[i]);
					dxglDirectDrawSurface7_GetSurfaceDesc(This->surfaces[i], &ddsd);
					ret = lpEnumSurfacesCallback((LPDIRECTDRAWSURFACE7)This->surfaces[i], &ddsd, lpContext);
					if (ret == DDENUMRET_CANCEL) break;
				}
			}
		}
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_FlipToGDISurface(glDirectDraw7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error = DD_OK;
	if(This->primary)
	{
		if (!(This->primary->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)) TRACE_RET(HRESULT, 23, DDERR_NOTFLIPPABLE);
		if(This->primary->flipcount)
		{
			while(This->primary->flipcount != 0)
			{
				error = dxglDirectDrawSurface7_Flip(This->primary, NULL, DDFLIP_WAIT);
				if(error != DD_OK) break;
			}
		}
		TRACE_EXIT(23,error);
		return(error);
	}
	else TRACE_RET(HRESULT,23,DDERR_NOTFOUND);
}
HRESULT WINAPI glDirectDraw7_GetCaps(glDirectDraw7 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	TRACE_ENTER(3,14,This,14,lpDDDriverCaps,14,lpDDHELCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	//TODO:  Fill in as implemented.
	DDCAPS_DX7 ddCaps;
	ZeroMemory(&ddCaps,sizeof(DDCAPS_DX7));
	if(lpDDDriverCaps) ddCaps.dwSize = lpDDDriverCaps->dwSize;
	else if(lpDDHELCaps) ddCaps.dwSize = lpDDHELCaps->dwSize;
	else TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(ddCaps.dwSize > sizeof(DDCAPS_DX7)) ddCaps.dwSize = sizeof(DDCAPS_DX7);
	if (ddCaps.dwSize < sizeof(DDCAPS_DX3)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ddCaps.dwCaps = DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTDEPTHFILL | DDCAPS_BLTFOURCC |
		DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY | DDCAPS_GDI | DDCAPS_PALETTE | DDCAPS_CANBLTSYSMEM |
		DDCAPS_3D | DDCAPS_CANCLIP | DDCAPS_CANCLIPSTRETCHED | DDCAPS_READSCANLINE |
		DDCAPS_OVERLAY | DDCAPS_OVERLAYSTRETCH;
	ddCaps.dwCaps2 = DDCAPS2_CANRENDERWINDOWED | DDCAPS2_WIDESURFACES | DDCAPS2_NOPAGELOCKREQUIRED |
		DDCAPS2_FLIPINTERVAL | DDCAPS2_FLIPNOVSYNC | DDCAPS2_NONLOCALVIDMEM;
	ddCaps.dwFXCaps = DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKY |
		DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHY | DDFXCAPS_BLTMIRRORLEFTRIGHT |
		DDFXCAPS_BLTMIRRORUPDOWN | DDFXCAPS_BLTROTATION90;
	ddCaps.dwPalCaps = DDPCAPS_1BIT | DDPCAPS_2BIT | DDPCAPS_4BIT | DDPCAPS_8BIT |
		DDPCAPS_PRIMARYSURFACE | DDPCAPS_ALLOW256;
	ddCaps.ddsOldCaps.dwCaps = ddCaps.ddsCaps.dwCaps =
		DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP |
		DDSCAPS_FRONTBUFFER | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PALETTE |
		DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE |
		DDSCAPS_NONLOCALVIDMEM | DDSCAPS_LOCALVIDMEM | DDSCAPS_TEXTURE |
		DDSCAPS_MIPMAP | DDSCAPS_OVERLAY;
	ddCaps.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
	ddCaps.dwMinOverlayStretch = 1;
	ddCaps.dwMaxOverlayStretch = 2147483647;
	ddCaps.dwCKeyCaps = DDCKEYCAPS_SRCBLT | DDCKEYCAPS_DESTBLT | 
		/*DDCKEYCAPS_SRCOVERLAY | */DDCKEYCAPS_DESTOVERLAY;
	ddCaps.dwZBufferBitDepths = DDBD_16 | DDBD_24 | DDBD_32;
	ddCaps.dwNumFourCCCodes = GetNumFOURCC();
	BOOL fullrop = FALSE;
	if (!This->renderer)
	{
		HWND hGLWnd = CreateWindow(_T("Test"), NULL, WS_POPUP, 0, 0, 16, 16, NULL, NULL, NULL, NULL);
		glRenderer *tmprenderer = (glRenderer*)malloc(sizeof(glRenderer));
		DEVMODE mode;
		mode.dmSize = sizeof(DEVMODE);
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode);
		glRenderer_Init(tmprenderer, 16, 16, mode.dmBitsPerPel, false, mode.dmDisplayFrequency, hGLWnd, NULL, FALSE);
		if ((tmprenderer->ext->glver_major >= 3) && !dxglcfg.DebugNoGLSL130) fullrop = TRUE;
		if (tmprenderer->ext->GLEXT_EXT_gpu_shader4) fullrop = TRUE;
		glRenderer_Delete(tmprenderer);
		free(tmprenderer);
	}
	else
	{
		if ((This->renderer->ext->glver_major >= 3) && !dxglcfg.DebugNoGLSL130) fullrop = TRUE;
		if (This->renderer->ext->GLEXT_EXT_gpu_shader4) fullrop = TRUE;
	}
	if (fullrop)
	{
		memcpy(ddCaps.dwRops, supported_rops, 8 * sizeof(DWORD));
		memcpy(ddCaps.dwNLVBRops, supported_rops, 8 * sizeof(DWORD));
		memcpy(ddCaps.dwSSBRops, supported_rops, 8 * sizeof(DWORD));
		memcpy(ddCaps.dwSVBRops, supported_rops, 8 * sizeof(DWORD));
		memcpy(ddCaps.dwVSBRops, supported_rops, 8 * sizeof(DWORD));
	}
	else
	{
		memcpy(ddCaps.dwRops, supported_rops_gl2, 8 * sizeof(DWORD));
		memcpy(ddCaps.dwNLVBRops, supported_rops_gl2, 8 * sizeof(DWORD));
		memcpy(ddCaps.dwSSBRops, supported_rops_gl2, 8 * sizeof(DWORD));
		memcpy(ddCaps.dwSVBRops, supported_rops_gl2, 8 * sizeof(DWORD));
		memcpy(ddCaps.dwVSBRops, supported_rops_gl2, 8 * sizeof(DWORD));
	}
	glDirectDraw7_GetAvailableVidMem(This,NULL,&ddCaps.dwVidMemTotal,&ddCaps.dwVidMemFree);
	if(lpDDDriverCaps)
	{
		if(lpDDDriverCaps->dwSize > sizeof(DDCAPS_DX7)) lpDDDriverCaps->dwSize = sizeof(DDCAPS_DX7);
		memcpy(lpDDDriverCaps,&ddCaps,lpDDDriverCaps->dwSize);
	}
	if(lpDDHELCaps)
	{
		if(lpDDHELCaps->dwSize > sizeof(DDCAPS_DX7)) lpDDHELCaps->dwSize = sizeof(DDCAPS_DX7);
		memcpy(lpDDHELCaps,&ddCaps,lpDDHELCaps->dwSize);
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_GetDisplayMode(glDirectDraw7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSurfaceDesc2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsdMode;
	ZeroMemory(&ddsdMode, sizeof(DDSURFACEDESC2));
	ddsdMode.dwSize = sizeof(DDSURFACEDESC2);
	ddsdMode.dwFlags = DDSD_REFRESHRATE | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_WIDTH | DDSD_HEIGHT;
	ddsdMode.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsdMode.ddpfPixelFormat.dwFlags = DDPF_RGB;
	DEVMODE currmode;
	ZeroMemory(&currmode,sizeof(DEVMODE));
	currmode.dmSize = sizeof(DEVMODE);
	if(This->fullscreen)
	{
		if(This->primarybpp == 8)
		{
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0;
			ddsdMode.ddpfPixelFormat.dwGBitMask = 0;
			ddsdMode.ddpfPixelFormat.dwBBitMask = 0;
			ddsdMode.ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
		}
		else if(This->primarybpp == 15)
		{
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0x7C00;
			ddsdMode.ddpfPixelFormat.dwGBitMask = 0x3E0;
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0x1F;
		}
		else if(This->primarybpp == 16)
		{
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0xF800;
			ddsdMode.ddpfPixelFormat.dwGBitMask = 0x7E0;
			ddsdMode.ddpfPixelFormat.dwBBitMask = 0x1F;
		}
		else
		{
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF0000;
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF00;
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF;
		}
		ddsdMode.ddpfPixelFormat.dwRGBBitCount = glDirectDraw7_GetBPPMultipleOf8(This);
		ddsdMode.dwWidth = This->primaryx;
		ddsdMode.dwHeight = This->primaryy;
		ddsdMode.dwRefreshRate = This->primaryrefresh;
		if(This->primarybpp == 15) ddsdMode.lPitch = NextMultipleOf4(This->primaryx * 2);
			else if(This->primarybpp == 4) ddsdMode.lPitch = NextMultipleOf2(This->primaryx / 2);
			else ddsdMode.lPitch = NextMultipleOf4(This->primaryx * (This->primarybpp / 8));
		if(lpDDSurfaceDesc2->dwSize < sizeof(DDSURFACEDESC)) ERR(DDERR_INVALIDPARAMS);
		if(lpDDSurfaceDesc2->dwSize > sizeof(DDSURFACEDESC2))
			lpDDSurfaceDesc2->dwSize = sizeof(DDSURFACEDESC2);
		memcpy(&lpDDSurfaceDesc2->dwSize+1,&ddsdMode.dwSize+1,lpDDSurfaceDesc2->dwSize-sizeof(DWORD)); // copy skipping first DWORD dwSize
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&currmode);
	if(currmode.dmBitsPerPel == 8) ddsdMode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
	else if(currmode.dmBitsPerPel == 4) ddsdMode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED4;
	else ddsdMode.ddpfPixelFormat.dwFlags = DDPF_RGB;
	if(currmode.dmBitsPerPel == 8)
	{
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0;
		ddsdMode.ddpfPixelFormat.dwGBitMask = 0;
		ddsdMode.ddpfPixelFormat.dwBBitMask = 0;
	}
	else if(currmode.dmBitsPerPel == 15)
	{
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0x7C00;
		ddsdMode.ddpfPixelFormat.dwGBitMask = 0x3E0;
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0x1F;
	}
	else if(currmode.dmBitsPerPel == 16)
	{
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0xF800;
		ddsdMode.ddpfPixelFormat.dwGBitMask = 0x7E0;
		ddsdMode.ddpfPixelFormat.dwBBitMask = 0x1F;
	}
	else
	{
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF0000;
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF00;
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF;
	}
	ddsdMode.ddpfPixelFormat.dwRGBBitCount = currmode.dmBitsPerPel;
	ddsdMode.dwWidth = currmode.dmPelsWidth;
	ddsdMode.dwHeight = currmode.dmPelsHeight;
	if(currmode.dmBitsPerPel == 15) ddsdMode.lPitch = currmode.dmPelsWidth * 2;
		else if(currmode.dmBitsPerPel == 4) ddsdMode.lPitch = currmode.dmPelsWidth / 2;
		else ddsdMode.lPitch = currmode.dmPelsWidth * (currmode.dmBitsPerPel / 8);
	if(lpDDSurfaceDesc2->dwSize < sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpDDSurfaceDesc2->dwSize > sizeof(DDSURFACEDESC2))
		lpDDSurfaceDesc2->dwSize = sizeof(DDSURFACEDESC2);
	memcpy(&lpDDSurfaceDesc2->dwSize+1,&ddsdMode.dwSize+1,lpDDSurfaceDesc2->dwSize-sizeof(DWORD)); // copy skipping first DWORD dwSize
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_GetFourCCCodes(glDirectDraw7 *This, LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	TRACE_ENTER(3,14,This,14,lpNumCodes,14,lpCodes);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpNumCodes) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*lpNumCodes = GetNumFOURCC();
	if (lpCodes)
	{
		for (int i = 0; i < *lpNumCodes; i++)
		{
			lpCodes[i] = dxglfourcc[i];
		}
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_GetGDISurface(glDirectDraw7 *This, LPDIRECTDRAWSURFACE7 FAR *lplpGDIDDSurface)
{
	TRACE_ENTER(2,14,This,14,lplpGDIDDSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpGDIDDSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("IDirectDraw_GetGDISurface: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7_GetMonitorFrequency(glDirectDraw7 *This, LPDWORD lpdwFrequency)
{
	TRACE_ENTER(2,14,This,14,lpdwFrequency);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpdwFrequency) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DEBUG("IDirectDraw7_GetMonitorFrequency: support multi-monitor\n");
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
	*lpdwFrequency = devmode.dmDisplayFrequency;
	TRACE_VAR("*lpdwFrequency",8,*lpdwFrequency);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_GetScanLine(glDirectDraw7 *This, LPDWORD lpdwScanLine)
{
	TRACE_ENTER(2,14,This,14,lpdwScanLine);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpdwScanLine) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->renderer) TRACE_RET(HRESULT,23,DDERR_NOTINITIALIZED);
	if(!This->primary) TRACE_RET(HRESULT,23,DDERR_NOTINITIALIZED);
	if(!This->initialized) TRACE_RET(HRESULT,23,DDERR_NOTINITIALIZED);
	*lpdwScanLine = glRenderer_GetScanLine(This->renderer);
	if(*lpdwScanLine >= This->primary->ddsd.dwHeight) TRACE_RET(HRESULT,23,DDERR_VERTICALBLANKINPROGRESS);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_GetVerticalBlankStatus(glDirectDraw7 *This, LPBOOL lpbIsInVB)
{
	TRACE_ENTER(2,14,This,14,lpbIsInVB);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpbIsInVB) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->renderer) TRACE_RET(HRESULT,23,DDERR_NOTINITIALIZED);
	if(!This->primary) TRACE_RET(HRESULT,23,DDERR_NOTINITIALIZED);
	if(!This->initialized) TRACE_RET(HRESULT,23,DDERR_NOTINITIALIZED);
	if(glRenderer_GetScanLine(This->renderer) >= This->primary->ddsd.dwHeight) *lpbIsInVB = TRUE;
	else *lpbIsInVB = FALSE;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_Initialize(glDirectDraw7 *This, GUID FAR *lpGUID)
{
	TRACE_ENTER(2,14,This,24,lpGUID);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(This->initialized) TRACE_RET(HRESULT,23,DDERR_ALREADYINITIALIZED);
	This->devid = devid_default;
	This->hWnd = NULL;
	This->primarylost = true;
	This->renderer = NULL;
	This->primary = NULL;
	This->lastsync = false;
	This->fullscreen = false;
	This->fpupreserve = false;
	This->fpusetup = false;
	This->threadsafe = false;
	This->nowindowchanges = false;
	This->cooplevel = 0;
	This->timer = timeGetTime();
	ZeroMemory(&This->oldmode,sizeof(DEVMODE));
	This->surfaces = (dxglDirectDrawSurface7 **)malloc(1024*sizeof(dxglDirectDrawSurface7 *));
	if(!This->surfaces) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	ZeroMemory(This->surfaces,1024*sizeof(dxglDirectDrawSurface7 *));
	This->surfacecount = 0;
	This->surfacecountmax = 1024;
	This->tmpsurface = NULL;
	This->clippers = (glDirectDrawClipper **)malloc(1024 * sizeof(glDirectDrawClipper *));
	if (!This->clippers) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	ZeroMemory(This->clippers, 1024 * sizeof(glDirectDrawClipper *));
	This->clippercount = 0;
	This->clippercountmax = 1024;
	bool useguid = false;
	switch((INT_PTR)lpGUID)
	{
	case NULL:
		break;
	case DDCREATE_EMULATIONONLY:
		DEBUG("DDRAW software emulation not supported, using OpenGL.\n");
		break;
	case DDCREATE_HARDWAREONLY:
		DEBUG("DDCREATE_HARDWAREONLY unnecessarily called.\n");
		break;
	default:
		useguid = true;
		DEBUG("Display GUIDs not yet supported, using primary.\n");
	}
	This->d3ddesc = d3ddesc_default;
	This->d3ddesc3 = d3ddesc3_default;
	memcpy(This->stored_devices, d3ddevices, 3 * sizeof(D3DDevice));
	This->initialized = true;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_RestoreDisplayMode(glDirectDraw7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(This->oldmode.dmSize != 0)
	{
		SetVidMode(NULL,&This->oldmode,0);
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
void glDirectDraw7_UnrestoreDisplayMode(glDirectDraw7 *This)
{
	if (!This) return;
	TRACE_ENTER(1, 14, This);
	if (This->currmode.dmSize != 0)
	{
		if((This->currmode.dmPelsWidth == 640) && (This->currmode.dmPelsHeight == 480)
			&& dxglcfg.HackCrop640480to640400) Try640400Mode(NULL, &This->currmode, CDS_FULLSCREEN, NULL);
		else SetVidMode(NULL, &This->currmode, CDS_FULLSCREEN);
	}
	TRACE_EXIT(0, 0);
}
void glDirectDraw7_SetWindowSize(glDirectDraw7 *glDD7, DWORD dwWidth, DWORD dwHeight)
{
	glDD7->internalx = glDD7->screenx = dwWidth;
	glDD7->internaly = glDD7->screeny = dwHeight;
	if ((glDD7->primaryx == 640) && (glDD7->primaryy == 480) && dxglcfg.HackCrop640480to640400)
		glDD7->internaly = (DWORD)((float)glDD7->internaly * 1.2f);
	if (glDD7->renderer && glDD7->primary) glRenderer_DrawScreen(glDD7->renderer, glDD7->primary->texture,
		glDD7->primary->texture->palette, 0, NULL, FALSE, NULL, 0);
}
BOOL glDirectDraw7_GetFullscreen(glDirectDraw7 *glDD7)
{
	if (!glDD7) return FALSE;
	if (glDD7->fullscreen) return TRUE;
	else return FALSE;
}

HRESULT WINAPI glDirectDraw7_SetCooperativeLevel(glDirectDraw7 *This, HWND hWnd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,13,hWnd,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(hWnd && !IsWindow(hWnd)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if ((dwFlags & DDSCL_EXCLUSIVE) && !hWnd) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if(dwFlags & 0xFFFFE020) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DWORD winver = GetVersion();
	DWORD winvermajor = (DWORD)(LOBYTE(LOWORD(winver)));
	DWORD winverminor = (DWORD)(HIBYTE(LOWORD(winver)));
	/*if (((hWnd != this->hWnd) && this->hWnd) || (this->hWnd && (dwFlags & DDSCL_NORMAL)))
	{
		if (winstyle)
		{
			SetWindowLongPtrA(hWnd, GWL_STYLE, winstyle);
			SetWindowLongPtrA(hWnd, GWL_EXSTYLE, winstyleex);
			ShowWindow(hWnd, SW_RESTORE);
			winstyle = winstyleex = 0;
			SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
		}
	}*/  // Currently breaks The Settlers IV
	if (This->hWnd) UninstallDXGLHook(This->hWnd);
	This->hWnd = hWnd;
	if (!This->winstyle && !This->winstyleex)
	{
		This->winstyle = GetWindowLongPtrA(hWnd, GWL_STYLE);
		This->winstyleex = GetWindowLongPtrA(hWnd, GWL_EXSTYLE);
	}
	bool exclusive = false;
	This->devwnd = false;
	if(dwFlags & DDSCL_ALLOWMODEX)
	{
		// Validate flags
		if(dwFlags & (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE))
		{
			DEBUG("IDirectDraw::SetCooperativeLevel: Mode X not supported, ignoring\n");
		}
		else DEBUG("IDirectDraw::SetCooperativeLevel: DDSCL_ALLOWMODEX without DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE\n");
	}
	if(dwFlags & DDSCL_ALLOWREBOOT)
		DEBUG("IDirectDraw::SetCooperativeLevel: DDSCL_ALLOWREBOOT unnecessary\n");
	if(dwFlags & DDSCL_EXCLUSIVE)
		exclusive = true;
	else exclusive = false;
	if(dwFlags & DDSCL_FULLSCREEN)
		This->fullscreen = true;
	else This->fullscreen = false;
	if(exclusive)
		if(!This->fullscreen) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if (dwFlags & DDSCL_CREATEDEVICEWINDOW)
	{
		if (!exclusive) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		This->devwnd = true;
	}
	if (dwFlags & DDSCL_FPUPRESERVE)
		This->fpupreserve = true;
	else This->fpupreserve = false;
	if(dwFlags & DDSCL_FPUSETUP)
		This->fpusetup = true;
	else This->fpusetup = false;
	if(dwFlags & DDSCL_MULTITHREADED)
		This->threadsafe = true;
	else This->threadsafe = false;
	if(dwFlags & DDSCL_NORMAL)
		if(exclusive) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwFlags & DDSCL_NOWINDOWCHANGES)
		This->nowindowchanges = true;
	else This->nowindowchanges = false;
	if(dwFlags & DDSCL_SETDEVICEWINDOW)
		FIXME("IDirectDraw::SetCooperativeLevel: DDSCL_SETDEVICEWINDOW unsupported\n");
	if(dwFlags & DDSCL_SETFOCUSWINDOW)
		FIXME("IDirectDraw::SetCooperativeLevel: DDSCL_SETFOCUSWINDOW unsupported\n");
	InstallDXGLHook(hWnd, (LPDIRECTDRAW7)This);
	EnableWindowScaleHook(FALSE);
	DEVMODE devmode;
	ZeroMemory(&devmode,sizeof(DEVMODE));
	devmode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
	int x,y,bpp;
	if(This->fullscreen)
	{
		x = devmode.dmPelsWidth;
		y = devmode.dmPelsHeight;
		This->internalx = This->screenx = This->primaryx = devmode.dmPelsWidth;
		This->internaly = This->screeny = This->primaryy = devmode.dmPelsHeight;
	}
	else
	{
		RECT rect;
		GetClientRect(hWnd,&rect);
		x = rect.right - rect.left;
		y = rect.bottom - rect.top;
		if ((winvermajor > 4) || ((winvermajor == 4) && (winverminor >= 1)))
		{
			This->screenx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
			This->screeny = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		}
		else
		{ // Windows versions below 4.1 don't support multi-monitor
			This->screenx = devmode.dmPelsWidth;
			This->screeny = devmode.dmPelsHeight;
		}
		This->internalx = This->primaryx = (DWORD)((float)This->screenx / dxglcfg.WindowScaleX);
		This->internaly = This->primaryy = (DWORD)((float)This->screeny / dxglcfg.WindowScaleY);
		if ((dxglcfg.WindowScaleX != 1.0f) || (dxglcfg.WindowScaleY != 1.0f))
		{
			GetWindowRect(hWnd, &rect);
			EnableWindowScaleHook(TRUE);
			SetWindowPos(hWnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
				SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING);
		}
	}
	bpp = devmode.dmBitsPerPel;
	This->internalrefresh = This->primaryrefresh = This->screenrefresh = devmode.dmDisplayFrequency;
	This->primarybpp = bpp;
	InitGL(x,y,bpp,This->fullscreen,This->internalrefresh,hWnd,This,This->devwnd);
	This->cooplevel = dwFlags;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

void DiscardUndersizedModes(DEVMODE **array, DWORD *count, DEVMODE comp)
{
	DEVMODE *newarray = (DEVMODE *)malloc(sizeof(DEVMODE)*(*count));
	if(!newarray) return;
	DWORD newcount = 0;
	bool match;
	for(DWORD x = 0; x < (*count); x++)
	{
		match = false;
		memcpy(&newarray[newcount],&(*array)[x],sizeof(DEVMODE));
		for(int y = newcount; y > 0; y--)
		{
			if((*array)[x].dmBitsPerPel != comp.dmBitsPerPel ||
				(*array)[x].dmPelsWidth < comp.dmPelsWidth ||
				(*array)[x].dmPelsHeight < comp.dmPelsHeight)
			{
				match = true;
				break;
			}
		}
		if(!match) newcount++;
	}
	newarray = (DEVMODE*)realloc(newarray,sizeof(DEVMODE)*newcount);
	free(*array);
	*array = newarray;
	*count = newcount;
}


DEVMODE FindClosestMode(const DEVMODE in)
{
	DEVMODE newmode;
	DEVMODE *candidates = (DEVMODE *)malloc(128*sizeof(DEVMODE));
	DEVMODE mode;
	ZeroMemory(&mode,sizeof(DEVMODE));
	mode.dmSize = sizeof(DEVMODE);
	DEVMODE *tmp = NULL;
	DWORD modenum = 0;
	DWORD modemax = 128;
	while(EnumDisplaySettings(NULL,modenum++,&mode))
	{
		candidates[modenum-1] = mode;
		if(modenum >= modemax)
		{
			modemax += 128;
			tmp = (DEVMODE*)realloc(candidates,modemax*sizeof(DEVMODE));
			if(tmp == NULL)
			{
				free(candidates);
				return in;
			}
			candidates = tmp;
		}
	}
	DiscardDuplicateModes(&candidates,&modenum);
	DiscardUndersizedModes(&candidates,&modenum,in);
	qsort(candidates,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortRes);
	newmode = candidates[0];
	newmode.dmFields = DM_BITSPERPEL| DM_PELSWIDTH | DM_PELSHEIGHT;
	free(candidates);
	return newmode;
}

int IsStretchedMode(DWORD width, DWORD height)
{
	if ((width == 320) || (width == 360))
	{
		if ((height == 400) || (height == 480)) return 1;
	}
	else if ((width == 640) && (height == 200)) return 2;
	else return 0;
}

LONG Try640400Mode(LPCTSTR devname, DEVMODE *mode, DWORD flags, BOOL *crop400)
{
	LONG error;
	DEVMODE newmode = *mode;
	newmode.dmPelsHeight = 400;
	error = SetVidMode(devname, &newmode, flags);
	if (error == DISP_CHANGE_SUCCESSFUL) return error;
	// Try setting refresh to 70
	newmode.dmDisplayFrequency = 70;
	error = SetVidMode(devname, &newmode, flags);
	if (error == DISP_CHANGE_SUCCESSFUL) return error;
	// Try without refresh
	newmode.dmFields &= (DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_POSITION);
	error = SetVidMode(devname, &newmode, flags);
	if (error == DISP_CHANGE_SUCCESSFUL) return error;
	// Finally try the original mode
	if(crop400) *crop400 = FALSE;
	return SetVidMode(devname, mode, flags);
}

HRESULT WINAPI glDirectDraw7_SetDisplayMode(glDirectDraw7 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,dwWidth,8,dwHeight,8,dwBPP,8,dwRefreshRate,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (dwFlags & 0xFFFFFFFE) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if ((dwWidth == -1) && (dwHeight == -1) && (dwBPP == -1) && (dwRefreshRate == -1))
	{
		glDirectDraw7_WaitForVerticalBlank(This, 0, NULL);
		glDirectDraw7_WaitForVerticalBlank(This, 0, NULL);
		glDirectDraw7_RestoreDisplayMode(This);
		TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	}
	if ((dwBPP != 4) && (dwBPP != 8) && (dwBPP != 15) && (dwBPP != 16) && (dwBPP != 24) && (dwBPP != 32))
		TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!This->fullscreen) TRACE_RET(HRESULT, 23, DDERR_NOEXCLUSIVEMODE);
	DEBUG("IDirectDraw::SetDisplayMode: implement multiple monitors\n");
	DEVMODE newmode,newmode2;
	DEVMODE currmode;
	float aspect,xmul,ymul,xscale,yscale;
	LONG error;
	DWORD flags;
	BOOL crop400 = FALSE;
	int stretchmode;
	if ((dwWidth == 640) && (dwHeight == 480) && dxglcfg.HackCrop640480to640400) crop400 = TRUE;
	if ((dxglcfg.AddColorDepths & 2) && !(dxglcfg.AddColorDepths & 4) && (dwBPP == 16))
		dwBPP = 15;
	if(!This->oldmode.dmSize)
	{
		This->oldmode.dmSize = sizeof(DEVMODE);
		EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&This->oldmode);
	}
	currmode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&currmode);
	This->currmode.dmSize = 0;
	if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
		(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
	{
		if (dwWidth <= 400) xscale = 2.0f;
		else xscale = 1.0f;
		if (dwHeight <= 300) yscale = 2.0f;
		else yscale = 1.0f;
	}
	else
	{
		xscale = dxglcfg.postsizex;
		yscale = dxglcfg.postsizey;
	}
	switch (dxglcfg.fullmode)
	{
	case 0:
	case 1:
	case 5:
	default:  // Fullscreen modes, scaled borderless window, and fallback
		switch (dxglcfg.scaler)
		{
		case 0: // No scaling, switch mode
		default:
			newmode.dmSize = sizeof(DEVMODE);
			newmode.dmDriverExtra = 0;
			if (!dxglcfg.primaryscale || (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
				(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f)))
			{
				newmode.dmPelsWidth = (DWORD)(dwWidth * xscale);
				newmode.dmPelsHeight = (DWORD)(dwHeight * yscale);
			}
			else
			{
				newmode.dmPelsWidth = dwWidth;
				newmode.dmPelsHeight = dwHeight;
			}
			if (dxglcfg.colormode)
				newmode.dmBitsPerPel = dwBPP;
			else newmode.dmBitsPerPel = currmode.dmBitsPerPel;
			newmode.dmDisplayFrequency = dwRefreshRate;
			if (dwRefreshRate) newmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
			else newmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			flags = 0;
			if (This->fullscreen) flags |= CDS_FULLSCREEN;
			if (crop400) error = Try640400Mode(NULL, &newmode, flags, &crop400);
			else error = SetVidMode(NULL, &newmode, flags);
			switch (error)
			{
			case DISP_CHANGE_SUCCESSFUL:
				if (This->fullscreen) This->currmode = newmode;
				if (dxglcfg.primaryscale)
				{
					This->primaryx = newmode.dmPelsWidth;
					This->primaryy = newmode.dmPelsHeight;
				}
				else
				{
					This->primaryx = (DWORD)((float)newmode.dmPelsWidth / xscale);
					This->primaryy = (DWORD)((float)newmode.dmPelsHeight / yscale);
				}
				This->screenx = newmode.dmPelsWidth;
				if (crop400) This->screeny = 400;
				else This->screeny = newmode.dmPelsHeight;
				This->internalx = newmode.dmPelsWidth;
				This->internaly = newmode.dmPelsHeight;
				This->internalbpp = This->screenbpp = newmode.dmBitsPerPel;
				This->primarybpp = dwBPP;
				if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
				else This->internalrefresh = This->primaryrefresh = This->screenrefresh = currmode.dmDisplayFrequency;
				InitGL(This->screenx, This->screeny, This->screenbpp, true,
					This->internalrefresh, This->hWnd, This, This->devwnd);
				//glRenderer_SetBPP(this->renderer, primarybpp);
				This->primarylost = true;
				TRACE_EXIT(23, DD_OK);
				return DD_OK;
			case DISP_CHANGE_BADMODE:
				TRACE_RET(HRESULT, 23, DDERR_INVALIDMODE);
			case DISP_CHANGE_BADFLAGS:
				TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			case DISP_CHANGE_BADDUALVIEW:
				TRACE_RET(HRESULT, 23, DDERR_GENERIC);
			case DISP_CHANGE_BADPARAM:
				TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			case DISP_CHANGE_FAILED:
				TRACE_RET(HRESULT, 23, DDERR_GENERIC);
			case DISP_CHANGE_NOTUPDATED:
				TRACE_RET(HRESULT, 23, DDERR_GENERIC);
			case DISP_CHANGE_RESTART:
				TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTEDMODE);
			default:
				TRACE_RET(HRESULT, 23, DDERR_GENERIC);
			}
			TRACE_RET(HRESULT, 23, DDERR_GENERIC);
			break;
		case 1: // Stretch to screen
			This->primaryx = dwWidth;
			This->internalx = This->screenx = currmode.dmPelsWidth;
			This->primaryy = dwHeight;
			This->internaly = This->screeny = currmode.dmPelsHeight;
			if (crop400) This->internaly = (DWORD)((float)This->internaly * 1.2f);
			if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
			else This->internalbpp = This->screenbpp = currmode.dmBitsPerPel;
			if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
			else This->internalrefresh = This->primaryrefresh = This->screenrefresh = currmode.dmDisplayFrequency;
			This->primarybpp = dwBPP;
			InitGL(This->screenx, This->screeny, This->screenbpp, true,
				This->internalrefresh, This->hWnd, This, This->devwnd);
			//glRenderer_SetBPP(this->renderer, primarybpp);
			This->primarylost = true;
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
			break;
		case 2: // Scale to screen, aspect corrected
			This->primaryx = dwWidth;
			This->screenx = currmode.dmPelsWidth;
			This->primaryy = dwHeight;
			This->screeny = currmode.dmPelsHeight;
			if (_isnan(dxglcfg.aspect) || dxglcfg.aspect <= 0)
			{
				aspect = (float)dwWidth / (float)dwHeight;
				switch (stretchmode = IsStretchedMode(dwWidth, dwHeight))
				{
				case 1:
					aspect *= 2.0f;
					break;
				case 2:
					aspect /= 2.0f;
					break;
				default:
					break;
				}
				if (stretchmode)
				{
					if (This->screenx / aspect > This->screeny)
					{
						This->internalx = (DWORD)((float)This->screeny * (float)aspect);
						This->internaly = This->screeny;
					}
					else
					{
						This->internalx = This->screenx;
						This->internaly = (DWORD)((float)This->screenx / (float)aspect);
					}
				}
				else
				{
					xmul = (float)This->screenx / (float)dwWidth;
					if (crop400) ymul = (float)This->screeny / 400.0f;
					else ymul = (float)This->screeny / (float)dwHeight;
					if ((float)dwWidth*(float)ymul > (float)This->screenx)
					{
						This->internalx = (DWORD)((float)dwWidth * (float)xmul);
						if(crop400) This->internaly = (DWORD)(400.0f * (float)xmul);
						else This->internaly = (DWORD)((float)dwHeight * (float)xmul);
					}
					else
					{
						This->internalx = (DWORD)((float)dwWidth * (float)ymul);
						if(crop400) This->internaly = (DWORD)(400.0f * (float)ymul);
						This->internaly = (DWORD)((float)dwHeight * (float)ymul);
					}
				}
			}
			else
			{
				aspect = dxglcfg.aspect;
				if (This->screenx / aspect > This->screeny)
				{
					This->internalx = (DWORD)((float)This->screeny * (float)aspect);
					This->internaly = This->screeny;
				}
				else
				{
					This->internalx = This->screenx;
					This->internaly = (DWORD)((float)This->screenx / (float)aspect);
				}
			}
			if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
			else This->internalbpp = This->screenbpp = currmode.dmBitsPerPel;
			if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
			else This->internalrefresh = This->primaryrefresh = This->screenrefresh = currmode.dmDisplayFrequency;
			This->primarybpp = dwBPP;
			InitGL(This->screenx, This->screeny, This->screenbpp, true,
				This->internalrefresh, This->hWnd, This, This->devwnd);
			//glRenderer_SetBPP(this->renderer, primarybpp);
			This->primarylost = true;
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
			break;
		case 3: // Center image
			This->primaryx = dwWidth;
			This->internalx = (DWORD)((float)dwWidth * xscale);
			This->screenx = currmode.dmPelsWidth;
			This->primaryy = dwHeight;
			This->internaly = (DWORD)((float)dwHeight * yscale);
			This->screeny = currmode.dmPelsHeight;
			This->primarybpp = dwBPP;
			if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
			else This->internalbpp = This->screenbpp = currmode.dmBitsPerPel;
			if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
			else This->internalrefresh = This->primaryrefresh = This->screenrefresh = currmode.dmDisplayFrequency;
			InitGL(This->screenx, This->screeny, This->screenbpp, true,
				This->internalrefresh, This->hWnd, This, This->devwnd);
			//glRenderer_SetBPP(this->renderer, primarybpp);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
			break;
		case 4: // Switch then stretch
		case 5: // Switch then scale
		case 6: // Switch then center
			newmode.dmSize = sizeof(DEVMODE);
			newmode.dmDriverExtra = 0;
			newmode.dmPelsWidth = dwWidth;
			newmode.dmPelsHeight = dwHeight;
			if (dxglcfg.colormode)
				newmode.dmBitsPerPel = dwBPP;
			else newmode.dmBitsPerPel = currmode.dmBitsPerPel;
			newmode.dmDisplayFrequency = dwRefreshRate;
			if (dwRefreshRate) newmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
			else newmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			flags = 0;
			if (This->fullscreen) flags |= CDS_FULLSCREEN;
			if (crop400) error = Try640400Mode(NULL, &newmode, flags, &crop400);
			else error = SetVidMode(NULL, &newmode, flags);
			if (error != DISP_CHANGE_SUCCESSFUL)
			{
				newmode2 = FindClosestMode(newmode);
				if (crop400) error = Try640400Mode(NULL, &newmode2, flags, &crop400);
				else error = SetVidMode(NULL, &newmode2, flags);
			}
			else newmode2 = newmode;
			if (crop400) newmode2.dmPelsHeight = 400;
			if (error == DISP_CHANGE_SUCCESSFUL) This->currmode = newmode2;
			switch (dxglcfg.scaler)
			{
			case 4:
				This->primaryx = dwWidth;
				This->internalx = This->screenx = newmode2.dmPelsWidth;
				This->primaryy = dwHeight;
				This->internaly = This->screeny = newmode2.dmPelsHeight;
				if (crop400) This->internaly = (DWORD)((float)This->internaly* 1.2f);
				if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
				else This->internalbpp = This->screenbpp = newmode2.dmBitsPerPel;
				if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
				else This->internalrefresh = This->primaryrefresh = This->screenrefresh = newmode2.dmDisplayFrequency;
				This->primarybpp = dwBPP;
				InitGL(This->screenx, This->screeny, This->screenbpp, true,
					This->internalrefresh, This->hWnd, This, This->devwnd);
				//glRenderer_SetBPP(this->renderer, primarybpp);
				This->primarylost = true;
				TRACE_EXIT(23, DD_OK);
				return DD_OK;
				break;
			case 5:
				This->primaryx = dwWidth;
				This->screenx = newmode2.dmPelsWidth;
				This->primaryy = dwHeight;
				This->screeny = newmode2.dmPelsHeight;
				if (_isnan(dxglcfg.aspect) || dxglcfg.aspect <= 0)
				{
					aspect = (float)dwWidth / (float)dwHeight;
					switch (stretchmode = IsStretchedMode(dwWidth, dwHeight))
					{
					case 1:
						aspect *= 2.0f;
						break;
					case 2:
						aspect /= 2.0f;
						break;
					default:
						break;
					}
					if (stretchmode)
					{
						if (This->screenx / aspect > This->screeny)
						{
							This->internalx = (DWORD)((float)This->screeny * (float)aspect);
							This->internaly = This->screeny;
						}
						else
						{
							This->internalx = This->screenx;
							This->internaly = (DWORD)((float)This->screenx / (float)aspect);
						}
					}
					else
					{
						xmul = (float)This->screenx / (float)dwWidth;
						if (crop400) ymul = (float)This->screeny / 400.0f;
						else ymul = (float)This->screeny / (float)dwHeight;
						if ((float)dwWidth*(float)ymul > (float)This->screenx)
						{
							This->internalx = (DWORD)((float)dwWidth * (float)xmul);
							if (crop400) This->internaly = (DWORD)(400.0f * (float)xmul);
							else This->internaly = (DWORD)((float)dwHeight * (float)xmul);
						}
						else
						{
							This->internalx = (DWORD)((float)dwWidth * (float)ymul);
							if (crop400) This->internaly = (DWORD)(400.0f * (float)ymul);
							This->internaly = (DWORD)((float)dwHeight * (float)ymul);
						}
					}
				}
				else
				{
					aspect = dxglcfg.aspect;
					if (This->screenx / aspect > This->screeny)
					{
						This->internalx = (DWORD)((float)This->screeny * (float)aspect);
						This->internaly = This->screeny;
					}
					else
					{
						This->internalx = This->screenx;
						This->internaly = (DWORD)((float)This->screenx / (float)aspect);
					}
				}
				if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
				else This->internalbpp = This->screenbpp = newmode2.dmBitsPerPel;
				if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
				else This->internalrefresh = This->primaryrefresh = This->screenrefresh = newmode2.dmDisplayFrequency;
				This->primarybpp = dwBPP;
				InitGL(This->screenx, This->screeny, This->screenbpp, true,
					This->internalrefresh, This->hWnd, This, This->devwnd);
				//glRenderer_SetBPP(this->renderer, primarybpp);
				This->primarylost = true;
				TRACE_EXIT(23, DD_OK);
				return DD_OK;
				break;
			case 6:
			default:
				This->primaryx = This->internalx = dwWidth;
				This->screenx = newmode2.dmPelsWidth;
				This->primaryy = This->internaly = dwHeight;
				This->screeny = newmode2.dmPelsHeight;
				This->primarybpp = dwBPP;
				if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
				else This->internalbpp = This->screenbpp = newmode2.dmBitsPerPel;
				if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
				else This->internalrefresh = This->primaryrefresh = This->screenrefresh = newmode2.dmDisplayFrequency;
				InitGL(This->screenx, This->screeny, This->screenbpp, true,
					This->internalrefresh, This->hWnd, This, This->devwnd);
				//glRenderer_SetBPP(this->renderer, primarybpp);
				This->primarylost = true;
				TRACE_EXIT(23, DD_OK);
				return DD_OK;
				break;
			}
			break;
		case 7: // Crop to screen, aspect corrected
			This->primaryx = dwWidth;
			This->screenx = currmode.dmPelsWidth;
			This->primaryy = dwHeight;
			This->screeny = currmode.dmPelsHeight;
			if (_isnan(dxglcfg.aspect) || dxglcfg.aspect <= 0)
			{
				aspect = (float)dwWidth / (float)dwHeight;
				switch (stretchmode = IsStretchedMode(dwWidth, dwHeight))
				{
				case 1:
					aspect *= 2.0f;
					break;
				case 2:
					aspect /= 2.0f;
					break;
				default:
					break;
				}
				if (stretchmode)
				{
					if (This->screenx / aspect < This->screeny)
					{
						This->internalx = (DWORD)((float)This->screeny * (float)aspect);
						This->internaly = This->screeny;
					}
					else
					{
						This->internalx = This->screenx;
						This->internaly = (DWORD)((float)This->screenx / (float)aspect);
					}
				}
				else
				{
					xmul = (float)This->screenx / (float)dwWidth;
					ymul = (float)This->screeny / (float)dwHeight;
					if ((float)dwWidth*(float)ymul < (float)This->screenx)
					{
						This->internalx = (DWORD)((float)dwWidth * (float)xmul);
						This->internaly = (DWORD)((float)dwHeight * (float)xmul);
					}
					else
					{
						This->internalx = (DWORD)((float)dwWidth * (float)ymul);
						This->internaly = (DWORD)((float)dwHeight * (float)ymul);
					}
				}
			}
			else
			{
				aspect = dxglcfg.aspect;
				if (This->screenx / aspect < This->screeny)
				{
					This->internalx = (DWORD)((float)This->screeny * (float)aspect);
					This->internaly = This->screeny;
				}
				else
				{
					This->internalx = This->screenx;
					This->internaly = (DWORD)((float)This->screenx / (float)aspect);
				}
			}
			if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
			else This->internalbpp = This->screenbpp = currmode.dmBitsPerPel;
			if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
			else This->internalrefresh = This->primaryrefresh = This->screenrefresh = currmode.dmDisplayFrequency;
			This->primarybpp = dwBPP;
			InitGL(This->screenx, This->screeny, This->screenbpp, true,
				This->internalrefresh, This->hWnd, This, This->devwnd);
			//glRenderer_SetBPP(this->renderer, primarybpp);
			This->primarylost = true;
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
			break;
		case 8:  // Custom size multiplier
			This->primaryx = dwWidth;
			This->internalx = (DWORD)((float)dwWidth * xscale);
			if (dxglcfg.DisplayMultiplierX) This->internalx =
				(DWORD)((float)This->internalx * dxglcfg.DisplayMultiplierX);
			This->screenx = currmode.dmPelsWidth;
			This->primaryy = dwHeight;
			This->internaly = (DWORD)((float)dwHeight * yscale);
			if (dxglcfg.DisplayMultiplierY) This->internaly =
				(DWORD)((float)This->internaly * dxglcfg.DisplayMultiplierY);
			This->screeny = currmode.dmPelsHeight;
			This->primarybpp = dwBPP;
			if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
			else This->internalbpp = This->screenbpp = currmode.dmBitsPerPel;
			if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
			else This->internalrefresh = This->primaryrefresh = This->screenrefresh = currmode.dmDisplayFrequency;
			InitGL(This->screenx, This->screeny, This->screenbpp, true,
				This->internalrefresh, This->hWnd, This, This->devwnd);
			//glRenderer_SetBPP(this->renderer, primarybpp);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
			break;
		case 9:  // Custom display mode
			newmode.dmSize = sizeof(DEVMODE);
			newmode.dmDriverExtra = 0;
			newmode.dmPelsWidth = dxglcfg.CustomResolutionX;
			newmode.dmPelsHeight = dxglcfg.CustomResolutionY;
			if (dxglcfg.colormode)
				newmode.dmBitsPerPel = dwBPP;
			else newmode.dmBitsPerPel = currmode.dmBitsPerPel;
			newmode.dmDisplayFrequency = dxglcfg.CustomRefresh;
			if (dxglcfg.CustomRefresh) newmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
			else newmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			flags = 0;
			if (This->fullscreen) flags |= CDS_FULLSCREEN;
			error = SetVidMode(NULL, &newmode, flags);
			if (error == DISP_CHANGE_SUCCESSFUL) currmode = newmode;
			This->primaryx = dwWidth;
			This->internalx = This->screenx = currmode.dmPelsWidth;
			This->primaryy = dwHeight;
			This->internaly = This->screeny = currmode.dmPelsHeight;
			if (crop400) This->internaly  = (DWORD)((float)This->internaly * 1.2f);
			if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
			else This->internalbpp = This->screenbpp = currmode.dmBitsPerPel;
			if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
			else This->internalrefresh = This->primaryrefresh = This->screenrefresh = currmode.dmDisplayFrequency;
			This->primarybpp = dwBPP;
			InitGL(This->screenx, This->screeny, This->screenbpp, true,
				This->internalrefresh, This->hWnd, This, This->devwnd);
			//glRenderer_SetBPP(this->renderer, primarybpp);
			This->primarylost = true;
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
			break;
		case 10: // Custom size, centered
			This->primaryx = dwWidth;
			This->internalx = dxglcfg.CustomResolutionX;
			This->screenx = currmode.dmPelsWidth;
			This->primaryy = dwHeight;
			This->internaly = dxglcfg.CustomResolutionY;
			This->screeny = currmode.dmPelsHeight;
			This->primarybpp = dwBPP;
			if (dxglcfg.colormode) This->internalbpp = This->screenbpp = dwBPP;
			else This->internalbpp = This->screenbpp = currmode.dmBitsPerPel;
			if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
			else This->internalrefresh = This->primaryrefresh = This->screenrefresh = currmode.dmDisplayFrequency;
			InitGL(This->screenx, This->screeny, This->screenbpp, true, 
				This->internalrefresh, This->hWnd, This, This->devwnd);
			//glRenderer_SetBPP(this->renderer, primarybpp);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
			break;
		}
		break;
	case 2:
	case 3:
	case 4:  // Forced windowed modes
		This->primaryx = dwWidth;
		This->internalx = This->screenx = (DWORD)((float)dwWidth * xscale);
		This->primaryy = dwHeight;
		if (crop400)
		{
			This->screeny = (DWORD)(400.0f * yscale);
			This->internaly = (DWORD)((float)dwHeight * yscale);
		}
		else This->internaly = This->screeny = (DWORD)((float)dwHeight * yscale);
		This->internalbpp = This->screenbpp = dwBPP;
		if (dwRefreshRate) This->internalrefresh = This->primaryrefresh = This->screenrefresh = dwRefreshRate;
		else This->internalrefresh = This->primaryrefresh = This->screenrefresh = currmode.dmDisplayFrequency;
		This->primarybpp = dwBPP;
		InitGL(This->screenx, This->screeny, This->screenbpp, true,
			This->internalrefresh, This->hWnd, This, This->devwnd);
		//glRenderer_SetBPP(this->renderer, primarybpp);
		This->primarylost = true;
		TRACE_EXIT(23, DD_OK);
		return DD_OK;
		break;
	}
	TRACE_EXIT(23, DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7_WaitForVerticalBlank(glDirectDraw7 *This, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwFlags & DDWAITVB_BLOCKBEGINEVENT)
		TRACE_RET(HRESULT,23,DDERR_UNSUPPORTED);
	if(dwFlags & 0xFFFFFFFA) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwFlags == 5) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->lastsync) This->lastsync = true;
	else if(This->primary) dxglDirectDrawSurface7_RenderScreen(This->primary,This->primary->texture,1,NULL,TRUE,NULL,0);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_GetAvailableVidMem(glDirectDraw7 *This, LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	TRACE_ENTER(4,14,This,14,lpDDSCaps2,14,lpdwTotal,14,lpdwFree);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	/*if(GLEXT_NVX_gpu_memory_info)
	{
		if(lpdwTotal) glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX,(GLint*)lpdwTotal);
		if(lpdwFree) glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX,(GLint*)lpdwFree);
		if(lpdwTotal) {TRACE_VAR("*lpdwTotal",8,*lpdwTotal);}
		if(lpdwFree) {TRACE_VAR("*lpdwFree",8,*lpdwFree);}
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else*/
	{
		MEMORYSTATUS memstat;
		GlobalMemoryStatus(&memstat);
		if(lpdwTotal) *lpdwTotal = memstat.dwTotalVirtual;
		if(lpdwFree) *lpdwFree = memstat.dwAvailVirtual;
		if(lpdwTotal) {TRACE_VAR("*lpdwTotal",8,*lpdwTotal);}
		if(lpdwFree) {TRACE_VAR("*lpdwFree",8,*lpdwFree);}
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
}
HRESULT WINAPI glDirectDraw7_GetSurfaceFromDC(glDirectDraw7 *This, HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS)
{
	TRACE_ENTER(3,14,This,13,hdc,14,lpDDS);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("IDirectDraw_GetSurfaceFromDC: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7_RestoreAllSurfaces(glDirectDraw7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	for (int i = 0; i < This->surfacecount; i++)
	{
		if (This->surfaces[i]) dxglDirectDrawSurface7_Restore(This->surfaces[i]);
	}
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_TestCooperativeLevel(glDirectDraw7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_GetDeviceIdentifier(glDirectDraw7 *This, LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpdddi,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpdddi) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	This->devid.guidDeviceIdentifier = device_template;
	memcpy(lpdddi,&This->devid,sizeof(DDDEVICEIDENTIFIER2));
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7_StartModeTest(glDirectDraw7 *This, LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,25,lpModesToTest,8,dwNumEntries,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("IDirectDraw_StartModeTest: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7_EvaluateMode(glDirectDraw7 *This, DWORD dwFlags, DWORD *pSecondsUntilTimeout)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,pSecondsUntilTimeout);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("IDirectDraw_EvaluateMode: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

void glDirectDraw7_GetSizes(glDirectDraw7 *This, LONG *sizes) // allocate 6 dwords
{
	sizes[0] = This->internalx;
	sizes[1] = This->internaly;
	sizes[2] = This->primaryx;
	sizes[3] = This->primaryy;
	sizes[4] = This->screenx;
	sizes[5] = This->screeny;
}

DWORD glDirectDraw7_GetBPPMultipleOf8(glDirectDraw7 *This)
{
	if (This->primarybpp == 15) return 16;
	else return This->primarybpp;
}

void glDirectDraw7_DeleteSurface(glDirectDraw7 *This, dxglDirectDrawSurface7 *surface)
{
	TRACE_ENTER(2,14,This,14,surface);
	for(int i = 0; i < This->surfacecount; i++)
		if(This->surfaces[i] == surface) This->surfaces[i] = NULL;
	if (surface == This->primary)
	{
		This->primary = NULL;
		glDirectDraw7_DeleteTempSurface(This);
	}
	TRACE_EXIT(0,0);
}

void glDirectDraw7_DeleteClipper(glDirectDraw7 *This, glDirectDrawClipper *clipper)
{
	TRACE_ENTER(2, 14, This, clipper);
	for (int i = 0; i < This->clippercount; i++)
		if (This->clippers[i] == clipper) This->clippers[i] = NULL;
	TRACE_EXIT(0, 0);
}

HRESULT glDirectDraw7_SetupTempSurface(glDirectDraw7 *This, DWORD width, DWORD height)
{
	DDSURFACEDESC2 ddsd;
	HRESULT error;
	if (!width || !height) return DDERR_INVALIDPARAMS;
	if (!This->tmpsurface)
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;
		error = glDirectDraw7_CreateSurface2(This, &ddsd,
			(LPDIRECTDRAWSURFACE7*)&This->tmpsurface, NULL, FALSE, 7);
		if (error == DDERR_OUTOFVIDEOMEMORY)
		{
			ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
			error = glDirectDraw7_CreateSurface2(This, &ddsd,
				(LPDIRECTDRAWSURFACE7*)&This->tmpsurface, NULL, FALSE, 7);
		}
		if (error != DD_OK) return error;
	}
	else
	{
		if ((This->tmpsurface->ddsd.dwWidth >= width) &&
			(This->tmpsurface->ddsd.dwHeight >= height)) return DD_OK;
		else
		{
			ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
			ddsd.dwSize = sizeof(DDSURFACEDESC2);
			ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
			ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
			ddsd.dwWidth = width > This->tmpsurface->ddsd.dwWidth ? width : This->tmpsurface->ddsd.dwWidth;
			ddsd.dwHeight = height > This->tmpsurface->ddsd.dwHeight ? height : This->tmpsurface->ddsd.dwHeight;
			dxglDirectDrawSurface7_Release(This->tmpsurface);
			This->tmpsurface = NULL;
			error = glDirectDraw7_CreateSurface(This, &ddsd,
				(LPDIRECTDRAWSURFACE7*)&This->tmpsurface, NULL);
			if (error == DDERR_OUTOFVIDEOMEMORY)
			{
				ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
				error = glDirectDraw7_CreateSurface(This, &ddsd,
					(LPDIRECTDRAWSURFACE7*)&This->tmpsurface, NULL);
			}
			if (error != DD_OK) return error;
		}
	}
	return DD_OK;
}

void glDirectDraw7_DeleteTempSurface(glDirectDraw7 *This)
{
	if (This->tmpsurface)
	{
		dxglDirectDrawSurface7_Release(This->tmpsurface);
		This->tmpsurface = 0;
	}
}

// DDRAW1 wrapper
glDirectDraw1Vtbl glDirectDraw1_impl =
{
	glDirectDraw1_QueryInterface,
	glDirectDraw1_AddRef,
	glDirectDraw1_Release,
	glDirectDraw1_Compact,
	glDirectDraw1_CreateClipper,
	glDirectDraw1_CreatePalette,
	glDirectDraw1_CreateSurface,
	glDirectDraw1_DuplicateSurface,
	glDirectDraw1_EnumDisplayModes,
	glDirectDraw1_EnumSurfaces,
	glDirectDraw1_FlipToGDISurface,
	glDirectDraw1_GetCaps,
	glDirectDraw1_GetDisplayMode,
	glDirectDraw1_GetFourCCCodes,
	glDirectDraw1_GetGDISurface,
	glDirectDraw1_GetMonitorFrequency,
	glDirectDraw1_GetScanLine,
	glDirectDraw1_GetVerticalBlankStatus,
	glDirectDraw1_Initialize,
	glDirectDraw1_RestoreDisplayMode,
	glDirectDraw1_SetCooperativeLevel,
	glDirectDraw1_SetDisplayMode,
	glDirectDraw1_WaitForVerticalBlank
};

HRESULT glDirectDraw1_Create(glDirectDraw7 *gl_DD7, glDirectDraw1 **glDD1)
{
	TRACE_ENTER(2,14,gl_DD7,14,glDD1);
	glDirectDraw1 *This = (glDirectDraw1*)malloc(sizeof(glDirectDraw1));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirectDraw1_impl;
	This->glDD7 = gl_DD7;
	*glDD1 = This;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw1_QueryInterface(glDirectDraw1 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if((riid == IID_IUnknown) || (riid == IID_IDirectDraw))
	{
		glDirectDraw1_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		return DD_OK;
	}
	HRESULT ret = glDirectDraw7_QueryInterface(This->glDD7,riid,ppvObj);
	TRACE_VAR("*ppvObj",14,*ppvObj);
	TRACE_EXIT(23,ret);
	return ret;
}
ULONG WINAPI glDirectDraw1_AddRef(glDirectDraw1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDraw7_AddRef1(This->glDD7));
}
ULONG WINAPI glDirectDraw1_Release(glDirectDraw1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDraw7_Release1(This->glDD7));
}
HRESULT WINAPI glDirectDraw1_Compact(glDirectDraw1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDraw7_Compact(This->glDD7));
}
HRESULT WINAPI glDirectDraw1_CreateClipper(glDirectDraw1 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lplpDDClipper,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDraw7_CreateClipper2(This->glDD7, dwFlags, lplpDDClipper, pUnkOuter);
	if (ret == DD_OK)
	{
		glDirectDraw1_AddRef(This);
		((glDirectDrawClipper*)*lplpDDClipper)->creator = (IUnknown*)This;
	}	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw1_CreatePalette(glDirectDraw1 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(5, 14, This, 9, dwFlags, 14, lpDDColorArray, 14, lplpDDPalette, 14, pUnkOuter);
	if (!IsReadablePointer(This, sizeof(glDirectDraw1))) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDraw7_CreatePalette2(This->glDD7, dwFlags, lpDDColorArray, lplpDDPalette, pUnkOuter);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw1_CreateSurface(glDirectDraw1 *This, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4,14,This,14,lpDDSurfaceDesc,14,lplpDDSurface,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSurfaceDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpDDSurfaceDesc->dwSize < sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 lpDDS7;
	DDSURFACEDESC2 ddsd2;
	ZeroMemory(&ddsd2, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd2, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	ddsd2.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT err = glDirectDraw7_CreateSurface2(This->glDD7,&ddsd2,&lpDDS7,pUnkOuter,TRUE,1);
	if(err == DD_OK)
	{
		lpDDS7->QueryInterface(IID_IDirectDrawSurface,(LPVOID*) lplpDDSurface);
		lpDDS7->Release();
		TRACE_VAR("*lplpDDSurface",14,lplpDDSurface);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else *lplpDDSurface = NULL;
	TRACE_EXIT(23,err);
	return err;
}
HRESULT WINAPI glDirectDraw1_DuplicateSurface(glDirectDraw1 *This, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSurface,14,lplpDupDDSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!lplpDupDDSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC ddsd;
	HRESULT ret;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	lpDDSurface->GetSurfaceDesc(&ddsd);
	ret = glDirectDraw1_CreateSurface(This, &ddsd, lplpDupDDSurface, NULL);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ret = (*lplpDupDDSurface)->Blt(NULL, lpDDSurface, NULL, 0, NULL);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw1_EnumDisplayModes(glDirectDraw1 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	TRACE_ENTER(5,14,This,9,dwFlags,14,lpDDSurfaceDesc,14,lpContext,14,lpEnumModesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,EnumDisplayModes1(dwFlags,lpDDSurfaceDesc,lpContext,lpEnumModesCallback));
}
HRESULT WINAPI glDirectDraw1_EnumSurfaces(glDirectDraw1 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(5,14,This,9,dwFlags,14,lpDDSD,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSD)
		if (lpDDSD->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	if (lpDDSD)
	{
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		memcpy(&ddsd, lpDDSD, sizeof(DDSURFACEDESC));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		TRACE_RET(HRESULT, 23, glDirectDraw7_EnumSurfaces(This->glDD7, dwFlags, &ddsd, context, EnumSurfacesCallback1));
	}
	else TRACE_RET(HRESULT, 23, glDirectDraw7_EnumSurfaces(This->glDD7, dwFlags, NULL, context, EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDraw1_FlipToGDISurface(glDirectDraw1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_FlipToGDISurface(This->glDD7));
}
HRESULT WINAPI glDirectDraw1_GetCaps(glDirectDraw1 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	TRACE_ENTER(3,14,This,14,lpDDDriverCaps,14,lpDDHELCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetCaps(This->glDD7,lpDDDriverCaps,lpDDHELCaps));
}
HRESULT WINAPI glDirectDraw1_GetDisplayMode(glDirectDraw1 *This, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetDisplayMode(This->glDD7, (LPDDSURFACEDESC2)lpDDSurfaceDesc));
}
HRESULT WINAPI glDirectDraw1_GetFourCCCodes(glDirectDraw1 *This, LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	TRACE_ENTER(3,14,This,14,lpNumCodes,14,lpCodes);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetFourCCCodes(This->glDD7,lpNumCodes,lpCodes));
}
HRESULT WINAPI glDirectDraw1_GetGDISurface(glDirectDraw1 *This, LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface)
{
	TRACE_ENTER(2,14,This,14,lplpGDIDDSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDraw1_GetGDISurface: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw1_GetMonitorFrequency(glDirectDraw1 *This, LPDWORD lpdwFrequency)
{
	TRACE_ENTER(2,14,This,14,lpdwFrequency);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetMonitorFrequency(This->glDD7, lpdwFrequency));
}
HRESULT WINAPI glDirectDraw1_GetScanLine(glDirectDraw1 *This, LPDWORD lpdwScanLine)
{
	TRACE_ENTER(2,14,This,14,lpdwScanLine);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, glDirectDraw7_GetScanLine(This->glDD7, lpdwScanLine));
}
HRESULT WINAPI glDirectDraw1_GetVerticalBlankStatus(glDirectDraw1 *This, LPBOOL lpbIsInVB)
{
	TRACE_ENTER(2,14,This,14,lpbIsInVB);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetVerticalBlankStatus(This->glDD7, lpbIsInVB));
}
HRESULT WINAPI glDirectDraw1_Initialize(glDirectDraw1 *This, GUID FAR *lpGUID)
{
	TRACE_ENTER(2,14,This,24,lpGUID);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_Initialize(This->glDD7, lpGUID));
}
HRESULT WINAPI glDirectDraw1_RestoreDisplayMode(glDirectDraw1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_RestoreDisplayMode(This->glDD7));
}
HRESULT WINAPI glDirectDraw1_SetCooperativeLevel(glDirectDraw1 *This, HWND hWnd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,13,hWnd,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_SetCooperativeLevel(This->glDD7,hWnd,dwFlags));
}
HRESULT WINAPI glDirectDraw1_SetDisplayMode(glDirectDraw1 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	TRACE_ENTER(4,14,This,8,dwWidth,8,dwHeight,8,dwBPP);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_SetDisplayMode(This->glDD7,dwWidth,dwHeight,dwBPP,0,0));
}
HRESULT WINAPI glDirectDraw1_WaitForVerticalBlank(glDirectDraw1 *This, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_WaitForVerticalBlank(This->glDD7,dwFlags,hEvent));
}
// DDRAW2 wrapper
glDirectDraw2Vtbl glDirectDraw2_impl =
{
	glDirectDraw2_QueryInterface,
	glDirectDraw2_AddRef,
	glDirectDraw2_Release,
	glDirectDraw2_Compact,
	glDirectDraw2_CreateClipper,
	glDirectDraw2_CreatePalette,
	glDirectDraw2_CreateSurface,
	glDirectDraw2_DuplicateSurface,
	glDirectDraw2_EnumDisplayModes,
	glDirectDraw2_EnumSurfaces,
	glDirectDraw2_FlipToGDISurface,
	glDirectDraw2_GetCaps,
	glDirectDraw2_GetDisplayMode,
	glDirectDraw2_GetFourCCCodes,
	glDirectDraw2_GetGDISurface,
	glDirectDraw2_GetMonitorFrequency,
	glDirectDraw2_GetScanLine,
	glDirectDraw2_GetVerticalBlankStatus,
	glDirectDraw2_Initialize,
	glDirectDraw2_RestoreDisplayMode,
	glDirectDraw2_SetCooperativeLevel,
	glDirectDraw2_SetDisplayMode,
	glDirectDraw2_WaitForVerticalBlank,
	glDirectDraw2_GetAvailableVidMem
};

HRESULT glDirectDraw2_Create(glDirectDraw7 *gl_DD7, glDirectDraw2 **glDD2)
{
	TRACE_ENTER(2, 14, gl_DD7, 14, glDD2);
	glDirectDraw2 *This = (glDirectDraw2*)malloc(sizeof(glDirectDraw2));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirectDraw2_impl;
	This->glDD7 = gl_DD7;
	*glDD2 = This;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw2_QueryInterface(glDirectDraw2 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if((riid == IID_IUnknown) || (riid == IID_IDirectDraw2))
	{
		glDirectDraw2_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObject",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23, glDirectDraw7_QueryInterface(This->glDD7,riid,ppvObj));
}
ULONG WINAPI glDirectDraw2_AddRef(glDirectDraw2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDraw7_AddRef2(This->glDD7));
}
ULONG WINAPI glDirectDraw2_Release(glDirectDraw2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDraw7_Release2(This->glDD7));
}
HRESULT WINAPI glDirectDraw2_Compact(glDirectDraw2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_Compact(This->glDD7));
}
HRESULT WINAPI glDirectDraw2_CreateClipper(glDirectDraw2 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lplpDDClipper,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDraw7_CreateClipper2(This->glDD7, dwFlags, lplpDDClipper, pUnkOuter);
	if (ret == DD_OK)
	{
		glDirectDraw2_AddRef(This);
		((glDirectDrawClipper*)*lplpDDClipper)->creator = (IUnknown*)This;
	}
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw2_CreatePalette(glDirectDraw2 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(5, 14, This, 9, dwFlags, 14, lpDDColorArray, 14, lplpDDPalette, 14, pUnkOuter);
	if (!IsReadablePointer(This, sizeof(glDirectDraw2))) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDraw7_CreatePalette2(This->glDD7, dwFlags, lpDDColorArray, lplpDDPalette, pUnkOuter);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw2_CreateSurface(glDirectDraw2 *This, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4,14,This,14,lpDDSurfaceDesc,14,lplpDDSurface,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDDSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpDDSurfaceDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpDDSurfaceDesc->dwSize < sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 lpDDS7;
	DDSURFACEDESC2 ddsd2;
	ZeroMemory(&ddsd2, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd2, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	ddsd2.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT err = glDirectDraw7_CreateSurface2(This->glDD7, &ddsd2, &lpDDS7, pUnkOuter, TRUE, 2);
	if(err == DD_OK)
	{
		lpDDS7->QueryInterface(IID_IDirectDrawSurface,(LPVOID*) lplpDDSurface);
		lpDDS7->Release();
		TRACE_EXIT(23, DD_OK);
		return DD_OK;
	}
	else *lplpDDSurface = NULL;
	TRACE_EXIT(23,err);
	return err;
}
HRESULT WINAPI glDirectDraw2_DuplicateSurface(glDirectDraw2 *This, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSurface,14,lplpDupDDSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!lplpDupDDSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC ddsd;
	HRESULT ret;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	lpDDSurface->GetSurfaceDesc(&ddsd);
	ret = glDirectDraw2_CreateSurface(This, &ddsd, lplpDupDDSurface, NULL);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ret = (*lplpDupDDSurface)->Blt(NULL, lpDDSurface, NULL, 0, NULL);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw2_EnumDisplayModes(glDirectDraw2 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	TRACE_ENTER(5,14,This,9,dwFlags,14,lpDDSurfaceDesc,14,lpContext,14,lpEnumModesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,EnumDisplayModes1(dwFlags,lpDDSurfaceDesc,lpContext,lpEnumModesCallback));
}
HRESULT WINAPI glDirectDraw2_EnumSurfaces(glDirectDraw2 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(5,14,This,9,dwFlags,14,lpDDSD,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (lpDDSD)
		if (lpDDSD->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	if (lpDDSD)
	{
		DDSURFACEDESC2 ddsd;
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		memcpy(&ddsd, lpDDSD, sizeof(DDSURFACEDESC));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		TRACE_RET(HRESULT, 23, glDirectDraw7_EnumSurfaces(This->glDD7, dwFlags, &ddsd, context, EnumSurfacesCallback1));
	}
	else TRACE_RET(HRESULT, 23, glDirectDraw7_EnumSurfaces(This->glDD7, dwFlags, NULL, context, EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDraw2_FlipToGDISurface(glDirectDraw2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_FlipToGDISurface(This->glDD7));
}
HRESULT WINAPI glDirectDraw2_GetCaps(glDirectDraw2 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	TRACE_ENTER(3,14,This,14,lpDDDriverCaps,14,lpDDHELCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetCaps(This->glDD7,lpDDDriverCaps,lpDDHELCaps));
}
HRESULT WINAPI glDirectDraw2_GetDisplayMode(glDirectDraw2 *This, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetDisplayMode(This->glDD7,(LPDDSURFACEDESC2)lpDDSurfaceDesc));
}
HRESULT WINAPI glDirectDraw2_GetFourCCCodes(glDirectDraw2 *This, LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	TRACE_ENTER(3,14,This,14,lpNumCodes,14,lpCodes);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetFourCCCodes(This->glDD7,lpNumCodes,lpCodes));
}
HRESULT WINAPI glDirectDraw2_GetGDISurface(glDirectDraw2 *This, LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface)
{
	TRACE_ENTER(2,14,This,14,lplpGDIDDSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDraw2_GetGDISurface: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw2_GetMonitorFrequency(glDirectDraw2 *This, LPDWORD lpdwFrequency)
{
	TRACE_ENTER(2,14,This,14,lpdwFrequency);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetMonitorFrequency(This->glDD7, lpdwFrequency));
}
HRESULT WINAPI glDirectDraw2_GetScanLine(glDirectDraw2 *This, LPDWORD lpdwScanLine)
{
	TRACE_ENTER(2,14,This,14,lpdwScanLine);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetScanLine(This->glDD7, lpdwScanLine));
}
HRESULT WINAPI glDirectDraw2_GetVerticalBlankStatus(glDirectDraw2 *This, LPBOOL lpbIsInVB)
{
	TRACE_ENTER(2,14,This,14,lpbIsInVB);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetVerticalBlankStatus(This->glDD7, lpbIsInVB));
}
HRESULT WINAPI glDirectDraw2_Initialize(glDirectDraw2 *This, GUID FAR *lpGUID)
{
	TRACE_ENTER(2,14,This,24,lpGUID);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_Initialize(This->glDD7, lpGUID));
}
HRESULT WINAPI glDirectDraw2_RestoreDisplayMode(glDirectDraw2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_RestoreDisplayMode(This->glDD7));
}
HRESULT WINAPI glDirectDraw2_SetCooperativeLevel(glDirectDraw2 *This, HWND hWnd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,13,hWnd,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_SetCooperativeLevel(This->glDD7,hWnd,dwFlags));
}
HRESULT WINAPI glDirectDraw2_SetDisplayMode(glDirectDraw2 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,dwWidth,8,dwHeight,8,dwBPP,8,dwRefreshRate,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_SetDisplayMode(This->glDD7,dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags));
}
HRESULT WINAPI glDirectDraw2_WaitForVerticalBlank(glDirectDraw2 *This, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_WaitForVerticalBlank(This->glDD7,dwFlags,hEvent));
}
HRESULT WINAPI glDirectDraw2_GetAvailableVidMem(glDirectDraw2 *This, LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	TRACE_ENTER(4,14,This,14,lpDDSCaps,14,lpdwTotal,14,lpdwFree);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	/*if(GLEXT_NVX_gpu_memory_info)
	{
		if(lpdwTotal) glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX,(GLint*)lpdwTotal);
		if(lpdwFree) glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX,(GLint*)lpdwFree);
		if(lpdwTotal) {TRACE_VAR("*lpdwTotal",8,*lpdwTotal);}
		if(lpdwFree) {TRACE_VAR("*lpdwFree",8,*lpdwFree);}
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else*/
	{
		MEMORYSTATUS memstat;
		GlobalMemoryStatus(&memstat);
		if(lpdwTotal) *lpdwTotal = memstat.dwTotalVirtual;
		if(lpdwFree) *lpdwFree = memstat.dwAvailVirtual;
		if(lpdwTotal) {TRACE_VAR("*lpdwTotal",8,*lpdwTotal);}
		if(lpdwFree) {TRACE_VAR("*lpdwFree",8,*lpdwFree);}
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
}
// DDRAW4 wrapper
glDirectDraw4Vtbl glDirectDraw4_impl =
{
	glDirectDraw4_QueryInterface,
	glDirectDraw4_AddRef,
	glDirectDraw4_Release,
	glDirectDraw4_Compact,
	glDirectDraw4_CreateClipper,
	glDirectDraw4_CreatePalette,
	glDirectDraw4_CreateSurface,
	glDirectDraw4_DuplicateSurface,
	glDirectDraw4_EnumDisplayModes,
	glDirectDraw4_EnumSurfaces,
	glDirectDraw4_FlipToGDISurface,
	glDirectDraw4_GetCaps,
	glDirectDraw4_GetDisplayMode,
	glDirectDraw4_GetFourCCCodes,
	glDirectDraw4_GetGDISurface,
	glDirectDraw4_GetMonitorFrequency,
	glDirectDraw4_GetScanLine,
	glDirectDraw4_GetVerticalBlankStatus,
	glDirectDraw4_Initialize,
	glDirectDraw4_RestoreDisplayMode,
	glDirectDraw4_SetCooperativeLevel,
	glDirectDraw4_SetDisplayMode,
	glDirectDraw4_WaitForVerticalBlank,
	glDirectDraw4_GetAvailableVidMem,
	glDirectDraw4_GetSurfaceFromDC,
	glDirectDraw4_RestoreAllSurfaces,
	glDirectDraw4_TestCooperativeLevel,
	glDirectDraw4_GetDeviceIdentifier
};

HRESULT glDirectDraw4_Create(glDirectDraw7 *gl_DD7, glDirectDraw4 **glDD4)
{
	TRACE_ENTER(2, 14, gl_DD7, 14, glDD4);
	glDirectDraw4 *This = (glDirectDraw4*)malloc(sizeof(glDirectDraw4));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirectDraw4_impl;
	This->glDD7 = gl_DD7;
	*glDD4 = This;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw4_QueryInterface(glDirectDraw4 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirectDraw4_AddRef(This);
		*ppvObj = This;
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23, glDirectDraw7_QueryInterface(This->glDD7,riid,ppvObj));
}
ULONG WINAPI glDirectDraw4_AddRef(glDirectDraw4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDraw7_AddRef4(This->glDD7));
}
ULONG WINAPI glDirectDraw4_Release(glDirectDraw4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDraw7_Release4(This->glDD7));
}
HRESULT WINAPI glDirectDraw4_Compact(glDirectDraw4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_Compact(This->glDD7));
}
HRESULT WINAPI glDirectDraw4_CreateClipper(glDirectDraw4 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lplpDDClipper,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDraw7_CreateClipper2(This->glDD7, dwFlags, lplpDDClipper, pUnkOuter);
	if (ret == DD_OK)
	{
		glDirectDraw4_AddRef(This);
		((glDirectDrawClipper*)*lplpDDClipper)->creator = (IUnknown*)This;
	}
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw4_CreatePalette(glDirectDraw4 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(5, 14, This, 9, dwFlags, 14, lpDDColorArray, 14, lplpDDPalette, 14, pUnkOuter);
	if (!IsReadablePointer(This, sizeof(glDirectDraw4))) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDraw7_CreatePalette2(This->glDD7, dwFlags, lpDDColorArray, lplpDDPalette, pUnkOuter);
	if (ret == DD_OK)
	{
		glDirectDraw4_AddRef(This);
		((glDirectDrawPalette*)*lplpDDPalette)->creator = (IUnknown*)This;
	}
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw4_CreateSurface(glDirectDraw4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE4 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4,14,This,14,lpDDSurfaceDesc,14,lplpDDSurface,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDDSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpDDSurfaceDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpDDSurfaceDesc->dwSize < sizeof(DDSURFACEDESC2)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 lpDDS7;
	HRESULT err = glDirectDraw7_CreateSurface2(This->glDD7,(LPDDSURFACEDESC2)lpDDSurfaceDesc,&lpDDS7,pUnkOuter,TRUE,4);
	if(err == DD_OK)
	{
		lpDDS7->QueryInterface(IID_IDirectDrawSurface4,(LPVOID*) lplpDDSurface);
		lpDDS7->Release();
		glDirectDraw4_AddRef(This);
		((dxglDirectDrawSurface7*)lpDDS7)->creator = (IUnknown*)This;
		TRACE_EXIT(23, DD_OK);
		return DD_OK;
	}
	else *lplpDDSurface = NULL;
	TRACE_EXIT(23,err);
	return err;
}
HRESULT WINAPI glDirectDraw4_DuplicateSurface(glDirectDraw4 *This, LPDIRECTDRAWSURFACE4 lpDDSurface, LPDIRECTDRAWSURFACE4 FAR *lplpDupDDSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSurface,14,lplpDupDDSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!lplpDupDDSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	HRESULT ret;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	lpDDSurface->GetSurfaceDesc(&ddsd);
	ret = glDirectDraw4_CreateSurface(This, &ddsd, lplpDupDDSurface, NULL);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ret = (*lplpDupDDSurface)->Blt(NULL, lpDDSurface, NULL, 0, NULL);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDraw4_EnumDisplayModes(glDirectDraw4 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
	TRACE_ENTER(5,14,This,9,dwFlags,14,lpDDSurfaceDesc,14,lpContext,14,lpEnumModesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,EnumDisplayModes2(dwFlags,lpDDSurfaceDesc,lpContext,lpEnumModesCallback));
}
HRESULT WINAPI glDirectDraw4_EnumSurfaces(glDirectDraw4 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback)
{
	TRACE_ENTER(5,14,This,9,dwFlags,14,lpDDSD,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSD)
		if (lpDDSD->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, glDirectDraw7_EnumSurfaces(This->glDD7, dwFlags, lpDDSD, context, EnumSurfacesCallback2));
}
HRESULT WINAPI glDirectDraw4_FlipToGDISurface(glDirectDraw4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_FlipToGDISurface(This->glDD7));
}
HRESULT WINAPI glDirectDraw4_GetCaps(glDirectDraw4 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	TRACE_ENTER(3,14,This,14,lpDDDriverCaps,14,lpDDHELCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetCaps(This->glDD7,lpDDDriverCaps,lpDDHELCaps));
}
HRESULT WINAPI glDirectDraw4_GetDisplayMode(glDirectDraw4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetDisplayMode(This->glDD7, lpDDSurfaceDesc2));
}
HRESULT WINAPI glDirectDraw4_GetFourCCCodes(glDirectDraw4 *This, LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	TRACE_ENTER(3,14,This,14,lpNumCodes,14,lpCodes);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetFourCCCodes(This->glDD7,lpNumCodes,lpCodes));
}
HRESULT WINAPI glDirectDraw4_GetGDISurface(glDirectDraw4 *This, LPDIRECTDRAWSURFACE4 FAR *lplpGDIDDSurface)
{
	TRACE_ENTER(2,14,This,14,lplpGDIDDSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDraw4_GetGDISurface: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw4_GetMonitorFrequency(glDirectDraw4 *This, LPDWORD lpdwFrequency)
{
	TRACE_ENTER(2,14,This,14,lpdwFrequency);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetMonitorFrequency(This->glDD7, lpdwFrequency));
}
HRESULT WINAPI glDirectDraw4_GetScanLine(glDirectDraw4 *This, LPDWORD lpdwScanLine)
{
	TRACE_ENTER(2,14,This,14,lpdwScanLine);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetScanLine(This->glDD7, lpdwScanLine));
}
HRESULT WINAPI glDirectDraw4_GetVerticalBlankStatus(glDirectDraw4 *This, LPBOOL lpbIsInVB)
{
	TRACE_ENTER(2,14,This,14,lpbIsInVB);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetVerticalBlankStatus(This->glDD7, lpbIsInVB));
}
HRESULT WINAPI glDirectDraw4_Initialize(glDirectDraw4 *This, GUID FAR *lpGUID)
{
	TRACE_ENTER(2,14,This,24,lpGUID);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_Initialize(This->glDD7, lpGUID));
}
HRESULT WINAPI glDirectDraw4_RestoreDisplayMode(glDirectDraw4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_RestoreDisplayMode(This->glDD7));
}
HRESULT WINAPI glDirectDraw4_SetCooperativeLevel(glDirectDraw4 *This, HWND hWnd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,13,hWnd,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_SetCooperativeLevel(This->glDD7,hWnd,dwFlags));
}
HRESULT WINAPI glDirectDraw4_SetDisplayMode(glDirectDraw4 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,dwWidth,8,dwHeight,8,dwBPP,8,dwRefreshRate,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_SetDisplayMode(This->glDD7,dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags));
}
HRESULT WINAPI glDirectDraw4_WaitForVerticalBlank(glDirectDraw4 *This, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_WaitForVerticalBlank(This->glDD7,dwFlags,hEvent));
}
HRESULT WINAPI glDirectDraw4_GetAvailableVidMem(glDirectDraw4 *This, LPDDSCAPS2 lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	TRACE_ENTER(4,14,This,14,lpDDSCaps,14,lpdwTotal,14,lpdwFree);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_GetAvailableVidMem(This->glDD7,lpDDSCaps,lpdwTotal,lpdwFree));
}
HRESULT WINAPI glDirectDraw4_GetSurfaceFromDC(glDirectDraw4 *This, HDC hdc, LPDIRECTDRAWSURFACE4 *lpDDS)
{
	TRACE_ENTER(3,14,This,13,hdc,14,lpDDS);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("IDirectDraw4_GetSurfaceFromDC: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw4_RestoreAllSurfaces(glDirectDraw4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_RestoreAllSurfaces(This->glDD7));
}
HRESULT WINAPI glDirectDraw4_TestCooperativeLevel(glDirectDraw4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDraw7_TestCooperativeLevel(This->glDD7));
}
HRESULT WINAPI glDirectDraw4_GetDeviceIdentifier(glDirectDraw4 *This, LPDDDEVICEIDENTIFIER lpdddi, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpdddi,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpdddi) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	This->glDD7->devid.guidDeviceIdentifier = device_template;
	memcpy(lpdddi,&This->glDD7->devid,sizeof(DDDEVICEIDENTIFIER));
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

}
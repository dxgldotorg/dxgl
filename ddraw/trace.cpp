// DXGL
// Copyright (C) 2013 William Feely

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

/* Data types:
-1 - C++ constructor/destructor
0 - void
1 - 8-bit signed
2 - 8-bit unsigned
3 - 8-bit hex
4 - 16 bit signed
5 - 16 bit unsigned
6 - 16 bit hex
7 - 32 bit signed
8 - 32 bit unsigned
9 - 32 bit hex
10 - pointer to 64 bit hex
11 - native signed
12 - native unsigned
13 - native hex
14 - generic pointer
15 - ASCII string
16 - Unicode string
17 - TCHAR string
18 - ASCII character
19 - pointer to 32 bit float
20 - pointer to 64 bit float
21 - c++ bool
22 - int BOOL
23 - HRESULT
24 - GUID pointer
25 - SIZE pointer
26 - RECT pointer
27 - D3DRENDERSTATETYPE
28 - D3DTEXTURESTAGESTATETYPE
*/

#ifdef _TRACE
static CRITICAL_SECTION trace_cs;
static bool trace_ready = false;
static bool trace_fail = false;
static HANDLE outfile = INVALID_HANDLE_VALUE;
unsigned int trace_depth = 0;
static void trace_decode_hresult(HRESULT hr)
{
	DWORD byteswritten;
	char str[64];
	switch(hr)
	{
	case DD_OK:
		strcpy(str,"DD_OK");
		break;
	case DD_FALSE:
		strcpy(str,"DD_FALSE");
		break;
	case DDERR_ALREADYINITIALIZED:
		strcpy(str,"DDERR_ALREADYINITIALIZED");
		break;
	case DDERR_CANNOTATTACHSURFACE:
		strcpy(str,"DDERR_CANNOTATTACHSURFACE");
		break;
	case DDERR_CANNOTDETACHSURFACE:
		strcpy(str,"DDERR_CANNOTDETACHSURFACE");
		break;
	case DDERR_CURRENTLYNOTAVAIL:
		strcpy(str,"DDERR_CURRENTLYNOTAVAIL");
		break;
	case DDERR_EXCEPTION:
		strcpy(str,"DDERR_EXCEPTION");
		break;
	case DDERR_GENERIC:
		strcpy(str,"DDERR_GENERIC");
		break;
	case DDERR_INCOMPATIBLEPRIMARY:
		strcpy(str,"DDERR_INCOMPATIBLEPRIMARY");
		break;
	case DDERR_INVALIDCAPS:
		strcpy(str,"DDERR_INVALIDCAPS");
		break;
	case DDERR_INVALIDCLIPLIST:
		strcpy(str,"DDERR_INVALIDCLIPLIST");
		break;
	case DDERR_INVALIDMODE:
		strcpy(str,"DDERR_INVALIDMODE");
		break;
	case DDERR_INVALIDPARAMS:
		strcpy(str,"DDERR_INVALIDPARAMS");
		break;
	case DDERR_INVALIDPIXELFORMAT:
		strcpy(str,"DDERR_INVALIDPIXELFORMAT");
		break;
	case DDERR_INVALIDRECT:
		strcpy(str,"DDERR_INVALIDRECT");
		break;
	case DDERR_NOTFOUND:
		strcpy(str,"DDERR_NOTFOUND");
		break;
	case DDERR_OUTOFMEMORY:
		strcpy(str,"DDERR_OUTOFMEMORY");
		break;
	case DDERR_OUTOFVIDEOMEMORY:
		strcpy(str,"DDERR_OUTOFVIDEOMEMORY");
		break;
	case DDERR_SURFACEALREADYATTACHED:
		strcpy(str,"DDERR_SURFACEALREADYATTACHED");
		break;
	case DDERR_SURFACEBUSY:
		strcpy(str,"DDERR_SURFACEBUSY");
		break;
	case DDERR_CANTLOCKSURFACE:
		strcpy(str,"DDERR_CANTLOCKSURFACE");
		break;
	case DDERR_SURFACELOST:
		strcpy(str,"DDERR_SURFACELOST");
		break;
	case DDERR_SURFACENOTATTACHED:
		strcpy(str,"DDERR_SURFACENOTATTACHED");
		break;
	case DDERR_UNSUPPORTED:
		strcpy(str,"DDERR_UNSUPPORTED");
		break;
	case DDERR_UNSUPPORTEDFORMAT:
		strcpy(str,"DDERR_UNSUPPORTEDFORMAT");
		break;
	case DDERR_UNSUPPORTEDMASK:
		strcpy(str,"DDERR_UNSUPPORTEDMASK");
		break;
	case DDERR_WASSTILLDRAWING:
		strcpy(str,"DDERR_WASSTILLDRAWING");
		break;
	case DDERR_INVALIDDIRECTDRAWGUID:
		strcpy(str,"DDERR_INVALIDDIRECTDRAWGUID");
		break;
	case DDERR_DIRECTDRAWALREADYCREATED:
		strcpy(str,"DDERR_DIRECTDRAWALREADYCREATED");
		break;
	case DDERR_NODIRECTDRAWHW:
		strcpy(str,"DDERR_NODIRECTDRAWHW");
		break;
	case DDERR_PRIMARYSURFACEALREADYEXISTS:
		strcpy(str,"DDERR_PRIMARYSURFACEALREADYEXISTS");
		break;
	case DDERR_CLIPPERISUSINGHWND:
		strcpy(str,"DDERR_CLIPPERISUSINGHWND");
		break;
	case DDERR_NOCLIPPERATTACHED:
		strcpy(str,"DDERR_NOCLIPPERATTACHED");
		break;
	case DDERR_NOHWND:
		strcpy(str,"DDERR_NOHWND");
		break;
	case DDERR_HWNDSUBCLASSED:
		strcpy(str,"DDERR_HWNDSUBCLASSED");
		break;
	case DDERR_HWNDALREADYSET:
		strcpy(str,"DDERR_HWNDALREADYSET");
		break;
	case DDERR_NOPALETTEATTACHED:
		strcpy(str,"DDERR_NOPALETTEATTACHED");
		break;
	case DDERR_NOPALETTEHW:
		strcpy(str,"DDERR_NOPALETTEHW");
		break;
	case DDERR_BLTFASTCANTCLIP:
		strcpy(str,"DDERR_BLTFASTCANTCLIP");
		break;
	case DDERR_OVERLAYNOTVISIBLE:
		strcpy(str,"DDERR_OVERLAYNOTVISIBLE");
		break;
	case DDERR_NOOVERLAYDEST:
		strcpy(str,"DDERR_NOOVERLAYDEST");
		break;
	case DDERR_EXCLUSIVEMODEALREADYSET:
		strcpy(str,"DDERR_EXCLUSIVEMODEALREADYSET");
		break;
	case DDERR_NOTFLIPPABLE:
		strcpy(str,"DDERR_NOTFLIPPABLE");
		break;
	case DDERR_CANTDUPLICATE:
		strcpy(str,"DDERR_CANTDUPLICATE");
		break;
	case DDERR_NOTLOCKED:
		strcpy(str,"DDERR_NOTLOCKED");
		break;
	case DDERR_CANTCREATEDC:
		strcpy(str,"DDERR_CANTCREATEDC");
		break;
	case DDERR_NODC:
		strcpy(str,"DDERR_NODC");
		break;
	case DDERR_WRONGMODE:
		strcpy(str,"DDERR_WRONGMODE");
		break;
	case DDERR_IMPLICITLYCREATED:
		strcpy(str,"DDERR_IMPLICITLYCREATED");
		break;
	case DDERR_NOTPALETTIZED:
		strcpy(str,"DDERR_NOTPALETTIZED");
		break;
	case DDERR_UNSUPPORTEDMODE:
		strcpy(str,"DDERR_UNSUPPORTEDMODE");
		break;
	case DDERR_INVALIDSURFACETYPE:
		strcpy(str,"DDERR_INVALIDSURFACETYPE");
		break;
	case DDERR_NOTONMIPMAPSUBLEVEL:
		strcpy(str,"DDERR_NOTONMIPMAPSUBLEVEL");
		break;
	case DDERR_DCALREADYCREATED:
		strcpy(str,"DDERR_DCALREADYCREATED");
		break;
	case DDERR_CANTPAGELOCK:
		strcpy(str,"DDERR_CANTPAGELOCK");
		break;
	case DDERR_CANTPAGEUNLOCK:
		strcpy(str,"DDERR_CANTPAGEUNLOCK");
		break;
	case DDERR_NOTPAGELOCKED:
		strcpy(str,"DDERR_NOTPAGELOCKED");
		break;
	case DDERR_MOREDATA:
		strcpy(str,"DDERR_MOREDATA");
		break;
	case DDERR_NOTINITIALIZED:
		strcpy(str,"DDERR_NOTINITIALIZED");
		break;
	case E_NOINTERFACE:
		strcpy(str,"E_NOINTERFACE");
		break;
	case CLASS_E_NOAGGREGATION:
		strcpy(str,"CLASS_E_NOAGGREGATION");
		break;
	default:
		sprintf(str,"(HRESULT)0x%08X",hr);
		break;
	}
	WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
}

static void trace_decode_d3drenderstate(DWORD rs)
{
	DWORD byteswritten;
	char str[64];
	switch(rs)
	{
	case D3DRENDERSTATE_TEXTUREHANDLE:
		strcpy(str,"D3DRENDERSTATE_TEXTUREHANDLE");
		break;
	case D3DRENDERSTATE_ANTIALIAS:
		strcpy(str,"D3DRENDERSTATE_ANTIALIAS");
		break;
	case D3DRENDERSTATE_TEXTUREADDRESS:
		strcpy(str,"D3DRENDERSTATE_TEXTUREADDRESS");
		break;
	case D3DRENDERSTATE_TEXTUREPERSPECTIVE:
		strcpy(str,"D3DRENDERSTATE_TEXTUREPERSPECTIVE");
		break;
	case D3DRENDERSTATE_WRAPU:
		strcpy(str,"D3DRENDERSTATE_WRAPU");
		break;
	case D3DRENDERSTATE_WRAPV:
		strcpy(str,"D3DRENDERSTATE_WRAPV");
		break;
	case D3DRENDERSTATE_ZENABLE:
		strcpy(str,"D3DRENDERSTATE_ZENABLE");
		break;
	case D3DRENDERSTATE_FILLMODE:
		strcpy(str,"D3DRENDERSTATE_FILLMODE");
		break;
	case D3DRENDERSTATE_SHADEMODE:
		strcpy(str,"D3DRENDERSTATE_SHADEMODE");
		break;
	case D3DRENDERSTATE_LINEPATTERN:
		strcpy(str,"D3DRENDERSTATE_LINEPATTERN");
		break;
	case D3DRENDERSTATE_MONOENABLE:
		strcpy(str,"D3DRENDERSTATE_MONOENABLE");
		break;
	case D3DRENDERSTATE_ROP2:
		strcpy(str,"D3DRENDERSTATE_ROP2");
		break;
	case D3DRENDERSTATE_PLANEMASK:
		strcpy(str,"D3DRENDERSTATE_PLANEMASK");
		break;
	case D3DRENDERSTATE_ZWRITEENABLE:
		strcpy(str,"D3DRENDERSTATE_ZWRITEENABLE");
		break;
	case D3DRENDERSTATE_ALPHATESTENABLE:
		strcpy(str,"D3DRENDERSTATE_ALPHATESTENABLE");
		break;
	case D3DRENDERSTATE_LASTPIXEL:
		strcpy(str,"D3DRENDERSTATE_LASTPIXEL");
		break;
	case D3DRENDERSTATE_TEXTUREMAG:
		strcpy(str,"D3DRENDERSTATE_TEXTUREMAG");
		break;
	case D3DRENDERSTATE_TEXTUREMIN:
		strcpy(str,"D3DRENDERSTATE_TEXTUREMIN");
		break;
	case D3DRENDERSTATE_SRCBLEND:
		strcpy(str,"D3DRENDERSTATE_SRCBLEND");
		break;
	case D3DRENDERSTATE_DESTBLEND:
		strcpy(str,"D3DRENDERSTATE_DESTBLEND");
		break;
	case D3DRENDERSTATE_TEXTUREMAPBLEND:
		strcpy(str,"D3DRENDERSTATE_TEXTUREMAPBLEND");
		break;
	case D3DRENDERSTATE_CULLMODE:
		strcpy(str,"D3DRENDERSTATE_CULLMODE");
		break;
	case D3DRENDERSTATE_ZFUNC:
		strcpy(str,"D3DRENDERSTATE_ZFUNC");
		break;
	case D3DRENDERSTATE_ALPHAREF:
		strcpy(str,"D3DRENDERSTATE_ALPHAREF");
		break;
	case D3DRENDERSTATE_ALPHAFUNC:
		strcpy(str,"D3DRENDERSTATE_ALPHAFUNC");
		break;
	case D3DRENDERSTATE_DITHERENABLE:
		strcpy(str,"D3DRENDERSTATE_DITHERENABLE");
		break;
	case D3DRENDERSTATE_ALPHABLENDENABLE:
		strcpy(str,"D3DRENDERSTATE_ALPHABLENDENABLE");
		break;
	case D3DRENDERSTATE_FOGENABLE:
		strcpy(str,"D3DRENDERSTATE_FOGENABLE");
		break;
	case D3DRENDERSTATE_SPECULARENABLE:
		strcpy(str,"D3DRENDERSTATE_SPECULARENABLE");
		break;
	case D3DRENDERSTATE_ZVISIBLE:
		strcpy(str,"D3DRENDERSTATE_ZVISIBLE");
		break;
	case D3DRENDERSTATE_SUBPIXEL:
		strcpy(str,"D3DRENDERSTATE_SUBPIXEL");
		break;
	case D3DRENDERSTATE_SUBPIXELX:
		strcpy(str,"D3DRENDERSTATE_SUBPIXELX");
		break;
	case D3DRENDERSTATE_STIPPLEDALPHA:
		strcpy(str,"D3DRENDERSTATE_STIPPLEDALPHA");
		break;
	case D3DRENDERSTATE_FOGCOLOR:
		strcpy(str,"D3DRENDERSTATE_FOGCOLOR");
		break;
	case D3DRENDERSTATE_FOGTABLEMODE:
		strcpy(str,"D3DRENDERSTATE_FOGTABLEMODE");
		break;
	case D3DRENDERSTATE_FOGSTART:
		strcpy(str,"D3DRENDERSTATE_FOGSTART");
		break;
	case D3DRENDERSTATE_FOGEND:
		strcpy(str,"D3DRENDERSTATE_FOGEND");
		break;
	case D3DRENDERSTATE_FOGDENSITY:
		strcpy(str,"D3DRENDERSTATE_FOGDENSITY");
		break;
	case D3DRENDERSTATE_STIPPLEENABLE:
		strcpy(str,"D3DRENDERSTATE_STIPPLEENABLE");
		break;
	case D3DRENDERSTATE_EDGEANTIALIAS:
		strcpy(str,"D3DRENDERSTATE_EDGEANTIALIAS");
		break;
	case D3DRENDERSTATE_COLORKEYENABLE:
		strcpy(str,"D3DRENDERSTATE_COLORKEYENABLE");
		break;
	case 42: // DX5 D3DRENDERSTATE_ALPHABLENDENABLE
		strcpy(str,"D3DRENDERSTATE_ALPHABLENDENABLE(DX5)");
		break;
	case D3DRENDERSTATE_BORDERCOLOR:
		strcpy(str,"D3DRENDERSTATE_BORDERCOLOR");
		break;
	case D3DRENDERSTATE_TEXTUREADDRESSU:
		strcpy(str,"D3DRENDERSTATE_TEXTUREADDRESSU");
		break;
	case D3DRENDERSTATE_TEXTUREADDRESSV:
		strcpy(str,"D3DRENDERSTATE_TEXTUREADDRESSV");
		break;
	case D3DRENDERSTATE_MIPMAPLODBIAS:
		strcpy(str,"D3DRENDERSTATE_MIPMAPLODBIAS");
		break;
	case D3DRENDERSTATE_ZBIAS:
		strcpy(str,"D3DRENDERSTATE_ZBIAS");
		break;
	case D3DRENDERSTATE_RANGEFOGENABLE:
		strcpy(str,"D3DRENDERSTATE_RANGEFOGENABLE");
		break;
	case D3DRENDERSTATE_ANISOTROPY:
		strcpy(str,"D3DRENDERSTATE_ANISOTROPY");
		break;
	case D3DRENDERSTATE_FLUSHBATCH:
		strcpy(str,"D3DRENDERSTATE_FLUSHBATCH");
		break;
	case D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT:
		strcpy(str,"D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT");
		break;
	case D3DRENDERSTATE_STENCILENABLE:
		strcpy(str,"D3DRENDERSTATE_STENCILENABLE");
		break;
	case D3DRENDERSTATE_STENCILFAIL:
		strcpy(str,"D3DRENDERSTATE_STENCILFAIL");
		break;
	case D3DRENDERSTATE_STENCILZFAIL:
		strcpy(str,"D3DRENDERSTATE_STENCILZFAIL");
		break;
	case D3DRENDERSTATE_STENCILPASS:
		strcpy(str,"D3DRENDERSTATE_STENCILPASS");
		break;
	case D3DRENDERSTATE_STENCILFUNC:
		strcpy(str,"D3DRENDERSTATE_STENCILFUNC");
		break;
	case D3DRENDERSTATE_STENCILREF:
		strcpy(str,"D3DRENDERSTATE_STENCILREF");
		break;
	case D3DRENDERSTATE_STENCILMASK:
		strcpy(str,"D3DRENDERSTATE_STENCILMASK");
		break;
	case D3DRENDERSTATE_STENCILWRITEMASK:
		strcpy(str,"D3DRENDERSTATE_STENCILWRITEMASK");
		break;
	case D3DRENDERSTATE_TEXTUREFACTOR:
		strcpy(str,"D3DRENDERSTATE_TEXTUREFACTOR");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN00:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN00");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN01:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN01");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN02:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN02");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN03:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN03");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN04:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN04");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN05:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN05");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN06:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN06");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN07:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN07");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN08:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN08");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN09:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN09");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN10:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN10");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN11:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN11");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN12:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN12");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN13:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN13");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN14:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN14");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN15:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN15");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN16:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN16");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN17:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN17");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN18:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN18");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN19:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN19");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN20:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN20");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN21:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN21");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN22:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN22");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN23:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN23");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN24:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN24");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN25:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN25");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN26:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN26");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN27:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN27");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN28:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN28");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN29:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN29");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN30:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN30");
		break;
	case D3DRENDERSTATE_STIPPLEPATTERN31:
		strcpy(str,"D3DRENDERSTATE_STIPPLEPATTERN31");
		break;
	case D3DRENDERSTATE_WRAP0:
		strcpy(str,"D3DRENDERSTATE_WRAP0");
		break;
	case D3DRENDERSTATE_WRAP1:
		strcpy(str,"D3DRENDERSTATE_WRAP1");
		break;
	case D3DRENDERSTATE_WRAP2:
		strcpy(str,"D3DRENDERSTATE_WRAP2");
		break;
	case D3DRENDERSTATE_WRAP3:
		strcpy(str,"D3DRENDERSTATE_WRAP3");
		break;
	case D3DRENDERSTATE_WRAP4:
		strcpy(str,"D3DRENDERSTATE_WRAP4");
		break;
	case D3DRENDERSTATE_WRAP5:
		strcpy(str,"D3DRENDERSTATE_WRAP5");
		break;
	case D3DRENDERSTATE_WRAP6:
		strcpy(str,"D3DRENDERSTATE_WRAP6");
		break;
	case D3DRENDERSTATE_WRAP7:
		strcpy(str,"D3DRENDERSTATE_WRAP7");
		break;
	case D3DRENDERSTATE_CLIPPING:
		strcpy(str,"D3DRENDERSTATE_CLIPPING");
		break;
	case D3DRENDERSTATE_LIGHTING:
		strcpy(str,"D3DRENDERSTATE_LIGHTING");
		break;
	case D3DRENDERSTATE_EXTENTS:
		strcpy(str,"D3DRENDERSTATE_EXTENTS");
		break;
	case D3DRENDERSTATE_AMBIENT:
		strcpy(str,"D3DRENDERSTATE_AMBIENT");
		break;
	case D3DRENDERSTATE_FOGVERTEXMODE:
		strcpy(str,"D3DRENDERSTATE_FOGVERTEXMODE");
		break;
	case D3DRENDERSTATE_COLORVERTEX:
		strcpy(str,"D3DRENDERSTATE_COLORVERTEX");
		break;
	case D3DRENDERSTATE_LOCALVIEWER:
		strcpy(str,"D3DRENDERSTATE_LOCALVIEWER");
		break;
	case D3DRENDERSTATE_NORMALIZENORMALS:
		strcpy(str,"D3DRENDERSTATE_NORMALIZENORMALS");
		break;
	case D3DRENDERSTATE_COLORKEYBLENDENABLE:
		strcpy(str,"D3DRENDERSTATE_COLORKEYBLENDENABLE");
		break;
	case D3DRENDERSTATE_DIFFUSEMATERIALSOURCE:
		strcpy(str,"D3DRENDERSTATE_DIFFUSEMATERIALSOURCE");
		break;
	case D3DRENDERSTATE_SPECULARMATERIALSOURCE:
		strcpy(str,"D3DRENDERSTATE_SPECULARMATERIALSOURCE");
		break;
	case D3DRENDERSTATE_AMBIENTMATERIALSOURCE:
		strcpy(str,"D3DRENDERSTATE_AMBIENTMATERIALSOURCE");
		break;
	case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE:
		strcpy(str,"D3DRENDERSTATE_EMISSIVEMATERIALSOURCE");
		break;
	case D3DRENDERSTATE_VERTEXBLEND:
		strcpy(str,"D3DRENDERSTATE_VERTEXBLEND");
		break;
	case D3DRENDERSTATE_CLIPPLANEENABLE:
		strcpy(str,"D3DRENDERSTATE_CLIPPLANEENABLE");
		break;
	default:
		sprintf(str,"(D3DRENDERSTATETYPE)%u",rs);
		break;
	}
	WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
}

static void trace_decode_d3dtexturestagestate(DWORD rs)
{
	DWORD byteswritten;
	char str[64];
	switch(rs)
	{
	case D3DTSS_COLOROP:
		strcpy(str,"D3DTSS_COLOROP");
		break;
	case D3DTSS_COLORARG1:
		strcpy(str,"D3DTSS_COLORARG1");
		break;
	case D3DTSS_COLORARG2:
		strcpy(str,"D3DTSS_COLORARG2");
		break;
	case D3DTSS_ALPHAOP:
		strcpy(str,"D3DTSS_ALPHAOP");
		break;
	case D3DTSS_ALPHAARG1:
		strcpy(str,"D3DTSS_ALPHAARG1");
		break;
	case D3DTSS_ALPHAARG2:
		strcpy(str,"D3DTSS_ALPHAARG2");
		break;
	case D3DTSS_BUMPENVMAT00:
		strcpy(str,"D3DTSS_BUMPENVMAT00");
		break;
	case D3DTSS_BUMPENVMAT01:
		strcpy(str,"D3DTSS_BUMPENVMAT01");
		break;
	case D3DTSS_BUMPENVMAT10:
		strcpy(str,"D3DTSS_BUMPENVMAT10");
		break;
	case D3DTSS_BUMPENVMAT11:
		strcpy(str,"D3DTSS_BUMPENVMAT11");
		break;
	case D3DTSS_TEXCOORDINDEX:
		strcpy(str,"D3DTSS_TEXCOORDINDEX");
		break;
	case D3DTSS_ADDRESS:
		strcpy(str,"D3DTSS_ADDRESS");
		break;
	case D3DTSS_ADDRESSU:
		strcpy(str,"D3DTSS_ADDRESSU");
		break;
	case D3DTSS_ADDRESSV:
		strcpy(str,"D3DTSS_ADDRESSV");
		break;
	case D3DTSS_BORDERCOLOR:
		strcpy(str,"D3DTSS_BORDERCOLOR");
		break;
	case D3DTSS_MAGFILTER:
		strcpy(str,"D3DTSS_MAGFILTER");
		break;
	case D3DTSS_MINFILTER:
		strcpy(str,"D3DTSS_MINFILTER");
		break;
	case D3DTSS_MIPFILTER:
		strcpy(str,"D3DTSS_MIPFILTER");
		break;
	case D3DTSS_MIPMAPLODBIAS:
		strcpy(str,"D3DTSS_MIPMAPLODBIAS");
		break;
	case D3DTSS_MAXMIPLEVEL:
		strcpy(str,"D3DTSS_MAXMIPLEVEL");
		break;
	case D3DTSS_MAXANISOTROPY:
		strcpy(str,"D3DTSS_MAXANISOTROPY");
		break;
	case D3DTSS_BUMPENVLSCALE:
		strcpy(str,"D3DTSS_BUMPENVLSCALE");
		break;
	case D3DTSS_BUMPENVLOFFSET:
		strcpy(str,"D3DTSS_BUMPENVLOFFSET");
		break;
	case D3DTSS_TEXTURETRANSFORMFLAGS:
		strcpy(str,"D3DTSS_TEXTURETRANSFORMFLAGS");
		break;
	default:
		sprintf(str,"(D3DTEXTURESTAGESTATETYPE)%u",rs);
		break;
	}
	WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
}

static void trace_decode_guid(GUID *guid)
{
	DWORD byteswritten;
	char str[64];
	if(*guid == CLSID_DirectDraw) strcpy(str,"CLSID_DirectDraw");
	else if(*guid == CLSID_DirectDraw7) strcpy(str,"CLSID_DirectDraw7");
	else if(*guid == CLSID_DirectDrawClipper) strcpy(str,"CLSID_DirectDrawClipper");
	else if(*guid == IID_IDirectDraw) strcpy(str,"IID_IDirectDraw");
	else if(*guid == IID_IDirectDraw2) strcpy(str,"IID_IDirectDraw2");
	else if(*guid == IID_IDirectDraw4) strcpy(str,"IID_IDirectDraw4");
	else if(*guid == IID_IDirectDraw7) strcpy(str,"IID_IDirectDraw7");
	else if(*guid == IID_IDirectDrawSurface) strcpy(str,"IID_IDirectDrawSurface");
	else if(*guid == IID_IDirectDrawSurface2) strcpy(str,"IID_IDirectDrawSurface2");
	else if(*guid == IID_IDirectDrawSurface3) strcpy(str,"IID_IDirectDrawSurface3");
	else if(*guid == IID_IDirectDrawSurface4) strcpy(str,"IID_IDirectDrawSurface4");
	else if(*guid == IID_IDirectDrawSurface7) strcpy(str,"IID_IDirectDrawSurface7");
	else if(*guid == IID_IDirectDrawPalette) strcpy(str,"IID_IDirectDrawPalette");
	else if(*guid == IID_IDirectDrawClipper) strcpy(str,"IID_IDirectDrawClipper");
	else if(*guid == IID_IDirectDrawColorControl) strcpy(str,"IID_IDirectDrawColorControl");
	else if(*guid == IID_IDirectDrawGammaControl) strcpy(str,"IID_IDirectDrawGammaControl");
	else if(*guid == IID_IDirect3D) strcpy(str,"IID_IDirect3D");
	else if(*guid == IID_IDirect3D2) strcpy(str,"IID_IDirect3D2");
	else if(*guid == IID_IDirect3D3) strcpy(str,"IID_IDirect3D3");
	else if(*guid == IID_IDirect3D7) strcpy(str,"IID_IDirect3D7");
	else
	{
		OLECHAR guidstr[41] = {0}; 
		StringFromGUID2(*guid,guidstr,40);
		WideCharToMultiByte(CP_UTF8,0,guidstr,-1,str,64,NULL,NULL);
	}
	WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
}
static void trace_decode_size(SIZE *size)
{
	DWORD byteswritten;
	char str[64];
	sprintf(str,"{%d,%d}",size->cx,size->cy);
	WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
}
static void trace_decode_rect(RECT *rect)
{
	DWORD byteswritten;
	char str[64];
	sprintf(str,"{%d,%d,%d,%d}",rect->left,rect->top,rect->right,rect->bottom);
	WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
}
static void init_trace()
{
	TCHAR path[MAX_PATH+1];
	InitializeCriticalSection(&trace_cs);
	GetModuleFileName(NULL,path,MAX_PATH);
	TCHAR *path_truncate = _tcsrchr(path,_T('\\'));
	if(path_truncate) *(path_truncate+1) = 0;
	_tcscat(path,_T("dxgl.log"));
	outfile = CreateFile(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(outfile == INVALID_HANDLE_VALUE)
	{
		trace_fail = true;
		return;
	}
	trace_ready = true;
}
static void trace_decode_arg(int type, void *arg)
{
	DWORD byteswritten;
	char str[128];
	char *mbcsbuffer;
	int buffersize;
	str[0] = 0;
	switch(type)
	{
	case -1: // C++ constructor/destructor
		// No return type in a constructor or destructor.
		break;
	case 0: // void
		WriteFile(outfile,"void",4,&byteswritten,NULL);
		break;
	case 1: // 8-bit signed
		sprintf(str,"%d",(signed char)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 2: // 8-bit unsigned
		sprintf(str,"%u",(unsigned char)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 3: // 8-bit hex
		sprintf(str,"0x%02X",(unsigned char)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 4: // 16-bit signed
		sprintf(str,"%d",(signed short)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 5: // 16-bit unsigned
		sprintf(str,"%u",(unsigned short)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 6: // 16-bit hex
		sprintf(str,"0x%04X",(unsigned short)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 7: // 32-bit signed
		sprintf(str,"%d",(signed long)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 8: // 32-bit unsigned
		sprintf(str,"%u",(unsigned long)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 9: // 32-bit hex
		sprintf(str,"0x%08X",(unsigned long)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 10: // pointer to 64-bit hex
		sprintf(str,"0x%016I64X",(unsigned __int64)*(unsigned __int64*)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 11: // native signed
		sprintf(str,"%d",(signed int)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 12: // native unsigned
		sprintf(str,"%u",(unsigned int)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 13: // native hex
		sprintf(str,"0x%08X",(unsigned int)arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 14: // generic pointer
		if(!arg) WriteFile(outfile,"NULL",4,&byteswritten,NULL);
		else
		{
#ifdef _M_X64
			sprintf(str,"0x%016I64X",arg);
			WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
#else
			sprintf(str,"0x%08X",arg);
			WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
#endif
		}
		break;
	case 15: // ASCII string
		if(!arg) WriteFile(outfile,"NULL",4,&byteswritten,NULL);
		else
		{
			WriteFile(outfile,"\"",1,&byteswritten,NULL);
			WriteFile(outfile,arg,strlen((char*)arg),&byteswritten,NULL);
			WriteFile(outfile,"\"",1,&byteswritten,NULL);
		}
		break;
	case 16: // Unicode string
		if(!arg) WriteFile(outfile,"NULL",4,&byteswritten,NULL);
		else
		{
			WriteFile(outfile,"L\"",1,&byteswritten,NULL);
			buffersize = WideCharToMultiByte(CP_UTF8,0,(wchar_t*)arg,-1,NULL,0,NULL,NULL);
			mbcsbuffer = (char*)malloc(buffersize);
			if(!mbcsbuffer) WriteFile(outfile,"OUT OF MEMORY",13,&byteswritten,NULL);
			else
			{
				WideCharToMultiByte(CP_UTF8,0,(wchar_t*)arg,-1,mbcsbuffer,buffersize,NULL,NULL);
				WriteFile(outfile,mbcsbuffer,strlen(mbcsbuffer),&byteswritten,NULL);
				free(mbcsbuffer);
			}
			WriteFile(outfile,"\"",1,&byteswritten,NULL);
		}
		break;
	case 17: // TCHAR string
		if(!arg) WriteFile(outfile,"NULL",4,&byteswritten,NULL);
#ifdef _UNICODE
		else
		{
			WriteFile(outfile,"_T(\"",1,&byteswritten,NULL);
			buffersize = WideCharToMultiByte(CP_UTF8,0,(wchar_t*)arg,-1,NULL,0,NULL,NULL);
			mbcsbuffer = (char*)malloc(buffersize);
			if(!mbcsbuffer) WriteFile(outfile,"OUT OF MEMORY",13,&byteswritten,NULL);
			else
			{
				WideCharToMultiByte(CP_UTF8,0,(wchar_t*)arg,-1,mbcsbuffer,buffersize,NULL,NULL);
				WriteFile(outfile,mbcsbuffer,strlen(mbcsbuffer),&byteswritten,NULL);
				free(mbcsbuffer);
			}
			WriteFile(outfile,"\")",1,&byteswritten,NULL);
		}
#else
		else
		{
			WriteFile(outfile,"_T(\"",1,&byteswritten,NULL);
			WriteFile(outfile,arg,strlen((char*)arg),&byteswritten,NULL);
			WriteFile(outfile,"\")",1,&byteswritten,NULL);
		}
#endif
		break;
	case 18: // ASCII character
		if(!(unsigned char)arg) WriteFile(outfile,"\'\\0\'",4,&byteswritten,NULL);
		else
		{
			str[0] = str[2] = '\'';
			str[1] = (unsigned char)arg;
			str[3] = NULL;
			WriteFile(outfile,str,3,&byteswritten,NULL);
		}
		break;
	case 19: // pointer to 32 bit float
		sprintf(str,"%f",arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 20: // pointer to 64 bit float
		sprintf(str,"%lf",arg);
		WriteFile(outfile,str,strlen(str),&byteswritten,NULL);
		break;
	case 21: // c++ bool
		if((bool)arg) WriteFile(outfile,"true",4,&byteswritten,NULL);
		else WriteFile(outfile,"false",5,&byteswritten,NULL);
		break;
	case 22: // c++ bool
		if(arg) WriteFile(outfile,"TRUE",4,&byteswritten,NULL);
		else WriteFile(outfile,"FALSE",5,&byteswritten,NULL);
		break;
	case 23: // HRESULT
		trace_decode_hresult((HRESULT)arg);
		break;
	case 24: // GUID pointer
		if(!arg) WriteFile(outfile,"NULL",4,&byteswritten,NULL);
		else if(arg == (void*)DDCREATE_HARDWAREONLY) WriteFile(outfile,"DDCREATE_HARDWAREONLY",21,&byteswritten,NULL);
		else if(arg == (void*)DDCREATE_EMULATIONONLY) WriteFile(outfile,"DDCREATE_EMULATIONONLY",22,&byteswritten,NULL);
		else trace_decode_guid((GUID*)arg);
		break;
	case 25: // SIZE pointer
		if(!arg) WriteFile(outfile,"NULL",4,&byteswritten,NULL);
		else trace_decode_size((SIZE*)arg);
		break;
	case 26: // RECT pointer
		if(!arg) WriteFile(outfile,"NULL",4,&byteswritten,NULL);
		else trace_decode_rect((RECT*)arg);
		break;
	case 27: // D3DRENDERSTATETYPE
		trace_decode_d3drenderstate((DWORD)arg);
		break;
	case 28: // D3DTEXTURESTAGESTATETYPE
		trace_decode_d3dtexturestagestate((DWORD)arg);
		break;
	default:
		WriteFile(outfile,"Unknown type",12,&byteswritten,NULL);
		break;
	}
}
void trace_enter(const char *function, int paramcount, ...)
{
	if(trace_fail) return;
	if(!trace_ready) init_trace();
	EnterCriticalSection(&trace_cs);
	va_list args;
	va_start(args,paramcount);
	DWORD byteswritten;
	for(unsigned int i = 0; i < trace_depth; i++)
		WriteFile(outfile,"    ",4,&byteswritten,NULL);
	WriteFile(outfile,function,strlen(function),&byteswritten,NULL);
	WriteFile(outfile,"(",1,&byteswritten,NULL);
	for(int i = 0; i < paramcount; i++)
	{
		if(i != 0) WriteFile(outfile,", ",2,&byteswritten,NULL);
		int argtype = va_arg(args,int);
		trace_decode_arg(argtype,va_arg(args,void*));
	}
	WriteFile(outfile,");\r\n",4,&byteswritten,NULL);
	trace_depth++;
	LeaveCriticalSection(&trace_cs);
}
void trace_exit(const char *function, int argtype, void *arg)
{
	if(trace_fail) return;
	if(!trace_ready) init_trace();
	EnterCriticalSection(&trace_cs);
	trace_depth--;
	DWORD byteswritten;
	for(unsigned int i = 0; i < trace_depth; i++)
		WriteFile(outfile,"    ",4,&byteswritten,NULL);
	WriteFile(outfile,function,strlen(function),&byteswritten,NULL);
	WriteFile(outfile," returned ",10,&byteswritten,NULL);
	trace_decode_arg(argtype,arg);
	WriteFile(outfile,"\r\n",2,&byteswritten,NULL);
	LeaveCriticalSection(&trace_cs);
}
void trace_var(const char *function, const char *var, int argtype, void *arg)
{
	if(trace_fail) return;
	if(!trace_ready) init_trace();
	EnterCriticalSection(&trace_cs);
	DWORD byteswritten;
	for(unsigned int i = 0; i < trace_depth-1; i++)
		WriteFile(outfile,"    ",4,&byteswritten,NULL);
	WriteFile(outfile,function,strlen(function),&byteswritten,NULL);
	WriteFile(outfile,": ",2,&byteswritten,NULL);
	WriteFile(outfile,var,strlen(var),&byteswritten,NULL);
	WriteFile(outfile," set to ",8,&byteswritten,NULL);
	trace_decode_arg(argtype,arg);
	WriteFile(outfile,"\r\n",2,&byteswritten,NULL);
	LeaveCriticalSection(&trace_cs);
}
#endif
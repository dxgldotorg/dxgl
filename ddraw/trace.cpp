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
	for(int i = 0; i < trace_depth; i++)
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
	for(int i = 0; i < trace_depth; i++)
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
	for(int i = 0; i < trace_depth-1; i++)
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
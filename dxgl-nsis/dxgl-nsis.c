// DXGL plugin for NSIS, used to verify runtime files in the DXGL installer,
// and to verify certain CPU features on certain builds of DXGL.
// This plugin links to LibSha512, from waterjuice.org, which is published as
// public domain.  In addition, the source code to this plugin is public
// domain.

// This file uses the NSIS API from the NSIS Plugin Example, which is licensed
// under the zlib/libpng license.


#include <Windows.h>
#include "LibSha512.h"
#include "pluginapi.h"
#include <intrin.h>
#ifndef _TCHAR_DEFINED
#include <tchar.h>
#endif

// Quick and dirty memcmp
int __memcmp(unsigned char *ptr1, unsigned char *ptr2, size_t size)
{
	size_t i;
	for (i = 0; i < size; i++)
	{
		if (ptr1[i] < ptr2[i]) return -1;
		if (ptr1[i] > ptr2[i]) return 1;
	}
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

unsigned char hexdigit(unsigned char c)
{
	if (c < 10) return c + '0';
	else return (c + 'A' - 10);
}

// API:  dxgl-nsis::CalculateSha512Sum file comp

void __declspec(dllexport) CalculateSha512Sum(HWND hwndParent, int string_size,
	TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	int i;
	TCHAR filename[1024];
	Sha512Context context;
	HANDLE file;
	char buffer[512];
	char comp[1024];
	DWORD bytesread;
	SHA512_HASH sha512;
	int compout;
	EXDLL_INIT();
	popstring(filename);
	popstring(comp);
	Sha512Initialise(&context);
	file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!file)
	{
		filename[0] = '0';
		filename[1] = 0;
		pushstring(comp);
		pushstring(filename);
		return;
	}
	while (1)
	{
		ReadFile(file, buffer, 512, &bytesread, NULL);
		if (!bytesread) break;
		Sha512Update(&context, buffer, bytesread);
		if (bytesread < 512) break;
	}
	Sha512Finalise(&context, &sha512);
	CloseHandle(file);
	for (i = 0; i < (512 / 8); i++)
	{
		buffer[i * 2] = hexdigit(sha512.bytes[i] >> 4);
		buffer[(i * 2) + 1] = hexdigit(sha512.bytes[i] & 0xF);
	}
	buffer[512 / 4] = 0;
	compout = __memcmp(buffer, comp, 128);
	if (compout)
	{
		filename[0] = '0';
	}
	else
	{
		filename[0] = '1';
	}
	filename[1] = filename[2] = filename[3] = 0;
	pushstring(comp);
	pushstring(filename);
}

// API:  dxgl-nsis::CheckSSE2

void __declspec(dllexport) CheckSSE2(HWND hwndParent, int string_size,
	TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	int cpuid[4];
	char out[256];
	EXDLL_INIT();
	__cpuid(cpuid, 1);
	if ((cpuid[3] >> 26) & 1) out[0] = '1';
	else out[0] = '0';
	out[1] = out[2] = out[3] = 0;
	pushstring(out);
}

void __declspec(dllexport) IsWine(HWND hwndParent, int string_size,
	TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	HMODULE ntdll;
	char* (__cdecl *wine_get_version)();
	char out[256];
	EXDLL_INIT();
	ntdll = LoadLibraryA("ntdll.dll");
	if (!ntdll)
	{
		out[0] = '0';
		out[1] = out[2] = out[3] = 0;
		pushstring(out);
		return;
	}
	wine_get_version = (char*(__cdecl*)())GetProcAddress(ntdll, "wine_get_version");
	if (wine_get_version) out[0] = '1';
	else out[0] = '0';
	FreeLibrary(ntdll);
	out[1] = out[2] = out[3] = 0;
	pushstring(out);
}

// ASM shift instructions to replace MSVC dependency
#ifndef _M_X64
void __declspec(naked) _aullshr()
{
	__asm
	{
		shrd eax, edx, cl
		shr edx, cl
		test ecx, 32
		jz done
		mov eax, edx
		xor edx, edx
		done:
	}
}

void __declspec(naked) _allshl()
{
	__asm
	{
		shld edx, eax, cl
		sal eax, cl
		test ecx, 32
		jz done2
		mov edx, eax
		xor eax, eax
		done2:
	}
}
#endif
// SHA-512 plugin for NSIS, used to verify runtime files in the DXGL installer.
// This plugin links to LibSha512, from waterjuice.org, which is published as
// public domain.  In addition, the source code to this plugin is public
// domain.

// This file uses the NSIS API from the NSIS Plugin Example, which is licensed
// under the zlib/libpng license.


#include <Windows.h>
#include "LibSha512.h"
#include "pluginapi.h"
#ifndef _TCHAR_DEFINED
#include <tchar.h>
#endif

BOOL WINAPI DllMain(HINSTANCE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

unsigned char hexdigit(unsigned char c)
{
	if (c < 10) return c + '0';
	else return (c + 'A' - 10);
}

// API:  sha512-nsis::CalculateSha512Sum file comp

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
	file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
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
	compout = memcmp(buffer, comp, 128);
	if (compout)
	{
		filename[0] = '0';
	}
	else
	{
		filename[0] = '1';
	}
	filename[1] = 0;
	pushstring(comp);
	pushstring(filename);
}


// Quick and dirty memcpy
void *memcpy(unsigned char *dest, unsigned char *src, size_t size)
{
	size_t i;
	for (i = 0; i < size; i++)
		dest[i] = src[i];
	return dest;
}

// Quick and dirty memcmp
int memcmp(unsigned char *ptr1, unsigned char *ptr2, size_t size)
{
	size_t i;
	for (i = 0; i < size; i++)
	{
		if (ptr1[i] < ptr2[i]) return -1;
		if (ptr1[i] > ptr2[i]) return 1;
	}
	return 0;
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
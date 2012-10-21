// DXGL
// Copyright (C) 2012 William Feely

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
#include <Windows.h>
#include <errno.h>

typedef struct
{
	HANDLE handle;
	unsigned long mode;
	DWORD DesiredAccess;
	DWORD ShareMode;
	DWORD CreationDisposition;
} minilibc_FILE;

minilibc_FILE *minilibc_files = NULL;
static int filecount = 0;
static int maxfiles = 0;

#define MODE_READ 1
#define MODE_WRITE 2
#define MODE_APPEND 4
#define MODE_PLUS 8
#define MODE_RWMASK 0xF
#define MODE_TEXT 0x10
#define MODE_BINARY 0x20
#define MODE_ERROR 0x80000000


static unsigned int decode_filemode(const char *mode, minilibc_FILE *file)
{
	unsigned int ret = 0;
	int i = 0;
	while(mode[i] != 0)
	{
		switch(mode[i])
		{
		case 'r':
			ret |= MODE_READ;
			break;
		case 'w':
			ret |= MODE_WRITE;
			break;
		case 'a':
			ret |= MODE_APPEND;
			break;
		case '+':
			ret |= MODE_PLUS;
			break;
		case 't':
			ret |= MODE_TEXT;
			break;
		case 'b':
			ret |= MODE_BINARY;
			break;
		}
		i++;
	}
	if(((ret & MODE_READ) && (ret & MODE_WRITE)) || ((ret & MODE_READ) && (ret & MODE_APPEND))
		|| ((ret & MODE_WRITE) && (ret & MODE_APPEND)) || ((ret & MODE_TEXT) && (ret & MODE_BINARY)))
		ret |= MODE_ERROR;
	switch(ret & MODE_RWMASK)
	{
	case 0:
	case MODE_PLUS:
		ret |= MODE_ERROR;
		break;
	case MODE_READ: 
		file->DesiredAccess = GENERIC_READ;
		file->ShareMode = FILE_SHARE_READ;
		file->CreationDisposition = OPEN_EXISTING;
		break;
	case MODE_WRITE:
		file->DesiredAccess = GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = CREATE_ALWAYS;
		break;
	case MODE_APPEND:
		file->DesiredAccess = GENERIC_READ|GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = OPEN_ALWAYS;
		break;
	case MODE_READ|MODE_PLUS:
		file->DesiredAccess = GENERIC_READ|GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = OPEN_EXISTING;
		break;
	case MODE_WRITE|MODE_PLUS:
		file->DesiredAccess = GENERIC_READ|GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = CREATE_ALWAYS;
		break;
	case MODE_APPEND|MODE_PLUS:
		file->DesiredAccess = GENERIC_READ|GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = OPEN_ALWAYS;
		break;
	}
	return ret;
}

FILE *fopen(const char *filename, const char *mode)
{
	int ptr = -1;
	int i;
	if(!filename) return NULL;
	if(!mode) return NULL;
	if(!minilibc_files)
	{
		minilibc_files = (minilibc_FILE*)malloc(128*sizeof(minilibc_FILE));
		if(!minilibc_files)
		{
			_set_errno(ENOMEM);
			return NULL;
		}
		memset(minilibc_files,0,128*sizeof(minilibc_FILE));
		maxfiles = 128;
	}
	for(i = 0; i < filecount; i++)
	{
		if(minilibc_files[i].handle == INVALID_HANDLE_VALUE)
		{
			ptr = i;
			break;
		}
	}
	if(ptr == -1) ptr = filecount++;
	if(filecount >= maxfiles)
	{
		minilibc_FILE *tmpptr = (minilibc_FILE*)realloc(minilibc_files,(128+maxfiles)*sizeof(minilibc_FILE));
		if(!tmpptr)
		{
			_set_errno(ENOMEM);
			return NULL;
		}
		maxfiles += 128;
		minilibc_files = tmpptr;
	}
	minilibc_files[ptr].mode = decode_filemode(mode,&minilibc_files[ptr]);
	if(minilibc_files[ptr].mode & MODE_ERROR)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	minilibc_files[ptr].handle = CreateFileA(filename,minilibc_files[ptr].DesiredAccess,
		minilibc_files[ptr].ShareMode,NULL,minilibc_files[ptr].CreationDisposition,
		FILE_ATTRIBUTE_NORMAL,NULL);
	if(minilibc_files[ptr].handle == INVALID_HANDLE_VALUE)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	return (FILE*)&minilibc_files[ptr];
}

static unsigned int _w_decode_filemode(const WCHAR *mode, minilibc_FILE *file)
{
	unsigned int ret = 0;
	int i = 0;
	while(mode[i] != 0)
	{
		switch(mode[i])
		{
		case L'r':
			ret |= MODE_READ;
			break;
		case L'w':
			ret |= MODE_WRITE;
			break;
		case L'a':
			ret |= MODE_APPEND;
			break;
		case L'+':
			ret |= MODE_PLUS;
			break;
		case L't':
			ret |= MODE_TEXT;
			break;
		case L'b':
			ret |= MODE_BINARY;
			break;
		}
		i++;
	}
	if(((ret & MODE_READ) && (ret & MODE_WRITE)) || ((ret & MODE_READ) && (ret & MODE_APPEND))
		|| ((ret & MODE_WRITE) && (ret & MODE_APPEND)) || ((ret & MODE_TEXT) && (ret & MODE_BINARY)))
		ret |= MODE_ERROR;
	switch(ret & MODE_RWMASK)
	{
	case 0:
	case MODE_PLUS:
		ret |= MODE_ERROR;
		break;
	case MODE_READ: 
		file->DesiredAccess = GENERIC_READ;
		file->ShareMode = FILE_SHARE_READ;
		file->CreationDisposition = OPEN_EXISTING;
		break;
	case MODE_WRITE:
		file->DesiredAccess = GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = CREATE_ALWAYS;
		break;
	case MODE_APPEND:
		file->DesiredAccess = GENERIC_READ|GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = OPEN_ALWAYS;
		break;
	case MODE_READ|MODE_PLUS:
		file->DesiredAccess = GENERIC_READ|GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = OPEN_EXISTING;
		break;
	case MODE_WRITE|MODE_PLUS:
		file->DesiredAccess = GENERIC_READ|GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = CREATE_ALWAYS;
		break;
	case MODE_APPEND|MODE_PLUS:
		file->DesiredAccess = GENERIC_READ|GENERIC_WRITE;
		file->ShareMode = 0;
		file->CreationDisposition = OPEN_ALWAYS;
		break;
	}
	return ret;
}

FILE *_wfopen(const WCHAR *filename, const WCHAR *mode)
{
	int ptr = -1;
	int i;
	if(!filename) return NULL;
	if(!mode) return NULL;
	if(!minilibc_files)
	{
		minilibc_files = (minilibc_FILE*)malloc(128*sizeof(minilibc_FILE));
		if(!minilibc_files)
		{
			_set_errno(ENOMEM);
			return NULL;
		}
		memset(minilibc_files,0,128*sizeof(minilibc_FILE));
		maxfiles = 128;
	}
	for(i = 0; i < filecount; i++)
	{
		if(minilibc_files[i].handle == INVALID_HANDLE_VALUE)
		{
			ptr = i;
			break;
		}
	}
	if(ptr == -1) ptr = filecount++;
	if(filecount >= maxfiles)
	{
		minilibc_FILE *tmpptr = (minilibc_FILE*)realloc(minilibc_files,(128+maxfiles)*sizeof(minilibc_FILE));
		if(!tmpptr)
		{
			_set_errno(ENOMEM);
			return NULL;
		}
		maxfiles += 128;
		minilibc_files = tmpptr;
	}
	minilibc_files[ptr].mode = _w_decode_filemode(mode,&minilibc_files[ptr]);
	if(minilibc_files[ptr].mode & MODE_ERROR)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	minilibc_files[ptr].handle = CreateFileW(filename,minilibc_files[ptr].DesiredAccess,
		minilibc_files[ptr].ShareMode,NULL,minilibc_files[ptr].CreationDisposition,
		FILE_ATTRIBUTE_NORMAL,NULL);
	if(minilibc_files[ptr].handle == INVALID_HANDLE_VALUE)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	return (FILE*)&minilibc_files[ptr];
}


int fclose(FILE *stream)
{
	minilibc_FILE *file;
	if(!stream)
	{
		_set_errno(EINVAL);
		return EOF;
	}
	file = (minilibc_FILE *)stream;
	if(file->handle == INVALID_HANDLE_VALUE)
	{
		_set_errno(EINVAL);
		return EOF;
	}
	if(!CloseHandle(file->handle))
	{
		_set_errno(EINVAL);
		return EOF;
	}
	return 0;
}

int _fcloseall()
{
	int i;
	int count = 0;
	if(!minilibc_files) return 0;
	for(i = 0; i < filecount; i++)
	{
		if(!fclose((FILE*)&minilibc_files[i])) count++;
	}
	free(minilibc_files);
	minilibc_files = NULL;
	filecount = 0;
	maxfiles = 0;
	return count;
}
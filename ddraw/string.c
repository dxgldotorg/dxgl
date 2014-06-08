// DXGL
// Copyright (C) 2014 William Feely

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
#include "string.h"

void String_Append(STRING *str, const char *str2)
{
	if (!str->ptr)
	{
		str->ptr = malloc(strlen(str2)+129);
		str->size = strlen(str2)+128;
		strcpy(str->ptr, str2);
		return;
	}
	else
	{
		if (str->size < strlen(str->ptr) + strlen(str2))
		{
			str->ptr = realloc(str->ptr, str->size + strlen(str2)+129);
			str->size += strlen(str2) + 128;
		}
		strcat(str->ptr, str2);
	}
}
void String_Assign(STRING *str, const char *str2)
{
	if (!str->ptr)
	{
		str->ptr = malloc(strlen(str2) + 129);
		str->size = strlen(str2) + 128;
		strcpy(str->ptr, str2);
		return;
	}
	else
	{
		if (str->size > strlen(str2))
		{
			str->ptr = realloc(str->ptr, strlen(str2) + 129);
			str->size = strlen(str2) + 128;
		}
		strcpy(str->ptr, str2);
	}
}

void String_Free(STRING *str)
{
	if (str->ptr)
	{
		free(str->ptr);
		ZeroMemory(str, sizeof(STRING));
	}
}
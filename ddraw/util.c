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
#include "util.h"

/**
  * Tests if a pointer is valid for reading from.  Compile in Visual C++ with /EHa
  * enabled Structed Exception Handling in C++ code, to prevent crashes on invalid
  * pointers.
  * @param ptr
  *  Pointer to test for validity.
  * @return
  *  Returns false if the pointer is valid, or true if an error occurs.
  */
char IsReadablePointer(void *ptr)
{
	char a;
	if(!ptr) return 0;
#ifdef _MSC_VER
	__try
	{
		a = *(char*)ptr;
		if (!a) a++;
		return a;
	}
	__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
	{
		return 0;
	}
#else
	if(IsBadReadPtr(ptr,1) return 0;
	else return 1;)
#endif
}

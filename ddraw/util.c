// DXGL
// Copyright (C) 2013-2016 William Feely

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

#ifdef _MSC_VER
#pragma optimize("g", off)
#endif
/**
  * Tests if a pointer is valid for reading from.  Uses SEH on Visual C++,
  * non-recommended Windows API on other systems.
  * @param ptr
  *  Pointer to test for validity.
  * @param size
  *  Size of block to check
  * @return
  *  Returns non-zero if the pointer is valid, or zero if an error occurs.
  */
char IsReadablePointer(void *ptr, LONG_PTR size)
{
	char a;
	char *ptr2 = ptr;
	if(!ptr) return 0;
#ifdef _MSC_VER
	__try
	{
		a = ptr2[0];
		if (!a) a++;
		if (size > 1) a = ptr2[size-1];
		if (!a) a++;
		return a;
	}
	__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
	{
		return 0;
	}
#else
	if(IsBadReadPtr(ptr,size)) return 0;
	else return 1;
#endif
}

/**
* Tests if a pointer is valid for writing to.  Uses SEH on Visual C++,
* non-recommended Windows API on other systems.
* @param ptr
*  Pointer to test for validity.
* @param size
*  Size of block to check
* @param preserve
*  TRUE to preserve the contents of the pointer.
* @return
*  Returns false if the pointer is valid, or true if an error occurs.
*/
char IsWritablePointer(void *ptr, LONG_PTR size, BOOL preserve)
{
	char a;
	char *ptr2 = ptr;
	if (!ptr) return 0;
#ifdef _MSC_VER
	__try
	{
		if (preserve) a = ptr2[0];
		else a = 1;
		ptr2[0] = a + 1;
		if (preserve) ptr2[0] = a;
		if (size > 1)
		{
			if (preserve) a = ptr2[size-1];
			ptr2[size-1] = a + 1;
			if (preserve) ptr2[size-1] = a;
		}
		return 1;
	}
	__except (GetExceptionCode() == STATUS_ACCESS_VIOLATION)
	{
		return 0;
	}
#else
	if (IsBadWritePtr(ptr, 1)) return 0;
	else return 1;
#endif
}	
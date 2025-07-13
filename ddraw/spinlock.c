// DXGL
// Copyright (C) 2024 William Feely

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
#include "spinlock.h"

#ifdef __NO_ASM

extern DXGLCFG dxglcfg;

void EnterSpinlock(unsigned long *spinlock, unsigned long max_lock)
{
	unsigned int x;
	unsigned int y = max_lock;
	if (max_lock)
	{
		LoopLimit:
		x = 1;
		x = InterlockedExchange(&spinlock, x);
		if (x)
		{
			if (!y)
			{
				Sleep(0);
				goto LoopLimit;
			}
			else 
			{
				y--;
				goto LoopLimit;
			}
		}
	}
	else
	{
		LoopNoLimit:
		x = 1;
		x = InterlockedExchange(spinlock, x);
		if (x) goto LoopNoLimit;
		return;
	}
}

void LeaveSpinlock(unsigned long *spinlock)
{
	InterlockedExchange(&spinlock, 0);
}


#endif //__NO_ASM
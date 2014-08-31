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

#pragma once
#ifndef _TIMER_H
#define _TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DXGLTimer
{
	int timertype;
	unsigned int lines;
	unsigned int vsync_lines;
	double monitor_period;
	double timer_frequency;
	LARGE_INTEGER timer_base;
} DXGLTimer;

void DXGLTimer_Init(DXGLTimer *timer);
void DXGLTimer_Calibrate(DXGLTimer *timer, unsigned int lines, unsigned int frequency);
unsigned int DXGLTimer_GetScanLine(DXGLTimer *timer);

#ifdef __cplusplus
}
#endif


#endif
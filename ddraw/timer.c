// DXGL
// Copyright (C) 2013-2014 William Feely

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
#include <math.h>
#include "timer.h"

void DXGLTimer_Init(DXGLTimer *timer)
{
	TIMECAPS mmcaps;
	LARGE_INTEGER freq;
	timer->timertype = 0;
	freq.QuadPart = 0;
	QueryPerformanceFrequency(&freq);
	if (!freq.QuadPart)
	{
		timeGetDevCaps(&mmcaps, sizeof(TIMECAPS));
		timeBeginPeriod(mmcaps.wPeriodMin);
		timer->timer_frequency = ((double) mmcaps.wPeriodMin / 1.0) * 1000.0;
		timer->timertype = 2;
	}
	else
	{
		timer->timer_frequency = (double)freq.QuadPart;
		timer->timertype = 1;
	}
}

void DXGLTimer_Calibrate(DXGLTimer *timer, unsigned int lines, unsigned int frequency)
{
	double linesperms;
	timer->monitor_period = (double) frequency / 1000.0;
	if (timer->timertype == 1)
	{
		QueryPerformanceCounter(&timer->timer_base);
		timer->vsync_lines = 16;
		timer->lines = lines + timer->vsync_lines;
	}
	else
	{
		timer->timer_base.QuadPart = timeGetTime();
		linesperms = (double) lines / ((1.0 / (double) frequency) * 1000.0);
		timer->vsync_lines = (unsigned int)(linesperms * (frequency / 1000.0));
		timer->lines = lines + timer->vsync_lines;
	}
}

unsigned int DXGLTimer_GetScanLine(DXGLTimer *timer)
{
	LARGE_INTEGER timerpos;
	double sync_pos;
	double milliseconds;
	if (timer->timertype == 1)	QueryPerformanceCounter(&timerpos);
	else timerpos.QuadPart = timeGetTime();
	timerpos.QuadPart -= timer->timer_base.QuadPart;
	if (timer->timertype == 1) milliseconds = ((double) timerpos.QuadPart / (double)timer->timer_frequency) * 1000.0;
	else milliseconds = (double) timerpos.QuadPart;
	sync_pos = fmod(milliseconds, timer->monitor_period);
	sync_pos /= timer->monitor_period;
	sync_pos *= (double)timer->lines;
	return (unsigned int)sync_pos;
}

void DXGLTimer_SetLastDraw(DXGLTimer *timer)
{
	if (timer->timertype == 1) QueryPerformanceCounter(&timer->lastdraw);
	else timer->lastdraw.QuadPart = timeGetTime();
}

BOOL DXGLTimer_CheckLastDraw(DXGLTimer *timer, DWORD ms)
{
	LARGE_INTEGER timerpos;
	double milliseconds;
	if (timer->timertype == 1)	QueryPerformanceCounter(&timerpos);
	else timerpos.QuadPart = timeGetTime();
	timerpos.QuadPart -= timer->timer_base.QuadPart;
	if (timer->timertype == 1) milliseconds = ((double)timerpos.QuadPart / (double)timer->timer_frequency) * 1000.0;
	else milliseconds = (double)timerpos.QuadPart;
	if (milliseconds < ms) return FALSE;
	else return TRUE;
}
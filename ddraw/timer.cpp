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
#include "timer.h"

DXGLTimer::DXGLTimer()
{
	TIMECAPS mmcaps;
	LARGE_INTEGER freq;
	timertype = 0;
	freq.QuadPart = 0;
	QueryPerformanceFrequency(&freq);
	if (!freq.QuadPart)
	{
		timeGetDevCaps(&mmcaps, sizeof(TIMECAPS));
		timeBeginPeriod(mmcaps.wPeriodMin);
		timer_frequency = ((double) mmcaps.wPeriodMin / 1.0) * 1000.0;
		timertype = 2;
	}
	else
	{
		timer_frequency = (double)freq.QuadPart;
		timertype = 1;
	}
}

void DXGLTimer::Calibrate(unsigned int lines, unsigned int frequency)
{
	monitor_period = (double) frequency / 1000.0;
	if (timertype == 1)
	{
		QueryPerformanceCounter(&timer_base);
		vsync_lines = 16;
		this->lines = lines + vsync_lines;
	}
	else
	{
		timer_base.QuadPart = timeGetTime();
		double linesperms = (double) lines / ((1.0 / (double) frequency) * 1000.0);
		vsync_lines = linesperms * (frequency / 1000.0);
		this->lines = lines + vsync_lines;
	}
}

unsigned int DXGLTimer::GetScanLine()
{
	LARGE_INTEGER timerpos;
	double sync_pos;
	if (timertype == 1)	QueryPerformanceCounter(&timerpos);
	else timerpos.QuadPart = timeGetTime();
	timerpos.QuadPart -= timer_base.QuadPart;
	double milliseconds;
	if (timertype == 1) milliseconds = ((double) timerpos.QuadPart / (double)timer_frequency) * 1000.0;
	else milliseconds = (double) timerpos.QuadPart;
	sync_pos = fmod(milliseconds, monitor_period);
	sync_pos /= monitor_period;
	sync_pos *= (double)lines;
	return sync_pos;
}
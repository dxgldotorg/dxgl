// DXGL
// Copyright (C) 2011 William Feely

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
#include <MMSystem.h>
#include "timer.h"

UINT msg;
double delay;
double lasttime;
double nexttime;
bool stoptimer = true;
HANDLE hThread = NULL;
HWND TimerhWnd;
UINT wndMessage;


DWORD WINAPI TimerThread(LPVOID params)
{
	int sleeptime;
	lasttime = timeGetTime();
	nexttime = lasttime + delay;
loop:
	while(stoptimer == false)
	{
		lasttime = timeGetTime();
		sleeptime = (int)(nexttime - lasttime - 1);
		if(sleeptime > 0) Sleep(sleeptime);
		while(nexttime > lasttime)
			lasttime = timeGetTime();
		SendMessage(TimerhWnd,wndMessage,0,0);
		lasttime = timeGetTime();
		if(lasttime > nexttime + delay) nexttime = lasttime;
		else nexttime += delay;
	}
	hThread = NULL;
	if(!stoptimer) goto loop;
	return 0;
}

void StartTimer(HWND hWnd, UINT message, double framerate)
{
	TimerhWnd = hWnd;
	wndMessage = message;
	delay = (1.0/framerate)*1000.0;
	stoptimer = false;
	if(!hThread)
	{
		hThread = CreateThread(NULL,0,TimerThread,NULL,0,NULL);
	}
}
void StopTimer()
{
	stoptimer = true;
}

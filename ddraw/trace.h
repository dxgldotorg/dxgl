// DXGL
// Copyright (C) 2013-2026 William Feely

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
#ifndef _DXGLTRACE_H
#define _DXGLTRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#define DXGLTRACE_SYSINFO     1
#define DXGLTRACE_ERROR       2
#define DXGLTRACE_API         4
#define DXGLTRACE_APIEXTENDED 8
#define DXGLTRACE_SHADER      16

#define DXGLTRACE_MOUSEHOOK   65536

extern BOOL trace_end;
#if _MSC_VER < 1400
#pragma warning (disable: 4002)
#define TRACE_ENTER(paramcount) if(dxglcfg.DebugTraceLevel & DXGLTRACE_API) trace_enter(__FUNCTION__,dxglcfg.DebugTraceLevel,paramcount)
#else
#define TRACE_ENTER(paramcount,...) if(dxglcfg.DebugTraceLevel & DXGLTRACE_API) trace_enter(__FUNCTION__,dxglcfg.DebugTraceLevel,paramcount,__VA_ARGS__)
#endif
#define TRACE_EXIT(argtype,arg) if(dxglcfg.DebugTraceLevel & DXGLTRACE_API) trace_exit(__FUNCTION__,dxglcfg.DebugTraceLevel,argtype,(void*)arg)
#define TRACE_VAR(var,argtype,arg) if(dxglcfg.DebugTraceLevel & DXGLTRACE_API) trace_var(__FUNCTION__,dxglcfg.DebugTraceLevel,var,argtype,(void*)arg)
#define TRACE_ERROR(str) if(dxglcfg.DebugTraceLevel & DXGLTRACE_ERROR) trace_string(str,dxglcfg.DebugTraceLevel)
#define TRACE_SHADER(str) if(dxglcfg.DebugTraceLevel & DXGLTRACE_SHADER) trace_string(str,dxglcfg.DebugTraceLevel)
#define TRACE_STRING(str) if(dxglcfg.DebugTraceLevel & DXGLTRACE_SYSINFO) trace_string(str,dxglcfg.DebugTraceLevel)
#define TRACE_SYSINFO() if(dxglcfg.DebugTraceLevel & DXGLTRACE_SYSINFO) trace_sysinfo(dxglcfg.DebugTraceLevel) // Must be in thread used by OpenGL.
#define TRACE_GETCURSORPOS(pin,pout) if(dxglcfg.DebugTraceLevel & DXGLTRACE_MOUSEHOOK) trace_getcursorpos(pin,pout)
#define TRACE_SETCURSORPOS(pin,pout) if(dxglcfg.DebugTraceLevel & DXGLTRACE_MOUSEHOOK) trace_setcursorpos(pin,pout)
#define TRACE_GETCLIPCURSOR(rin,rout) if(dxglcfg.DebugTraceLevel & DXGLTRACE_MOUSEHOOK) trace_getclipcursor(rin,rout)
#define TRACE_CLIPCURSOR(rin,rout) if(dxglcfg.DebugTraceLevel & DXGLTRACE_MOUSEHOOK) trace_clipcursor(rin,rout)
void trace_enter(const char *function, int level, int paramcount, ...);
void trace_exit(const char *function, int level, int argtype, void *arg);
void *trace_ret(const char *function, int level, int argtype, void *arg);
void trace_var(const char *function, int level, const char *var, int argtype, void *arg);
void trace_string(const char *str, int level);
void trace_sysinfo(int level);
void trace_getcursorpos(POINT *pin, POINT *pout);
void trace_setcursorpos(POINT *pin, POINT *pout);
void trace_getclipcursor(RECT *rin, RECT *rout);
void trace_clipcursor(RECT *rin, RECT *rout);
#define TRACE_RET(type, argtype, arg) if(dxglcfg.DebugTraceLevel & DXGLTRACE_API) \
return (type)trace_ret(__FUNCTION__,dxglcfg.DebugTraceLevel,argtype,(void*)arg); \
else return arg;

#ifdef __cplusplus
}
#endif

#endif //_DXGLTRACE_H

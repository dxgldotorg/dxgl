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
#ifndef _TRACE_H
#define _TRACE_H


#ifdef _TRACE
extern bool trace_end;
#define TRACE_ENTER(paramcount,...) trace_enter(__FUNCTION__,paramcount,__VA_ARGS__)
#define TRACE_EXIT(argtype,arg) trace_exit(__FUNCTION__,argtype,(void*)arg)
#define TRACE_VAR(var,argtype,arg) trace_var(__FUNCTION__,var,argtype,(void*)arg)
#define TRACE_SYSINFO() trace_sysinfo() // Must be in thread used by OpenGL.
void trace_enter(const char *function, int paramcount, ...);
void trace_exit(const char *function, int argtype, void *arg);
void *trace_ret(const char *function, int argtype, void *arg);
void trace_var(const char *function, const char *var, int argtype, void *arg);
void trace_sysinfo();
#define TRACE_RET(type, argtype, arg) return (type)trace_ret(__FUNCTION__,argtype,(void*)arg);
#else
#define TRACE_ENTER(a,...)
#define TRACE_EXIT(a,b)
#define TRACE_VAR(a,b,c)
#define TRACE_RET(type, argtype, arg) return arg;
#define TRACE_SYSINFO()
#endif

#endif //_TRACE_H
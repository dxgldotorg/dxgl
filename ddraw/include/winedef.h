// Part of windef.h from Wine source code, subject to the following license:
/*
 * Basic types definitions
 *
 * Copyright 1996 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */



#pragma once
#ifndef _WINEDEF_H
#define _WINEDEF_H

# define DECL_WINELIB_TYPE_AW(type)  typedef WINELIB_NAME_AW(type) type;
# ifdef UNICODE
#  define WINELIB_NAME_AW(func) func##W
# else
#  define WINELIB_NAME_AW(func) func##A
# endif

#endif //_WINEDEF_H

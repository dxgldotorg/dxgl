// DXGL
// Copyright (C) 2015-2016 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY W	ARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#pragma once
#ifndef __PBO_H
#define __PBO_H

#ifdef __cplusplus
extern "C" {
#endif

struct glUtil;

void BufferObject_Create(BufferObject **out, glExtensions *ext, struct glUtil *util);
void BufferObject_AddRef(BufferObject *This);
void BufferObject_Release(BufferObject *This);
void BufferObject_SetData(BufferObject *This, GLenum target, GLsizeiptr size, GLvoid *data, GLenum usage);
void BufferObject_Bind(BufferObject *This, GLenum target);
void BufferObject_Unbind(BufferObject *This, GLenum target);
void *BufferObject_Map(BufferObject *This, GLenum target, GLenum access);
GLboolean BufferObject_Unmap(BufferObject *This, GLenum target);

#ifdef __cplusplus
}
#endif


#endif //__PBO_H
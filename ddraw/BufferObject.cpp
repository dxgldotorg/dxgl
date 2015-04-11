// DXGL
// Copyright (C) 2015 William Feely

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

#include "common.h"
#include "glUtil.h"
#include "BufferObject.h"

extern "C" {

void BufferObject_Create(BufferObject **out, glExtensions *ext, glUtil *util)
{
	BufferObject *buffer = (BufferObject*)malloc(sizeof(BufferObject));
	if (!buffer)
	{
		*out = NULL;
		return;
	}
	ZeroMemory(buffer, sizeof(BufferObject));
	buffer->ext = ext;
	buffer->util = util;
	ext->glGenBuffers(1, &buffer->buffer);
	*out = buffer;
}

void BufferObject_AddRef(BufferObject *This)
{
	InterlockedIncrement(&This->refcount);
}

void BufferObject_Release(BufferObject *This)
{
	InterlockedDecrement(&This->refcount);
	if (!This->refcount)
	{
		This->ext->glDeleteBuffers(1, &This->buffer);
		free(This);
	}
}

void BufferObject_SetData(BufferObject *This, GLenum target, GLsizeiptr size, GLvoid *data, GLenum usage)
{
	if (This->ext->GLEXT_ARB_direct_state_access)
	{
		This->ext->glNamedBufferData(This->buffer, size, data, usage);
	}
	else if (This->ext->GLEXT_EXT_direct_state_access)
	{
		This->ext->glNamedBufferDataEXT(This->buffer, size, data, usage);
	}
	else
	{
		This->util->BindBuffer(This, target);
		This->ext->glBufferData(target, size, data, usage);
		This->util->UndoBindBuffer(target);
	}
}

void BufferObject_Bind(BufferObject *This, GLenum target)
{
	This->util->BindBuffer(This, target);
}
void BufferObject_Unbind(BufferObject *This, GLenum target)
{
	This->util->BindBuffer(NULL, target);
}

void *BufferObject_Map(BufferObject *This, GLenum target, GLenum access)
{
	void *ptr;
	if (This->ext->GLEXT_ARB_direct_state_access)
	{
		ptr = This->ext->glMapNamedBuffer(This->buffer, access);
	}
	else if (This->ext->GLEXT_EXT_direct_state_access)
	{
		ptr = This->ext->glMapNamedBufferEXT(This->buffer, access);
	}
	else
	{
		This->util->BindBuffer(This, target);
		ptr = This->ext->glMapBuffer(target, access);
		This->util->UndoBindBuffer(target);
	}
	return ptr;
}
GLboolean BufferObject_Unmap(BufferObject *This, GLenum target)
{
	GLboolean ret;
	if (This->ext->GLEXT_ARB_direct_state_access)
	{
		ret = This->ext->glUnmapNamedBuffer(This->buffer);
	}
	else if (This->ext->GLEXT_EXT_direct_state_access)
	{
		ret = This->ext->glUnmapNamedBufferEXT(This->buffer);
	}
	else
	{
		This->util->BindBuffer(This, target);
		ret = This->ext->glUnmapBuffer(target);
		This->util->UndoBindBuffer(target);
	}
	return ret;
}

}
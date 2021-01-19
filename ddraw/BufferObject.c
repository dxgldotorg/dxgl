// DXGL
// Copyright (C) 2015-2021 William Feely

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
#include "glUtil.h"
#include "BufferObject.h"

void BufferObject_Create(BufferObject **out, glExtensions *ext, struct glUtil *util)
{
	BufferObject *buffer = (BufferObject*)malloc(sizeof(BufferObject));
	if (!buffer)
	{
		*out = NULL;
		return;
	}
	ZeroMemory(buffer, sizeof(BufferObject));
	buffer->refcount = 1;
	buffer->ext = ext;
	buffer->util = util;
	buffer->size = 0;
	glUtil_AddRef(util);
	ext->glGenBuffers(1, &buffer->buffer);
	*out = buffer;
}

void BufferObject_AddRef(BufferObject *This)
{
	InterlockedIncrement((LONG*)&This->refcount);
}

void BufferObject_Release(BufferObject *This)
{
	InterlockedDecrement((LONG*)&This->refcount);
	if (!This->refcount)
	{
		This->ext->glDeleteBuffers(1, &This->buffer);
		glUtil_Release(This->util);
		free(This);
	}
}

void BufferObject_SetData(BufferObject *This, GLenum target, GLsizeiptr size, GLvoid *data, GLenum usage)
{
	/*if (This->ext->GLEXT_ARB_direct_state_access)
	{
		This->ext->glNamedBufferData(This->buffer, size, data, usage);
	}
	else if (This->ext->GLEXT_EXT_direct_state_access)
	{
		This->ext->glNamedBufferDataEXT(This->buffer, size, data, usage);
	}
	else
	{*/
		glUtil_BindBuffer(This->util, This, target);
		This->ext->glBufferData(target, size, data, usage);
		glUtil_UndoBindBuffer(This->util, target);
	/*}*/
}

void BufferObject_Bind(BufferObject *This, GLenum target)
{
	glUtil_BindBuffer(This->util, This, target);
}
void BufferObject_Unbind(BufferObject *This, GLenum target)
{
	glUtil_BindBuffer(This->util, NULL, target);
}

void *BufferObject_Map(BufferObject *This, GLenum target, GLenum access)
{
	void *ptr;
	/*if (This->ext->GLEXT_ARB_direct_state_access)
	{
		ptr = This->ext->glMapNamedBuffer(This->buffer, access);
	}
	else if (This->ext->GLEXT_EXT_direct_state_access)
	{
		ptr = This->ext->glMapNamedBufferEXT(This->buffer, access);
	}
	else
	{*/
		glUtil_BindBuffer(This->util, This, target);
		ptr = This->ext->glMapBuffer(target, access);
		glUtil_UndoBindBuffer(This->util, target);
	/*}*/
	return ptr;
}
GLboolean BufferObject_Unmap(BufferObject *This, GLenum target)
{
	GLboolean ret;
	/*if (This->ext->GLEXT_ARB_direct_state_access)
	{
		ret = This->ext->glUnmapNamedBuffer(This->buffer);
	}
	else if (This->ext->GLEXT_EXT_direct_state_access)
	{
		ret = This->ext->glUnmapNamedBufferEXT(This->buffer);
	}
	else
	{*/
		glUtil_BindBuffer(This->util, This, target);
		ret = This->ext->glUnmapBuffer(target);
		glUtil_UndoBindBuffer(This->util, target);
	/*}*/
	return ret;
}
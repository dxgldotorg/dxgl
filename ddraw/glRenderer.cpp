// DXGL
// Copyright (C) 2012-2018 William Feely

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
#include "BufferObject.h"
#include "glTexture.h"
#include "glUtil.h"
#include "timer.h"
#include "glDirectDraw.h"
#include "glRenderWindow.h"
#include "glRenderer.h"
#include "ddraw.h"
#include "ShaderGen3D.h"
#include "matrix.h"
#include "util.h"
#include <stdarg.h>

extern "C" {

static const DDSURFACEDESC2 ddsdbackbuffer =
{
	sizeof(DDSURFACEDESC2),
	DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NULL,
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{
		sizeof(DDPIXELFORMAT),
		DDPF_RGB,
		0,
		32,
		0xFF,
		0xFF00,
		0xFF0000,
		0
	},
	{
		DDSCAPS_TEXTURE,
		0,
		0,
		0
	},
	0,
};

const GLushort bltindices[4] = {0,1,2,3};

/**
  * Expands a 5-bit value to 8 bits.
  * @param number
  *  5-bit value to convert to 8 bits.
  * @return
  *  Converted 8-bit value
  */
inline int _5to8bit(int number)
{
	return (number << 3)+(number>>2);
}

/**
  * Expands a 6-bit value to 8 bits.
  * @param number
  *  6-bit value to convert to 8 bits.
  * @return
  *  Converted 8-bit value
  */
inline int _6to8bit(int number)
{
	return (number<<2)+(number>>4);
}

/**
  * Checks the command buffer and flips it if too full.
  * @param This
  *  Pointer to glRenderer object
  * @param cmdsize
  *  Requested size for command buffer
  * @return
  *  TRUE if ending command, FALSE otherwise
  */
BOOL CheckCmdBuffer(glRenderer *This, DWORD cmdsize, DWORD uploadsize, DWORD vertexsize, DWORD indexsize)
{
	BOOL over = FALSE;
	if (cmdsize)
	{
		if ((This->state.cmd->write_ptr_cmd + cmdsize) > This->state.cmd->cmdsize)
			over = TRUE;
	}
	if (uploadsize)
	{
		if ((This->state.cmd->write_ptr_upload + uploadsize) > This->state.cmd->uploadsize)
			over = TRUE;
	}
	if (vertexsize)
	{
		if ((This->state.cmd->write_ptr_vertex + vertexsize) > This->state.cmd->vertices->size)
			over = TRUE;
	}
	if (indexsize)
	{
		if ((This->state.cmd->write_ptr_index + indexsize) > This->state.cmd->indices->size)
			over = TRUE;
	}
	if (over)
	{
		glRenderer_EndCommand(This, FALSE, TRUE);
		return TRUE;
	}
	else return FALSE;
}

/**
  * Check buffer for SetUniformCmd command and post it if it would overflow
  * @param This
  *  Pointer to glRenderer object
  * @param cmd
  *  Command to check, will be reinitialized if posted
  * @param ptr
  *  Address of SetUniformCmdData pointer, will be updated if command is posted
  * @param buffer_size
  *  Size of buffer holding command
  * @param request_size
  *  Size of buffer to be requested
  * @param inner
  *  Inner parameter for glRenderer_AddCommand if command is posted
  * @return
  *  TRUE if command gets posted and reinitialized
  */
BOOL check_uniform_cmd_buffer(glRenderer *This, SetUniformCmd *cmd, SetUniformCmdData **ptr, DWORD buffer_size, DWORD request_size, BOOL inner)
{
	if (request_size > buffer_size)
	{
		glRenderer_AddCommand(This, (QueueCmd*)cmd, inner, FALSE);
		cmd->size = sizeof(SetUniformCmdBase) - 8;
		cmd->tmp_ptr = (BYTE*)&cmd->data;
		cmd->count = 0;
		*ptr = (SetUniformCmdData*)cmd->tmp_ptr;
		return TRUE;
	}
	else return FALSE;
}

/**
* Check buffer for SetAttribCmd command and post it if it would overflow
* @param This
*  Pointer to glRenderer object
* @param cmd
*  Command to check, will be reinitialized if posted
* @param ptr
*  Address of SetUniformCmdData pointer, will be updated if command is posted
* @param buffer_size
*  Size of buffer holding command
* @param request_size
*  Size of buffer to be requested
* @param inner
*  Inner parameter for glRenderer_AddCommand if command is posted
* @return
*  TRUE if command gets posted and reinitialized
*/
BOOL check_attrib_cmd_buffer(glRenderer *This, SetAttribCmd *cmd, DWORD buffer_size, DWORD request_size, BOOL inner)
{
	if (request_size > buffer_size)
	{
		glRenderer_AddCommand(This, (QueueCmd*)cmd, inner, FALSE);
		cmd->size = sizeof(SetAttribCmdBase) - 8;
		cmd->count = 0;
		return TRUE;
	}
	else return FALSE;
}


/**
  * Appends data to a SetUniformCmd command.
  * The size, count, and tmp_ptr fields must be initialized before the first
  * call to this function.
  * @param This
  *  Pointer to glRenderer object
  * @param cmd
  *  Pointer to SetUniformCmd command
  * @param uniform
  *  GLSL uniform to modify
  *  For builtin shaders, this is the actual GLSL uniform.
  *  For generated shaders, this is the array offset of the generated structure for the uniform.
  * @param type
  *  Encoded type for uniform, see struct_command.h for more info
  * @param count
  *  Count for certain types of uniforms, color order for colorkey
  * @param transpose
  *  Transpose parameter for matrix uniforms
  * @param inner
  *  Inner parameter for glRenderer_AddCommand if buffer overflows
  * @param buffer_size
  *  Size of the buffer containing the SetUniformCmd command
  * @param data
  *  First value to pass to glUniform function
  * @param ...
  *  Additional commands for certain glUniform functions
  */
void append_uniform_cmd(glRenderer *This, SetUniformCmd *cmd, GLint uniform, DWORD type, GLsizei count, BOOL transpose, BOOL inner, size_t buffer_size, DWORD_PTR data, ...)
{
	DWORD r, g, b, a;
	DWORD *colorsizes;
	DWORD *colorbits;
	size_t size;
	size_t multiplier;
	SetUniformCmdData *uniform_ptr = (SetUniformCmdData*)cmd->tmp_ptr;
	va_list va;
	va_start(va, data);
	switch (type)
	{
	case 0: // glUniform1i()
	default:
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData);
		uniform_ptr->type = type;
		uniform_ptr->data[0] = (GLint)data;
		break;
	case 1: // glUniform2i()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) + sizeof(GLint), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) + sizeof(GLint);
		uniform_ptr->type = type;
		uniform_ptr->data[0] = (GLint)data;
		uniform_ptr->data[1] = va_arg(va, GLint);
		break;
	case 2: // glUniform3i()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) + (2 * sizeof(GLint)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) + (2 * sizeof(GLint));
		uniform_ptr->type = type;
		uniform_ptr->data[0] = (GLint)data;
		uniform_ptr->data[1] = va_arg(va, GLint);
		uniform_ptr->data[2] = va_arg(va, GLint);
		break;
	case 3: // glUniform4i()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) + (3 * sizeof(GLint)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) + (3 * sizeof(GLint));
		uniform_ptr->type = type;
		uniform_ptr->data[0] = (GLint)data;
		uniform_ptr->data[1] = va_arg(va, GLint);
		uniform_ptr->data[2] = va_arg(va, GLint);
		uniform_ptr->data[3] = va_arg(va, GLint);
		break;
	case 4: // glUniform1f()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) + sizeof(GLfloat), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) + sizeof(GLfloat);
		uniform_ptr->type = type;
		*((GLfloat*)&uniform_ptr->data[0]) = (GLfloat)data;
		break;
	case 5: // glUniform2f()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) + (2 * sizeof(GLfloat)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) + (2 * sizeof(GLfloat));
		uniform_ptr->type = type;
		*((GLfloat*)&uniform_ptr->data[0]) = (GLfloat)data;
		*((GLfloat*)&uniform_ptr->data[1]) = va_arg(va, GLfloat);
		break;
	case 6: // glUniform3f()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) + (3 * sizeof(GLfloat)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) + (3 * sizeof(GLfloat));
		uniform_ptr->type = type;
		*((GLfloat*)&uniform_ptr->data[0]) = (GLfloat)data;
		*((GLfloat*)&uniform_ptr->data[1]) = va_arg(va, GLfloat);
		*((GLfloat*)&uniform_ptr->data[2]) = va_arg(va, GLfloat);
		break;
	case 7: // glUniform4f()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) + (4 * sizeof(GLfloat)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) + (4 * sizeof(GLfloat));
		uniform_ptr->type = type;
		*((GLfloat*)&uniform_ptr->data[0]) = (GLfloat)data;
		*((GLfloat*)&uniform_ptr->data[1]) = va_arg(va, GLfloat);
		*((GLfloat*)&uniform_ptr->data[2]) = va_arg(va, GLfloat);
		*((GLfloat*)&uniform_ptr->data[3]) = va_arg(va, GLfloat);
		break;
	case 8: // glUniform1ui()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			uniform_ptr->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) + sizeof(GLuint), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) + sizeof(GLuint);
		uniform_ptr->type = type;
		*((GLuint*)&uniform_ptr->data[0]) = (GLuint)data;
		break;
	case 9: // glUniform2ui()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) + (2 * sizeof(GLuint)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) + (2 * sizeof(GLuint));
		uniform_ptr->type = type;
		*((GLuint*)&uniform_ptr->data[0]) = (GLuint)data;
		*((GLuint*)&uniform_ptr->data[1]) = va_arg(va, GLuint);
		break;
	case 10: // glUniform3ui()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) + (3 * sizeof(GLuint)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) + (3 * sizeof(GLuint));
		uniform_ptr->type = type;
		*((GLuint*)&uniform_ptr->data[0]) = (GLuint)data;
		*((GLuint*)&uniform_ptr->data[1]) = va_arg(va, GLuint);
		*((GLuint*)&uniform_ptr->data[2]) = va_arg(va, GLuint);
		break;
	case 11: // glUniform4ui()
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) + (4 * sizeof(GLuint)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) + (4 * sizeof(GLuint));
		uniform_ptr->type = type;
		*((GLuint*)&uniform_ptr->data[0]) = (GLuint)data;
		*((GLuint*)&uniform_ptr->data[1]) = va_arg(va, GLuint);
		*((GLuint*)&uniform_ptr->data[2]) = va_arg(va, GLuint);
		*((GLuint*)&uniform_ptr->data[3]) = va_arg(va, GLuint);
		break;
	case 16: // glUniform1iv()
	case 17: // glUniform12v()
	case 18: // glUniform13v()
	case 19: // glUniform14v()
		multiplier = (type & 3) - 1;
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) +
			(multiplier * count * sizeof(GLint)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) +
			(multiplier * count * sizeof(GLint));
		uniform_ptr->type = type;
		uniform_ptr->count = count;
		memcpy(uniform_ptr->data, (GLint*)data, (multiplier * count * sizeof(GLint)));
		break;
	case 20: // glUniform1fv()
	case 21: // glUniform2fv()
	case 22: // glUniform3fv()
	case 23: // glUniform4fv()
		multiplier = (type & 3) - 1;
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) +
			(multiplier * count * sizeof(GLfloat)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) +
			(multiplier * count * sizeof(GLfloat));
		uniform_ptr->type = type;
		uniform_ptr->count = count;
		memcpy(uniform_ptr->data, (GLfloat*)data, (multiplier * count * sizeof(GLfloat)));
		break;
	case 24: // glUniform1uiv()
	case 25: // glUniform2uiv()
	case 26: // glUniform3uiv()
	case 27: // glUniform4uiv()
		multiplier = (type & 3) - 1;
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) +
			(multiplier * count * sizeof(GLint)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) +
			(multiplier * count * sizeof(GLint));
		uniform_ptr->type = type;
		uniform_ptr->count = count;
		memcpy(uniform_ptr->data, (GLint*)data, (multiplier * count * sizeof(GLint)));
		break;
	case 33: // glUniformMatrix2fv()
	case 34: // glUniformMatrix3fv()
	case 35: // glUniformMatrix4fv()
		if (type == 33) multiplier = 4;
		else if (type == 34) multiplier = 9;
		else if (type == 35) multiplier = 16;
		check_uniform_cmd_buffer(This, cmd, &uniform_ptr, buffer_size,
			cmd->size + 8 + sizeof(SetUniformCmdData) - sizeof(GLint) +
			(multiplier * count * sizeof(GLfloat)), inner);
		uniform_ptr->size = sizeof(SetUniformCmdData) - sizeof(GLint) +
			(multiplier * count * sizeof(GLfloat));
		uniform_ptr->type = type;
		uniform_ptr->count = count;
		uniform_ptr->transpose = transpose;
		memcpy(uniform_ptr->data, (GLfloat*)data, (multiplier * count * sizeof(GLfloat)));
		break;
	case 256: // Color key, uses glUniform3i() or glUniform4i()
		colorsizes = va_arg(va, DWORD*);
		colorbits = va_arg(va, DWORD*);
		switch (count)
		{
		case 0:
			r = data & colorsizes[0];
			data >>= colorbits[0];
			g = data & colorsizes[1];
			data >>= colorbits[1];
			b = data & colorsizes[2];
			append_uniform_cmd(This, cmd, uniform, 2, 0, FALSE, inner, buffer_size, r, g, b);
			break;
		case 1:
			b = data & colorsizes[2];
			data >>= colorbits[2];
			g = data & colorsizes[1];
			data >>= colorbits[1];
			r = data & colorsizes[0];
			append_uniform_cmd(This, cmd, uniform, 2, 0, FALSE, inner, buffer_size, r, g, b);
			break;
		case 2:
			a = data & colorsizes[3];
			data >>= colorbits[3];
			r = data & colorsizes[0];
			data >>= colorbits[0];
			g = data & colorsizes[1];
			data >>= colorbits[1];
			b = data & colorsizes[2];
			append_uniform_cmd(This, cmd, uniform, 2, 0, FALSE, inner, buffer_size, r, g, b);
			break;
		case 3:
			a = data & colorsizes[3];
			data >>= colorbits[3];
			b = data & colorsizes[2];
			data >>= colorbits[2];
			g = data & colorsizes[1];
			data >>= colorbits[1];
			r = data & colorsizes[0];
			append_uniform_cmd(This, cmd, uniform, 2, 0, FALSE, inner, buffer_size, r, g, b);
			break;
		case 4:
			r = data & colorsizes[0];
			if (This->ext->glver_major >= 3)
				append_uniform_cmd(This, cmd, uniform, 2, 0, FALSE, inner, buffer_size, r, 0, 0);
			append_uniform_cmd(This, cmd, uniform, 2, 0, FALSE, inner, buffer_size, r, r, r);
			break;
		case 5:
			r = data & colorsizes[0];
			append_uniform_cmd(This, cmd, uniform, 2, 0, FALSE, inner, buffer_size, r, r, r);
			break;
		case 6:
			a = data & colorsizes[3];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, 0, 0, 0, a);
			break;
		case 7:
			r = data & colorsizes[0];
			data >>= colorbits[0];
			a = data & colorsizes[3];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, r, r, r, a);
			break;
		}
		va_end(va);
		return;
	case 257:
		colorsizes = va_arg(va, DWORD*);
		colorbits = va_arg(va, DWORD*);
		switch (count)
		{
		case 0:
			r = data & colorsizes[0];
			data >>= colorbits[0];
			g = data & colorsizes[1];
			data >>= colorbits[1];
			b = data & colorsizes[2];
			data >>= colorbits[2];
			a = data & colorsizes[3];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, r, g, b, a);
			break;
		case 1:
			b = data & colorsizes[2];
			data >>= colorbits[2];
			g = data & colorsizes[1];
			data >>= colorbits[1];
			r = data & colorsizes[0];
			data >>= colorbits[0];
			a = data & colorsizes[3];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, r, g, b, a);
			break;
		case 2:
			a = data & colorsizes[3];
			data >>= colorbits[3];
			r = data & colorsizes[0];
			data >>= colorbits[0];
			g = data & colorsizes[1];
			data >>= colorbits[1];
			b = data & colorsizes[2];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, r, g, b, a);
			break;
		case 3:
			a = data & colorsizes[3];
			data >>= colorbits[3];
			b = data & colorsizes[2];
			data >>= colorbits[2];
			g = data & colorsizes[1];
			data >>= colorbits[1];
			r = data & colorsizes[0];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, r, g, b, a);
			break;
		case 4:
			r = data & colorsizes[0];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, r, r, r, r);
			break;
		case 5:
			r = data & colorsizes[0];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, r, r, r, r);
			break;
		case 6:
			a = data & colorsizes[3];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, a, a, a, a);
			break;
		case 7:
			r = data & colorsizes[0];
			data >>= colorbits[0];
			a = data & colorsizes[3];
			append_uniform_cmd(This, cmd, uniform, 3, 0, FALSE, inner, buffer_size, r, r, r, a);
			break;
		}
		va_end(va);
		return;
	}
	cmd->size += uniform_ptr->size;
	cmd->tmp_ptr += uniform_ptr->size;
	cmd->count++;
	va_end(va);
}


/**
  * Appends data to a SetAttribCmd command.
  * The size, count, and tmp_ptr fields must be initialized before the first
  * call to this function.
  * @param This
  *  Pointer to glRenderer object
  * @param cmd
  *  Pointer to SetAttribCmd command
  * @param attrib
  *  GLSL attribute to set up
  *  For builtin shaders, this is the actual GLSL attribute.
  *  For generated shaders, this is the array offset of the generated structure for the attribute.
  * @param size
  *  Number of components per attribute, or GL_BGRA
  * @param type
  *  Type of components in the attribute
  * @param normalized
  *  Whether or not integer types are normalized to -1 to 1 for signed, 0 to 1 for unsinged.
  * @param stride
  *  Number of bytes between the beginning of one attribute element and the beginning of the next.
  * @param inner
  *  Inner parameter for glRenderer_AddCommand if buffer overflows
  * @param buffer_size
  *  Size of the buffer containing the SetAttribCmd command
  */
void append_attrib_cmd(glRenderer *This, SetAttribCmd *cmd, GLuint attrib, GLint size,
	GLint type, GLboolean normalized, GLsizei stride, const GLvoid *pointer, BOOL inner, size_t buffer_size)
{
	check_attrib_cmd_buffer(This, cmd, buffer_size, cmd->size + sizeof(SetAttribCmdBase), inner);
	cmd->attrib[cmd->count].index = attrib;
	cmd->attrib[cmd->count].size = size;
	cmd->attrib[cmd->count].type = type;
	cmd->attrib[cmd->count].normalized = normalized;
	cmd->attrib[cmd->count].stride = stride;
	cmd->attrib[cmd->count].ptr = pointer;
	cmd->size += sizeof(SetAttribCmdBase);
	cmd->count++;
}

const WORD indexbase[6] = { 0,1,2,2,3,0 };
/**
  * Adds a command to the active command buffer.
  * @param This
  *  Pointer to glRenderer object
  * @param command
  *  Formatted command to add to buffer.
  *  Command format:
  *  First DWORD:  Command ID, see glRenderer.h
  *  Second DWORD:  Size of command data, rounded up to nearest DWORD
  *  Third DWORD and beyond:  command data
  * @param inner
  *  TRUE if called from within this function.
  * @param wait
  *  TRUE to force the queue to wait until completion
  * @return
  *  Return value specific to command, DD_OK if succeeded.
  */
HRESULT glRenderer_AddCommand(glRenderer *This, QueueCmd *cmd, BOOL inner, BOOL wait)
{
	HRESULT error;
	QueueCmd tmp_cmd;
	RECT r1, r2;
	int i;
	// Command specific variables
	RECT wndrect;
	int screenx, screeny;
	LONG_PTR winstyle, winstyleex;
	BOOL restart_cmd = FALSE;
	__int64 shaderid;
	BltVertex *vertex;
	BOOL flip;
	if (wait) flip = TRUE;
	else flip = FALSE;
	if (!inner) EnterCriticalSection(&This->cs);
	switch (cmd->Generic.opcode)
	{
	case OP_NULL:
		error = DD_OK;  // No need to write to the command buffer
		break;
	case OP_SETWND:  // Should be invoked from glRenderer_SetWnd which flushes the
		             // command buffer then executes the command on its own.
		error = DDERR_UNSUPPORTED;
		break;
	case OP_DELETE:  // Should be executed by itself, flushes command buffer then
		             // destroys glRenderer object.
		error = DDERR_UNSUPPORTED;
		break;
	case OP_CREATE:  // Creates a texture.  Needs to sync in order to return the
		             // texture object.
		CheckCmdBuffer(This, cmd->MakeTexture.size + 8, 0, 0, 0);
		memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, cmd, cmd->MakeTexture.size + 8);
		This->state.cmd->write_ptr_cmd += (cmd->MakeTexture.size + 8);
		error = DD_OK;
		break;
	case OP_UPLOAD:  // This one can fill the upload buffer fast; upload buffer is
		             // initialized to 2x primary for <=128MB, 1.5x for <=256MB, or
		             // 1.25x for >256MB upload buffer size.
		cmd->UploadTexture.texturesize =
			cmd->UploadTexture.texture->levels[cmd->UploadTexture.level].ddsd.lPitch
			* cmd->UploadTexture.texture->levels[cmd->UploadTexture.level].ddsd.dwHeight;
		cmd->UploadTexture.content =
			(BYTE*)cmd->UploadTexture.texture->levels[cmd->UploadTexture.level].buffer;
		cmd->UploadTexture.offset = This->state.cmd->write_ptr_upload;
		CheckCmdBuffer(This, cmd->UploadTexture.size + 8, 0, 0, 0);
		memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, cmd, cmd->UploadTexture.size + 8);
		This->state.cmd->write_ptr_cmd += (cmd->UploadTexture.size + 8);
		memcpy(This->state.cmd->uploadbuffer + This->state.cmd->write_ptr_upload,
			cmd->UploadTexture.content, cmd->UploadTexture.texturesize);
		error = DD_OK;
		break;
	case OP_DOWNLOAD: // Downloads a texture.  Needs to sync in order to receive the
		              // texture data.
		CheckCmdBuffer(This, cmd->DownloadTexture.size + 8, 0, 0, 0);
		memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, cmd, cmd->DownloadTexture.size + 8);
		This->state.cmd->write_ptr_cmd += (cmd->DownloadTexture.size + 8);
		error = DD_OK;
		break;
	case OP_DELETETEX: // Deletes a texture.  Non-blocking becuase frontend has
		               // forgotten the texture.
		CheckCmdBuffer(This, cmd->DeleteTexture.size + 8, 0, 0, 0);
		memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, cmd, cmd->DeleteTexture.size + 8);
		This->state.cmd->write_ptr_cmd += (cmd->DeleteTexture.size + 8);
		error = DD_OK;
		break;
	case OP_BLT:  // Perform a Blt() operation, issuing necessary commands to set it
		          // up.
		// Check if last major command is Blt().
		if (This->state.last_cmd.Generic.opcode == OP_BLT)
		{
			// Compare last command minus rotation
			if (((cmd->Blt.cmd.flags & 0xBFABFFF) == (This->state.last_cmd.Blt.cmd.flags & 0xBFABFFF)) &&
				!comp_bltfx(&This->state.last_cmd.Blt.cmd.bltfx, &cmd->Blt.cmd.bltfx, cmd->Blt.cmd.flags))
				restart_cmd = FALSE;
			else restart_cmd = TRUE;
			if ((cmd->Blt.cmd.bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->Blt.cmd.flags & DDBLT_ROP))
			{
				if (rop_texture_usage[(cmd->Blt.cmd.bltfx.dwROP >> 16) & 0xFF] & 2) restart_cmd = TRUE;
			}
			// Check if buffer can accomodate command
			if(!restart_cmd) restart_cmd = CheckCmdBuffer(This, 0, 0, 4 * sizeof(BltVertex), 6 * sizeof(WORD));
			if(!restart_cmd)
			{
				Vertex2DCmd *lastcmd = (Vertex2DCmd*)((BYTE*)(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd));
				int rotates = 0;
				// cmdout.flags & 1 = usedest
				// Generate vertices
				if (!memcmp(&cmd->Blt.cmd.srcrect, &nullrect, sizeof(RECT)))
				{
					r1.left = r1.top = 0;
					if (cmd->Blt.cmd.src)
					{
						r1.right = cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwWidth;
						r1.bottom = cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwHeight;
					}
					else r1.right = r1.bottom = 0;
				}
				else r1 = cmd->Blt.cmd.srcrect;
				if (!memcmp(&cmd->Blt.cmd.destrect, &nullrect, sizeof(RECT)))
				{
					r2.left = r2.top = 0;
					r2.right = cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwWidth;
					r2.bottom = cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight;
				}
				else r2 = cmd->Blt.cmd.destrect;
				tmp_cmd.BltVertex_STORAGE.vertex[1].x = tmp_cmd.BltVertex_STORAGE.vertex[3].x = (GLfloat)r2.left;
				tmp_cmd.BltVertex_STORAGE.vertex[0].x = tmp_cmd.BltVertex_STORAGE.vertex[2].x = (GLfloat)r2.right;
				tmp_cmd.BltVertex_STORAGE.vertex[0].y = tmp_cmd.BltVertex_STORAGE.vertex[1].y = (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight - (GLfloat)r2.top;
				tmp_cmd.BltVertex_STORAGE.vertex[2].y = tmp_cmd.BltVertex_STORAGE.vertex[3].y = (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight - (GLfloat)r2.bottom;
				tmp_cmd.BltVertex_STORAGE.vertex[1].s = tmp_cmd.BltVertex_STORAGE.vertex[3].s = (GLfloat)r1.left / (GLfloat)cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwWidth;
				tmp_cmd.BltVertex_STORAGE.vertex[0].s = tmp_cmd.BltVertex_STORAGE.vertex[2].s = (GLfloat)r1.right / (GLfloat)cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwWidth;
				tmp_cmd.BltVertex_STORAGE.vertex[0].t = tmp_cmd.BltVertex_STORAGE.vertex[1].t = (GLfloat)r1.top / (GLfloat)cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwHeight;
				tmp_cmd.BltVertex_STORAGE.vertex[2].t = tmp_cmd.BltVertex_STORAGE.vertex[3].t = (GLfloat)r1.bottom / (GLfloat)cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwHeight;
				if (lastcmd->flags & 1)
				{
					tmp_cmd.BltVertex_STORAGE.vertex[1].dests =
						tmp_cmd.BltVertex_STORAGE.vertex[3].dests = 0.;
					tmp_cmd.BltVertex_STORAGE.vertex[0].dests =
						tmp_cmd.BltVertex_STORAGE.vertex[2].dests = (GLfloat)(r1.right - r1.left) / (GLfloat)This->backbuffer->levels[0].ddsd.dwWidth;
					tmp_cmd.BltVertex_STORAGE.vertex[0].destt =
						tmp_cmd.BltVertex_STORAGE.vertex[1].destt = 1.;
					tmp_cmd.BltVertex_STORAGE.vertex[2].destt =
						tmp_cmd.BltVertex_STORAGE.vertex[3].destt = 1.0 - ((GLfloat)(r1.bottom - r1.top) / (GLfloat)This->backbuffer->levels[0].ddsd.dwHeight);
				}
				if (cmd->Blt.cmd.flags & 0x10000000)
				{
					tmp_cmd.BltVertex_STORAGE.vertex[1].stencils = tmp_cmd.BltVertex_STORAGE.vertex[3].stencils = tmp_cmd.BltVertex_STORAGE.vertex[1].x / (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwWidth;
					tmp_cmd.BltVertex_STORAGE.vertex[0].stencils = tmp_cmd.BltVertex_STORAGE.vertex[2].stencils = tmp_cmd.BltVertex_STORAGE.vertex[0].x / (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwWidth;
					tmp_cmd.BltVertex_STORAGE.vertex[0].stencilt = tmp_cmd.BltVertex_STORAGE.vertex[1].stencilt = tmp_cmd.BltVertex_STORAGE.vertex[0].y / (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight;
					tmp_cmd.BltVertex_STORAGE.vertex[2].stencilt = tmp_cmd.BltVertex_STORAGE.vertex[3].stencilt = tmp_cmd.BltVertex_STORAGE.vertex[2].y / (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight;
				}
				// Rotate vertices if necessary
				if ((cmd->Blt.cmd.bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->Blt.cmd.flags & DDBLT_DDFX))
				{
					if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT)
						BltFlipLR(tmp_cmd.BltVertex_STORAGE.vertex);
					if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_MIRRORUPDOWN)
						BltFlipUD(tmp_cmd.BltVertex_STORAGE.vertex);
					if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_ROTATE90) rotates++;
					if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_ROTATE180) rotates += 2;
					if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_ROTATE270) rotates += 3;
					rotates &= 3;
					if (rotates)
					{
						RotateBlt90(tmp_cmd.BltVertex_STORAGE.vertex, rotates);
					}
				}
				// Write vertices to VBO
				memcpy(tmp_cmd.BltVertex_STORAGE.index, indexbase, 6 * sizeof(WORD));
				for (i = 0; i < 6; i++)
					tmp_cmd.BltVertex_STORAGE.index[i] += lastcmd->count;
				memcpy(This->state.cmd->vertices->pointer + This->state.cmd->write_ptr_vertex,
					tmp_cmd.BltVertex_STORAGE.vertex, 4 * sizeof(BltVertex));
				memcpy(This->state.cmd->indices->pointer + This->state.cmd->write_ptr_index,
					tmp_cmd.BltVertex_STORAGE.index, 6 * sizeof(WORD));
				This->state.cmd->write_ptr_vertex += 4 * sizeof(BltVertex);
				This->state.cmd->write_ptr_index += 6 * sizeof(WORD);
				// Update command in buffer
				lastcmd->count += 4;
				lastcmd->indexcount += 6;
			}

		}
		if(restart_cmd)
		{
			BOOL usedest = FALSE;
			BOOL usepattern = FALSE;
			BOOL usetexture = FALSE;
			int rotates = 0;
			if ((cmd->Blt.cmd.bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->Blt.cmd.flags & DDBLT_ROP))
			{
				if (rop_texture_usage[(cmd->Blt.cmd.bltfx.dwROP >> 16) & 0xFF] & 2) usedest = TRUE;
				if (rop_texture_usage[(cmd->Blt.cmd.bltfx.dwROP >> 16) & 0xFF] & 4) usepattern = TRUE;
			}
			// Set render mode to 2D
			if (This->state.mode_3d)
			{
				tmp_cmd.SetMode3D.opcode = OP_SETMODE3D;
				tmp_cmd.SetMode3D.size = sizeof(SetMode3DCmd) - 8;
				tmp_cmd.SetMode3D.enable = FALSE;
				glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			}
			// Run backbuffer if using dest
			if (usedest)
			{
				r1.left = r1.top = 0;
				if (memcmp(&cmd->Blt.cmd.destrect, &nullrect, sizeof(RECT)))
				{
					r1.right = cmd->Blt.cmd.destrect.right - cmd->Blt.cmd.destrect.left;
					r1.bottom = cmd->Blt.cmd.destrect.bottom - cmd->Blt.cmd.destrect.top;
				}
				else
				{
					r1.right = cmd->Blt.cmd.dest->levels[0].ddsd.dwWidth;
					r1.bottom = cmd->Blt.cmd.dest->levels[0].ddsd.dwHeight;
				}
				// Check backbuffer size and resize
				if((This->backbuffer->levels[0].ddsd.dwWidth < r1.right) ||
					(This->backbuffer->levels[0].ddsd.dwHeight < r1.bottom))
				{
					DDSURFACEDESC2 newdesc = This->backbuffer->levels[0].ddsd;
					if (newdesc.dwWidth < r1.right) newdesc.dwWidth = r1.right;
					if (newdesc.dwHeight < r1.bottom) newdesc.dwHeight = r1.bottom;
					tmp_cmd.SetTextureSurfaceDesc.opcode = OP_SETTEXTURESURFACEDESC;
					tmp_cmd.SetTextureSurfaceDesc.size = sizeof(SetTextureSurfaceDescCmd) - 8;
					tmp_cmd.SetTextureSurfaceDesc.level = 0;
					tmp_cmd.SetTextureSurfaceDesc.desc = newdesc;
					glRenderer_AddCommand(This, &tmp_cmd, TRUE, TRUE);
				}
				tmp_cmd.Blt.opcode = OP_BLT;
				tmp_cmd.Blt.size = sizeof(BltCmd) - 8;
				tmp_cmd.Blt.cmd.flags = 0;
				tmp_cmd.Blt.cmd.destrect = r1;
				tmp_cmd.Blt.cmd.srcrect = cmd->Blt.cmd.destrect;
				tmp_cmd.Blt.cmd.src = cmd->Blt.cmd.dest;
				tmp_cmd.Blt.cmd.dest = This->backbuffer;
				tmp_cmd.Blt.cmd.srclevel = cmd->Blt.cmd.destlevel;
				tmp_cmd.Blt.cmd.destlevel = 0;
				glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			}
			// Set Src texture (Unit 8)
			i = -1;
			tmp_cmd.SetTexture.opcode = OP_SETTEXTURE;
			tmp_cmd.SetTexture.size = sizeof(SetTextureCmd) - (sizeof(DWORD) + sizeof(glTexture*)) - 8;
			if (cmd->Blt.cmd.src)
			{
				tmp_cmd.SetTexture.size += (sizeof(DWORD) + sizeof(glTexture*));
				i++;
				usetexture = TRUE;
				tmp_cmd.SetTexture.texstage[i].stage = 8;
				tmp_cmd.SetTexture.texstage[i].texture = cmd->Blt.cmd.src;
			}
			// Set Dest texture (Unit 9) - reads from temporary buffer
			if (usedest)
			{
				tmp_cmd.SetTexture.size += (sizeof(DWORD) + sizeof(glTexture*));
				i++;
				usetexture = TRUE;
				tmp_cmd.SetTexture.texstage[i].stage = 9;
				tmp_cmd.SetTexture.texstage[i].texture = This->backbuffer;
			}
			// Set Pattern texture (Unit 10)
			if (usepattern)
			{
				tmp_cmd.SetTexture.size += (sizeof(DWORD) + sizeof(glTexture*));
				i++;
				usetexture = TRUE;
				tmp_cmd.SetTexture.texstage[i].stage = 10;
				tmp_cmd.SetTexture.texstage[i].texture = cmd->Blt.cmd.pattern;
			}
			// Set clipper texture (Unit 11)
			if (cmd->Blt.cmd.dest->stencil)
			{
				tmp_cmd.SetTexture.size += (sizeof(DWORD) + sizeof(glTexture*));
				i++;
				usetexture = TRUE;
				tmp_cmd.SetTexture.texstage[i].stage = 11;
				tmp_cmd.SetTexture.texstage[i].texture = cmd->Blt.cmd.dest->stencil;
			}
			tmp_cmd.SetTexture.count = i + 1;
			if(usetexture) glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			// Set shader
			tmp_cmd.SetShader2D.opcode = OP_SETSHADER2D;
			tmp_cmd.SetShader2D.size = sizeof(SetShader2DCmd) - 8;
			tmp_cmd.SetShader2D.type = 1;
			if ((cmd->Blt.cmd.bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->Blt.cmd.flags & DDBLT_ROP))
				tmp_cmd.SetShader2D.id = PackROPBits(cmd->Blt.cmd.bltfx.dwROP, cmd->Blt.cmd.flags);
			else tmp_cmd.SetShader2D.id = cmd->Blt.cmd.flags & 0xF2FAADFF;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			// Set shader uniforms
			tmp_cmd.SetUniform.opcode = OP_SETUNIFORM;
			tmp_cmd.SetUniform.size = 0;
			SetUniformCmdData *uniform_ptr = &tmp_cmd.SetUniform.data;
			tmp_cmd.SetUniform.size = sizeof(SetUniformCmdBase) - 8;
			tmp_cmd.SetUniform.tmp_ptr = (BYTE*)&tmp_cmd.SetUniform.data;
			tmp_cmd.SetUniform.count = 0;
			append_uniform_cmd(This, &tmp_cmd.SetUniform, 0, 7, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD),
				0.0f, (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwWidth,
				0.0f, (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight);
			if (!(cmd->Blt.cmd.flags & DDBLT_COLORFILL))
			{
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 1, 0, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), 8);
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 10, 19, 1, FALSE, TRUE, sizeof(MIN_STORAGE_CMD),
					(DWORD_PTR)&cmd->Blt.cmd.src->colorsizes[0]);
			}
			if (usedest)
			{
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 2, 0, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), 9);
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 11, 19, 1, FALSE, TRUE, sizeof(MIN_STORAGE_CMD),
					(DWORD_PTR)&cmd->Blt.cmd.dest->colorsizes[0]);
			}
			if (usepattern)
			{
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 3, 0, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), 10);
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 9, 1, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD),
					cmd->Blt.cmd.pattern->levels[cmd->Blt.cmd.patternlevel].ddsd.dwWidth,
					cmd->Blt.cmd.pattern->levels[cmd->Blt.cmd.patternlevel].ddsd.dwHeight);
			}
			if (cmd->Blt.cmd.dest->stencil)
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 4, 0, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), 11);
			if ((cmd->Blt.cmd.flags & DDBLT_KEYSRC) && (cmd->Blt.cmd.src &&
				(cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwFlags & DDSD_CKSRCBLT))
				&& !(cmd->Blt.cmd.flags & DDBLT_COLORFILL))
			{
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 5, 256, cmd->Blt.cmd.src->colororder, FALSE, TRUE,
					sizeof(MIN_STORAGE_CMD), cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.ddckCKSrcBlt.dwColorSpaceLowValue,
					cmd->Blt.cmd.src->colorsizes, cmd->Blt.cmd.src->colorbits);
				if(cmd->Blt.cmd.flags & 0x20000000)
					append_uniform_cmd(This, &tmp_cmd.SetUniform, 7, 256, cmd->Blt.cmd.src->colororder, FALSE, TRUE,
						sizeof(MIN_STORAGE_CMD), cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.ddckCKSrcBlt.dwColorSpaceHighValue,
						cmd->Blt.cmd.src->colorsizes, cmd->Blt.cmd.src->colorbits);
			}
			if ((cmd->Blt.cmd.flags & DDBLT_KEYDEST) && (This && (cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwFlags & DDSD_CKDESTBLT)))
			{
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 6, 256, cmd->Blt.cmd.dest->colororder, FALSE, TRUE,
					sizeof(MIN_STORAGE_CMD), cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.ddckCKSrcBlt.dwColorSpaceLowValue,
					cmd->Blt.cmd.dest->colorsizes, cmd->Blt.cmd.dest->colorbits);
				if (cmd->Blt.cmd.flags & 0x40000000)
					append_uniform_cmd(This, &tmp_cmd.SetUniform, 8, 256, cmd->Blt.cmd.dest->colororder, FALSE, TRUE,
						sizeof(MIN_STORAGE_CMD), cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.ddckCKSrcBlt.dwColorSpaceHighValue,
						cmd->Blt.cmd.dest->colorsizes, cmd->Blt.cmd.dest->colorbits);
			}
			if (cmd->Blt.cmd.flags & DDBLT_COLORFILL)
				append_uniform_cmd(This, &tmp_cmd.SetUniform, 12, 257, cmd->Blt.cmd.src->colororder, FALSE, TRUE,
					sizeof(MIN_STORAGE_CMD), cmd->Blt.cmd.bltfx.dwFillColor,
					cmd->Blt.cmd.src->colorsizes, cmd->Blt.cmd.src->colorbits);
			//  Set shader attributes
			tmp_cmd.SetAttrib.opcode = OP_SETATTRIB;
			tmp_cmd.SetAttrib.size = sizeof(SetAttribCmdBase) - 8;
			tmp_cmd.SetAttrib.count = 0;
			vertex = (BltVertex*)This->state.cmd->write_ptr_vertex;
			append_attrib_cmd(This, &tmp_cmd.SetAttrib, 0, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &vertex->x,
				TRUE, sizeof(MIN_STORAGE_CMD));
			if (!(cmd->Blt.cmd.flags & DDBLT_COLORFILL))
				append_attrib_cmd(This, &tmp_cmd.SetAttrib, 3, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &vertex->s,
					TRUE, sizeof(MIN_STORAGE_CMD));
			if(usedest)
				append_attrib_cmd(This, &tmp_cmd.SetAttrib, 4, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &vertex->dests,
					TRUE, sizeof(MIN_STORAGE_CMD));
			if (cmd->Blt.cmd.flags & 0x10000000)  // Use clipper
				append_attrib_cmd(This, &tmp_cmd.SetAttrib, 5, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &vertex->stencils,
					TRUE, sizeof(MIN_STORAGE_CMD));
			// Set render target
			if ((This->state.target.target != cmd->Blt.cmd.dest)
				|| (This->state.target.level != cmd->Blt.cmd.destlevel))
			{
				tmp_cmd.SetRenderTarget.opcode = OP_SETRENDERTARGET;
				tmp_cmd.SetRenderTarget.size = sizeof(SetRenderTargetCmd) - 8;
				tmp_cmd.SetRenderTarget.target.target = cmd->Blt.cmd.dest;
				tmp_cmd.SetRenderTarget.target.level = cmd->Blt.cmd.destlevel;
				tmp_cmd.SetRenderTarget.target.zbuffer = NULL;
				tmp_cmd.SetRenderTarget.target.zlevel = 0;
				if (cmd->Blt.cmd.destlevel == 0 && (cmd->Blt.cmd.dest->levels[0].ddsd.dwWidth != cmd->Blt.cmd.dest->bigwidth) ||
					(cmd->Blt.cmd.dest->levels[0].ddsd.dwHeight != cmd->Blt.cmd.dest->bigheight))
				{
					tmp_cmd.SetRenderTarget.target.mulx = (GLfloat)cmd->Blt.cmd.dest->bigwidth / (GLfloat)cmd->Blt.cmd.dest->levels[0].ddsd.dwWidth;
					tmp_cmd.SetRenderTarget.target.muly = (GLfloat)cmd->Blt.cmd.dest->bigheight / (GLfloat)cmd->Blt.cmd.dest->levels[0].ddsd.dwHeight;
				}
				else tmp_cmd.SetRenderTarget.target.mulx = tmp_cmd.SetRenderTarget.target.muly = 1.0f;
				glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			}
			// Set viewport
			if (!cmd->Blt.cmd.destlevel)
			{
				r2.right = cmd->Blt.cmd.dest->bigwidth;
				r2.bottom = cmd->Blt.cmd.dest->bigheight;
			}
			else
			{
				r2.right = cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwWidth;
				r2.bottom = cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight;
			}
			if (This->state.viewport.x || This->state.viewport.y ||
				(This->state.viewport.width != r2.right) || (This->state.viewport.hieght != r2.bottom))
			{
				tmp_cmd.SetViewport.opcode = OP_SETVIEWPORT;
				tmp_cmd.SetViewport.size = sizeof(SetViewportCmd) - 8;
				tmp_cmd.SetViewport.viewport.x = tmp_cmd.SetViewport.viewport.y = 0;
				tmp_cmd.SetViewport.viewport.width = r2.right;
				tmp_cmd.SetViewport.viewport.hieght = r2.bottom;
				glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			}
			// Generate vertices
			if (!memcmp(&cmd->Blt.cmd.srcrect, &nullrect, sizeof(RECT)))
			{
				r1.left = r1.top = 0;
				if (cmd->Blt.cmd.src)
				{
					r1.right = cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwWidth;
					r1.bottom = cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwHeight;
				}
				else r1.right = r1.bottom = 0;
			}
			else r1 = cmd->Blt.cmd.srcrect;
			if (!memcmp(&cmd->Blt.cmd.destrect, &nullrect, sizeof(RECT)))
			{
				r2.left = r2.top = 0;
				r2.right = cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwWidth;
				r2.bottom = cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight;
			}
			else r2 = cmd->Blt.cmd.destrect;
			tmp_cmd.BltVertex_STORAGE.vertex[1].x = tmp_cmd.BltVertex_STORAGE.vertex[3].x = (GLfloat)r2.left;
			tmp_cmd.BltVertex_STORAGE.vertex[0].x = tmp_cmd.BltVertex_STORAGE.vertex[2].x = (GLfloat)r2.right;
			tmp_cmd.BltVertex_STORAGE.vertex[0].y = tmp_cmd.BltVertex_STORAGE.vertex[1].y = (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight - (GLfloat)r2.top;
			tmp_cmd.BltVertex_STORAGE.vertex[2].y = tmp_cmd.BltVertex_STORAGE.vertex[3].y = (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight - (GLfloat)r2.bottom;
			tmp_cmd.BltVertex_STORAGE.vertex[1].s = tmp_cmd.BltVertex_STORAGE.vertex[3].s = (GLfloat)r1.left / (GLfloat)cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwWidth;
			tmp_cmd.BltVertex_STORAGE.vertex[0].s = tmp_cmd.BltVertex_STORAGE.vertex[2].s = (GLfloat)r1.right / (GLfloat)cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwWidth;
			tmp_cmd.BltVertex_STORAGE.vertex[0].t = tmp_cmd.BltVertex_STORAGE.vertex[1].t = (GLfloat)r1.top / (GLfloat)cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwHeight;
			tmp_cmd.BltVertex_STORAGE.vertex[2].t = tmp_cmd.BltVertex_STORAGE.vertex[3].t = (GLfloat)r1.bottom / (GLfloat)cmd->Blt.cmd.src->levels[cmd->Blt.cmd.srclevel].ddsd.dwHeight;
			if (usedest)
			{
				tmp_cmd.BltVertex_STORAGE.vertex[1].dests =
					tmp_cmd.BltVertex_STORAGE.vertex[3].dests = 0.;
				tmp_cmd.BltVertex_STORAGE.vertex[0].dests =
					tmp_cmd.BltVertex_STORAGE.vertex[2].dests = (GLfloat)(r1.right - r1.left) / (GLfloat)This->backbuffer->levels[0].ddsd.dwWidth;
				tmp_cmd.BltVertex_STORAGE.vertex[0].destt =
					tmp_cmd.BltVertex_STORAGE.vertex[1].destt = 1.;
				tmp_cmd.BltVertex_STORAGE.vertex[2].destt =
					tmp_cmd.BltVertex_STORAGE.vertex[3].destt = 1.0 - ((GLfloat)(r1.bottom - r1.top) / (GLfloat)This->backbuffer->levels[0].ddsd.dwHeight);
			}
			if (cmd->Blt.cmd.flags & 0x10000000)
			{
				tmp_cmd.BltVertex_STORAGE.vertex[1].stencils = tmp_cmd.BltVertex_STORAGE.vertex[3].stencils = tmp_cmd.BltVertex_STORAGE.vertex[1].x / (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwWidth;
				tmp_cmd.BltVertex_STORAGE.vertex[0].stencils = tmp_cmd.BltVertex_STORAGE.vertex[2].stencils = tmp_cmd.BltVertex_STORAGE.vertex[0].x / (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwWidth;
				tmp_cmd.BltVertex_STORAGE.vertex[0].stencilt = tmp_cmd.BltVertex_STORAGE.vertex[1].stencilt = tmp_cmd.BltVertex_STORAGE.vertex[0].y / (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight;
				tmp_cmd.BltVertex_STORAGE.vertex[2].stencilt = tmp_cmd.BltVertex_STORAGE.vertex[3].stencilt = tmp_cmd.BltVertex_STORAGE.vertex[2].y / (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight;
			}
			// Rotate vertices if necessary
			if ((cmd->Blt.cmd.bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->Blt.cmd.flags & DDBLT_DDFX))
			{
				if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT)
					BltFlipLR(tmp_cmd.BltVertex_STORAGE.vertex);
				if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_MIRRORUPDOWN)
					BltFlipUD(tmp_cmd.BltVertex_STORAGE.vertex);
				if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_ROTATE90) rotates++;
				if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_ROTATE180) rotates += 2;
				if (cmd->Blt.cmd.bltfx.dwDDFX & DDBLTFX_ROTATE270) rotates += 3;
				rotates &= 3;
				if (rotates)
				{
					RotateBlt90(tmp_cmd.BltVertex_STORAGE.vertex, rotates);
				}
			}
			// Create command and check buffers
			Vertex2DCmd cmdout;
			cmdout.opcode = OP_VERTEX2D;
			cmdout.size = sizeof(Vertex2DCmd) - 8;
			cmdout.count = 4;
			cmdout.indexcount = 6;
			if (usedest) cmdout.flags = 1;
			else cmdout.flags = 0;
			CheckCmdBuffer(This, cmdout.size + 8, 0, 4*sizeof(BltVertex), 6*sizeof(WORD));
			// Write vertices to VBO
			cmdout.offset = This->state.cmd->write_ptr_vertex;
			cmdout.indexoffset = This->state.cmd->write_ptr_index;
			memcpy(This->state.cmd->vertices->pointer + This->state.cmd->write_ptr_vertex,
				tmp_cmd.BltVertex_STORAGE.vertex, 4 * sizeof(BltVertex));
			memcpy(This->state.cmd->indices->pointer + This->state.cmd->write_ptr_index,
				indexbase, 6 * sizeof(WORD));
			This->state.cmd->write_ptr_vertex += 4 * sizeof(BltVertex);
			This->state.cmd->write_ptr_index += 6 * sizeof(WORD);
			// Write command to buffer
			memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, &cmdout, cmdout.size + 8);
			This->state.cmd->write_ptr_cmd_modify = This->state.cmd->write_ptr_cmd;
			This->state.cmd->write_ptr_cmd += (cmdout.size + 8);
		}
		error = DD_OK;
		break;
	case OP_DRAWSCREEN:  // Draws the screen.  Flip command buffer after executing.
		// First get window dimensions and adjust renderer window if necessary
		if (cmd->DrawScreen.texture->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			GetClientRect(This->hWnd, &r1);
			GetClientRect(This->RenderWnd->GetHWnd(), &r2);
			if (memcmp(&r2, &r1, sizeof(RECT)))
				SetWindowPos(This->RenderWnd->GetHWnd(), NULL, 0, 0, r1.right, r1.bottom, SWP_SHOWWINDOW);
		}
		LONG sizes[6];
		GLfloat view[4];
		GLint viewport[4];
		This->ddInterface->GetSizes(sizes);
		if (cmd->DrawScreen.texture->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			if (This->ddInterface->GetFullscreen())
			{
				viewport[0] = viewport[1] = 0;
				viewport[2] = sizes[4];
				viewport[3] = sizes[5];
				view[0] = (GLfloat)-(sizes[4] - sizes[0]) / 2;
				view[1] = (GLfloat)(sizes[4] - sizes[0]) / 2 + sizes[0];
				view[2] = (GLfloat)(sizes[5] - sizes[1]) / 2 + sizes[1];
				view[3] = (GLfloat)-(sizes[5] - sizes[1]) / 2;
			}
			else
			{
				viewport[0] = viewport[1] = 0;
				viewport[2] = r2.right;
				viewport[3] = r2.bottom;
				ClientToScreen(This->RenderWnd->GetHWnd(), (LPPOINT)&r2.left);
				ClientToScreen(This->RenderWnd->GetHWnd(), (LPPOINT)&r2.right);
				view[0] = (GLfloat)r2.left;
				view[1] = (GLfloat)r2.right;
				view[2] = (GLfloat)cmd->DrawScreen.texture->bigheight - (GLfloat)r2.top;
				view[3] = (GLfloat)cmd->DrawScreen.texture->bigheight - (GLfloat)r2.bottom;
			}
		}
		else
		{
			view[0] = 0;
			view[1] = (GLfloat)cmd->DrawScreen.texture->bigwidth;
			view[2] = 0;
			view[3] = (GLfloat)cmd->DrawScreen.texture->bigheight;
		}
		//  Set render mode to 2D
		if (This->state.mode_3d)
		{
			tmp_cmd.SetMode3D.opcode = OP_SETMODE3D;
			tmp_cmd.SetMode3D.size = sizeof(SetMode3DCmd) - 8;
			tmp_cmd.SetMode3D.enable = FALSE;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		}
		//  Mark primary as front buffer and clear frontbuffer bit on previous
		tmp_cmd.SetFrontBufferBits.opcode = OP_SETFRONTBUFFERBITS;
		tmp_cmd.SetFrontBufferBits.size = sizeof(SetFrontBufferBitsCmd) - 8;
		tmp_cmd.SetFrontBufferBits.front = cmd->DrawScreen.texture;
		tmp_cmd.SetFrontBufferBits.back = cmd->DrawScreen.previous;
		glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		//  Set Src texture (Unit 8) to primary
		i = 0;
		tmp_cmd.SetTexture.opcode = OP_SETTEXTURE;
		tmp_cmd.SetTexture.size = sizeof(SetTextureCmd);
		tmp_cmd.SetTexture.texstage[0].stage = 8;
		tmp_cmd.SetTexture.texstage[0].texture = cmd->DrawScreen.texture;
		//  Set Palette texture (Unit 9) to palette if 8-bit
		if (This->ddInterface->GetBPP() == 8)
		{
			i++;
			tmp_cmd.SetTexture.size += (sizeof(DWORD) + sizeof(glTexture*));
			tmp_cmd.SetTexture.texstage[1].stage = 9;
			tmp_cmd.SetTexture.texstage[1].texture = cmd->DrawScreen.paltex;
		}
		tmp_cmd.SetTexture.count = i + 1;
		glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		// If 8 bit scaled linear:
		if ((This->ddInterface->GetBPP() == 8) && (dxglcfg.scalingfilter) &&
			((cmd->DrawScreen.texture->bigwidth != (view[1]-view[0])) ||
			(cmd->DrawScreen.texture->bigheight != (view[3]-view[2]))))
		{
			// Check backbuffer size and resize
			if ((This->backbuffer->levels[0].ddsd.dwWidth < cmd->DrawScreen.texture->bigwidth) ||
				(This->backbuffer->levels[0].ddsd.dwHeight < cmd->DrawScreen.texture->bigheight))
			{
				DDSURFACEDESC2 newdesc = This->backbuffer->levels[0].ddsd;
				if (newdesc.dwWidth < cmd->DrawScreen.texture->bigwidth)
					newdesc.dwWidth = cmd->DrawScreen.texture->bigwidth;
				if (newdesc.dwHeight < cmd->DrawScreen.texture->bigheight)
					newdesc.dwHeight = cmd->DrawScreen.texture->bigheight;
				tmp_cmd.SetTextureSurfaceDesc.opcode = OP_SETTEXTURESURFACEDESC;
				tmp_cmd.SetTextureSurfaceDesc.size = sizeof(SetTextureSurfaceDescCmd) - 8;
				tmp_cmd.SetTextureSurfaceDesc.level = 0;
				tmp_cmd.SetTextureSurfaceDesc.desc = newdesc;
				glRenderer_AddCommand(This, &tmp_cmd, TRUE, TRUE);
			}
			//  Set shader to 256-color palette
			tmp_cmd.SetShader2D.opcode = OP_SETSHADER2D;
			tmp_cmd.SetShader2D.size = sizeof(SetShader2DCmd) - 8;
			tmp_cmd.SetShader2D.type = 0;
			tmp_cmd.SetShader2D.id = PROG_PAL256;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			//  Set shader uniforms
			tmp_cmd.SetUniform.opcode = OP_SETUNIFORM;
			tmp_cmd.SetUniform.size = sizeof(SetUniformCmdBase) - 8;
			tmp_cmd.SetUniform.tmp_ptr = (BYTE*)&tmp_cmd.SetUniform.data;
			tmp_cmd.SetUniform.count = 0;
			append_uniform_cmd(This, &tmp_cmd.SetUniform, This->shaders->shaders[PROG_PAL256].tex0,
				0, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), 8);
			append_uniform_cmd(This, &tmp_cmd.SetUniform, This->shaders->shaders[PROG_PAL256].pal,
				0, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), 9);
			append_uniform_cmd(This, &tmp_cmd.SetUniform, This->shaders->shaders[PROG_PAL256].view,
				7, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), view[0], view[1], view[2], view[3]);
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			//  Set shader attributes
			tmp_cmd.SetAttrib.opcode = OP_SETATTRIB;
			tmp_cmd.SetAttrib.size = sizeof(SetAttribCmdBase) - 8;
			tmp_cmd.SetAttrib.count = 0;
			vertex = (BltVertex*)This->state.cmd->write_ptr_vertex;
			append_attrib_cmd(This, &tmp_cmd.SetAttrib, This->shaders->shaders[PROG_PAL256].pos, 2, GL_FLOAT, GL_FALSE,
				sizeof(BltVertex), &vertex->x, TRUE, sizeof(MIN_STORAGE_CMD));
			append_attrib_cmd(This, &tmp_cmd.SetAttrib, This->shaders->shaders[PROG_PAL256].texcoord, 2, GL_FLOAT, GL_FALSE,
				sizeof(BltVertex), &vertex->s, TRUE, sizeof(MIN_STORAGE_CMD));
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			//  Set render target to backbuffer
			if ((This->state.target.target != This->backbuffer) || (This->state.target.target != 0))
			{
				tmp_cmd.SetRenderTarget.opcode = OP_SETRENDERTARGET;
				tmp_cmd.SetRenderTarget.size = sizeof(SetRenderTargetCmd) - 8;
				tmp_cmd.SetRenderTarget.target.target = This->backbuffer;
				tmp_cmd.SetRenderTarget.target.level = 0;
				tmp_cmd.SetRenderTarget.target.zbuffer = NULL;
				tmp_cmd.SetRenderTarget.target.zlevel = 0;
				tmp_cmd.SetRenderTarget.target.mulx = tmp_cmd.SetRenderTarget.target.muly = 1.0f;
				glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			}
			//  Set viewport to backbuffer
			r2.right = This->backbuffer->levels[0].ddsd.dwWidth;
			r2.bottom = This->backbuffer->levels[0].ddsd.dwHeight;
			if (This->state.viewport.x || This->state.viewport.y ||
				(This->state.viewport.width != r2.right) || (This->state.viewport.hieght != r2.bottom))
			{
				tmp_cmd.SetViewport.opcode = OP_SETVIEWPORT;
				tmp_cmd.SetViewport.size = sizeof(SetViewportCmd) - 8;
				tmp_cmd.SetViewport.viewport.x = tmp_cmd.SetViewport.viewport.y = 0;
				tmp_cmd.SetViewport.viewport.width = r2.right;
				tmp_cmd.SetViewport.viewport.hieght = r2.bottom;
				glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			}
			//  Generate vertices
			r2.left = r2.top = 0;
			r2.right = cmd->DrawScreen.texture->bigwidth;
			r2.bottom = cmd->DrawScreen.texture->bigheight;
			tmp_cmd.BltVertex_STORAGE.vertex[1].x = tmp_cmd.BltVertex_STORAGE.vertex[3].x = (GLfloat)r2.left;
			tmp_cmd.BltVertex_STORAGE.vertex[0].x = tmp_cmd.BltVertex_STORAGE.vertex[2].x = (GLfloat)r2.right;
			tmp_cmd.BltVertex_STORAGE.vertex[0].y = tmp_cmd.BltVertex_STORAGE.vertex[1].y = (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight - (GLfloat)r2.top;
			tmp_cmd.BltVertex_STORAGE.vertex[2].y = tmp_cmd.BltVertex_STORAGE.vertex[3].y = (GLfloat)cmd->Blt.cmd.dest->levels[cmd->Blt.cmd.destlevel].ddsd.dwHeight - (GLfloat)r2.bottom;
			tmp_cmd.BltVertex_STORAGE.vertex[1].s = tmp_cmd.BltVertex_STORAGE.vertex[3].s =
				tmp_cmd.BltVertex_STORAGE.vertex[0].t = tmp_cmd.BltVertex_STORAGE.vertex[1].t = 0.0f;
			tmp_cmd.BltVertex_STORAGE.vertex[0].s = tmp_cmd.BltVertex_STORAGE.vertex[2].s =
				tmp_cmd.BltVertex_STORAGE.vertex[2].t = tmp_cmd.BltVertex_STORAGE.vertex[3].t = 1.0f;
			//  Create Palette draw command and check buffers
			Vertex2DCmd cmdout;
			cmdout.opcode = sizeof(Vertex2DCmd) - 8;
			cmdout.count = 4;
			cmdout.indexcount = 6;
			cmdout.flags = 0;
			CheckCmdBuffer(This, cmdout.size + 8, 0, 4 * sizeof(BltVertex), 6 * sizeof(WORD));
			//  Write vertices to VBO
			cmdout.offset = This->state.cmd->write_ptr_vertex;
			cmdout.indexoffset = This->state.cmd->write_ptr_index;
			memcpy(This->state.cmd->vertices->pointer + This->state.cmd->write_ptr_vertex,
				tmp_cmd.BltVertex_STORAGE.vertex, 4 * sizeof(BltVertex));
			memcpy(This->state.cmd->indices->pointer + This->state.cmd->write_ptr_index,
				indexbase, 6 * sizeof(WORD));
			This->state.cmd->write_ptr_vertex += 4 * sizeof(BltVertex);
			This->state.cmd->write_ptr_index += 6 * sizeof(WORD);
			// Write command to buffer
			memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, &cmdout, cmdout.size + 8);
			This->state.cmd->write_ptr_cmd_modify = This->state.cmd->write_ptr_cmd;
			This->state.cmd->write_ptr_cmd += (cmdout.size + 8);
			//  Set Src texture (Unit 8) to backbuffer
			tmp_cmd.SetTexture.opcode = OP_SETTEXTURE;
			tmp_cmd.SetTexture.size = sizeof(SetTextureCmd) + sizeof(DWORD) + sizeof(glTexture*);
			tmp_cmd.SetTexture.texstage[0].stage = 8;
			tmp_cmd.SetTexture.texstage[0].texture = This->backbuffer;
			tmp_cmd.SetTexture.count = 1;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		}
		//  Set shader and texture filter
		if ((This->ddInterface->GetBPP() == 8) && (!dxglcfg.scalingfilter ||
			((cmd->DrawScreen.texture->bigwidth == (view[1] - view[0])) &&
			(cmd->DrawScreen.texture->bigheight == (view[3] - view[2])))))
		{
			// 256 color, unscaled or scaled nearest
			tmp_cmd.SetShader2D.opcode = OP_SETSHADER2D;
			tmp_cmd.SetShader2D.size = sizeof(SetShader2DCmd) - 8;
			tmp_cmd.SetShader2D.type = 0;
			tmp_cmd.SetShader2D.id = PROG_PAL256;
			shaderid = PROG_PAL256;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			tmp_cmd.SetTextureStageState.opcode = OP_SETTEXTURESTAGESTATE;
			tmp_cmd.SetTextureStageState.size = sizeof(SetTextureStageStateCmd) - 8 + sizeof(DWORD)
				+ sizeof(D3DTEXTURESTAGESTATETYPE);
			tmp_cmd.SetTextureStageState.dwStage = 8;
			tmp_cmd.SetTextureStageState.count = 2;
			tmp_cmd.SetTextureStageState.state[0].dwState = D3DTSS_MAGFILTER;
			tmp_cmd.SetTextureStageState.state[0].dwValue = D3DTFG_POINT;
			tmp_cmd.SetTextureStageState.state[1].dwState = D3DTSS_MINFILTER;
			tmp_cmd.SetTextureStageState.state[1].dwValue = D3DTFN_POINT;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		}
		else
		{
			// Full-color mode or 256-color scaled linear
			tmp_cmd.SetShader2D.opcode = OP_SETSHADER2D;
			tmp_cmd.SetShader2D.size = sizeof(SetShader2DCmd) - 8;
			tmp_cmd.SetShader2D.type = 0;
			tmp_cmd.SetShader2D.id = PROG_TEXTURE;
			shaderid = PROG_TEXTURE;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
			tmp_cmd.SetTextureStageState.opcode = OP_SETTEXTURESTAGESTATE;
			tmp_cmd.SetTextureStageState.size = sizeof(SetTextureStageStateCmd) - 8 + sizeof(DWORD)
				+ sizeof(D3DTEXTURESTAGESTATETYPE);
			tmp_cmd.SetTextureStageState.dwStage = 8;
			tmp_cmd.SetTextureStageState.count = 2;
			tmp_cmd.SetTextureStageState.state[0].dwState = D3DTSS_MAGFILTER;
			if(dxglcfg.scalingfilter)
				tmp_cmd.SetTextureStageState.state[0].dwValue = D3DTFG_LINEAR;
			else tmp_cmd.SetTextureStageState.state[0].dwValue = D3DTFG_POINT;
			tmp_cmd.SetTextureStageState.state[1].dwState = D3DTSS_MINFILTER;
			if (dxglcfg.scalingfilter)
				tmp_cmd.SetTextureStageState.state[1].dwValue = D3DTFG_LINEAR;
			else tmp_cmd.SetTextureStageState.state[1].dwValue = D3DTFN_POINT;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		}
		//  Set shader uniforms
		tmp_cmd.SetUniform.opcode = OP_SETUNIFORM;
		tmp_cmd.SetUniform.size = sizeof(SetUniformCmdBase) - 8;
		tmp_cmd.SetUniform.tmp_ptr = (BYTE*)&tmp_cmd.SetUniform.data;
		tmp_cmd.SetUniform.count = 0;
		append_uniform_cmd(This, &tmp_cmd.SetUniform, This->shaders->shaders[shaderid].tex0,
			0, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), 8);
		if (shaderid == PROG_PAL256)
			append_uniform_cmd(This, &tmp_cmd.SetUniform, This->shaders->shaders[shaderid].pal,
				0, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), 9);
		append_uniform_cmd(This, &tmp_cmd.SetUniform, This->shaders->shaders[shaderid].view,
			7, 0, FALSE, TRUE, sizeof(MIN_STORAGE_CMD), view[0], view[1], view[2], view[3]);
		glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		//  Set shader attributes
		tmp_cmd.SetAttrib.opcode = OP_SETATTRIB;
		tmp_cmd.SetAttrib.size = sizeof(SetAttribCmdBase) - 8;
		tmp_cmd.SetAttrib.count = 0;
		vertex = (BltVertex*)This->state.cmd->write_ptr_vertex;
		append_attrib_cmd(This, &tmp_cmd.SetAttrib, This->shaders->shaders[shaderid].pos, 2, GL_FLOAT, GL_FALSE,
			sizeof(BltVertex), &vertex->x, TRUE, sizeof(MIN_STORAGE_CMD));
		append_attrib_cmd(This, &tmp_cmd.SetAttrib, This->shaders->shaders[shaderid].pos, 2, GL_FLOAT, GL_FALSE,
			sizeof(BltVertex), &vertex->s, TRUE, sizeof(MIN_STORAGE_CMD));
		glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		//  Set render target to null
		if (This->state.target.target)
		{
			tmp_cmd.SetRenderTarget.opcode = OP_SETRENDERTARGET;
			tmp_cmd.SetRenderTarget.size = sizeof(SetRenderTargetCmd) - 8;
			tmp_cmd.SetRenderTarget.target.target = NULL;
			tmp_cmd.SetRenderTarget.target.level = 0;
			tmp_cmd.SetRenderTarget.target.zbuffer = NULL;
			tmp_cmd.SetRenderTarget.target.zlevel = 0;
			tmp_cmd.SetRenderTarget.target.mulx = tmp_cmd.SetRenderTarget.target.muly = 1.0f;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		}
		//  Set viewport to window buffer
		glUtil_SetViewport(This->util, viewport[0], viewport[1], viewport[2], viewport[3]);
		if (This->state.viewport.x != viewport[0] || This->state.viewport.y != viewport[1] ||
			(This->state.viewport.width != viewport[2]) || (This->state.viewport.hieght != viewport[3]))
		{
			tmp_cmd.SetViewport.opcode = OP_SETVIEWPORT;
			tmp_cmd.SetViewport.size = sizeof(SetViewportCmd) - 8;
			tmp_cmd.SetViewport.viewport.x = viewport[0];
			tmp_cmd.SetViewport.viewport.y = viewport[1];
			tmp_cmd.SetViewport.viewport.width = viewport[2];
			tmp_cmd.SetViewport.viewport.hieght = viewport[3];
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		}
		//  Generate vertices
		if (This->ddInterface->GetFullscreen())
		{
			This->bltvertices[0].x = This->bltvertices[2].x = (GLfloat)sizes[0];
			This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
			This->bltvertices[2].y = This->bltvertices[3].y = (GLfloat)sizes[1];
		}
		else
		{
			This->bltvertices[0].x = This->bltvertices[2].x = (GLfloat)cmd->DrawScreen.texture->bigwidth;
			This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
			This->bltvertices[2].y = This->bltvertices[3].y = (GLfloat)cmd->DrawScreen.texture->bigheight;
		}
		if ((This->ddInterface->GetBPP() == 8) && (dxglcfg.scalingfilter) &&
			((cmd->DrawScreen.texture->bigwidth != (view[1] - view[0])) ||
			(cmd->DrawScreen.texture->bigheight != (view[3] - view[2]))))
		{
			This->bltvertices[0].s = This->bltvertices[2].s = 
				(GLfloat)cmd->DrawScreen.texture->bigwidth / (GLfloat)This->backbuffer->bigwidth;
			This->bltvertices[0].t = This->bltvertices[1].t =
				(GLfloat)cmd->DrawScreen.texture->bigheight / (GLfloat)This->backbuffer->bigheight;
			This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.0f;
		}
		else
		{
			This->bltvertices[0].s = This->bltvertices[0].t = This->bltvertices[1].t = This->bltvertices[2].s = 1.0f;
			This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.0f;
		}
		//  Create command and check buffers
		Vertex2DCmd cmdout;
		cmdout.opcode = OP_VERTEX2D;
		cmdout.size = sizeof(Vertex2DCmd) - 8;
		cmdout.count = 4;
		cmdout.indexcount = 6;
		cmdout.flags = 0;
		CheckCmdBuffer(This, cmdout.size + 8, 0, 4 * sizeof(BltVertex), 6 * sizeof(WORD));
		//  Write vertices to VBO
		cmdout.offset = This->state.cmd->write_ptr_vertex;
		cmdout.indexoffset = This->state.cmd->write_ptr_index;
		memcpy(This->state.cmd->vertices->pointer + This->state.cmd->write_ptr_vertex,
			tmp_cmd.BltVertex_STORAGE.vertex, 4 * sizeof(BltVertex));
		memcpy(This->state.cmd->indices->pointer + This->state.cmd->write_ptr_index,
			indexbase, 6 * sizeof(WORD));
		This->state.cmd->write_ptr_vertex += 4 * sizeof(BltVertex);
		This->state.cmd->write_ptr_index += 6 * sizeof(WORD);
		// Write command to buffer
		memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, &cmdout, cmdout.size + 8);
		This->state.cmd->write_ptr_cmd_modify = This->state.cmd->write_ptr_cmd;
		This->state.cmd->write_ptr_cmd += (cmdout.size + 8);
		//  Set swap interval
		tmp_cmd.SetSwap.opcode = OP_SETSWAP;
		tmp_cmd.SetSwap.size = sizeof(GLint);
		tmp_cmd.SetSwap.interval = cmd->DrawScreen.vsync;
		glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		//  Swap buffers
		tmp_cmd.SwapBuffers.opcode = OP_SWAPBUFFERS;
		tmp_cmd.SwapBuffers.size = 0;
		glRenderer_AddCommand(This, &tmp_cmd, TRUE, TRUE);
		error = DD_OK;
		break;
	case OP_INITD3D:  // Initialize renderer for Direct3D rendering.
		// Set initial viewport
		tmp_cmd.SetD3DViewport.opcode = OP_SETD3DVIEWPORT;
		tmp_cmd.SetD3DViewport.size = sizeof(SetD3DViewportCmd) - 8;
		tmp_cmd.SetD3DViewport.viewport.dwX = 0;
		tmp_cmd.SetD3DViewport.viewport.dwY = 0;
		tmp_cmd.SetD3DViewport.viewport.dwWidth = cmd->InitD3D.x;
		tmp_cmd.SetD3DViewport.viewport.dwHeight = cmd->InitD3D.y;
		tmp_cmd.SetD3DViewport.viewport.dvMinZ = 0.0f;
		tmp_cmd.SetD3DViewport.viewport.dvMaxZ = 1.0f;
		glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		// Initialize texture stages 0 through 7
		tmp_cmd.InitTextureStage.opcode = OP_INITTEXTURESTAGE;
		tmp_cmd.InitTextureStage.size = sizeof(InitTextureStageCmd) - 8 + (7 * sizeof(DWORD));
		tmp_cmd.InitTextureStage.count = 8;
		for (i = 0; i < 8; i++)
			tmp_cmd.InitTextureStage.stage[i] = i;
		glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		// Post InitD3D command
		CheckCmdBuffer(This, cmd->InitD3D.size + 8, 0, 0, 0);
		memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, cmd, cmd->InitD3D.size + 8);
		This->state.cmd->write_ptr_cmd += (cmd->InitD3D.size + 8);
		error = DD_OK;
		break;
	case OP_CLEAR:  // Clears full renderbuffer or one or more rects.
					// Size should hold number of rects plus each rect, or 0 for screen.
		CheckCmdBuffer(This, cmd->Clear.size + 8, 0, 0, 0);
		memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, cmd, cmd->Clear.size + 8);
		This->state.cmd->write_ptr_cmd += (cmd->Clear.size + 8);
		error = DD_OK;
		break;
	case OP_FLUSH:  // Probably should consider retiring this one.  Flip buffers if called
					// to ensure the renderer gets flushed.
		CheckCmdBuffer(This, cmd->Flush.size + 8, 0, 0, 0);
		memcpy(This->state.cmd->cmdbuffer + This->state.cmd->write_ptr_cmd, cmd, cmd->Flush.size + 8);
		This->state.cmd->write_ptr_cmd += (cmd->Flush.size + 8);
		error = DDERR_CURRENTLYNOTAVAIL;
		flip = TRUE;
		break;
	case OP_DRAWPRIMITIVES:  // Add primitives to the render buffer, check and adjust buffer
		                     // state.
		// Set render mode to 3D
		if (!This->state.mode_3d)
		{
			tmp_cmd.SetMode3D.opcode = OP_SETMODE3D;
			tmp_cmd.SetMode3D.size = sizeof(SetMode3DCmd) - 8;
			tmp_cmd.SetMode3D.enable = TRUE;
			glRenderer_AddCommand(This, &tmp_cmd, TRUE, FALSE);
		}
		if((This->state.last_cmd.DrawPrimitives.opcode == OP_DRAWPRIMITIVES) &&
			(cmd->DrawPrimitives.mode == This->state.last_cmd.DrawPrimitives.mode) &&
			(cmd->DrawPrimitives.target == This->state.last_cmd.DrawPrimitives.target) &&
			!memcmp(cmd->DrawPrimitives.texformats, This->state.last_cmd.DrawPrimitives.texformats, 8 * sizeof(int)))
		{}
		// check for zbuffer on target
		// check if transformed
		// if no vertex positions fail
		// AND shaderstate with 0xFFFA3FF87FFFFFFFi64
		// AND all texstage ids with 0xFFE7FFFFFFFFFFFFi64
		// Set bit 51 of texture id for each texture stage in vertices
		// add number of textures to shaderstate bits 31-33
		// If more than 0 textures used set bit 34 of shaderstate
		// Set bits 46-48 of shaderstate to number of blendweights
		// Set bit 50 of shaderstate if vertices are pre-transformed
		// Set bit 35 of shaderstate if diffuse color is in vertices
		// Set bit 36 of shaderstate if specular color is in vertices
		// Set bit 37 of shaderstate if vertices have normals
		// Set shader program
		// Tell renderer to set depth compare mode, depth test, and depth write
		// Set shader attributes
		// Set modelview matrix
		// Set projection matrix
		// Set material
		// Set lights
		// Set ambient color uniform
		// For each texture stage:
		// 	Skip if colorop is disable
		// 	Upload texture if dirty
		// 	Set texure filter
		// 	Bind texture to texture unit
		// 	Set texture wrap
		// 	Set shader uniform texture unit
		// 	Set color key
		// Set viewport uniforms
		// Set alpha reference
		// Set color bits uniform
		// Set rendertarget to D3D target
		// Set viewport to D3D viewport
		// Set depth range
		// Set blend enable mode
		// Set blend mode
		// Set cull mode
		// Set fog color, start, end, density
		// Set polygon mode
		// Set shade mode
		// Create command and check buffers
		// Write vertices to VBO
		// Write command to buffer
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_UPDATECLIPPER:  // Add pre-processed vertices to update the clipper.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_DEPTHFILL:  // Performs a depth fill on a depth surface, using a BltCommand structure.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETRENDERSTATE:  // Sets a Direct3D Render State.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETTEXTURE:  // Binds a texture object to a Direct3D texture stage.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETTEXTURESTAGESTATE:  // Sets a state object for a Direct3D texture stage.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETTRANSFORM:  // Sets one of the Direct3D fixed-function matrices.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETMATERIAL:  // Sets the Direct3D fixed-function material.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETLIGHT:  // Sets one of the Direct3D fixed-function light states.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETD3DVIEWPORT:  // Sets the Direct3D viewport.  May be 
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETTEXTURECOLORKEY:  // Sets a color key or colorkey range on a texture object.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_MAKETEXTUREPRIMARY:  // Sets or removes primary scaling on a z-buffer.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_DXGLBREAK:  // Breakpoint command, flip buffers and wait or just add to command stream?
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_ENDCOMMAND:  // Not for this section.
		error = DDERR_INVALIDPARAMS;
		break;
	case OP_INITTEXTURESTAGE:  // Initializes a texture stage.
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETTEXTURESURFACEDESC:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETSHADER2D:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETSHADER:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETRENDERTARGET:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETVIEWPORT:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_VERTEX2D:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETD3DDEPTHMODE:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_BLENDENABLE:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETDEPTHTEST:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETFRONTBUFFERBITS:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETSWAP:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SWAPBUFFERS:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETUNIFORM:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETATTRIB:
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	case OP_SETMODE3D:
		//  Disable depth test
		// Disable blending
		// Disable polygon culling
		// Set fill mode to solid
		error = DDERR_CURRENTLYNOTAVAIL;
		break;
	default:
		FIXME("Unknown opcode detected.");
		error = DDERR_INVALIDPARAMS;
		break;
	}
	This->state.last_cmd.Generic.opcode = cmd->Generic.opcode;
	if (!inner) LeaveCriticalSection(&This->cs);
	return error;
}

/**
  * Sets the Windows OpenGL swap interval
  * @param This
  *  Pointer to glRenderer object
  * @param swap
  *  Number of vertical retraces to wait per frame, 0 disable vsync
  */
void glRenderer__SetSwap(glRenderer *This, int swap)
{
	if (dxglcfg.vsync == 1) swap = 0;
	else if (dxglcfg.vsync == 2) swap = 1;
	if(swap != This->oldswap)
	{
		This->ext->wglSwapIntervalEXT(swap);
		This->ext->wglGetSwapIntervalEXT();
		This->oldswap = swap;
	}
}

/**
  * glRenderer wrapper for glTexture__Upload
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to upload to
  * @param level
  *  Mipmap level of texture to write
  */
void glRenderer__UploadTexture(glRenderer *This, glTexture *texture, GLint level)
{
	glTexture__Upload(texture, level);
}

/**
  * glRenderer wrapper function for glTexture__Download
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to download from
  * @param level
  *  Mipmap level of texture to read
  */
void glRenderer__DownloadTexture(glRenderer *This, glTexture *texture, GLint level) 
{
	glTexture__Download(texture, level);
}

/**
  * Initializes a glRenderer object
  * @param This
  *  Pointer to glRenderer object to initialize
  * @param width,height,bpp
  *  Width, height, and BPP of the rendering window
  * @param fullscreen
  *  True if fullscreen mode is required, false for windowed
  * @param hwnd
  *  Handle of the window to render into.  If this value is NULL, then a transparent
  *  layered window will be created for the renderer.
  * @param glDD7
  *  Pointer to the glDirectDraw7 object that is managing the glRenderer object
  * @param devwnd
  *  True if creating window with name "DirectDrawDeviceWnd"
  */
void glRenderer_Init(glRenderer *This, int width, int height, int bpp, BOOL fullscreen, unsigned int frequency, HWND hwnd, glDirectDraw7 *glDD7, BOOL devwnd)
{
	RECT wndrect;
	WINDOWPLACEMENT wndplace;
	int screenx, screeny;
	LONG_PTR winstyle, winstyleex;
	This->oldswap = 0;
	This->fogcolor = 0;
	This->fogstart = 0.0f;
	This->fogend = 1.0f;
	This->fogdensity = 1.0f;
	This->backbuffer = NULL;
	This->hDC = NULL;
	This->hRC = NULL;
	This->pbo = NULL;
	This->last_fvf = 0xFFFFFFFF; // Bogus value to force initial FVF change
	This->mode_3d = FALSE;
	ZeroMemory(&This->dib, sizeof(DIB));
	This->hWnd = hwnd;
	InitializeCriticalSection(&This->cs);
	This->busy = CreateEvent(NULL,FALSE,FALSE,NULL);
	This->start = CreateEvent(NULL,FALSE,FALSE,NULL);
	if(fullscreen)
	{
		switch (dxglcfg.fullmode)
		{
		case 0:    // Fullscreen
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, (winstyle | WS_POPUP) & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER));
			ShowWindow(This->hWnd, SW_MAXIMIZE);
			break;
		case 1:    // Non-exclusive Fullscreen
		case 5:    // Windowed borderless scaled
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(This->hWnd, SW_MAXIMIZE);
			break;
		case 2:     // Windowed non-resizable
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP));
			ShowWindow(This->hWnd, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect.left = dxglcfg.WindowX;
				wndrect.top = dxglcfg.WindowY;
				wndrect.right = dxglcfg.WindowX + dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowY + dxglcfg.WindowHeight;
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.right = dxglcfg.WindowWidth + (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.bottom = dxglcfg.WindowHeight + (screeny / 2) - (dxglcfg.WindowHeight / 2);
				wndrect.left = (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.top = (screeny / 2) - (dxglcfg.WindowHeight / 2);
			}
			AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
				(winstyleex | WS_EX_APPWINDOW));
			SetWindowPos(This->hWnd, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			break;
		case 3:     // Windowed resizable
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP);
			ShowWindow(This->hWnd, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect.left = dxglcfg.WindowX;
				wndrect.top = dxglcfg.WindowY;
				wndrect.right = dxglcfg.WindowX + dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowY + dxglcfg.WindowHeight;
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.right = dxglcfg.WindowWidth + (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.bottom = dxglcfg.WindowHeight + (screeny / 2) - (dxglcfg.WindowHeight / 2);
				wndrect.left = (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.top = (screeny / 2) - (dxglcfg.WindowHeight / 2);
			}
			AdjustWindowRectEx(&wndrect, winstyle | WS_OVERLAPPEDWINDOW, FALSE, (winstyleex | WS_EX_APPWINDOW));
			wndplace.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(This->hWnd, &wndplace);
			wndplace.flags = WPF_ASYNCWINDOWPLACEMENT;
			if (dxglcfg.WindowMaximized == 1) wndplace.showCmd = SW_SHOWMAXIMIZED;
			else wndplace.showCmd = SW_SHOWNORMAL;
			wndplace.rcNormalPosition = wndrect;
			SetWindowPlacement(This->hWnd, &wndplace);
			break;
		case 4:     // Windowed borderless
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(This->hWnd, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect.left = dxglcfg.WindowX;
				wndrect.top = dxglcfg.WindowY;
				wndrect.right = dxglcfg.WindowX + dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowY + dxglcfg.WindowHeight;
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.right = dxglcfg.WindowWidth + (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.bottom = dxglcfg.WindowHeight + (screeny / 2) - (dxglcfg.WindowHeight / 2);
				wndrect.left = (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.top = (screeny / 2) - (dxglcfg.WindowHeight / 2);
			}
			SetWindowPos(This->hWnd, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			break;
		}
	}
	if(width)
	{
		// TODO:  Adjust window rect
	}
	SetWindowPos(This->hWnd,HWND_TOP,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	This->RenderWnd = new glRenderWindow(width,height,fullscreen,This->hWnd,glDD7,devwnd);
	This->inputs[0] = (void*)width;
	This->inputs[1] = (void*)height;
	This->inputs[2] = (void*)bpp;
	This->inputs[3] = (void*)fullscreen;
	This->inputs[4] = (void*)frequency;
	This->inputs[5] = (void*)This->hWnd;
	This->inputs[6] = glDD7;
	This->inputs[7] = This;
	This->inputs[8] = (void*)devwnd;
	This->hThread = CreateThread(NULL, 0, glRenderer_ThreadEntry, This->inputs, 0, NULL);
	WaitForSingleObject(This->busy,INFINITE);
}

__inline BOOL UnadjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
	RECT r;
	ZeroMemory(&r, sizeof(RECT));
	BOOL ret = AdjustWindowRectEx(&r, dwStyle, bMenu, dwExStyle);
	if (!ret) return ret;
	else
	{
		lpRect->left -= r.left;
		lpRect->top -= r.top;
		lpRect->right -= r.right;
		lpRect->bottom -= r.bottom;
		return ret;
	}
}

/**
  * Deletes a glRenderer object
  * @param This
  *  Pointer to glRenderer object
  */
void glRenderer_Delete(glRenderer *This)
{
	BOOL hasmenu;
	RECT wndrect;
	LONG_PTR winstyle, winstyleex;
	WINDOWPLACEMENT wndplace;
	switch (dxglcfg.fullmode)
	{
	case 2:
	case 3:
	case 4:
		wndplace.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(This->hWnd, &wndplace);
		wndrect = wndplace.rcNormalPosition;
		if (dxglcfg.fullmode != 4)
		{
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			if (GetMenu(This->hWnd)) hasmenu = TRUE;
			else hasmenu = FALSE;
			UnadjustWindowRectEx(&wndrect, winstyle, hasmenu, winstyleex);
		}
		dxglcfg.WindowX = wndrect.left;
		dxglcfg.WindowY = wndrect.top;
		dxglcfg.WindowWidth = wndrect.right - wndrect.left;
		dxglcfg.WindowHeight = wndrect.bottom - wndrect.top;
		if (dxglcfg.fullmode == 3)
		{
			if (wndplace.showCmd == SW_MAXIMIZE) dxglcfg.WindowMaximized = TRUE;
			else dxglcfg.WindowMaximized = FALSE;
		}
		SaveWindowSettings(&dxglcfg);
		break;
	default:
		break;
	}
	EnterCriticalSection(&This->cs);
	This->opcode = OP_DELETE;
	SetEvent(This->start);
	WaitForObjectAndMessages(This->busy);
	CloseHandle(This->start);
	CloseHandle(This->busy);
	LeaveCriticalSection(&This->cs);
	DeleteCriticalSection(&This->cs);
	CloseHandle(This->hThread);
}

/**
  * Gets the BPP of the DirectDraw object that owns the renderer
  * @param This
  *  Pointer to glRenderer object
  */
DWORD glRenderer_GetBPP(glRenderer *This)
{
	return This->ddInterface->GetBPP();
}

/**
  * Entry point for the renderer thread
  * @param entry
  *  Pointer to the inputs passed by the CreateThread function
  */
DWORD WINAPI glRenderer_ThreadEntry(void *entry)
{
	void **inputsin = (void**)entry;
	glRenderer *This = (glRenderer*)inputsin[7];
	return glRenderer__Entry(This);
}

/**
  * Finishes creating an OpenGL texture.  
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to finish creating
  * @param wraps,wrapt
  *  OpenGL texture wrap parameters
  */
void glRenderer_MakeTexture(glRenderer *This, glTexture *texture)
{
	/*MakeTextureCmd cmd;
	cmd.opcode = OP_CREATE;
	cmd.size = sizeof(glTexture*);
	cmd.texture = texture;
	glRenderer_AddCommand(This, (BYTE*)&cmd);
	glRenderer_EndCommand(This, TRUE, FALSE);*/
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->opcode = OP_CREATE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Uploads the content of a surface to an OpenGL texture.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to upload to
  * @param level
  *  Mipmap level of texture to write
  */
void glRenderer_UploadTexture(glRenderer *This, glTexture *texture, GLint level) 
{
	/*UploadTextureCmd cmd;
	cmd.opcode = OP_UPLOAD;
	cmd.size = sizeof(UploadTextureCmd) - 8;
	cmd.texture = texture;
	cmd.level = level;
	glRenderer_AddCommand(This, (BYTE*)&cmd);*/
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)level;
	This->opcode = OP_UPLOAD;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Downloads the contents of an OpenGL texture to a the texture object's buffer.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to download from
  * @param level
  *  Mipmap level of texture to read
  */
void glRenderer_DownloadTexture(glRenderer *This, glTexture *texture, GLint level)
{
	/*DownloadTextureCmd cmd;
	cmd.opcode = OP_DOWNLOAD;
	cmd.size = sizeof(DownloadTextureCmd) - 8;
	cmd.texture = texture;
	cmd.level = level;
	glRenderer_AddCommand(This, (BYTE*)&cmd);
	glRenderer_EndCommand(This, TRUE, FALSE);*/
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)level;
	This->opcode = OP_DOWNLOAD;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Deletes an OpenGL texture.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  OpenGL texture to be deleted
  */
void glRenderer_DeleteTexture(glRenderer *This, glTexture * texture)
{
	/*DeleteTextureCmd cmd;
	cmd.opcode = OP_DELETE;
	cmd.size = sizeof(glTexture*);
	cmd.texture = texture;
	glRenderer_AddCommand(This, (BYTE*)&cmd);*/
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->opcode = OP_DELETETEX;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Copies part or all of the contents of one texture to another.
  * @param This
  *  Pointer to glRenderer object
  * @param cmd
  *  Pointer to structure contaning all paramaters for a Blt operation.
  * @return
  *  DD_OK if the call succeeds, or DDERR_WASSTILLDRAWING if queue is full and not waiting.
  */
HRESULT glRenderer_Blt(glRenderer *This, BltCommand *cmd)
{
	/*BltCmd bltcmd;
	bltcmd.opcode = OP_DELETE;
	bltcmd.zise = sizeof(BltCmd) - 8;
	bltcmd.cmd = *cmd;
	glRenderer_AddCommand(This, (BYTE*)&bltcmd);*/
	EnterCriticalSection(&This->cs);
	RECT r,r2;
	if(((cmd->dest->levels[0].ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(cmd->dest->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((cmd->dest->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(cmd->dest->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
	{
		GetClientRect(This->hWnd,&r);
		GetClientRect(This->RenderWnd->GetHWnd(),&r2);
		if(memcmp(&r2,&r,sizeof(RECT)) != 0)
			SetWindowPos(This->RenderWnd->GetHWnd(),NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
	}
	This->inputs[0] = cmd;
	This->opcode = OP_BLT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
  * Updates the display with the current primary texture.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture to use as the primary
  * @param paltex
  *  Texture that contains the color palette for 8-bit modes
  * @param vsync
  *  Vertical sync count
  * @param previous
  *  Texture previously used as primary before a flip
  */
void glRenderer_DrawScreen(glRenderer *This, glTexture *texture, glTexture *paltex, GLint vsync, glTexture *previous)
{
	/*DrawScreenCmd cmd;
	cmd.opcode = OP_DRAWSCREEN;
	cmd.size = sizeof(DrawScreenCmd)-8;
	cmd.texture = texture;
	cmd.paltex =paltex;
	cmd.vsync = vsync;
	cmd.previous = previous;
	glRenderer_AddCommand(This, (BYTE*)&cmd);*/
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = paltex;
	This->inputs[2] = (void*)vsync;
	This->inputs[3] = previous;
	This->opcode = OP_DRAWSCREEN;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Ensures the renderer is set up for handling Direct3D commands.
  * @param This
  *  Pointer to glRenderer object
  * @param zbuffer
  *  Nonzero if a Z buffer is present.
  * @param x
  *  Width of the initial viewport
  * @param y
  *  Height of the initial viewport
  */
void glRenderer_InitD3D(glRenderer *This, int zbuffer, int x, int y)
{
	/*
	InitD3DCmd cmd;
	cmd.opcode = OP_INITD3D;
	cmd.size = sizeof(InitD3DCmd)-8;
	cmd.zbuffer = zbuffer;
	cmd.x = x;
	cmd.y = y;
	glRenderer_AddCommand(This, (BYTE*)&bltcmd);*/
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)zbuffer;
	This->inputs[1] = (void*)x;
	This->inputs[2] = (void*)y;
	This->opcode = OP_INITD3D;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Clears the viewport.
  * @param This
  *  Pointer to glRenderer object
  * @param cmd
  *  Pointer to structure contaning all paramaters for a Clear operation.
  * @return
  *  Returns D3D_OK
  */
HRESULT glRenderer_Clear(glRenderer *This, ClearCommand *cmd)
{
	/*
	ClearCmd cmd;
	clearcmd.opcode = OP_CLEAR;
	clearcmd.size = sizeof(ClearCmd) - 8;
	clearcmd.cmd = *cmd;
	glRenderer_AddCommand(This, (BYTE*)&clearcmd); */
	EnterCriticalSection(&This->cs);
	This->inputs[0] = cmd;
	This->opcode = OP_CLEAR;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
  * Instructs the OpenGL driver to send all queued commands to the GPU.
  * @param This
  *  Pointer to glRenderer object
  */
void glRenderer_Flush(glRenderer *This)
{
	EnterCriticalSection(&This->cs);
	This->opcode = OP_FLUSH;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Changes the window used for rendering.
  * @param This
  *  Pointer to glRenderer object
  * @param width,height
  *  Width and height of the new window.
  * @param fullscreen
  *  True if fullscreen
  * @param newwnd
  *  HWND of the new window
  * @param devwnd
  *  True if creating window with name "DirectDrawDeviceWnd"
  */
void glRenderer_SetWnd(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND newwnd, BOOL devwnd)
{
	RECT wndrect, wndrect2;
	WINDOWPLACEMENT wndplace;
	BOOL hasmenu;
	int screenx, screeny;
	LONG_PTR winstyle, winstyleex;
	EnterCriticalSection(&This->cs);
	if(fullscreen && newwnd)
	{
		switch (dxglcfg.fullmode)
		{
		case 0:    // Fullscreen
			/*winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(newwnd, GWL_STYLE, (winstyle | WS_POPUP) & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER));
			ShowWindow(newwnd, SW_MAXIMIZE);*/  //This seems to cause a black screen in some cases
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
			SetWindowLongPtrA(newwnd, GWL_STYLE, WS_OVERLAPPED|WS_POPUP);
			ShowWindow(newwnd, SW_MAXIMIZE);
			break;
		case 1:    // Non-exclusive Fullscreen
		case 5:    // Windowed borderless scaled
			/*winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(newwnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(newwnd, SW_MAXIMIZE);*/  //This seems to cause a black screen in some cases
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
			SetWindowLongPtrA(newwnd, GWL_STYLE, WS_OVERLAPPED);
			ShowWindow(newwnd, SW_MAXIMIZE);
			break;
		case 2:     // Windowed
			winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(newwnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP));
			ShowWindow(newwnd, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				GetWindowRect(newwnd, &wndrect);
				if (GetMenu(newwnd)) hasmenu = TRUE;
				else hasmenu = FALSE;
				UnadjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP),
					hasmenu, winstyleex | WS_EX_APPWINDOW);
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.left = (screenx / 2) - (width / 2);
				wndrect.top = (screeny / 2) - (height / 2);
			}
			wndrect.right = wndrect.left + width;
			wndrect.bottom = wndrect.top + height;
			dxglcfg.WindowX = wndrect.left;
			dxglcfg.WindowY = wndrect.top;
			dxglcfg.WindowWidth = wndrect.right - wndrect.left;
			dxglcfg.WindowHeight = wndrect.bottom - wndrect.top;
			AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
				(winstyleex | WS_EX_APPWINDOW));
			SetWindowPos(newwnd, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			SaveWindowSettings(&dxglcfg);
			break;
		case 3:     // Windowed resizable
			winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(newwnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP);
			wndplace.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(newwnd, &wndplace);
			if(wndplace.showCmd == SW_SHOWMAXIMIZED) ShowWindow(newwnd, SW_SHOWMAXIMIZED);
			else ShowWindow(newwnd, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect = wndplace.rcNormalPosition;
				GetWindowRect(newwnd, &wndrect2);
				if (GetMenu(newwnd)) hasmenu = TRUE;
				else hasmenu = FALSE;
				UnadjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP,
					hasmenu, (winstyleex | WS_EX_APPWINDOW));
				UnadjustWindowRectEx(&wndrect2, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP,
					hasmenu, (winstyleex | WS_EX_APPWINDOW));
			}
			else
			{
				if (dxglcfg.NoResizeWindow)
				{
					GetWindowRect(newwnd, &wndrect2);
					if (GetMenu(newwnd)) hasmenu = TRUE;
					else hasmenu = FALSE;
					UnadjustWindowRectEx(&wndrect2, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP,
						hasmenu, (winstyleex | WS_EX_APPWINDOW));
					glDirectDraw7_SetWindowSize(This->ddInterface,
						wndrect2.right - wndrect2.left, wndrect2.bottom - wndrect2.top);
					break;
				}
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.left = (screenx / 2) - (width / 2);
				wndrect.top = (screeny / 2) - (height / 2);
			}
			if (!dxglcfg.NoResizeWindow)
			{
				wndrect.right = wndrect.left + width;
				wndrect.bottom = wndrect.top + height;
			}
			dxglcfg.WindowX = wndrect.left;
			dxglcfg.WindowY = wndrect.top;
			dxglcfg.WindowWidth = wndrect.right - wndrect.left;
			dxglcfg.WindowHeight = wndrect.bottom - wndrect.top;
			AdjustWindowRectEx(&wndrect, winstyle | WS_OVERLAPPEDWINDOW, FALSE, (winstyleex | WS_EX_APPWINDOW));
			wndplace.flags = WPF_ASYNCWINDOWPLACEMENT;
			if (!dxglcfg.NoResizeWindow) wndplace.showCmd = SW_SHOWNORMAL;
			wndplace.rcNormalPosition = wndrect;
			SetWindowPlacement(newwnd, &wndplace);
			if(dxglcfg.NoResizeWindow) glDirectDraw7_SetWindowSize(This->ddInterface,
				wndrect2.right - wndrect2.left, wndrect2.bottom - wndrect2.top);
			SaveWindowSettings(&dxglcfg);
			break;
		case 4:     // Windowed borderless
			winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(newwnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(newwnd, SW_NORMAL);
			screenx = GetSystemMetrics(SM_CXSCREEN);
			screeny = GetSystemMetrics(SM_CYSCREEN);
			if (dxglcfg.WindowPosition == 1) GetWindowRect(newwnd, &wndrect);
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.left = (screenx / 2) - (width / 2);
				wndrect.top = (screeny / 2) - (height / 2);
			}
			wndrect.right = wndrect.left + width;
			wndrect.bottom = wndrect.top + height;
			dxglcfg.WindowX = wndrect.left;
			dxglcfg.WindowY = wndrect.top;
			dxglcfg.WindowWidth = wndrect.right - wndrect.left;
			dxglcfg.WindowHeight = wndrect.bottom - wndrect.top;
			SetWindowPos(newwnd, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			SaveWindowSettings(&dxglcfg);
			break;
		}
	}
	This->inputs[0] = (void*)width;
	This->inputs[1] = (void*)height;
	This->inputs[2] = (void*)bpp;
	This->inputs[3] = (void*)fullscreen;
	This->inputs[4] = (void*)frequency;
	This->inputs[5] = (void*)newwnd;
	This->inputs[6] = (void*)devwnd;
	This->opcode = OP_SETWND;
	SetEvent(This->start);
	WaitForObjectAndMessages(This->busy);
	LeaveCriticalSection(&This->cs);
}
/**
  * Draws one or more primitives to the currently selected render target.
  * @param This
  *  Pointer to glRenderer object
  * @param target
  *  Textures and mip levels of the current render target
  * @param mode
  *  OpenGL primitive drawing mode to use
  * @param vertices
  *  Pointer to vertex data
  * @param packed
  *  True if vertex data is packed (e.g. xyz,normal,texcoord,xyz,normal,etc.)
  * @param texformats
  *  Pointer to texture coordinate formats used in the call
  * @param count
  *  Number of vertices to copy to the draw command
  * @param indices
  *  List of vertex indices to use in the drawing command, may be NULL for
  *  non-indexed mode.
  * @param indexcount
  *  Number of vertex indices.  May be 0 for non-indexed mode.
  * @param flags
  *  Set to D3DDP_WAIT to wait until the queue has processed the call. (not yet
  *  implemented)
  * @return
  *  D3D_OK if the call succeeds, or D3DERR_INVALIDVERTEXTYPE if the vertex format
  *  has no position coordinates.
  */
HRESULT glRenderer_DrawPrimitives(glRenderer *This, RenderTarget *target, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags)
{
	EnterCriticalSection(&This->cs);
	This->inputs[1] = (void*)mode;
	This->inputs[2] = vertices;
	This->inputs[3] = texformats;
	This->inputs[4] = (void*)count;
	This->inputs[5] = indices;
	This->inputs[6] = (void*)indexcount;
	This->inputs[7] = (void*)flags;
	memcpy(&This->inputs[8], target, sizeof(RenderTarget));
	This->opcode = OP_DRAWPRIMITIVES;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
  * Updates a clipping stencil.
  * @param This
  *  Pointer to glRenderer object
  * @param stencil
  *  Stencil texture to update
  * @param indices
  *  Pointer to array of indices representing the clip list
  * @param vertices
  *  Pointer to array of vertices representing the clip list
  * @param count
  *  Number of entries in the clip list
  * @param width
  *  Width of surface the stencil is attached to
  * @param height
  *  Height of surface the stencil is attached to
  */
void glRenderer_UpdateClipper(glRenderer *This, glTexture *stencil, GLushort *indices, BltVertex *vertices,
	GLsizei count, GLsizei width, GLsizei height)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = stencil;
	This->inputs[1] = indices;
	This->inputs[2] = vertices;
	This->inputs[3] = (void*)count;
	This->inputs[4] = (void*)width;
	This->inputs[5] = (void*)height;
	This->opcode = OP_UPDATECLIPPER;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}


/**
  * Gets an estimate of the scanline currently being drawn.
  * @param This
  *  Pointer to glRenderer object
  */
unsigned int glRenderer_GetScanLine(glRenderer *This)
{
	return DXGLTimer_GetScanLine(&This->timer);
}

/**
* Fills a depth surface with a specified value.
* @param This
*  Pointer to glRenderer object
* @param cmd
*  Pointer to structure contaning all paramaters for a Blt operation, with
*  appropriate depth fill parameters filled in.
* @param parent
*  Texture representing parent surface
* @param parentlevel
*  Mipmap level of parent surface
* @return
*  DD_OK if the depth fill succeeded.
*/
HRESULT glRenderer_DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent, GLint parentlevel)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = cmd;
	This->inputs[1] = parent;
	This->inputs[2] = (void*)parentlevel;
	This->opcode = OP_DEPTHFILL;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
  * Sets a render state within the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param dwRendStateType
  *  Render state to change
  * @param dwRenderState
  *  New render state value
  */
void glRenderer_SetRenderState(glRenderer *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)dwRendStateType;
	This->inputs[1] = (void*)dwRenderState;
	This->opcode = OP_SETRENDERSTATE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Binds a surface to a texture stage in the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param dwStage
  *  Texture stage to bind (8 through 11 are reserved for 2D drawing)
  * @param Texture
  *  Texture to bind to the stage; NULL to unbind
  */
void glRenderer_SetTexture(glRenderer *This, DWORD dwStage, glTexture *Texture)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)dwStage;
	This->inputs[1] = Texture;
	This->opcode = OP_SETTEXTURE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets a texture stage state within the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param dwStage
  *  Texture stage to modify
  * @param dwState
  *  Texture stage state to modify
  * @param dwValue
  *  New value for texture stage state.
  */
void glRenderer_SetTextureStageState(glRenderer *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)dwStage;
	This->inputs[1] = (void*)dwState;
	This->inputs[2] = (void*)dwValue;
	This->opcode = OP_SETTEXTURESTAGESTATE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets a transform matrix in the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param dtstTransformStateType
  *  Transform matrix to replace
  * @param lpD3DMatrix
  *  New transform matrix
  */
void glRenderer_SetTransform(glRenderer *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)dtstTransformStateType;
	This->inputs[1] = lpD3DMatrix;
	This->opcode = OP_SETTRANSFORM;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets the material in the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param lpMaterial
  *  New material parameters
  */
void glRenderer_SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = lpMaterial;
	This->opcode = OP_SETMATERIAL;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets a light in the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param index
  *  Index of light to set
  * @param light
  *  Pointer to light to change, ignored if remove is TRUE
  */

void glRenderer_SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)index;
	This->inputs[1] = light;
	This->opcode = OP_SETLIGHT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Removes a light from the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param index
  *  Index of light to remove
  */
void glRenderer_RemoveLight(glRenderer *This, DWORD index)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)index;
	This->opcode = OP_REMOVELIGHT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets the Direct3D viewport for the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param lpViewport
  *  New viewport parameters for renderer.
  */
void glRenderer_SetD3DViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = lpViewport;
	This->opcode = OP_SETD3DVIEWPORT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Sets a color key for a texture object.
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to set color key on
* @param dwFlags
*  DDraw color key flags to select color key to add or update
* @param lpDDColorKey
*  Pointer to a DDraw color key structure to set in the texture
* @param level
*  Mip level of color key to set for DirectDraw; Direct3D colorkey operations use level 0
*/
void glRenderer_SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey, GLint level)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)dwFlags;
	This->inputs[2] = lpDDColorKey;
	This->inputs[3] = (void*)level;
	This->opcode = OP_SETTEXTURECOLORKEY;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Sets whether a texure has primary scaling
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to set primary scaling
* @param parent
*  Parent texture this one is attached to, needed only if primary is TRUE
* @param primary
*  TRUE if texture should have primary scaling, FALSE to remove scaling
*/
void glRenderer_MakeTexturePrimary(glRenderer *This, glTexture *texture, glTexture *parent, BOOL primary)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = parent;
	This->inputs[2] = (void*)primary;
	This->opcode = OP_MAKETEXTUREPRIMARY;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Generates a glFrameTerminatorGREMEDY command in OpenGL if the 
  * glFrameTerminatorGREMEDY command is available
  * (i.e. running under gDebugger or CodeXL).
  * @param This
  *  Pointer to glRenderer object
  */
void glRenderer_DXGLBreak(glRenderer *This)
{
	EnterCriticalSection(&This->cs);
	This->opcode = OP_DXGLBREAK;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Ends a command buffer.
  * @param This
  *  Pointer to glRenderer object
  * @param wait
  *  TRUE to wait for the command buffer to complete
  * @param in_cs
  *  If TRUE, do not take the Crtitical Section because it is already taken
  */
void glRenderer_EndCommand(glRenderer *This, BOOL wait, BOOL in_cs)
{
	if (!in_cs) EnterCriticalSection(&This->cs);
	if (!This->state.cmd->write_ptr_cmd)
	{
		if (!in_cs) LeaveCriticalSection(&This->cs);
		return;  // Don't flip buffers if the front one is empty.
	}
	This->opcode = OP_ENDCOMMAND;
	This->inputs[0] = (void*)wait;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	if (!in_cs) LeaveCriticalSection(&This->cs);
}

/**
  * Main loop for glRenderer class
  * @param This
  *  Pointer to glRenderer object
  * @return
  *  Returns 0 to signal successful thread termination
  */
DWORD glRenderer__Entry(glRenderer *This)
{
	float tmpfloats[16];
	EnterCriticalSection(&This->cs);
	glRenderer__InitGL(This,(int)This->inputs[0],(int)This->inputs[1],(int)This->inputs[2],
		(int)This->inputs[3],(unsigned int)This->inputs[4],(HWND)This->inputs[5],
		(glDirectDraw7*)This->inputs[6]);
	LeaveCriticalSection(&This->cs);
	SetEvent(This->busy);
	while(1)
	{
		WaitForSingleObject(This->start,INFINITE);
		switch(This->opcode)
		{
		case OP_DELETE:
			if(This->hRC)
			{
				if(This->dib.enabled)
				{
					if(This->dib.hbitmap) DeleteObject(This->dib.hbitmap);
					if(This->dib.hdc) DeleteDC(This->dib.hdc);
					ZeroMemory(&This->dib,sizeof(DIB));
				}
				glUtil_DeleteFBO(This->util, &This->fbo);
				if(This->pbo)
				{
					BufferObject_Release(This->pbo);
					This->pbo = NULL;
				}
				if(This->backbuffer)
				{
					glTexture_Release(This->backbuffer, TRUE);
					This->backbuffer = NULL;
				}
				//glRenderer__DeleteCommandBuffer(&This->cmd1);
				//glRenderer__DeleteCommandBuffer(&This->cmd2);
				ShaderManager_Delete(This->shaders);
				glUtil_Release(This->util);
				free(This->shaders);
				free(This->ext);
				This->ext = NULL;
				wglMakeCurrent(NULL,NULL);
				wglDeleteContext(This->hRC);
				This->hRC = NULL;
			};
			if(This->hDC) ReleaseDC(This->RenderWnd->GetHWnd(),This->hDC);
			This->hDC = NULL;
			if (This->dib.info) free(This->dib.info);
			This->dib.info = NULL;
			delete This->RenderWnd;
			This->RenderWnd = NULL;
			SetEvent(This->busy);
			return 0;
			break;
		case OP_SETWND:
			glRenderer__SetWnd(This,(int)This->inputs[0],(int)This->inputs[1],(int)This->inputs[2],
				(int)This->inputs[3],(unsigned int)This->inputs[4],(HWND)This->inputs[5],(BOOL)This->inputs[6]);
			break;
		case OP_CREATE:
			glRenderer__MakeTexture(This,(glTexture*)This->inputs[0]);
			SetEvent(This->busy);
			break;
		case OP_UPLOAD:
			glRenderer__UploadTexture(This,(glTexture*)This->inputs[0],(GLint)This->inputs[1]);
			SetEvent(This->busy);
			break;
		case OP_DOWNLOAD:
			glRenderer__DownloadTexture(This,(glTexture*)This->inputs[0],(GLint)This->inputs[1]);
			SetEvent(This->busy);
			break;
		case OP_DELETETEX:
			glRenderer__DeleteTexture(This,(glTexture*)This->inputs[0]);
			break;
		case OP_BLT:
			glRenderer__Blt(This, (BltCommand*)This->inputs[0]);
			break;
		case OP_DRAWSCREEN:
			glRenderer__DrawScreen(This,(glTexture*)This->inputs[0],(glTexture*)This->inputs[1],
				(GLint)This->inputs[2],(glTexture*)This->inputs[3],true);
			break;
		case OP_INITD3D:
			glRenderer__InitD3D(This,(int)This->inputs[0],(int)This->inputs[1],(int)This->inputs[2]);
			break;
		case OP_CLEAR:
			glRenderer__Clear(This,(ClearCommand*)This->inputs[0]);
			break;
		case OP_FLUSH:
			glRenderer__Flush(This);
			break;
		case OP_DRAWPRIMITIVES:
			glRenderer__DrawPrimitivesOld(This,(RenderTarget*)&This->inputs[8],(GLenum)This->inputs[1],
				(GLVERTEX*)This->inputs[2],(int*)This->inputs[3],(DWORD)This->inputs[4],(LPWORD)This->inputs[5],
				(DWORD)This->inputs[6],(DWORD)This->inputs[7]);
			break;
		case OP_UPDATECLIPPER:
			glRenderer__UpdateClipper(This,(glTexture*)This->inputs[0], (GLushort*)This->inputs[1],
				(BltVertex*)This->inputs[2], (GLsizei)This->inputs[3], (GLsizei)This->inputs[4], (GLsizei)This->inputs[5]);
			break;
		case OP_DEPTHFILL:
			glRenderer__DepthFill(This, (BltCommand*)This->inputs[0], (glTexture*)This->inputs[1], (GLint)This->inputs[2]);
			break;
		case OP_SETRENDERSTATE:
			glRenderer__SetRenderState(This, (D3DRENDERSTATETYPE)(DWORD)This->inputs[0], (DWORD)This->inputs[1]);
			break;
		case OP_SETTEXTURE:
			glRenderer__SetTexture(This, (DWORD)This->inputs[0], (glTexture*)This->inputs[1]);
			break;
		case OP_SETTEXTURESTAGESTATE:
			glRenderer__SetTextureStageState(This, (DWORD)This->inputs[0], (D3DTEXTURESTAGESTATETYPE)(DWORD)This->inputs[1],
				(DWORD)This->inputs[2]);
			break;
		case OP_SETTRANSFORM:
			glRenderer__SetTransform(This, (D3DTRANSFORMSTATETYPE)(DWORD)This->inputs[0], (LPD3DMATRIX)This->inputs[1]);
			break;
		case OP_SETMATERIAL:
			glRenderer__SetMaterial(This, (LPD3DMATERIAL7)This->inputs[0]);
			break;
		case OP_SETLIGHT:
			glRenderer__SetLight(This, (DWORD)This->inputs[0], (LPD3DLIGHT7)This->inputs[1]);
			break;
		case OP_REMOVELIGHT:
			glRenderer__RemoveLight(This, (DWORD)This->inputs[0]);
			break;
		case OP_SETD3DVIEWPORT:
			glRenderer__SetD3DViewport(This, (LPD3DVIEWPORT7)This->inputs[0]);
			break;
		case OP_SETTEXTURECOLORKEY:
			glRenderer__SetTextureColorKey(This, (glTexture*)This->inputs[0], (DWORD)This->inputs[1],
				(LPDDCOLORKEY)This->inputs[2], (GLint)This->inputs[3]);
			break;
		case OP_MAKETEXTUREPRIMARY:
			glRenderer__MakeTexturePrimary(This, (glTexture*)This->inputs[0], (glTexture*)This->inputs[1], (DWORD)This->inputs[2]);
			break;
		case OP_DXGLBREAK:
			glRenderer__DXGLBreak(This);
			break;
		case OP_ENDCOMMAND:
			glRenderer__EndCommand(This, (BOOL)This->inputs[0]);
			break;
		}
	}
	return 0;
}

/**
  * Creates a render window and initializes OpenGL.
  * @param This
  *  Pointer to glRenderer object
  * @param width,height
  *  Width and height of the render window.
  * @param bpp
  *  Color depth of the screen.
  * @param fullscreen
  *  True if full screen mode is requested.
  * @param hWnd
  *  Handle to the window to use as the renderer.  If NULL, then creates a
  *  transparent overlay window.
  * @param glDD7
  *  Pointer to the glDirectDraw7 interface that creates the renderer.
  * @return
  *  TRUE if OpenGL has been initialized, FALSE otherwise.
  */
BOOL glRenderer__InitGL(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND hWnd, glDirectDraw7 *glDD7)
{
	EnterCriticalSection(&dll_cs);
	This->ddInterface = glDD7;
	if(This->hRC)
	{
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(This->hRC);
	};
	PIXELFORMATDESCRIPTOR pfd;
	GLuint pf;
	ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	if (dxglcfg.SingleBufferDevice) pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	else pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = bpp;
	pfd.iLayerType = PFD_MAIN_PLANE;
	InterlockedIncrement(&gllock);
	This->hDC = GetDC(This->RenderWnd->GetHWnd());
	if(!This->hDC)
	{
		DEBUG("glRenderer::InitGL: Can not create hDC\n");
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	pf = ChoosePixelFormat(This->hDC,&pfd);
	if(!pf)
	{
		DEBUG("glRenderer::InitGL: Can not get pixelformat\n");
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	if(!SetPixelFormat(This->hDC,pf,&pfd))
		DEBUG("glRenderer::InitGL: Can not set pixelformat\n");
	This->hRC = wglCreateContext(This->hDC);
	if(!This->hRC)
	{
		DEBUG("glRenderer::InitGL: Can not create GL context\n");
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	if(!wglMakeCurrent(This->hDC,This->hRC))
	{
		DEBUG("glRenderer::InitGL: Can not activate GL context\n");
		wglDeleteContext(This->hRC);
		This->hRC = NULL;
		ReleaseDC(This->RenderWnd->GetHWnd(),This->hDC);
		This->hDC = NULL;
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	InterlockedDecrement(&gllock);
	LeaveCriticalSection(&dll_cs);
	This->ext = (glExtensions *)malloc(sizeof(glExtensions));
	glExtensions_Init(This->ext);
	glUtil_Create(This->ext, &This->util);
	glRenderer__SetSwap(This,1);
	glFinish();
	DXGLTimer_Init(&This->timer);
	DXGLTimer_Calibrate(&This->timer, height, frequency);
	if (dxglcfg.vsync == 1) This->oldswap = 1;
	glRenderer__SetSwap(This,0);
	glUtil_SetViewport(This->util,0,0,width,height);
	glViewport(0,0,width,height);
	glUtil_SetDepthRange(This->util,0.0,1.0);
	glUtil_DepthWrite(This->util,TRUE);
	glUtil_DepthTest(This->util,FALSE);
	glUtil_MatrixMode(This->util,GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);
	glUtil_SetDepthComp(This->util,GL_LESS);
	const GLubyte *glver = glGetString(GL_VERSION);
	This->gl_caps.Version = (GLfloat)atof((char*)glver);
	if(This->gl_caps.Version >= 2)
	{
		glver = glGetString(GL_SHADING_LANGUAGE_VERSION);
		This->gl_caps.ShaderVer = (GLfloat)atof((char*)glver);
	}
	else This->gl_caps.ShaderVer = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&This->gl_caps.TextureMax);
	This->shaders = (ShaderManager*)malloc(sizeof(ShaderManager));
	ShaderManager_Init(This->ext, This->shaders);
	This->fbo.fbo = 0;
	glUtil_InitFBO(This->util,&This->fbo);
	glUtil_ClearColor(This->util, 0.0f, 0.0f, 0.0f, 0.0f);
	glUtil_ClearDepth(This->util, 1.0);
	glUtil_ClearStencil(This->util, 0);
	glUtil_EnableArray(This->util,-1,FALSE);
	glUtil_BlendFunc(This->util,GL_ONE,GL_ZERO);
	glUtil_BlendEnable(This->util,FALSE);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	glUtil_SetScissor(This->util,FALSE,0,0,0,0);
	glDisable(GL_SCISSOR_TEST);
	glUtil_SetCull(This->util,D3DCULL_CCW);
	glEnable(GL_CULL_FACE);
	SwapBuffers(This->hDC);
	glUtil_SetActiveTexture(This->util,0);
	glRenderer__SetFogColor(This,0);
	glRenderer__SetFogStart(This,0);
	glRenderer__SetFogEnd(This,1);
	glRenderer__SetFogDensity(This,1);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	glUtil_SetShadeMode(This->util, D3DSHADE_GOURAUD);
	if(hWnd)
	{
		This->dib.enabled = TRUE;
		This->dib.width = width;
		This->dib.height = height;
		This->dib.pitch = (((width<<3)+31)&~31) >>3;
		This->dib.pixels = NULL;
		This->dib.hdc = CreateCompatibleDC(NULL);
		if(!This->dib.info)
			This->dib.info = (BITMAPINFO*)malloc(sizeof(BITMAPINFO));
		ZeroMemory(This->dib.info,sizeof(BITMAPINFO));
		This->dib.info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		This->dib.info->bmiHeader.biBitCount = 32;
		This->dib.info->bmiHeader.biWidth = width;
		This->dib.info->bmiHeader.biHeight = height;
		This->dib.info->bmiHeader.biCompression = BI_RGB;
		This->dib.info->bmiHeader.biPlanes = 1;
		This->dib.hbitmap = CreateDIBSection(This->dib.hdc,This->dib.info,
			DIB_RGB_COLORS,(void**)&This->dib.pixels,NULL,0);
	}
	BufferObject_Create(&This->pbo, This->ext, This->util);
	BufferObject_SetData(This->pbo, GL_PIXEL_PACK_BUFFER, width*height * 4, NULL, GL_STREAM_READ);
	ZeroMemory(&This->state, sizeof(RenderState));
	This->state.cmd = &This->cmd1;
	//glRenderer__InitCommandBuffer(This, &This->cmd1, width * height * (NextMultipleOf8(bpp) / 8));
	//glRenderer__InitCommandBuffer(This, &This->cmd2, width * height * (NextMultipleOf8(bpp) / 8));
	//BufferObject_Map(This->cmd1.vertices, GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	//BufferObject_Map(This->cmd1.indices, GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
		(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
	{
		This->postsizex = 1.0f;
		This->postsizey = 1.0f;
	}
	else
	{
		This->postsizex = dxglcfg.postsizex;
		This->postsizey = dxglcfg.postsizey;
	}
	TRACE_SYSINFO();
	return TRUE;
}

void glRenderer__InitCommandBuffer(glRenderer *This, CommandBuffer *cmd, size_t framesize)
{
	size_t uploadsize = NextMultipleOf1024(framesize * 2);
	if (uploadsize > 134217728) // Over 4K resolution
		uploadsize = NextMultipleOf1024((int)((float)framesize * 1.5f));
	if (uploadsize > 268435456) // Over 8K resolution
		uploadsize = NextMultipleOf1024((int)((float)framesize * 1.5f));
	ZeroMemory(cmd, sizeof(CommandBuffer));
	cmd->uploadsize = uploadsize;
	cmd->uploadbuffer = (unsigned char *)malloc(uploadsize);
	cmd->cmdsize = 1048576;
	cmd->cmdbuffer = (unsigned char *)malloc(cmd->cmdsize);
	BufferObject_Create(&cmd->vertices, This->ext, This->util);
	BufferObject_SetData(cmd->vertices, GL_ARRAY_BUFFER, 4194304, NULL, GL_DYNAMIC_DRAW);
	BufferObject_Create(&cmd->indices, This->ext, This->util);
	BufferObject_SetData(cmd->vertices, GL_ELEMENT_ARRAY_BUFFER, 262144, NULL, GL_DYNAMIC_DRAW);
}

void glRenderer__DeleteCommandBuffer(CommandBuffer *cmd)
{
	if (cmd->uploadbuffer) free(cmd->uploadbuffer);
	if (cmd->cmdbuffer) free(cmd->cmdbuffer);
	if (cmd->vertices) BufferObject_Release(cmd->vertices);
	if (cmd->indices) BufferObject_Release(cmd->indices);
}

void SetColorFillUniform(DWORD color, DWORD *colorsizes, int colororder, DWORD *colorbits, GLint uniform, glExtensions *ext)
{
	DWORD r, g, b, a;
	switch (colororder)
	{
	case 0:
		r = color & colorsizes[0];
		color >>= colorbits[0];
		g = color & colorsizes[1];
		color >>= colorbits[1];
		b = color & colorsizes[2];
		color >>= colorbits[2];
		a = color & colorsizes[3];
		ext->glUniform4i(uniform, r, g, b, a);
		break;
	case 1:
		b = color & colorsizes[2];
		color >>= colorbits[2];
		g = color & colorsizes[1];
		color >>= colorbits[1];
		r = color & colorsizes[0];
		color >>= colorbits[0];
		a = color & colorsizes[3];
		ext->glUniform4i(uniform, r, g, b, a);
		break;
	case 2:
		a = color & colorsizes[3];
		color >>= colorbits[3];
		r = color & colorsizes[0];
		color >>= colorbits[0];
		g = color & colorsizes[1];
		color >>= colorbits[1];
		b = color & colorsizes[2];
		ext->glUniform4i(uniform, r, g, b, a);
		break;
	case 3:
		a = color & colorsizes[3];
		color >>= colorbits[3];
		b = color & colorsizes[2];
		color >>= colorbits[2];
		g = color & colorsizes[1];
		color >>= colorbits[1];
		r = color & colorsizes[0];
		ext->glUniform4i(uniform, r, g, b, a);
		break;
	case 4:
		r = color & colorsizes[0];
		ext->glUniform4i(uniform, r, r, r, r);
		break;
	case 5:
		r = color & colorsizes[0];
		ext->glUniform4i(uniform, r, r, r, r);
		break;
	case 6:
		a = color & colorsizes[3];
		ext->glUniform4i(uniform, a, a, a, a);
		break;
	case 7:
		r = color & colorsizes[0];
		color >>= colorbits[0];
		a = color & colorsizes[3];
		ext->glUniform4i(uniform, r, r, r, a);
		break;
	}
}

void SetColorKeyUniform(DWORD key, DWORD *colorsizes, int colororder, GLint uniform, DWORD *colorbits, glExtensions *ext)
{
	DWORD r, g, b, a;
	switch (colororder)
	{
	case 0:
		r = key & colorsizes[0];
		key >>= colorbits[0];
		g = key & colorsizes[1];
		key >>= colorbits[1];
		b = key & colorsizes[2];
		ext->glUniform3i(uniform, r, g, b);
		break;
	case 1:
		b = key & colorsizes[2];
		key >>= colorbits[2];
		g = key & colorsizes[1];
		key >>= colorbits[1];
		r = key & colorsizes[0];
		ext->glUniform3i(uniform, r, g, b);
		break;
	case 2:
		a = key & colorsizes[3];
		key >>= colorbits[3];
		r = key & colorsizes[0];
		key >>= colorbits[0];
		g = key & colorsizes[1];
		key >>= colorbits[1];
		b = key & colorsizes[2];
		ext->glUniform3i(uniform, r, g, b);
		break;
	case 3:
		a = key & colorsizes[3];
		key >>= colorbits[3];
		b = key & colorsizes[2];
		key >>= colorbits[2];
		g = key & colorsizes[1];
		key >>= colorbits[1];
		r = key & colorsizes[0];
		ext->glUniform3i(uniform, r, g, b);
		break;
	case 4:
		r = key & colorsizes[0];
		if (ext->glver_major >= 3) ext->glUniform3i(uniform, r, 0, 0);
		else ext->glUniform3i(uniform, r, r, r);
		break;
	case 5:
		r = key & colorsizes[0];
		ext->glUniform3i(uniform, r, r, r);
		break;
	case 6:
		a = key & colorsizes[3];
		ext->glUniform4i(uniform, 0, 0, 0, a);
		break;
	case 7:
		r = key & colorsizes[0];
		key >>= colorbits[0];
		a = key & colorsizes[3];
		ext->glUniform4i(uniform, r, r, r, a);
		break;
	}
}

void BltFlipLR(BltVertex *vertices)
{
	GLfloat s1, s2;
	s1 = vertices[0].s;
	s2 = vertices[2].s;
	vertices[0].s = vertices[1].s;
	vertices[2].s = vertices[3].s;
	vertices[1].s = s1;
	vertices[3].s = s2;
}

void BltFlipUD(BltVertex *vertices)
{
	GLfloat t1, t2;
	t1 = vertices[0].t;
	t2 = vertices[1].t;
	vertices[0].t = vertices[2].t;
	vertices[1].t = vertices[3].t;
	vertices[2].t = t1;
	vertices[3].t = t2;
}

void RotateBlt90(BltVertex *vertices, int times)
{
	GLfloat s0, s1, s2, s3;
	GLfloat t0, t1, t2, t3;
	switch (times)
	{
	case 0:
	default:
		return;
	case 1:
		s0 = vertices[0].s; t0 = vertices[0].t;
		s1 = vertices[1].s; t1 = vertices[1].t;
		s2 = vertices[2].s; t2 = vertices[2].t;
		s3 = vertices[3].s; t3 = vertices[3].t;
		vertices[0].s = s1; vertices[0].t = t1;
		vertices[1].s = s3; vertices[1].t = t3;
		vertices[2].s = s0; vertices[2].t = t0;
		vertices[3].s = s2; vertices[3].t = t2;
		break;
	case 2:
		s0 = vertices[0].s; t0 = vertices[0].t;
		s1 = vertices[1].s; t1 = vertices[1].t;
		s2 = vertices[2].s; t2 = vertices[2].t;
		s3 = vertices[3].s; t3 = vertices[3].t;
		vertices[0].s = s3; vertices[0].t = t3;
		vertices[1].s = s2; vertices[1].t = t2;
		vertices[2].s = s1; vertices[2].t = t1;
		vertices[3].s = s0; vertices[3].t = t0;
		break;
	case 3:
		s0 = vertices[0].s; t0 = vertices[0].t;
		s1 = vertices[1].s; t1 = vertices[1].t;
		s2 = vertices[2].s; t2 = vertices[2].t;
		s3 = vertices[3].s; t3 = vertices[3].t;
		vertices[0].s = s2; vertices[0].t = t2;
		vertices[1].s = s0; vertices[1].t = t0;
		vertices[2].s = s3; vertices[2].t = t3;
		vertices[3].s = s1; vertices[3].t = t1;
		break;
	}
}

void glRenderer__Blt(glRenderer *This, BltCommand *cmd)
{
	int rotates = 0;
	BOOL usedest = FALSE;
	BOOL usepattern = FALSE;
	LONG sizes[6];
	RECT srcrect;
	RECT destrect, destrect2;
	This->ddInterface->GetSizes(sizes);
	DWORD shaderid;
	DDSURFACEDESC2 ddsd;
	ddsd = cmd->dest->levels[cmd->destlevel].ddsd;
	if (!memcmp(&cmd->destrect, &nullrect, sizeof(RECT)))
	{
		destrect.left = 0;
		destrect.top = 0;
		destrect.right = ddsd.dwWidth;
		destrect.bottom = ddsd.dwHeight;
		destrect2 = destrect;
	}
	else
	{
		destrect = cmd->destrect;
		destrect2.left = 0;
		destrect2.top = 0;
		destrect2.right = destrect.right - destrect.left;
		destrect2.bottom = destrect.bottom - destrect.top;
	}
	if ((cmd->bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->flags & DDBLT_ROP))
	{
		shaderid = PackROPBits(cmd->bltfx.dwROP, cmd->flags);
		if (rop_texture_usage[(cmd->bltfx.dwROP >> 16) & 0xFF] & 2) usedest = TRUE;
		if (rop_texture_usage[(cmd->bltfx.dwROP >> 16) & 0xFF] & 4) usepattern = TRUE;
	}
	else shaderid = cmd->flags & 0xF2FAADFF;
	if (cmd->flags & DDBLT_KEYDEST) usedest = TRUE;
	if (IsAlphaCKey())
	{

	}
	else if (usedest)
	{
		ShaderManager_SetShader(This->shaders, PROG_TEXTURE, NULL, 0);
		glRenderer__DrawBackbufferRect(This, cmd->dest, destrect, destrect2, PROG_TEXTURE);
		This->bltvertices[1].dests = This->bltvertices[3].dests = 0.;
		This->bltvertices[0].dests = This->bltvertices[2].dests = (GLfloat)(destrect.right - destrect.left) / (GLfloat)This->backbuffer->levels[0].ddsd.dwWidth;
		This->bltvertices[0].destt = This->bltvertices[1].destt = 1.;
		This->bltvertices[2].destt = This->bltvertices[3].destt = 1.0 - ((GLfloat)(destrect.bottom - destrect.top) / (GLfloat)This->backbuffer->levels[0].ddsd.dwHeight);
	}
	ShaderManager_SetShader(This->shaders, shaderid, NULL, 1);
	GenShader2D *shader = (GenShader2D*)This->shaders->gen3d->current_genshader;
	glUtil_BlendEnable(This->util, FALSE);
	do
	{
		if (glUtil_SetFBOSurface(This->util, cmd->dest, NULL, cmd->destlevel, 0, TRUE) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!cmd->dest->internalformats[1]) break;
		glTexture__Repair(cmd->dest, TRUE);
		glUtil_SetFBO(This->util, NULL);
		cmd->dest->levels[cmd->destlevel].fbo.fbcolor = NULL;
		cmd->dest->levels[cmd->destlevel].fbo.fbz = NULL;
	} while (1);
	glUtil_SetViewport(This->util,0,0,cmd->dest->bigwidth,cmd->dest->bigheight);
	glUtil_DepthTest(This->util, FALSE);
	DDSURFACEDESC2 ddsdSrc;
	ddsdSrc.dwSize = sizeof(DDSURFACEDESC2);
	if (cmd->src)
	{
		ddsdSrc = cmd->src->levels[cmd->srclevel].ddsd;
		if (cmd->src->levels[cmd->srclevel].dirty & 1) glTexture__Upload(cmd->src, cmd->srclevel);
	}
	if (cmd->dest->levels[cmd->destlevel].dirty & 1)
		glTexture__Upload(cmd->dest, cmd->destlevel);
	if (!memcmp(&cmd->srcrect, &nullrect, sizeof(RECT)))
	{
		srcrect.left = 0;
		srcrect.top = 0;
		srcrect.right = ddsdSrc.dwWidth;
		srcrect.bottom = ddsdSrc.dwHeight;
	}
	else srcrect = cmd->srcrect;
	This->bltvertices[1].x = This->bltvertices[3].x = (GLfloat)destrect.left;
	This->bltvertices[0].x = This->bltvertices[2].x = (GLfloat)destrect.right;
	This->bltvertices[0].y = This->bltvertices[1].y = (GLfloat)ddsd.dwHeight - (GLfloat)destrect.top;
	This->bltvertices[2].y = This->bltvertices[3].y = (GLfloat)ddsd.dwHeight - (GLfloat)destrect.bottom;
	This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left / (GLfloat)ddsdSrc.dwWidth;
	This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right / (GLfloat)ddsdSrc.dwWidth;
	This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top / (GLfloat)ddsdSrc.dwHeight;
	This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom / (GLfloat)ddsdSrc.dwHeight;
	if ((cmd->bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->flags & DDBLT_DDFX))
	{
		if (cmd->bltfx.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT)
			BltFlipLR(This->bltvertices);
		if (cmd->bltfx.dwDDFX & DDBLTFX_MIRRORUPDOWN)
			BltFlipUD(This->bltvertices);
		if (cmd->bltfx.dwDDFX & DDBLTFX_ROTATE90) rotates++;
		if (cmd->bltfx.dwDDFX & DDBLTFX_ROTATE180) rotates += 2;
		if (cmd->bltfx.dwDDFX & DDBLTFX_ROTATE270) rotates += 3;
		rotates &= 3;
		if (rotates)
		{
			RotateBlt90(This->bltvertices, rotates);
		}
	}
	if (cmd->flags & 0x10000000)
	{
		This->bltvertices[1].stencils = This->bltvertices[3].stencils = This->bltvertices[1].x / (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwWidth;
		This->bltvertices[0].stencils = This->bltvertices[2].stencils = This->bltvertices[0].x / (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwWidth;
		This->bltvertices[0].stencilt = This->bltvertices[1].stencilt = This->bltvertices[0].y / (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwHeight;
		This->bltvertices[2].stencilt = This->bltvertices[3].stencilt = This->bltvertices[2].y / (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwHeight;
	}
	if (cmd->dest->levels[cmd->destlevel].fbo.fbz) glClear(GL_DEPTH_BUFFER_BIT);
	if (cmd->flags & DDBLT_COLORFILL) SetColorFillUniform(cmd->bltfx.dwFillColor, cmd->dest->colorsizes,
		cmd->dest->colororder, cmd->dest->colorbits, shader->shader.uniforms[12], This->ext);
	if ((cmd->flags & DDBLT_KEYSRC) && (cmd->src && ((cmd->src->levels[cmd->srclevel].ddsd.dwFlags & DDSD_CKSRCBLT))
		|| (cmd->flags & DDBLT_KEYSRCOVERRIDE)) && !(cmd->flags & DDBLT_COLORFILL))
	{
		if (cmd->flags & DDBLT_KEYSRCOVERRIDE)
		{
			SetColorKeyUniform(cmd->bltfx.ddckSrcColorkey.dwColorSpaceLowValue, cmd->src->colorsizes,
				cmd->src->colororder, shader->shader.uniforms[5], cmd->src->colorbits, This->ext);
			if (cmd->flags & 0x20000000) SetColorKeyUniform(cmd->bltfx.ddckSrcColorkey.dwColorSpaceHighValue, cmd->src->colorsizes,
				cmd->src->colororder, shader->shader.uniforms[7], cmd->src->colorbits, This->ext);
		}
		else
		{
			SetColorKeyUniform(cmd->src->levels[cmd->srclevel].ddsd.ddckCKSrcBlt.dwColorSpaceLowValue, cmd->src->colorsizes,
				cmd->src->colororder, shader->shader.uniforms[5], cmd->src->colorbits, This->ext);
			if (cmd->flags & 0x20000000) SetColorKeyUniform(cmd->src->levels[cmd->srclevel].ddsd.ddckCKSrcBlt.dwColorSpaceHighValue, cmd->src->colorsizes,
				cmd->src->colororder, shader->shader.uniforms[7], cmd->src->colorbits, This->ext);
		}
	}
	if (!(cmd->flags & DDBLT_COLORFILL)) This->ext->glUniform1i(shader->shader.uniforms[1], 8);
	if ((cmd->flags & DDBLT_KEYDEST) && (This && ((cmd->dest->levels[cmd->destlevel].ddsd.dwFlags & DDSD_CKDESTBLT)
		|| (cmd->flags & DDBLT_KEYDESTOVERRIDE))))
	{
		if (cmd->flags & DDBLT_KEYDESTOVERRIDE)
		{
			SetColorKeyUniform(cmd->bltfx.ddckDestColorkey.dwColorSpaceLowValue, cmd->dest->colorsizes,
				cmd->dest->colororder, shader->shader.uniforms[6], cmd->dest->colorbits, This->ext);
			if (cmd->flags & 0x40000000) SetColorKeyUniform(cmd->bltfx.ddckDestColorkey.dwColorSpaceHighValue, cmd->dest->colorsizes,
				cmd->dest->colororder, shader->shader.uniforms[8], cmd->dest->colorbits, This->ext);
		}
		else
		{
			SetColorKeyUniform(cmd->dest->levels[cmd->destlevel].ddsd.ddckCKDestBlt.dwColorSpaceLowValue, cmd->dest->colorsizes,
				cmd->dest->colororder, shader->shader.uniforms[6], cmd->dest->colorbits, This->ext);
			if (cmd->flags & 0x40000000) SetColorKeyUniform(cmd->dest->levels[cmd->destlevel].ddsd.ddckCKDestBlt.dwColorSpaceHighValue, cmd->dest->colorsizes,
				cmd->dest->colororder, shader->shader.uniforms[8], cmd->dest->colorbits, This->ext);
		}
	}
	if (usedest && (shader->shader.uniforms[2] != -1))
	{
		glUtil_SetTexture(This->util, 9, This->backbuffer);
		This->ext->glUniform1i(shader->shader.uniforms[2], 9);
	}
	if (usepattern && (shader->shader.uniforms[3] != -1))
	{
		if (cmd->pattern->levels[cmd->patternlevel].dirty & 1) glTexture__Upload(cmd->pattern, cmd->patternlevel);
		glUtil_SetTexture(This->util, 10, cmd->pattern);
		This->ext->glUniform1i(shader->shader.uniforms[3], 10);
		This->ext->glUniform2i(shader->shader.uniforms[9],
			cmd->pattern->levels[cmd->patternlevel].ddsd.dwWidth, cmd->pattern->levels[cmd->patternlevel].ddsd.dwHeight);
	}
	if (cmd->flags & 0x10000000)  // Use clipper
	{
		glUtil_SetTexture(This->util, 11, cmd->dest->stencil);
		This->ext->glUniform1i(shader->shader.uniforms[4],11);
		glUtil_EnableArray(This->util, shader->shader.attribs[5], TRUE);
		This->ext->glVertexAttribPointer(shader->shader.attribs[5], 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].stencils);
	}
	if (cmd->src)
	{
		glUtil_SetTexture(This->util, 8, cmd->src);
		if(This->ext->GLEXT_ARB_sampler_objects)
		{
			if((dxglcfg.BltScale == 0) || (This->ddInterface->GetBPP() == 8))
				glTexture__SetFilter(cmd->src, 8, GL_NEAREST, GL_NEAREST, This);
			else glTexture__SetFilter(cmd->src, 8, GL_LINEAR, GL_LINEAR, This);
		}
	}
	else glUtil_SetTexture(This->util,8,NULL);
	This->ext->glUniform4f(shader->shader.uniforms[0], 0,
		(GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwWidth, 0, (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwHeight);
	if (cmd->src) This->ext->glUniform4i(shader->shader.uniforms[10], cmd->src->colorsizes[0], cmd->src->colorsizes[1],
		cmd->src->colorsizes[2], cmd->src->colorsizes[3]);
	if (cmd->dest) This->ext->glUniform4i(shader->shader.uniforms[11], cmd->dest->colorsizes[0], cmd->dest->colorsizes[1],
		cmd->dest->colorsizes[2], cmd->dest->colorsizes[3]);
	cmd->dest->levels[cmd->destlevel].dirty |= 2;
	glUtil_EnableArray(This->util, shader->shader.attribs[0], TRUE);
	This->ext->glVertexAttribPointer(shader->shader.attribs[0],2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].x);
	if((!(cmd->flags & DDBLT_COLORFILL)) && (shader->shader.attribs[3] != -1))
	{
		glUtil_EnableArray(This->util, shader->shader.attribs[3], TRUE);
		This->ext->glVertexAttribPointer(shader->shader.attribs[3],2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].s);
	}
	if (usedest)
	{
		glUtil_EnableArray(This->util, shader->shader.attribs[4], TRUE);
		This->ext->glVertexAttribPointer(shader->shader.attribs[4],2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].dests);
	}
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	glUtil_SetFBO(This->util, NULL);
	if(((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
		glRenderer__DrawScreen(This,cmd->dest,cmd->dest->palette,0,NULL,false);
	This->outputs[0] = DD_OK;
	SetEvent(This->busy);
}

void glRenderer__MakeTexture(glRenderer *This, glTexture *texture)
{
	glTexture__FinishCreate(texture);
}

void glRenderer__DrawBackbuffer(glRenderer *This, glTexture **texture, int x, int y, int progtype, BOOL paletted, BOOL firstpass)
{
	GLfloat view[4];
	DDSURFACEDESC2 ddsd;
	DWORD x2, y2;
	x2 = x * This->postsizex;
	y2 = y * This->postsizey;
	glUtil_SetActiveTexture(This->util,8);
	if(!This->backbuffer)
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		memcpy(&ddsd, &ddsdbackbuffer, sizeof(DDSURFACEDESC2));
		ddsd.dwWidth = x2;
		ddsd.lPitch = x2 * 4;
		ddsd.dwHeight = y2;
		glTexture_Create(&ddsd, &This->backbuffer, This, x2, y2, FALSE, TRUE);
	}
	if((This->backbuffer->levels[0].ddsd.dwWidth != x2) || (This->backbuffer->levels[0].ddsd.dwHeight != y2))
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwWidth = x2;
		ddsd.dwHeight = y2;
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		glTexture__SetSurfaceDesc(This->backbuffer, &ddsd);
	}
	glUtil_SetFBOTextures(This->util,&This->fbo,This->backbuffer,NULL,0,0,FALSE);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)x2;
	view[3] = (GLfloat)y2;
	glUtil_SetViewport(This->util,0,0,x2,y2);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glUtil_SetTexture(This->util,8,*texture);
	*texture = This->backbuffer;
	if (!paletted && firstpass && (dxglcfg.postfilter == 1))
		glTexture__SetFilter(*texture, 8, GL_LINEAR, GL_LINEAR, This);
	else glTexture__SetFilter(*texture, 8, GL_NEAREST, GL_NEAREST, This);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view,view[0],view[1],view[2],view[3]);
	This->bltvertices[0].s = This->bltvertices[0].t = This->bltvertices[1].t = This->bltvertices[2].s = 1.;
	This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.;
	This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
	This->bltvertices[0].x = This->bltvertices[2].x = (float)x2;
	This->bltvertices[2].y = This->bltvertices[3].y = (float)y2;
	glUtil_EnableArray(This->util,This->shaders->shaders[progtype].pos,TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].x);
	glUtil_EnableArray(This->util,This->shaders->shaders[progtype].texcoord,TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].s);
	glUtil_SetCull(This->util,D3DCULL_NONE);
	glUtil_SetPolyMode(This->util,D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	glUtil_SetFBO(This->util, NULL);
}

void glRenderer__DrawBackbufferRect(glRenderer *This, glTexture *texture, RECT srcrect, RECT destrect, int progtype)
{
	GLfloat view[4];
	DDSURFACEDESC2 ddsd;
	int x1 = destrect.left;
	int x2 = destrect.right;
	int y1 = destrect.top;
	int y2 = destrect.bottom;
	glUtil_SetActiveTexture(This->util, 0);
	if (!This->backbuffer)
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		memcpy(&ddsd, &ddsdbackbuffer, sizeof(DDSURFACEDESC2));
		ddsd.dwWidth = x2;
		ddsd.lPitch = x2 * 4;
		ddsd.dwHeight = y2;
		glTexture_Create(&ddsd, &This->backbuffer, This, x2, y2, FALSE, TRUE);
	}
	if ((This->backbuffer->levels[0].ddsd.dwWidth < x2) || (This->backbuffer->levels[0].ddsd.dwHeight < y2))
	{
		if (This->backbuffer->levels[0].ddsd.dwWidth > x2) x2 = This->backbuffer->levels[0].ddsd.dwWidth;
		if (This->backbuffer->levels[0].ddsd.dwHeight > y2) y2 = This->backbuffer->levels[0].ddsd.dwHeight;
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwWidth = x2;
		ddsd.dwHeight = y2;
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		glTexture__SetSurfaceDesc(This->backbuffer, &ddsd);
		x2 = destrect.right;
		y2 = destrect.bottom;
	}
	glUtil_SetFBOTextures(This->util, &This->fbo, This->backbuffer, NULL, 0, 0, FALSE);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)This->backbuffer->levels[0].ddsd.dwWidth;
	view[3] = (GLfloat)This->backbuffer->levels[0].ddsd.dwHeight;
	glUtil_SetViewport(This->util, 0, 0, This->backbuffer->levels[0].ddsd.dwWidth, This->backbuffer->levels[0].ddsd.dwHeight);
	glUtil_SetScissor(This->util, TRUE, 0, 0, This->backbuffer->levels[0].ddsd.dwWidth, This->backbuffer->levels[0].ddsd.dwHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUtil_SetScissor(This->util, FALSE, 0, 0, 0, 0);
	glUtil_SetTexture(This->util, 8, texture);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view, view[0], view[1], view[2], view[3]);
	This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left / (GLfloat)texture->levels[0].ddsd.dwWidth;
	This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right / (GLfloat)texture->levels[0].ddsd.dwWidth;
	This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top / (GLfloat)texture->levels[0].ddsd.dwHeight;
	This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom / (GLfloat)texture->levels[0].ddsd.dwHeight;
	This->bltvertices[1].x = This->bltvertices[3].x = (float)x1;
	This->bltvertices[0].x = This->bltvertices[2].x = (float)x2;
	This->bltvertices[0].y = This->bltvertices[1].y = (float)y1;
	This->bltvertices[2].y = This->bltvertices[3].y = (float)y2;
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].pos, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].x);
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].texcoord, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].s);
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_SHORT, bltindices);
	glUtil_SetFBO(This->util, NULL);
}

BOOL Is512448Scale(glRenderer *This, glTexture *primary, glTexture *palette)
{
	DWORD *ptr32;
	WORD *ptr16;
	BYTE *ptr8;
	if (dxglcfg.HackAutoScale512448to640480)
	{
		if ((primary->levels[0].ddsd.dwWidth == 640) && (primary->levels[0].ddsd.dwHeight == 480))
		{
			if (primary->levels[0].dirty & 2) glTexture__Download(primary, 0);
			if (primary->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
			{
				ptr8 = (BYTE*)primary->levels[0].buffer;
				ptr32 = (DWORD*)palette->levels[0].buffer;
				if (!(ptr32[ptr8[0]] & 0xFFFFFF)) return TRUE;
				else return FALSE;
			}
			else if (primary->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
			{
				ptr16 = (WORD*)primary->levels[0].buffer;
				if (!ptr16[0]) return TRUE;
				else return FALSE;
			}
			else
			{
				ptr32 = (DWORD*)primary->levels[0].buffer;
				if (!(ptr32[0] & 0xFFFFFF)) return TRUE;
				else return FALSE;
			}
		}
		else if ((primary->levels[0].ddsd.dwWidth == 320) && (primary->levels[0].ddsd.dwHeight == 240))
		{
			if (primary->levels[0].dirty & 2) glTexture__Download(primary, 0);
			if (primary->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
			{
				ptr8 = (BYTE*)primary->levels[0].buffer;
				ptr32 = (DWORD*)palette->levels[0].buffer;
				if (!(ptr32[ptr8[0]] & 0xFFFFFF)) return TRUE;
				else return FALSE;
			}
			else if (primary->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
			{
				ptr16 = (WORD*)primary->levels[0].buffer;
				if (!ptr16[0]) return TRUE;
				else return FALSE;
			}
			else
			{
				ptr32 = (DWORD*)primary->levels[0].buffer;
				if (!(ptr32[0] & 0xFFFFFF)) return TRUE;
				else return FALSE;
			}
		}
		else return FALSE;
	}
	else return FALSE;
}

void glRenderer__DrawScreen(glRenderer *This, glTexture *texture, glTexture *paltex, GLint vsync, glTexture *previous, BOOL setsync)
{
	int progtype;
	RECT r, r2;
	BOOL scale512448 = Is512448Scale(This, texture, paltex);
	glUtil_BlendEnable(This->util, FALSE);
	if (previous) previous->levels[0].ddsd.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
	texture->levels[0].ddsd.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
	if((texture->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
	{
		GetClientRect(This->hWnd,&r);
		GetClientRect(This->RenderWnd->GetHWnd(),&r2);
		if(memcmp(&r2,&r,sizeof(RECT)))
		SetWindowPos(This->RenderWnd->GetHWnd(),NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
	}
	glUtil_DepthTest(This->util, FALSE);
	RECT *viewrect = &r2;
	glRenderer__SetSwap(This,vsync);
	LONG sizes[6];
	GLfloat view[4];
	GLint viewport[4];
	if(texture->levels[0].dirty & 1) glTexture__Upload(texture, 0);
	if(texture->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(This->ddInterface->GetFullscreen())
		{
			This->ddInterface->GetSizes(sizes);
			if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
				(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
			{
				if (sizes[2] <= 400) This->postsizex = 2.0f;
				else This->postsizex = 1.0f;
				if (sizes[3] <= 300) This->postsizey = 2.0f;
				else This->postsizey = 1.0f;
			}
			else
			{
				This->postsizex = dxglcfg.postsizex;
				This->postsizey = dxglcfg.postsizey;
			}
			viewport[0] = viewport[1] = 0;
			viewport[2] = sizes[4];
			viewport[3] = sizes[5];
			view[0] = (GLfloat)-(sizes[4]-sizes[0])/2;
			view[1] = (GLfloat)(sizes[4]-sizes[0])/2+sizes[0];
			view[2] = (GLfloat)(sizes[5]-sizes[1])/2+sizes[1];
			view[3] = (GLfloat)-(sizes[5]-sizes[1])/2;
		}
		else
		{
			This->postsizex = 1.0f;
			This->postsizey = 1.0f;
			viewport[0] = viewport[1] = 0;
			viewport[2] = viewrect->right;
			viewport[3] = viewrect->bottom;
			ClientToScreen(This->RenderWnd->GetHWnd(),(LPPOINT)&viewrect->left);
			ClientToScreen(This->RenderWnd->GetHWnd(),(LPPOINT)&viewrect->right);
			view[0] = (GLfloat)viewrect->left;
			view[1] = (GLfloat)viewrect->right;
			view[2] = (GLfloat)texture->bigheight-(GLfloat)viewrect->top;
			view[3] = (GLfloat)texture->bigheight-(GLfloat)viewrect->bottom;
		}
	}
	else
	{
		view[0] = 0;
		view[1] = (GLfloat)texture->bigwidth;
		view[2] = 0;
		view[3] = (GLfloat)texture->bigheight;
	}
	glUtil_SetFBO(This->util, NULL);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if(This->ddInterface->GetBPP() == 8)
	{
		ShaderManager_SetShader(This->shaders,PROG_PAL256,NULL,0);
		progtype = PROG_PAL256;
		glTexture__Upload(paltex, 0);
		This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,8);
		This->ext->glUniform1i(This->shaders->shaders[progtype].pal,9);
		glUtil_SetTexture(This->util,8,texture);
		glUtil_SetTexture(This->util,9,paltex);
		if(dxglcfg.scalingfilter || (This->postsizex != 1.0f) || (This->postsizey != 1.0f))
		{
			glRenderer__DrawBackbuffer(This,&texture,texture->bigwidth,texture->bigheight,progtype,TRUE,TRUE);
			ShaderManager_SetShader(This->shaders,PROG_TEXTURE,NULL,0);
			progtype = PROG_TEXTURE;
			glUtil_SetTexture(This->util,8,texture);
			This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,8);
		}
	}
	else
	{
		if ((This->postsizex != 1.0f) || (This->postsizey != 1.0f))
		{
			progtype = PROG_TEXTURE;
			ShaderManager_SetShader(This->shaders, PROG_TEXTURE, NULL, 0);
			glRenderer__DrawBackbuffer(This, &texture, texture->bigwidth, texture->bigheight, progtype, FALSE, TRUE);
			glUtil_SetTexture(This->util, 8, texture);
			This->ext->glUniform1i(This->shaders->shaders[progtype].tex0, 0);
		}
		ShaderManager_SetShader(This->shaders,PROG_TEXTURE,NULL,0);
		progtype = PROG_TEXTURE;
		glUtil_SetTexture(This->util,8,texture);
		This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,8);
	}
	if (dxglcfg.scalingfilter) glTexture__SetFilter(texture, 8, GL_LINEAR, GL_LINEAR, This);
	else glTexture__SetFilter(texture, 8, GL_NEAREST, GL_NEAREST, This);
	glUtil_SetViewport(This->util,viewport[0],viewport[1],viewport[2],viewport[3]);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view,view[0],view[1],view[2],view[3]);
	if(This->ddInterface->GetFullscreen())
	{
		This->bltvertices[0].x = This->bltvertices[2].x = (float)sizes[0];
		This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
		This->bltvertices[2].y = This->bltvertices[3].y = (float)sizes[1];
	}
	else
	{
		This->bltvertices[0].x = This->bltvertices[2].x = (float)texture->bigwidth;
		This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
		This->bltvertices[2].y = This->bltvertices[3].y = (float)texture->bigheight;
	}
	if (scale512448)
	{
		This->bltvertices[0].s = This->bltvertices[2].s = 0.9f;
		This->bltvertices[0].t = This->bltvertices[1].t = 0.966666667f;
		This->bltvertices[1].s = This->bltvertices[3].s = 0.1f;
		This->bltvertices[2].t = This->bltvertices[3].t = 0.0333333333f;
	}
	else
	{
		This->bltvertices[0].s = This->bltvertices[0].t = This->bltvertices[1].t = This->bltvertices[2].s = 1.;
		This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.;
	}
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].pos, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].x);
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].texcoord, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].s);
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	glFlush();
	if(This->hWnd) SwapBuffers(This->hDC);
	else
	{
		glReadBuffer(GL_FRONT);
		BufferObject_Bind(This->pbo, GL_PIXEL_PACK_BUFFER);
		GLint packalign;
		glGetIntegerv(GL_PACK_ALIGNMENT,&packalign);
		glPixelStorei(GL_PACK_ALIGNMENT,1);
		This->ddInterface->GetSizes(sizes);
		glReadPixels(0,0,sizes[4],sizes[5],GL_BGRA,GL_UNSIGNED_BYTE,0);
		BufferObject_Map(This->pbo, GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		GLubyte *pixels = (GLubyte*)This->pbo->pointer;
		for(int i = 0; i < sizes[5];i++)
		{
			memcpy(&This->dib.pixels[This->dib.pitch*i],
				&pixels[((sizes[5]-1)-i)*(sizes[4]*4)],sizes[4]*4);
		}
		BufferObject_Unmap(This->pbo, GL_PIXEL_PACK_BUFFER);
		BufferObject_Unbind(This->pbo, GL_PIXEL_PACK_BUFFER);
		glPixelStorei(GL_PACK_ALIGNMENT,packalign);
		HDC hRenderDC = (HDC)::GetDC(This->RenderWnd->GetHWnd());
		HGDIOBJ hPrevObj = 0;
		POINT dest = {0,0};
		POINT srcpoint = {0,0};
		SIZE wnd = {This->dib.width,This->dib.height};
		BLENDFUNCTION func = {AC_SRC_OVER,0,255,AC_SRC_ALPHA};
		hPrevObj = SelectObject(This->dib.hdc,This->dib.hbitmap);
		ClientToScreen(This->RenderWnd->GetHWnd(),&dest);
		UpdateLayeredWindow(This->RenderWnd->GetHWnd(),hRenderDC,&dest,&wnd,
			This->dib.hdc,&srcpoint,0,&func,ULW_ALPHA);
		SelectObject(This->dib.hdc,hPrevObj);
		ReleaseDC(This->RenderWnd->GetHWnd(),hRenderDC);
	}
	if(setsync) SetEvent(This->busy);

}

void glRenderer__DeleteTexture(glRenderer *This, glTexture *texture)
{
	glTexture__Destroy(texture);
	SetEvent(This->busy);
}

__int64 InitShaderState(glRenderer *renderer, DWORD *renderstate, TEXTURESTAGE *texstages, D3DLIGHT7 *lights)
{
	int i;
	__int64 shader = 0;
	switch (renderstate[D3DRENDERSTATE_SHADEMODE])
	{
	case D3DSHADE_FLAT:
	default:
		break;
	case D3DSHADE_GOURAUD:
		shader |= 1;
		break;
	case D3DSHADE_PHONG:
		shader |= 3;
		break;
	}
	if (renderstate[D3DRENDERSTATE_ALPHATESTENABLE]) shader |= 4;
	shader |= ((((__int64)renderstate[D3DRENDERSTATE_ALPHAFUNC] - 1) & 7) << 3);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_FOGTABLEMODE] & 3) << 6);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_FOGVERTEXMODE] & 3) << 8);
	if (renderstate[D3DRENDERSTATE_RANGEFOGENABLE]) shader |= (1i64 << 10);
	if (renderstate[D3DRENDERSTATE_SPECULARENABLE]) shader |= (1i64 << 11);
	if (renderstate[D3DRENDERSTATE_STIPPLEDALPHA]) shader |= (1i64 << 12);
	if (renderstate[D3DRENDERSTATE_COLORKEYENABLE]) shader |= (1i64 << 13);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_ZBIAS] & 15) << 14);
	int numlights = 0;
	for (i = 0; i < 8; i++)
		if (lights[i].dltType) numlights++;
	shader |= (__int64)numlights << 18;
	if (renderstate[D3DRENDERSTATE_LOCALVIEWER]) shader |= (1i64 << 21);
	if (renderstate[D3DRENDERSTATE_COLORKEYBLENDENABLE]) shader |= (1i64 << 22);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE] & 3) << 23);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_SPECULARMATERIALSOURCE] & 3) << 25);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_AMBIENTMATERIALSOURCE] & 3) << 27);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE] & 3) << 29);
	int lightindex = 0;
	for (i = 0; i < 8; i++)
	{
		if (lights[i].dltType)
		{
			if (lights[i].dltType != D3DLIGHT_DIRECTIONAL)
				shader |= (1i64 << (38 + lightindex));
			if (lights[i].dltType == D3DLIGHT_SPOT)
				shader |= (1i64 << (51 + lightindex));
			lightindex++;
		}
	}
	if (renderstate[D3DRENDERSTATE_NORMALIZENORMALS]) shader |= (1i64 << 49);
	if (renderstate[D3DRENDERSTATE_TEXTUREMAPBLEND] == D3DTBLEND_MODULATE)
	{
		bool noalpha = false;;
		if (!texstages[0].texture) noalpha = true;
		if (texstages[0].texture)
			if (!(texstages[0].texture->levels[0].ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS))
				noalpha = true;
		if (noalpha) texstages[0].alphaop = D3DTOP_SELECTARG2;
		else texstages[0].alphaop = D3DTOP_MODULATE;
	}
	if (renderstate[D3DRENDERSTATE_LIGHTING]) shader |= (1i64 << 59);
	if (renderstate[D3DRENDERSTATE_COLORVERTEX]) shader |= (1i64 << 60);
	if (renderstate[D3DRENDERSTATE_FOGENABLE]) shader |= (1i64 << 61);
	if (renderstate[D3DRENDERSTATE_DITHERENABLE]) shader |= (1i64 << 62);
	for (i = 0; i < 8; i++)
	{
		renderer->shaderstate3d.texstageid[i] = texstages[i].colorop & 31;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].colorarg1 & 63) << 5;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].colorarg2 & 63) << 11;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].alphaop & 31) << 17;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].alphaarg1 & 63) << 22;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].alphaarg2 & 63) << 28;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].texcoordindex & 7) << 34;
		renderer->shaderstate3d.texstageid[i] |= (__int64)((texstages[i].texcoordindex >> 16) & 3) << 37;
		renderer->shaderstate3d.texstageid[i] |= (__int64)((texstages[i].addressu - 1) & 3) << 39;
		renderer->shaderstate3d.texstageid[i] |= (__int64)((texstages[i].addressv - 1) & 3) << 41;
		//renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].magfilter & 7) << 43;
		//renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].minfilter & 3) << 46;
		//renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].mipfilter & 3) << 48;
		if (texstages[i].textransform & 7)
		{
			renderer->shaderstate3d.texstageid[i] |= 1i64 << 50;
			renderer->shaderstate3d.texstageid[i] |= (__int64)(((texstages[i].textransform & 7) - 1) & 3) << 51;
		}
		if (texstages[i].textransform & D3DTTFF_PROJECTED) renderer->shaderstate3d.texstageid[i] |= 1i64 << 53;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].texcoordindex & 7) << 54;
		renderer->shaderstate3d.texstageid[i] |= (__int64)((texstages[i].texcoordindex >> 16) & 3) << 57;
		if (texstages[i].texture)
		{
			renderer->shaderstate3d.texstageid[i] |= 1i64 << 59;
			if (texstages[i].texture->levels[0].ddsd.dwFlags & DDSD_CKSRCBLT) renderer->shaderstate3d.texstageid[i] |= 1i64 << 60;
		}
	}
	return shader;
}

void glRenderer__InitD3D(glRenderer *This, int zbuffer, int x, int y)
{
	SetEvent(This->busy);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	GLfloat ambient[] = {0.0,0.0,0.0,0.0};
	if (zbuffer) glUtil_DepthTest(This->util, TRUE);
	glUtil_SetDepthComp(This->util, GL_LEQUAL);
	GLfloat identity[16];
	__gluMakeIdentityf(identity);
	glUtil_SetMatrix(This->util, GL_MODELVIEW, identity, identity, NULL);
	glUtil_SetMatrix(This->util, GL_PROJECTION, identity, NULL, NULL);
	for (int i = 0; i < 24; i++)
		memcpy(&This->transform[i], identity, sizeof(D3DMATRIX));
	GLfloat one[4] = {1,1,1,1};
	GLfloat zero[4] = {0,0,0,1};
	glUtil_SetMaterial(This->util, one, one, zero, zero, 0);
	ZeroMemory(&This->material, sizeof(D3DMATERIAL7));
	ZeroMemory(&This->lights, 8 * sizeof(D3DLIGHT7));
	memcpy(&This->renderstate, &renderstate_default, RENDERSTATE_COUNT * sizeof(DWORD));
	This->texstages[0] = texstagedefault0;
	This->texstages[1] = This->texstages[2] = This->texstages[3] = This->texstages[4] =
		This->texstages[5] = This->texstages[6] = This->texstages[7] = This->texstages[8] =
		This->texstages[9] = This->texstages[10] = This->texstages[11] = texstagedefault1;
	This->viewport.dwX = 0;
	This->viewport.dwY = 0;
	This->viewport.dwWidth = x;
	This->viewport.dwHeight = y;
	This->viewport.dvMinZ = 0.0f;
	This->viewport.dvMaxZ = 1.0f;
	This->shaderstate3d.stateid = InitShaderState(This, This->renderstate, This->texstages, This->lights);
}

void glRenderer__Clear(glRenderer *This, ClearCommand *cmd)
{
	This->outputs[0] = (void*)D3D_OK;
	GLfloat color[4];
	glTexture *ztexture = NULL;
	GLint zlevel = 0;
	GLfloat mulx, muly;
	GLsizei x1, y1, x2, y2;
	if (cmd->zbuffer)
	{
		ztexture = cmd->zbuffer;
		zlevel = cmd->zlevel;
	}
	dwordto4float(cmd->dwColor,color);
	do
	{
		if (glUtil_SetFBOSurface(This->util, cmd->target, ztexture,
			cmd->targetlevel, zlevel, FALSE) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!cmd->target->internalformats[1]) break;
		glTexture__Repair(cmd->target, TRUE);
		glUtil_SetFBO(This->util, NULL);
		cmd->target->levels[cmd->targetlevel].fbo.fbcolor = NULL;
		cmd->target->levels[cmd->targetlevel].fbo.fbz = NULL;
	} while (1);
	int clearbits = 0;
	if(cmd->dwFlags & D3DCLEAR_TARGET)
	{
		clearbits |= GL_COLOR_BUFFER_BIT;
		glUtil_ClearColor(This->util, color[0], color[1], color[2], color[3]);
	}
	if(cmd->dwFlags & D3DCLEAR_ZBUFFER)
	{
		clearbits |= GL_DEPTH_BUFFER_BIT;
		glUtil_ClearDepth(This->util, cmd->dvZ);
		glUtil_DepthWrite(This->util, TRUE);
	}
	if(cmd->dwFlags & D3DCLEAR_STENCIL)
	{
		clearbits |= GL_STENCIL_BUFFER_BIT;
		glUtil_ClearStencil(This->util, cmd->dwStencil);
	}
	if(cmd->dwCount)
	{
		if (cmd->targetlevel == 0 && (cmd->target->levels[0].ddsd.dwWidth != cmd->target->bigwidth) ||
			(cmd->target->levels[0].ddsd.dwHeight != cmd->target->bigheight))
		{
			mulx = (GLfloat)cmd->target->bigwidth / (GLfloat)cmd->target->levels[0].ddsd.dwWidth;
			muly = (GLfloat)cmd->target->bigheight / (GLfloat)cmd->target->levels[0].ddsd.dwHeight;
			for (DWORD i = 0; i < cmd->dwCount; i++)
			{
				x1 = (GLsizei)((GLfloat)cmd->lpRects[i].x1) * mulx;
				x2 = ((GLsizei)((GLfloat)cmd->lpRects[i].x2) * mulx) - x1;
				y1 = (GLsizei)((GLfloat)cmd->lpRects[i].y1) * muly;
				y2 = ((GLsizei)((GLfloat)cmd->lpRects[i].y2) * muly) - y1;
				glUtil_SetScissor(This->util, TRUE, x1, y1, x2, y2);
				glClear(clearbits);
			}
		}
		else
		{
			for (DWORD i = 0; i < cmd->dwCount; i++)
			{
				glUtil_SetScissor(This->util, TRUE, cmd->lpRects[i].x1, cmd->lpRects[i].y1,
					(cmd->lpRects[i].x2 - cmd->lpRects[i].x1), cmd->lpRects[i].y2 - cmd->lpRects[i].y1);
				glClear(clearbits);
			}
		}
		glUtil_SetScissor(This->util, false, 0, 0, 0, 0);
	}
	else glClear(clearbits);
	if(cmd->zbuffer) cmd->zbuffer->levels[zlevel].dirty |= 2;
	cmd->target->levels[cmd->targetlevel].dirty |= 2;
	SetEvent(This->busy);
}

void glRenderer__Flush(glRenderer *This)
{
	glFlush();
	SetEvent(This->busy);
}

void glRenderer__SetWnd(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND newwnd, BOOL devwnd)
{
	if(newwnd != This->hWnd)
	{
		EnterCriticalSection(&dll_cs);
		wglMakeCurrent(NULL, NULL);
		ReleaseDC(This->hWnd,This->hDC);
		delete This->RenderWnd;
		This->RenderWnd = new glRenderWindow(width,height,fullscreen,newwnd,This->ddInterface, devwnd);
		PIXELFORMATDESCRIPTOR pfd;
		GLuint pf;
		InterlockedIncrement(&gllock);
		ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		if (dxglcfg.SingleBufferDevice) pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
		else pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = bpp;
		pfd.iLayerType = PFD_MAIN_PLANE;
		This->hDC = GetDC(This->RenderWnd->GetHWnd());
		if(!This->hDC)
			DEBUG("glRenderer::SetWnd: Can not create hDC\n");
		pf = ChoosePixelFormat(This->hDC,&pfd);
		if(!pf)
			DEBUG("glRenderer::SetWnd: Can not get pixelformat\n");
		if(!SetPixelFormat(This->hDC,pf,&pfd))
			DEBUG("glRenderer::SetWnd: Can not set pixelformat\n");
		if(!wglMakeCurrent(This->hDC,This->hRC))
			DEBUG("glRenderer::SetWnd: Can not activate GL context\n");
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		glRenderer__SetSwap(This,1);
		SwapBuffers(This->hDC);
		DXGLTimer_Init(&This->timer);
		DXGLTimer_Calibrate(&This->timer, height, frequency);
		glRenderer__SetSwap(This,0);
		glUtil_SetViewport(This->util, 0, 0, width, height);
	}
	if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
		(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
	{
		if (width <= 400) This->postsizex = 2.0f;
		else This->postsizex = 1.0f;
		if (height <= 300) This->postsizey = 2.0f;
		else This->postsizey = 1.0f;
	}
	else
	{
		This->postsizex = dxglcfg.postsizex;
		This->postsizey = dxglcfg.postsizey;
	}
	SetEvent(This->busy);
}

void glRenderer__SetBlend(glRenderer *This, DWORD src, DWORD dest)
{
	GLenum glsrc, gldest;
	bool bothalpha = false;
	switch(src)
	{
	case D3DBLEND_ZERO:
		glsrc = GL_ZERO;
		break;
	case D3DBLEND_ONE:
	default:
		glsrc = GL_ONE;
		break;
	case D3DBLEND_SRCCOLOR:
		glsrc = GL_SRC_COLOR;
		break;
	case D3DBLEND_INVSRCCOLOR:
		glsrc = GL_ONE_MINUS_SRC_COLOR;
		break;
	case D3DBLEND_SRCALPHA:
		glsrc = GL_SRC_ALPHA;
		break;
	case D3DBLEND_INVSRCALPHA:
		glsrc = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case D3DBLEND_DESTALPHA:
		glsrc = GL_DST_ALPHA;
		break;
	case D3DBLEND_INVDESTALPHA:
		glsrc = GL_ONE_MINUS_DST_ALPHA;
		break;
	case D3DBLEND_DESTCOLOR:
		glsrc = GL_DST_COLOR;
		break;
	case D3DBLEND_INVDESTCOLOR:
		glsrc = GL_ONE_MINUS_DST_COLOR;
		break;
	case D3DBLEND_SRCALPHASAT:
		glsrc = GL_SRC_ALPHA_SATURATE;
		break;
	case D3DBLEND_BOTHSRCALPHA:
		bothalpha = true;
		glsrc = GL_SRC_ALPHA;
		gldest = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case D3DBLEND_BOTHINVSRCALPHA:
		bothalpha = true;
		glsrc = GL_ONE_MINUS_SRC_ALPHA;
		gldest = GL_SRC_ALPHA;
		break;
	}

	if(!bothalpha) switch(dest)
	{
	case D3DBLEND_ZERO:
	default:
		gldest = GL_ZERO;
		break;
	case D3DBLEND_ONE:
		gldest = GL_ONE;
		break;
	case D3DBLEND_SRCCOLOR:
		gldest = GL_SRC_COLOR;
		break;
	case D3DBLEND_INVSRCCOLOR:
		gldest = GL_ONE_MINUS_SRC_COLOR;
		break;
	case D3DBLEND_SRCALPHA:
		gldest = GL_SRC_ALPHA;
		break;
	case D3DBLEND_INVSRCALPHA:
		gldest = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case D3DBLEND_DESTALPHA:
		gldest = GL_DST_ALPHA;
		break;
	case D3DBLEND_INVDESTALPHA:
		gldest = GL_ONE_MINUS_DST_ALPHA;
		break;
	case D3DBLEND_DESTCOLOR:
		gldest = GL_DST_COLOR;
		break;
	case D3DBLEND_INVDESTCOLOR:
		gldest = GL_ONE_MINUS_DST_COLOR;
		break;
	case D3DBLEND_SRCALPHASAT:
		gldest = GL_SRC_ALPHA_SATURATE;
		break;
	case D3DBLEND_BOTHSRCALPHA:
		bothalpha = true;
		glsrc = GL_SRC_ALPHA;
		gldest = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case D3DBLEND_BOTHINVSRCALPHA:
		bothalpha = true;
		glsrc = GL_ONE_MINUS_SRC_ALPHA;
		gldest = GL_SRC_ALPHA;
		break;
	}
	glUtil_BlendFunc(This->util, glsrc, gldest);
}

void glRenderer__UpdateFVF(glRenderer *This, DWORD fvf)
{
}

void glRenderer__DrawPrimitives(glRenderer *This, RenderTarget *target, GLenum mode, DWORD fvf,
	BYTE *vertices, BOOL strided, DWORD count, LPWORD indices, DWORD indexcount, DWORD flags)
{
	BOOL transformed;
	int i;
	glTexture *ztexture = NULL;
	GLint zlevel = 0;
	if (!vertices)
	{
		This->outputs[0] = (void*)DDERR_INVALIDPARAMS;
		SetEvent(This->busy);
		return;
	}
	if (fvf != This->last_fvf) glRenderer__UpdateFVF(This, fvf);
	glRenderer__SetDepthComp(This);
	glUtil_DepthTest(This->util, This->renderstate[D3DRENDERSTATE_ZENABLE]);
	glUtil_DepthWrite(This->util, This->renderstate[D3DRENDERSTATE_ZWRITEENABLE]);
	_GENSHADER *prog = &This->shaders->gen3d->current_genshader->shader;
	switch (fvf & D3DFVF_POSITION_MASK)
	{
	case 0: // Missing vertex position
		This->outputs[0] = (void*)DDERR_INVALIDPARAMS;
		SetEvent(This->busy);
		return;
	case D3DFVF_XYZ:
		glUtil_EnableArray(This->util, prog->attribs[0], TRUE);
		This->ext->glVertexAttribPointer(prog->attribs[0], 3, GL_FLOAT, GL_FALSE, This->fvf_stride, vertices);
		break;
	case D3DFVF_XYZRHW:
		glUtil_EnableArray(This->util, prog->attribs[0], TRUE);
		This->ext->glVertexAttribPointer(prog->attribs[0], 3, GL_FLOAT, GL_FALSE, This->fvf_stride, vertices);
		if (prog->attribs[1] != -1)
		{
			glUtil_EnableArray(This->util, prog->attribs[1], TRUE);
			This->ext->glVertexAttribPointer(prog->attribs[1], 1, GL_FLOAT, GL_FALSE, This->fvf_stride, vertices+(3*sizeof(float)));
		}
		break;
	case D3DFVF_XYZB1:
	case D3DFVF_XYZB2:
	case D3DFVF_XYZB3:
	case D3DFVF_XYZB4:
	case D3DFVF_XYZB5:
		FIXME("glRenderer__DrawPrimitives: Blend weights not yet supported");
		This->outputs[0] = (void*)DDERR_INVALIDPARAMS;
		SetEvent(This->busy);
		return;
		break;
	}
}

void glRenderer__DrawPrimitivesOld(glRenderer *This, RenderTarget *target, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags)
{
	bool transformed;
	int i;
	glTexture *ztexture = NULL;
	GLint zlevel = 0;
	if (target->zbuffer)
	{
		ztexture = target->zbuffer;
		zlevel = target->zlevel;
	}
	if(vertices[1].data) transformed = true;
	else transformed = false;
	if(!vertices[0].data)
	{
		This->outputs[0] = (void*)DDERR_INVALIDPARAMS;
		SetEvent(This->busy);
		return;
	}
	This->shaderstate3d.stateid &= 0xFFFA3FF87FFFFFFFi64;
	int numtextures = 0;
	for (i = 0; i < 8; i++)
	{
		This->shaderstate3d.texstageid[i] &= 0xFFE7FFFFFFFFFFFFi64;
		This->shaderstate3d.texstageid[i] |= (__int64)(texformats[i] - 1) << 51;
		if (vertices[i + 10].data) numtextures++;
	}
	This->shaderstate3d.stateid |= (__int64)((numtextures-1)&7) << 31;
	if (numtextures) This->shaderstate3d.stateid |= (1i64 << 34);
	int blendweights = 0;
	for (i = 0; i < 5; i++)
		if (vertices[i + 2].data) blendweights++;
	This->shaderstate3d.stateid |= (__int64)blendweights << 46;
	if (vertices[1].data) This->shaderstate3d.stateid |= (1i64 << 50);
	if (vertices[8].data) This->shaderstate3d.stateid |= (1i64 << 35);
	if (vertices[9].data) This->shaderstate3d.stateid |= (1i64 << 36);
	if (vertices[7].data) This->shaderstate3d.stateid |= (1i64 << 37);
	ShaderManager_SetShader(This->shaders,This->shaderstate3d.stateid,This->shaderstate3d.texstageid,2);
	glRenderer__SetDepthComp(This);
	glUtil_DepthTest(This->util, This->renderstate[D3DRENDERSTATE_ZENABLE]);
	glUtil_DepthWrite(This->util, This->renderstate[D3DRENDERSTATE_ZWRITEENABLE]);
	_GENSHADER *prog = &This->shaders->gen3d->current_genshader->shader;
	glUtil_EnableArray(This->util, prog->attribs[0], TRUE);
	This->ext->glVertexAttribPointer(prog->attribs[0],3,GL_FLOAT,GL_FALSE,vertices[0].stride,vertices[0].data);
	if(transformed)
	{
		if(prog->attribs[1] != -1)
		{
			glUtil_EnableArray(This->util, prog->attribs[1], TRUE);
			This->ext->glVertexAttribPointer(prog->attribs[1],1,GL_FLOAT,GL_FALSE,vertices[1].stride,vertices[1].data);
		}
	}
	for(i = 0; i < 5; i++)
	{
		if(vertices[i+2].data)
		{
			if(prog->attribs[i+2] != -1)
			{
				glUtil_EnableArray(This->util, prog->attribs[i + 2], TRUE);
				This->ext->glVertexAttribPointer(prog->attribs[i+2],1,GL_FLOAT,GL_FALSE,vertices[i+2].stride,vertices[i+2].data);
			}
		}
	}
	if(vertices[7].data)
	{
		if(prog->attribs[7] != -1)
		{
			glUtil_EnableArray(This->util, prog->attribs[7], TRUE);
			This->ext->glVertexAttribPointer(prog->attribs[7],3,GL_FLOAT,GL_FALSE,vertices[7].stride,vertices[7].data);
		}
	}
	for(i = 0; i < 2; i++)
	{
		if(vertices[i+8].data)
		{
			if(prog->attribs[8+i] != -1)
			{
				glUtil_EnableArray(This->util, prog->attribs[8 + i], TRUE);
				This->ext->glVertexAttribPointer(prog->attribs[8+i],4,GL_UNSIGNED_BYTE,GL_TRUE,vertices[i+8].stride,vertices[i+8].data);
			}
		}
	}
	for(i = 0; i < 8; i++)
	{
		{
			switch(texformats[i])
			{
			case -1: // Null
				break;
			case 1: // s
				if (prog->attribs[i + 10] != -1)
				{
					glUtil_EnableArray(This->util, prog->attribs[i + 10], TRUE);
					This->ext->glVertexAttribPointer(prog->attribs[i + 10], 1, GL_FLOAT, GL_FALSE, vertices[i + 10].stride, vertices[i + 10].data);
				}
				break;
			case 2: // st
				if(prog->attribs[i+18] != -1)
				{
					glUtil_EnableArray(This->util, prog->attribs[i + 18], TRUE);
					This->ext->glVertexAttribPointer(prog->attribs[i+18],2,GL_FLOAT,GL_FALSE,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			case 3: // str
				if(prog->attribs[i+26] != -1)
				{
					glUtil_EnableArray(This->util, prog->attribs[i + 26], TRUE);
					This->ext->glVertexAttribPointer(prog->attribs[i+26],3,GL_FLOAT,GL_FALSE,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			case 4: // strq
				if(prog->attribs[i+34] != -1)
				{
					glUtil_EnableArray(This->util, prog->attribs[i + 34], TRUE);
					This->ext->glVertexAttribPointer(prog->attribs[i+34],4,GL_FLOAT,false,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			}

		}
	}
	glUtil_SetMatrix(This->util, GL_MODELVIEW, (GLfloat*)&This->transform[D3DTRANSFORMSTATE_VIEW],
		(GLfloat*)&This->transform[D3DTRANSFORMSTATE_WORLD],NULL);
	glUtil_SetMatrix(This->util, GL_PROJECTION, (GLfloat*)&This->transform[D3DTRANSFORMSTATE_PROJECTION], NULL, NULL);

	glUtil_SetMaterial(This->util, (GLfloat*)&This->material.ambient, (GLfloat*)&This->material.diffuse, (GLfloat*)&This->material.specular,
		(GLfloat*)&This->material.emissive, This->material.power);

	int lightindex = 0;
	char lightname[] = "lightX.xxxxxxxxxxxxxxxx";
	for(i = 0; i < 8; i++)
	{
		if(This->lights[i].dltType)
		{
			if(prog->uniforms[0] != -1) This->ext->glUniformMatrix4fv(prog->uniforms[0],1,false,
				(GLfloat*)&This->transform[D3DTRANSFORMSTATE_WORLD]);
			if(prog->uniforms[20+(i*12)] != -1)
				This->ext->glUniform4fv(prog->uniforms[20+(i*12)],1,(GLfloat*)&This->lights[i].dcvDiffuse);
			if(prog->uniforms[21+(i*12)] != -1)
				This->ext->glUniform4fv(prog->uniforms[21+(i*12)],1,(GLfloat*)&This->lights[i].dcvSpecular);
			if(prog->uniforms[22+(i*12)] != -1)
				This->ext->glUniform4fv(prog->uniforms[22+(i*12)],1,(GLfloat*)&This->lights[i].dcvAmbient);
			if(prog->uniforms[24+(i*12)] != -1)
				This->ext->glUniform3fv(prog->uniforms[24+(i*12)],1,(GLfloat*)&This->lights[i].dvDirection);
			if(This->lights[i].dltType != D3DLIGHT_DIRECTIONAL)
			{
				if(prog->uniforms[23+(i*12)] != -1)
					This->ext->glUniform3fv(prog->uniforms[23+(i*12)],1,(GLfloat*)&This->lights[i].dvPosition);
				if(prog->uniforms[25+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[25+(i*12)],This->lights[i].dvRange);
				if(prog->uniforms[26+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[26+(i*12)],This->lights[i].dvFalloff);
				if(prog->uniforms[27+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[27+(i*12)],This->lights[i].dvAttenuation0);
				if(prog->uniforms[28+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[28+(i*12)],This->lights[i].dvAttenuation1);
				if(prog->uniforms[29+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[29+(i*12)],This->lights[i].dvAttenuation2);
				if(prog->uniforms[30+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[30+(i*12)],This->lights[i].dvTheta);
				if(prog->uniforms[31+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[31+(i*12)],This->lights[i].dvPhi);
			}
		}
		lightindex++;
	}

	DWORD ambient = This->renderstate[D3DRENDERSTATE_AMBIENT];
	if(prog->uniforms[136] != -1)
		This->ext->glUniform4f(prog->uniforms[136],RGBA_GETRED(ambient),RGBA_GETGREEN(ambient),
			RGBA_GETBLUE(ambient),RGBA_GETALPHA(ambient));
	GLint keycolor[4];
	for(i = 0; i < 8; i++)
	{
		if(This->texstages[i].colorop == D3DTOP_DISABLE) break;
		if(This->texstages[i].texture)
		{
			if(This->texstages[i].texture->levels[0].dirty & 1)
			{
				glTexture__Upload(This->texstages[i].texture, 0);
			}
			if (This->texstages[i].texture)
				glTexture__SetFilter(This->texstages[i].texture, i, This->texstages[i].glmagfilter, This->texstages[i].glminfilter, This);
			glUtil_SetTexture(This->util,i,This->texstages[i].texture);
			glUtil_SetWrap(This->util, i, 0, This->texstages[i].addressu);
			glUtil_SetWrap(This->util, i, 1, This->texstages[i].addressv);
		}
		else glUtil_SetTexture(This->util,i,0);
		This->ext->glUniform1i(prog->uniforms[128+i],i);
		if(This->renderstate[D3DRENDERSTATE_COLORKEYENABLE] && This->texstages[i].texture && (prog->uniforms[142+i] != -1))
		{
			if(This->texstages[i].texture->levels[0].ddsd.dwFlags & DDSD_CKSRCBLT)
			{
				SetColorKeyUniform(This->texstages[i].texture->levels[0].ddsd.ddckCKSrcBlt.dwColorSpaceLowValue,
					This->texstages[i].texture->colorsizes, This->texstages[i].texture->colororder,
					prog->uniforms[142 + i], This->texstages[i].texture->colorbits, This->ext);
				This->ext->glUniform4i(prog->uniforms[153+i], This->texstages[i].texture->colorsizes[0], 
					This->texstages[i].texture->colorsizes[1],
					This->texstages[i].texture->colorsizes[2],
					This->texstages[i].texture->colorsizes[3]);
			}
		}
	}
	if(prog->uniforms[137]!= -1) This->ext->glUniform1f(prog->uniforms[137],This->viewport.dwWidth);
	if(prog->uniforms[138]!= -1) This->ext->glUniform1f(prog->uniforms[138],This->viewport.dwHeight);
	if(prog->uniforms[139]!= -1) This->ext->glUniform1f(prog->uniforms[139],This->viewport.dwX);
	if(prog->uniforms[140]!= -1) This->ext->glUniform1f(prog->uniforms[140],This->viewport.dwY);
	if(prog->uniforms[141]!= -1) This->ext->glUniform1i(prog->uniforms[141],This->renderstate[D3DRENDERSTATE_ALPHAREF]);
	if(prog->uniforms[150]!= -1) This->ext->glUniform4iv(prog->uniforms[150],1,(GLint*)target->target->colorbits);
	do
	{
		if (glUtil_SetFBOSurface(This->util, target->target, ztexture,
			target->level, zlevel, FALSE) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!target->target->internalformats[1]) break;
		glTexture__Repair(target->target, TRUE);
		glUtil_SetFBO(This->util, NULL);
		target->target->levels[target->level].fbo.fbcolor = NULL;
		target->target->levels[target->level].fbo.fbz = NULL;
	} while (1);
	glUtil_SetViewport(This->util, (int)((float)This->viewport.dwX*target->mulx),
		(int)((float)This->viewport.dwY*target->muly),
		(int)((float)This->viewport.dwWidth*target->mulx),
		(int)((float)This->viewport.dwHeight*target->muly));
	glUtil_SetDepthRange(This->util, This->viewport.dvMinZ, This->viewport.dvMaxZ);
	if (This->renderstate[D3DRENDERSTATE_ALPHABLENDENABLE]) glUtil_BlendEnable(This->util, TRUE);
	else glUtil_BlendEnable(This->util, FALSE);
	glRenderer__SetBlend(This,This->renderstate[D3DRENDERSTATE_SRCBLEND],This->renderstate[D3DRENDERSTATE_DESTBLEND]);
	glUtil_SetCull(This->util, (D3DCULL)This->renderstate[D3DRENDERSTATE_CULLMODE]);
	glRenderer__SetFogColor(This,This->renderstate[D3DRENDERSTATE_FOGCOLOR]);
	glRenderer__SetFogStart(This,*(GLfloat*)(&This->renderstate[D3DRENDERSTATE_FOGSTART]));
	glRenderer__SetFogEnd(This,*(GLfloat*)(&This->renderstate[D3DRENDERSTATE_FOGEND]));
	glRenderer__SetFogDensity(This,*(GLfloat*)(&This->renderstate[D3DRENDERSTATE_FOGDENSITY]));
	glUtil_SetPolyMode(This->util, (D3DFILLMODE)This->renderstate[D3DRENDERSTATE_FILLMODE]);
	glUtil_SetShadeMode(This->util, (D3DSHADEMODE)This->renderstate[D3DRENDERSTATE_SHADEMODE]);
	if(indices) glDrawElements(mode,indexcount,GL_UNSIGNED_SHORT,indices);
	else glDrawArrays(mode,0,count);
	if(target->zbuffer) target->zbuffer->levels[target->zlevel].dirty |= 2;
	target->target->levels[target->level].dirty |= 2;
	if(flags & D3DDP_WAIT) glFlush();
	This->outputs[0] = (void*)D3D_OK;
	SetEvent(This->busy);
	return;
}

void glRenderer__DeleteFBO(glRenderer *This, FBO *fbo)
{
	glUtil_DeleteFBO(This->util, fbo);
	SetEvent(This->busy);
}

void glRenderer__UpdateClipper(glRenderer *This, glTexture *stencil, GLushort *indices, BltVertex *vertices,
	GLsizei count, GLsizei width, GLsizei height)
{
	GLfloat view[4];
	DDSURFACEDESC2 ddsd;
	if ((width != stencil->levels[0].ddsd.dwWidth) || (height != stencil->levels[0].ddsd.dwHeight))
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		glTexture__SetSurfaceDesc(stencil, &ddsd);
	}
	glUtil_SetFBOTextures(This->util, &stencil->levels[0].fbo, stencil, NULL, 0, 0, FALSE);
	view[0] = view[2] = 0;
	view[1] = width;
	view[3] = height;
	glUtil_SetViewport(This->util, 0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);
	ShaderManager_SetShader(This->shaders,PROG_CLIPSTENCIL,NULL,0);
	This->ext->glUniform4f(This->shaders->shaders[PROG_CLIPSTENCIL].view,view[0],view[1],view[2],view[3]);
	glUtil_EnableArray(This->util, This->shaders->shaders[PROG_CLIPSTENCIL].pos, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[PROG_CLIPSTENCIL].pos,
		2,GL_FLOAT,false,sizeof(BltVertex),&vertices[0].x);
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLES, 0, (6 * count) - 1,
		6 * count, GL_UNSIGNED_SHORT, indices);
	glUtil_SetFBO(This->util, NULL);
	SetEvent(This->busy);
}

void glRenderer__DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent, GLint parentlevel)
{
	RECT destrect;
	DDSURFACEDESC2 ddsd;
	DDSURFACEDESC2 tmpddsd;
	BOOL usedestrect = FALSE;
	ddsd = cmd->dest->levels[cmd->destlevel].ddsd;
	if (!memcmp(&cmd->destrect, &nullrect, sizeof(RECT)))
	{
		destrect.left = 0;
		destrect.top = 0;
		destrect.right = ddsd.dwWidth;
		destrect.bottom = ddsd.dwHeight;
	}
	else
	{
		destrect = cmd->destrect;
		usedestrect = TRUE;
	}
	if (parent)
	{
		do
		{
			if (glUtil_SetFBOSurface(This->util, parent, NULL, parentlevel, 0, TRUE) == GL_FRAMEBUFFER_COMPLETE) break;
			if (!parent->internalformats[1]) break;
			glTexture__Repair(parent, TRUE);
			glUtil_SetFBO(This->util, NULL);
			parent->levels[parentlevel].fbo.fbcolor = NULL;
			parent->levels[parentlevel].fbo.fbz = NULL;
		} while (1);
	}
	else
	{
		if (!cmd->dest->dummycolor)
		{
			glTexture_CreateDummyColor(cmd->dest, TRUE);
		}
		if ((cmd->dest->levels[cmd->destlevel].ddsd.dwWidth != cmd->dest->dummycolor->levels[0].ddsd.dwWidth) ||
			(cmd->dest->levels[cmd->destlevel].ddsd.dwHeight != cmd->dest->dummycolor->levels[0].ddsd.dwHeight))
		{
			ZeroMemory(&tmpddsd, sizeof(DDSURFACEDESC2));
			tmpddsd.dwSize = sizeof(DDSURFACEDESC2);
			tmpddsd.dwWidth = cmd->dest->levels[cmd->destlevel].ddsd.dwWidth;
			tmpddsd.dwHeight = cmd->dest->levels[cmd->destlevel].ddsd.dwHeight;
			tmpddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
			glTexture__SetSurfaceDesc(cmd->dest->dummycolor, &tmpddsd);
		}			
		glUtil_SetFBOTextures(This->util, &cmd->dest->dummycolor->levels[0].fbo, cmd->dest->dummycolor,
			cmd->dest, cmd->destlevel, 0, FALSE);
	}
	glUtil_SetViewport(This->util, 0, 0, cmd->dest->levels[cmd->destlevel].ddsd.dwWidth,
		cmd->dest->levels[cmd->destlevel].ddsd.dwHeight);
	if (usedestrect) glUtil_SetScissor(This->util, TRUE, cmd->destrect.left, cmd->destrect.top,
		cmd->destrect.right, cmd->destrect.bottom);
	glUtil_DepthWrite(This->util, TRUE);
	glUtil_ClearDepth(This->util, cmd->bltfx.dwFillDepth / (double)0xFFFF); // FIXME:  SOTE depth workaround
	glClear(GL_DEPTH_BUFFER_BIT);
	if (usedestrect)glUtil_SetScissor(This->util, false, 0, 0, 0, 0);
	This->outputs[0] = DD_OK;
	SetEvent(This->busy);
}

void glRenderer__SetRenderState(glRenderer *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	SetEvent(This->busy);
	if (This->renderstate[dwRendStateType] == dwRenderState) return;
	This->renderstate[dwRendStateType] = dwRenderState;
	switch (dwRendStateType)
	{
	case D3DRENDERSTATE_SHADEMODE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFFFCi64;
		switch (dwRenderState)
		{
		case D3DSHADE_FLAT:
		default:
			break;
		case D3DSHADE_GOURAUD:
			This->shaderstate3d.stateid |= 1;
			break;
		case D3DSHADE_PHONG:
			This->shaderstate3d.stateid |= 3;
			break;
		}
		break;
	case D3DRENDERSTATE_ALPHATESTENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= 4;
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFFFBi64;
		break;
	case D3DRENDERSTATE_ALPHAFUNC:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFFC7i64;
		This->shaderstate3d.stateid |= ((((__int64)dwRenderState - 1) & 7) << 3);
		break;
	case D3DRENDERSTATE_FOGTABLEMODE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFF3Fi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 6);
		break;
	case D3DRENDERSTATE_FOGVERTEXMODE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFCFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 8);
		break;
	case D3DRENDERSTATE_RANGEFOGENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 10);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFBFFi64;
		break;
	case D3DRENDERSTATE_SPECULARENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 11);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFF7FFi64;
		break;
	case D3DRENDERSTATE_STIPPLEDALPHA:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 12);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFEFFFi64;
		break;
	case D3DRENDERSTATE_COLORKEYENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 13);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFDFFFi64;
		break;
	case D3DRENDERSTATE_LOCALVIEWER:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 21);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFDFFFFFi64;
		break;
	case D3DRENDERSTATE_COLORKEYBLENDENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 22);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFBFFFFFi64;
		break;
	case D3DRENDERSTATE_DIFFUSEMATERIALSOURCE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFE7FFFFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 23);
		break;
	case D3DRENDERSTATE_SPECULARMATERIALSOURCE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFF9FFFFFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 25);
		break;
	case D3DRENDERSTATE_AMBIENTMATERIALSOURCE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFE7FFFFFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 27);
		break;
	case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFBFFFFFFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 29);
		break;
	case D3DRENDERSTATE_NORMALIZENORMALS:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 49);
		else This->shaderstate3d.stateid &= 0xFFFDFFFFFFFFFFFFi64;
		break;
	case D3DRENDERSTATE_LIGHTING:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 59);
		else This->shaderstate3d.stateid &= 0xF7FFFFFFFFFFFFFFi64;
		break;
	case D3DRENDERSTATE_COLORVERTEX:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 60);
		else This->shaderstate3d.stateid &= 0xEFFFFFFFFFFFFFFFi64;
		break;
	case D3DRENDERSTATE_FOGENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 61);
		else This->shaderstate3d.stateid &= 0xDFFFFFFFFFFFFFFFi64;
		break;
	case D3DRENDERSTATE_DITHERENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 62);
		else This->shaderstate3d.stateid &= 0xBFFFFFFFFFFFFFFFi64;
		break;
	default:
		break;
	}
}

void glRenderer__RemoveTextureFromD3D(glRenderer *This, glTexture *texture)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		if (This->texstages[i].texture == texture)
		{
			This->texstages[i].texture = NULL;
			This->shaderstate3d.texstageid[i] &= 0xE7FFFFFFFFFFFFFFi64;
		}
	}
}

void glRenderer__SetTexture(glRenderer *This, DWORD dwStage, glTexture *Texture)
{
	if (This->texstages[dwStage].texture == Texture)
	{
		SetEvent(This->busy);
		return;
	}
	This->texstages[dwStage].texture = Texture;
	if (Texture)
	{
		This->shaderstate3d.texstageid[dwStage] |= 1i64 << 59;
		if (Texture->levels[0].ddsd.dwFlags & DDSD_CKSRCBLT) This->shaderstate3d.texstageid[dwStage] |= 1i64 << 60;
		else This->shaderstate3d.texstageid[dwStage] &= 0xEFFFFFFFFFFFFFFFi64;
	}
	else This->shaderstate3d.texstageid[dwStage] &= 0xE7FFFFFFFFFFFFFFi64;
	SetEvent(This->busy);
}

void glRenderer__SetTextureStageState(glRenderer *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	SetEvent(This->busy);
	switch (dwState)
	{
	case D3DTSS_COLOROP:
		This->texstages[dwStage].colorop = (D3DTEXTUREOP)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFFFFFFFE0i64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)dwValue & 31);
		break;
	case D3DTSS_COLORARG1:
		This->texstages[dwStage].colorarg1 = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFFFFFF81Fi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 63) << 5);
		break;
	case D3DTSS_COLORARG2:
		This->texstages[dwStage].colorarg2 = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFFFFE07FFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 63) << 11);
		break;
	case D3DTSS_ALPHAOP:
		This->texstages[dwStage].alphaop = (D3DTEXTUREOP)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFFFC1FFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 31) << 17);
		break;
	case D3DTSS_ALPHAARG1:
		This->texstages[dwStage].alphaarg1 = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFF03FFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 63) << 22);
		break;
	case D3DTSS_ALPHAARG2:
		This->texstages[dwStage].alphaarg2 = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFC0FFFFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 63) << 28);
		break;
	case D3DTSS_BUMPENVMAT00:
		This->texstages[dwStage].bumpenv00 = dwValue;
		break;
	case D3DTSS_BUMPENVMAT01:
		This->texstages[dwStage].bumpenv01 = dwValue;
		break;
	case D3DTSS_BUMPENVMAT10:
		This->texstages[dwStage].bumpenv10 = dwValue;
		break;
	case D3DTSS_BUMPENVMAT11:
		This->texstages[dwStage].bumpenv11 = dwValue;
		break;
	case D3DTSS_TEXCOORDINDEX:
		This->texstages[dwStage].texcoordindex = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFF83FFFFFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 7) << 34);
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)((dwValue>>16) & 7) << 37);
		break;
	case D3DTSS_ADDRESSU:
		This->texstages[dwStage].addressu = (D3DTEXTUREADDRESS)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFE7FFFFFFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 3) << 39);
		break;
	case D3DTSS_ADDRESSV:
		This->texstages[dwStage].addressv = (D3DTEXTUREADDRESS)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFF9FFFFFFFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 3) << 41);
		break;
	case D3DTSS_BORDERCOLOR:
		This->texstages[dwStage].bordercolor = dwValue;
		break;
	case D3DTSS_MAGFILTER:
		This->texstages[dwStage].magfilter = (D3DTEXTUREMAGFILTER)dwValue;
		//This->shaderstate3d.texstageid[dwStage] &= 0xFFFFC7FFFFFFFFFFi64;
		//This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 7) << 43);
		switch (This->texstages[dwStage].magfilter)
		{
		case 1:
		default:
			This->texstages[dwStage].glmagfilter = GL_NEAREST;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			This->texstages[dwStage].glmagfilter = GL_LINEAR;
			break;
		}
		break;
	case D3DTSS_MINFILTER:
		This->texstages[dwStage].minfilter = (D3DTEXTUREMINFILTER)dwValue;
		//This->shaderstate3d.texstageid[dwStage] &= 0xFFFF3FFFFFFFFFFFi64;
		//This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 7) << 46);
		switch (This->texstages[dwStage].minfilter)
		{
		case 1:
		default:
			switch (This->texstages[dwStage].mipfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_NEAREST;
				break;
			case 2:
				This->texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_NEAREST;
				break;
			case 3:
				This->texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_LINEAR;
				break;
			}
			break;
		case 2:
		case 3:
			switch (This->texstages[dwStage].mipfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_LINEAR;
				break;
			case 2:
				This->texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_NEAREST;
				break;
			case 3:
				This->texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_LINEAR;
				break;
			}
			break;
		}
		break;
	case D3DTSS_MIPFILTER:
		This->texstages[dwStage].mipfilter = (D3DTEXTUREMIPFILTER)dwValue;
		//This->shaderstate3d.texstageid[dwStage] &= 0xFFFCFFFFFFFFFFFFi64;
		//This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 7) << 48);
		switch (This->texstages[dwStage].mipfilter)
		{
		case 1:
		default:
			switch (This->texstages[dwStage].minfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_NEAREST;
			case 2:
			case 3:
				This->texstages[dwStage].glminfilter = GL_LINEAR;
			}
			break;
		case 2:
			switch (This->texstages[dwStage].minfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_NEAREST;
			case 2:
			case 3:
				This->texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_NEAREST;
			}
			break;
		case 3:
			switch (This->texstages[dwStage].minfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_LINEAR;
			case 2:
			case 3:
				This->texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_LINEAR;
			}
			break;
		}
		break;
	case D3DTSS_MIPMAPLODBIAS:
		memcpy(&This->texstages[dwStage].lodbias, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_MAXMIPLEVEL:
		This->texstages[dwStage].miplevel = dwValue;
		break;
	case D3DTSS_MAXANISOTROPY:
		This->texstages[dwStage].anisotropy = dwValue;
		break;
	case D3DTSS_BUMPENVLSCALE:
		memcpy(&This->texstages[dwStage].bumpenvlscale, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_BUMPENVLOFFSET:
		memcpy(&This->texstages[dwStage].bumpenvloffset, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_TEXTURETRANSFORMFLAGS:
		This->texstages[dwStage].textransform = (D3DTEXTURETRANSFORMFLAGS)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFC3FFFFFFFFFFFFi64;
		if (dwValue & 7)
		{
			This->shaderstate3d.texstageid[dwStage] |= 1i64 << 50;
			This->shaderstate3d.texstageid[dwStage] |= (__int64)(((dwValue & 7) - 1) & 3) << 51;
		}
		if (dwValue & D3DTTFF_PROJECTED) This->shaderstate3d.texstageid[dwStage] |= 1i64 << 53;
		break;
	default:
		break;
	}
}

void glRenderer__SetTransform(glRenderer *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if (dtstTransformStateType > 23)
	{
		SetEvent(This->busy);
		return;
	}
	memcpy(&This->transform[dtstTransformStateType], lpD3DMatrix, sizeof(D3DMATRIX));
	SetEvent(This->busy);
}

void glRenderer__SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial)
{
	memcpy(&This->material, lpMaterial, sizeof(D3DMATERIAL7));
	SetEvent(This->busy);
}

void glRenderer__SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light)
{
	int numlights = 0;
	int lightindex = 0;
	memcpy(&This->lights[index], light, sizeof(D3DLIGHT7));
	SetEvent(This->busy);
	for (int i = 0; i < 8; i++)
		if (This->lights[i].dltType) numlights++;
	This->shaderstate3d.stateid &= 0xF807C03FFFE3FFFFi64;
	This->shaderstate3d.stateid |= ((__int64)numlights << 18);
	for (int i = 0; i < 8; i++)
	{
		if (This->lights[i].dltType != 1)
		{
			if (This->lights[i].dltType != D3DLIGHT_DIRECTIONAL)
				This->shaderstate3d.stateid |= (1i64 << (38 + lightindex));
			if (This->lights[i].dltType == D3DLIGHT_SPOT)
				This->shaderstate3d.stateid |= (1i64 << (51 + lightindex));
			lightindex++;
		}
	}

}

void glRenderer__RemoveLight(glRenderer *This, DWORD index)
{
	int numlights = 0;
	int lightindex = 0;
	ZeroMemory(&This->lights[index], sizeof(D3DLIGHT7));
	SetEvent(This->busy);
	for (int i = 0; i < 8; i++)
		if (This->lights[i].dltType) numlights++;
	This->shaderstate3d.stateid &= 0xF807C03FFFE3FFFFi64;
	This->shaderstate3d.stateid |= ((__int64)numlights << 18);
	for (int i = 0; i < 8; i++)
	{
		if (This->lights[i].dltType != 1)
		{
			if (This->lights[i].dltType != D3DLIGHT_DIRECTIONAL)
				This->shaderstate3d.stateid |= (1i64 << (38 + lightindex));
			if (This->lights[i].dltType == D3DLIGHT_SPOT)
				This->shaderstate3d.stateid |= (1i64 << (51 + lightindex));
			lightindex++;
		}
	}
}

void glRenderer__SetD3DViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport)
{
	memcpy(&This->viewport, lpViewport, sizeof(D3DVIEWPORT7));
	SetEvent(This->busy);
}

void glRenderer__SetFogColor(glRenderer *This, DWORD color)
{
	if (color == This->fogcolor) return;
	This->fogcolor = color;
	GLfloat colors[4];
	colors[0] = (GLfloat)((color >> 16) & 255) / 255.0f;
	colors[1] = (GLfloat)((color >> 8) & 255) / 255.0f;
	colors[2] = (GLfloat)(color & 255) / 255.0f;
	colors[3] = (GLfloat)((color >> 24) & 255) / 255.0f;
	glFogfv(GL_FOG_COLOR, colors);
}

void glRenderer__SetFogStart(glRenderer *This, GLfloat start)
{
	if (start == This->fogstart) return;
	This->fogstart = start;
	glFogf(GL_FOG_START, start);
}

void glRenderer__SetFogEnd(glRenderer *This, GLfloat end)
{
	if (end == This->fogend) return;
	This->fogend = end;
	glFogf(GL_FOG_END, end);
}

void glRenderer__SetFogDensity(glRenderer *This, GLfloat density)
{
	if (density == This->fogdensity) return;
	This->fogdensity = density;
	glFogf(GL_FOG_DENSITY, density);
}

void glRenderer__SetDepthComp(glRenderer *This)
{
	switch (This->renderstate[D3DRENDERSTATE_ZFUNC])
	{
	case D3DCMP_NEVER:
		glUtil_SetDepthComp(This->util, GL_NEVER);
		break;
	case D3DCMP_LESS:
		glUtil_SetDepthComp(This->util, GL_LESS);
		break;
	case D3DCMP_EQUAL:
		glUtil_SetDepthComp(This->util, GL_EQUAL);
		break;
	case D3DCMP_LESSEQUAL:
		glUtil_SetDepthComp(This->util, GL_LEQUAL);
		break;
	case D3DCMP_GREATER:
		glUtil_SetDepthComp(This->util, GL_GREATER);
		break;
	case D3DCMP_NOTEQUAL:
		glUtil_SetDepthComp(This->util, GL_NOTEQUAL);
		break;
	case D3DCMP_GREATEREQUAL:
		glUtil_SetDepthComp(This->util, GL_GEQUAL);
		break;
	case D3DCMP_ALWAYS:
	default:
		glUtil_SetDepthComp(This->util, GL_ALWAYS);
		break;
	}
}

void glRenderer__SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey, GLint level)
{
	if (dwFlags & DDCKEY_SRCBLT)
	{
		texture->levels[level].ddsd.dwFlags |= DDSD_CKSRCBLT;
		texture->levels[level].ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
		if (DDCKEY_COLORSPACE) texture->levels[level].ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
		else texture->levels[level].ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
	}
	if (dwFlags & DDCKEY_DESTBLT)
	{
		texture->levels[level].ddsd.dwFlags |= DDSD_CKDESTBLT;
		texture->levels[level].ddsd.ddckCKDestBlt.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
		if (DDCKEY_COLORSPACE) texture->levels[level].ddsd.ddckCKDestBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
		else texture->levels[level].ddsd.ddckCKDestBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
	}
	if (dwFlags & DDCKEY_SRCOVERLAY)
	{
		texture->levels[level].ddsd.dwFlags |= DDSD_CKSRCOVERLAY;
		texture->levels[level].ddsd.ddckCKSrcOverlay.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
		if (DDCKEY_COLORSPACE) texture->levels[level].ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
		else texture->levels[level].ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
	}
	if (dwFlags & DDCKEY_DESTOVERLAY)
	{
		texture->levels[level].ddsd.dwFlags |= DDSD_CKDESTOVERLAY;
		texture->levels[level].ddsd.ddckCKDestOverlay.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
		if (DDCKEY_COLORSPACE) texture->levels[level].ddsd.ddckCKDestOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
		else texture->levels[level].ddsd.ddckCKDestOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
	}
	SetEvent(This->busy);
}

void glRenderer__MakeTexturePrimary(glRenderer *This, glTexture *texture, glTexture *parent, BOOL primary)
{
	if (primary)
	{
		if (!parent) return;
		glTexture__SetPrimaryScale(texture, parent->bigwidth, parent->bigheight, TRUE);
	}
	else glTexture__SetPrimaryScale(texture, 0, 0, FALSE);
	SetEvent(This->busy);
}

void glRenderer__DXGLBreak(glRenderer *This)
{
	if (This->ext->GLEXT_GREMEDY_frame_terminator) This->ext->glFrameTerminatorGREMEDY();
	SetEvent(This->busy);
}

void glRenderer__EndCommand(glRenderer *This, BOOL wait)
{
	// Do set-up and flip here
	if (!wait) SetEvent(This->busy);
	// Do command execution here
	if (wait) SetEvent(This->busy);
}

void glRenderer__SetMode3D(glRenderer *This, BOOL enabled)
{
// TODO:  Set 3D mode
}

}
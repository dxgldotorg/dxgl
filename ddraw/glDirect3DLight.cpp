// DXGL
// Copyright (C) 2012 William Feely

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
#include "glDirect3DLight.h"
#define _USE_MATH_DEFINES
#include <math.h>



glDirect3DLight::glDirect3DLight()
{
	refcount=1;
	gllight = -1;
	ZeroMemory(&light,sizeof(D3DLIGHT7));
	light.dltType = D3DLIGHT_DIRECTIONAL;
	light.dcvAmbient.r = light.dcvAmbient.g = light.dcvAmbient.b = 1.0f;
	light.dvDirection = D3DVECTOR(0,0,1.0);
}
glDirect3DLight::glDirect3DLight(D3DLIGHT7 *light_in)
{
	refcount=1;
	gllight = -1;
	memcpy(&light,light_in,sizeof(D3DLIGHT7));
}

glDirect3DLight::~glDirect3DLight()
{
	if(gllight != -1)
		glDisable(GL_LIGHT0+gllight);
}

ULONG WINAPI glDirect3DLight::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirect3DLight::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DLight::Initialize(LPDIRECT3D lpDirect3D)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI glDirect3DLight::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return E_NOINTERFACE;
}

void glDirect3DLight::GetLight7(LPD3DLIGHT7 lpLight7)
{
	memcpy(lpLight7,&light,sizeof(D3DLIGHT7));
}
void glDirect3DLight::SetLight7(LPD3DLIGHT7 lpLight7)
{
	memcpy(&light,lpLight7,sizeof(D3DLIGHT7));
	if(gllight != -1) SetGLLight(gllight);
}

HRESULT WINAPI glDirect3DLight::GetLight(LPD3DLIGHT lpLight)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DLight::GetLight: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DLight::SetLight(LPD3DLIGHT lpLight)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DLight::SetLight: stub");
	ERR(DDERR_GENERIC);
}

void glDirect3DLight::SetGLLight(int gllightin)
{
	const float angle_dx_gl = 90.0f / (float)M_PI;
	if(gllightin != -1)
	{
		if(gllightin != gllight) glDisable(GL_LIGHT0+gllight);
		GLfloat ambient[] = {light.dcvAmbient.r,light.dcvAmbient.g,light.dcvAmbient.b,light.dcvAmbient.a};
		glLightfv(GL_LIGHT0+gllightin,GL_AMBIENT,ambient);
		GLfloat diffuse[] = {light.dcvDiffuse.r,light.dcvDiffuse.g,light.dcvDiffuse.b,light.dcvDiffuse.a};
		glLightfv(GL_LIGHT0+gllightin,GL_DIFFUSE,diffuse);
		GLfloat specular[] = {light.dcvSpecular.r,light.dcvSpecular.g,light.dcvSpecular.b,light.dcvSpecular.a};
		glLightfv(GL_LIGHT0+gllightin,GL_DIFFUSE,specular);
		if(light.dltType == D3DLIGHT_DIRECTIONAL)
			GLfloat position[] = {light.dvPosition.x,light.dvPosition.y,light.dvPosition.z,0.};
		else GLfloat position[] = {light.dvPosition.x,light.dvPosition.y,light.dvPosition.z,1.};
		glLightfv(GL_LIGHT0+gllightin,GL_POSITION,specular);
		GLfloat direction[] = {light.dvDirection.x,light.dvDirection.y,light.dvDirection.z};
		glLightfv(GL_LIGHT0+gllightin,GL_SPOT_DIRECTION,specular);
		if(light.dltType == D3DLIGHT_SPOT)
			glLightf(GL_LIGHT0+gllightin,GL_SPOT_CUTOFF,angle_dx_gl*light.dvPhi);
		else glLightf(GL_LIGHT0+gllightin,GL_SPOT_CUTOFF,180.0);
		glLightf(GL_LIGHT0+gllightin,GL_CONSTANT_ATTENUATION,light.dvAttenuation0);
		glLightf(GL_LIGHT0+gllightin,GL_LINEAR_ATTENUATION,light.dvAttenuation1);
		glLightf(GL_LIGHT0+gllightin,GL_QUADRATIC_ATTENUATION,light.dvAttenuation2);
		glEnable(GL_LIGHT0+gllightin);
	}
	else
	{
		glDisable(GL_LIGHT0+gllightin);
	}
	gllight = gllightin;
}
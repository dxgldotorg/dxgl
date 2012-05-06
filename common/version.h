#pragma once
#ifndef __VERSION_H
#define __VERSION_H

#define DXGLMAJOR 0
#define DXGLMINOR 2
#define DXGLPOINT 0
#define DXGLBUILD 0

#define DXGLVERNUMBER DXGLMAJOR,DXGLMINOR,DXGLPOINT,DXGLBUILD
#define DXGLVERQWORD (((unsigned __int64)DXGLMAJOR<<48)+((unsigned __int64)DXGLMINOR<<32)+((unsigned __int64)DXGLPOINT<<16)+(unsigned __int64)DXGLBUILD)
#define DXGLVERSTRING "0.2.0.0"


#endif //__VERSION_H

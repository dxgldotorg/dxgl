#pragma once
#ifndef __VERSION_H
#define __VERSION_H

#define DXGLMAJORVER 0
#define DXGLMINORVER 3
#define DXGLPOINTVER 0

#define STR2(x) #x
#define STR(x) STR2(x)

#define DXGLSTRVER STR(DXGLMAJORVER) "." STR(DXGLMINORVER) "." STR(DXGLPOINTVER)



#endif //__VERSION_H

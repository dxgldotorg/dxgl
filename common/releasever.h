#pragma once
#ifndef __VERSION_H
#define __VERSION_H

#define DXGLMAJORVER 0
#define DXGLMINORVER 5
#define DXGLPOINTVER 15
#define DXGLBETA 1

#define STR2(x) #x
#define STR(x) STR2(x)

#define DXGLSTRVER STR(DXGLMAJORVER) "." STR(DXGLMINORVER) "." STR(DXGLPOINTVER)



#endif //__VERSION_H

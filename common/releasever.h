#pragma once
#ifndef __VERSION_H
#define __VERSION_H

/* Removed due to transition to Git - Git tags will set the version number
#define DXGLMAJORVER 0
#define DXGLMINORVER 5
#define DXGLPOINTVER 25
#define DXGLBETA 0


#define STR2(x) #x
#define STR(x) STR2(x)

#define DXGLSTRVER STR(DXGLMAJORVER) "." STR(DXGLMINORVER) "." STR(DXGLPOINTVER)
*/

/* sign modes
0 - never sign
1 - sign non-beta only
2 - sign release only
3 - sign all */
#define SIGNMODE 0

#endif //__VERSION_H

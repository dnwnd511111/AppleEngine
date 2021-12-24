#ifndef AP_SHADERINTEROP_GPUSORTLIB_H
#define AP_SHADERINTEROP_GPUSORTLIB_H

#include "ShaderInterop.h"

struct SortConstants
{
	int3 job_params;
	uint counterReadOffset;
};
PUSHCONSTANT(sort, SortConstants);

#endif // AP_SHADERINTEROP_GPUSORTLIB_H

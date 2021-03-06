#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

Texture2D<float2> tile_mindepth_maxcoc_horizontal : register(t0);
Texture2D<float> tile_mincoc_horizontal : register(t1);

RWTexture2D<float2> mindepth_maxcoc : register(u0);
RWTexture2D<float> mincoc : register(u1);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = uint2(DTid.x, DTid.y * DEPTHOFFIELD_TILESIZE);
	float min_depth = 1;
	float max_coc = 0;
	float min_coc = 1000000;

	int2 dim;
	tile_mindepth_maxcoc_horizontal.GetDimensions(dim.x, dim.y);

	[loop]
	for (uint i = 0; i < DEPTHOFFIELD_TILESIZE; ++i)
	{
		const int2 pixel = uint2(tile_upperleft.x, tile_upperleft.y + i);
		if (pixel.x >= 0 && pixel.y >= 0 && pixel.x < dim.x && pixel.y < dim.y)
		{
			const float2 mindepth_maxcoc = tile_mindepth_maxcoc_horizontal[pixel];
			min_depth = min(min_depth, mindepth_maxcoc.x);
			max_coc = max(max_coc, mindepth_maxcoc.y);
			min_coc = min(min_coc, tile_mincoc_horizontal[pixel]);
		}
	}

	mindepth_maxcoc[DTid.xy] = float2(min_depth, max_coc);
	mincoc[DTid.xy] = min_coc;
}

#pragma once
#include "CommonInclude.h"
#include "apGraphicsDevice.h"
#include "apScene_Decl.h"

namespace ap
{
	struct GPUBVH
	{
		// Scene BVH intersection resources:
		ap::graphics::GPUBuffer bvhNodeBuffer;
		ap::graphics::GPUBuffer bvhParentBuffer;
		ap::graphics::GPUBuffer bvhFlagBuffer;
		ap::graphics::GPUBuffer primitiveCounterBuffer;
		ap::graphics::GPUBuffer primitiveIDBuffer;
		ap::graphics::GPUBuffer primitiveBuffer;
		ap::graphics::GPUBuffer primitiveMortonBuffer;
		uint32_t primitiveCapacity = 0;
		bool IsValid() const { return primitiveCounterBuffer.IsValid(); }

		void Update(const ap::scene::Scene& scene);
		void Build(const ap::scene::Scene& scene, ap::graphics::CommandList cmd) const;

		void Clear();

		static void Initialize();
	};
}

#pragma once
#include "CommonInclude.h"
#include "apGraphicsDevice.h"
#include "apEnums.h"
#include "apMath.h"
#include "apECS.h"
#include "apPrimitive.h"
#include "apVector.h"
#include "apScene_Decl.h"

#include <memory>

namespace ap
{
	class Archive;
}

namespace ap
{
	class HairParticleSystem
	{
	public:
		ap::graphics::GPUBuffer constantBuffer;
		ap::graphics::GPUBuffer simulationBuffer;
		ap::graphics::GPUBuffer vertexBuffer_POS[2];
		ap::graphics::GPUBuffer vertexBuffer_TEX;
		ap::graphics::GPUBuffer primitiveBuffer;
		ap::graphics::GPUBuffer culledIndexBuffer;
		ap::graphics::GPUBuffer indirectBuffer;
		ap::graphics::GPUBuffer subsetBuffer;

		ap::graphics::GPUBuffer indexBuffer;
		ap::graphics::GPUBuffer vertexBuffer_length;

		ap::graphics::RaytracingAccelerationStructure BLAS;

		void UpdateCPU(
			const ap::scene::TransformComponent& transform,
			const ap::scene::MeshComponent& mesh,
			float dt
		);
		void UpdateGPU(
			uint32_t instanceIndex,
			uint32_t materialIndex,
			const ap::scene::MeshComponent& mesh,
			const ap::scene::MaterialComponent& material,
			ap::graphics::CommandList cmd
		) const;
		void Draw(
			const ap::scene::MaterialComponent& material,
			ap::enums::RENDERPASS renderPass,
			ap::graphics::CommandList cmd
		) const;

		enum FLAGS
		{
			EMPTY = 0,
			_DEPRECATED_REGENERATE_FRAME = 1 << 0,
			REBUILD_BUFFERS = 1 << 1,
		};
		uint32_t _flags = EMPTY;

		ap::ecs::Entity meshID = ap::ecs::INVALID_ENTITY;

		uint32_t strandCount = 0;
		uint32_t segmentCount = 1;
		uint32_t randomSeed = 1;
		float length = 1.0f;
		float stiffness = 10.0f;
		float randomness = 0.2f;
		float viewDistance = 200;
		ap::vector<float> vertex_lengths;

		// Sprite sheet properties:
		uint32_t framesX = 1;
		uint32_t framesY = 1;
		uint32_t frameCount = 1;
		uint32_t frameStart = 0;

		// Non-serialized attributes:
		XMFLOAT4X4 world;
		XMFLOAT4X4 worldPrev;
		ap::primitive::AABB aabb;
		ap::vector<uint32_t> indices; // it is dependent on vertex_lengths and contains triangles with non-zero lengths
		uint32_t layerMask = ~0u;
		mutable bool regenerate_frame = true;

		void Serialize(ap::Archive& archive, ap::ecs::EntitySerializer& seri);

		static void Initialize();
	};
}

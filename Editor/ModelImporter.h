#pragma once
#include <string>

namespace ap::scene
{
	struct Scene;
}

void ImportModel_OBJ(const std::string& fileName, ap::scene::Scene& scene);
void ImportModel_GLTF(const std::string& fileName, ap::scene::Scene& scene);


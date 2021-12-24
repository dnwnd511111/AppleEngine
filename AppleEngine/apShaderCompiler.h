#pragma once
#include "apGraphics.h"
#include "apVector.h"

#include <string>

namespace ap::shadercompiler
{

	enum class Flags
	{
		NONE = 0,
		DISABLE_OPTIMIZATION = 1 << 0,
	};
	struct CompilerInput
	{
		Flags flags = Flags::NONE;
		ap::graphics::ShaderFormat format = ap::graphics::ShaderFormat::NONE;
		ap::graphics::ShaderStage stage = ap::graphics::ShaderStage::Count;
		// if the shader relies on a higher shader model feature, it must be declared here.
		//	But the compiler can also choose a higher one internally, if needed
		ap::graphics::ShaderModel minshadermodel = ap::graphics::ShaderModel::SM_5_0;
		std::string shadersourcefilename;
		std::string entrypoint = "main";
		ap::vector<std::string> include_directories;
		ap::vector<std::string> defines;
	};
	struct CompilerOutput
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }

		const uint8_t* shaderdata = nullptr;
		size_t shadersize = 0;
		ap::vector<uint8_t> shaderhash;
		std::string error_message;
		ap::vector<std::string> dependencies;
	};
	void Compile(const CompilerInput& input, CompilerOutput& output);

	bool SaveShaderAndMetadata(const std::string& shaderfilename, const CompilerOutput& output);
	bool IsShaderOutdated(const std::string& shaderfilename);

	void RegisterShader(const std::string& shaderfilename);
	size_t GetRegisteredShaderCount();
	bool CheckRegisteredShadersOutdated();
}

template<>
struct enable_bitmask_operators<ap::shadercompiler::Flags> {
	static const bool enable = true;
};

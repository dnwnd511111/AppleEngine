#pragma once

#include "apScene.h"

#include <vector>
#include <string>
#include "DirectXMath.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "apImGuiColor.h"


namespace ap::graphics
{
	struct Texture;
}


namespace ap::imgui
{

	bool PropertyGridHeader(const std::string& name, bool openByDefault = true);
	void BeginCheckboxGroup(const char* label);
	void EndCheckboxGroup();
	bool PropertyCheckboxGroup(const char* label, bool& value);

	void BeginPropertyGrid();
	void EndPropertyGrid();
	void PropertyGridSpacing();

	bool DrawInputText(const char* label, std::string& value, bool error = false);
	void DrawInputText(const char* label, const std::string& value);
	void DrawInputText(const char* label, const char* value);
	bool DrawCombo(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected);
	bool DrawCheckbox(const char* label, bool& value);
	bool DrawDragFloat(const char* label, float& value, float delta = 0.1f, float min = 0.0f, float max = 0.0f, bool readOnly = false);
	bool DrawDragFloat2(const char* label, DirectX::XMFLOAT2& value, float delta = 0.1f);
	bool DrawDragFloat3(const char* label, DirectX::XMFLOAT3& value, float delta = 0.1f);
	bool DrawDragFloat4(const char* label, DirectX::XMFLOAT4& value, float delta = 0.1f);
	bool DrawSliderInt(const char* label, int& value, int min, int max);
	bool DrawSliderInt(const char* label, uint32_t& value, uint32_t min, uint32_t max);
	bool DrawSliderFloat(const char* label, float& value, float min, float max, const char* format = "%.3f");
	bool DrawSliderFloat2(const char* label, DirectX::XMFLOAT2& value, float min, float max);
	bool DrawSliderFloat3(const char* label, DirectX::XMFLOAT3& value, float min, float max);
	bool DrawSliderFloat4(const char* label, DirectX::XMFLOAT4& value, float min, float max);
	bool DrawColorEdit3(const char* label, DirectX::XMFLOAT3& value);
	bool DrawColorEdit4(const char* label, DirectX::XMFLOAT4& value);
	bool DrawColorEdit3(const char* label, float* value);
	bool DrawButton(const char* label, const ImVec2& size = ImVec2(0, 0));
	bool DrawButton2(const char* label, const ImVec2& size = ImVec2(90.0f, 20.0f));
	void DrawVec3Control(const std::string& label, DirectX::XMFLOAT3& values, float resetValue = 0.0f, bool isScale = false, float columnWidth = 100.0f);

	bool DrawImage(const char* label, ap::scene::MaterialComponent::TextureMap* textureMap);
	bool DrawImage(const char* label, const ap::graphics::Texture* texture, const  DirectX::XMFLOAT2& size = DirectX::XMFLOAT2(70.0f, 70.0f));





}
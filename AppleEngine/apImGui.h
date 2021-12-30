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
	void Initialize();


	void ShiftCursorX(float distance);
	void ShiftCursorY(float distance);
	void ShiftCursor(float x, float y);

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

	bool ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));

	bool DrawImage(const char* label, ap::ecs::Entity materialEntity, int textureIndex);
	bool DrawImage(const char* label, const ap::graphics::Texture* texture, const  DirectX::XMFLOAT2& size = DirectX::XMFLOAT2(70.0f, 70.0f));

	void DrawButtonImage(ImTextureID image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle);
	void DrawButtonImage(ImTextureID imageNormal, ImTextureID imageHovered, ImTextureID imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax);
	
	
	
	bool SearchWidget(std::string& searchString, const char* hint = "Search...", bool* grabFocus = nullptr);



}
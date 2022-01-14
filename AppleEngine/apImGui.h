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
    void DrawBlueprint();

    const char* GenerateID();
	void ShiftCursorX(float distance);
	void ShiftCursorY(float distance);
	void ShiftCursor(float x, float y);

    bool IsItemDisabled();
    ImRect GetItemRect();
    ImRect RectExpanded(const ImRect& rect, float x, float y);
    void DrawItemActivityOutline(float rounding = 0.0f, bool drawWhenInactive = false, ImColor colourWhenActive = ImColor(80, 80, 80));

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
	bool DrawSliderFloat(const char* label, float& value, float min, float max, const char* format = "%.3f", float power = 1.0f);
	bool DrawSliderFloat2(const char* label, DirectX::XMFLOAT2& value, float min, float max);
	bool DrawSliderFloat3(const char* label, DirectX::XMFLOAT3& value, float min, float max);
	bool DrawSliderFloat4(const char* label, DirectX::XMFLOAT4& value, float min, float max);
	bool DrawColorEdit3(const char* label, DirectX::XMFLOAT3& value);
	bool DrawColorEdit4(const char* label, DirectX::XMFLOAT4& value);
	bool DrawColorEdit3(const char* label, float* value);
	bool DrawButton(const char* label, const ImVec2& size = ImVec2(0, 0));
	bool DrawButton2(const char* label, bool fill = false ,const ImVec2& size = ImVec2(90.0f, 20.0f));
	bool DrawVec3Control(const std::string& label, DirectX::XMFLOAT3& values, float resetValue = 0.0f, bool isScale = false, float columnWidth = 100.0f);

	//gonna be deleted
	bool ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));

	bool DrawImage(const char* label, ap::ecs::Entity materialEntity, int textureIndex);
	bool DrawImage(const char* label, ImTextureID texture, const  DirectX::XMFLOAT2& size = DirectX::XMFLOAT2(70.0f, 70.0f));
	void DrawImage(ImTextureID image, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0));

	void DrawButtonImage(ImTextureID image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle);
	void DrawButtonImage(ImTextureID imageNormal, ImTextureID imageHovered, ImTextureID imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax);
	
	// Utility


    class ScopedStyle
    {
    public:
        ScopedStyle(const ScopedStyle&) = delete;
        ScopedStyle operator=(const ScopedStyle&) = delete;
        template<typename T>
        ScopedStyle(ImGuiStyleVar styleVar, T value) { ImGui::PushStyleVar(styleVar, value); }
        ~ScopedStyle() { ImGui::PopStyleVar(); }
    };

    class ScopedColour
    {
    public:
        ScopedColour(const ScopedColour&) = delete;
        ScopedColour operator=(const ScopedColour&) = delete;
        template<typename T>
        ScopedColour(ImGuiCol colourId, T colour) { ImGui::PushStyleColor(colourId, colour); }
        ~ScopedColour() { ImGui::PopStyleColor(); }
    };

    class ScopedFont
    {
    public:
        ScopedFont(const ScopedFont&) = delete;
        ScopedFont operator=(const ScopedFont&) = delete;
        ScopedFont(ImFont* font) { ImGui::PushFont(font); }
        ~ScopedFont() { ImGui::PopFont(); }
    };

    class ScopedID
    {
    public:
        ScopedID(const ScopedID&) = delete;
        ScopedID operator=(const ScopedID&) = delete;
        template<typename T>
        ScopedID(T id) { ImGui::PushID(id); }
        ~ScopedID() { ImGui::PopID(); }
    };

    class ScopedColourStack
    {
    public:
        ScopedColourStack(const ScopedColourStack&) = delete;
        ScopedColourStack operator=(const ScopedColourStack&) = delete;

        template <typename ColourType, typename... OtherColours>
        ScopedColourStack(ImGuiCol firstColourID, ColourType firstColour, OtherColours&& ... otherColourPairs)
            : m_Count((sizeof... (otherColourPairs) / 2) + 1)
        {
            static_assert ((sizeof... (otherColourPairs) & 1u) == 0,
                "ScopedColourStack constructor expects a list of pairs of colour IDs and colours as its arguments");

            PushColour(firstColourID, firstColour, std::forward<OtherColours>(otherColourPairs)...);
        }

        ~ScopedColourStack() { ImGui::PopStyleColor(m_Count); }

    private:
        int m_Count;

        template <typename ColourType, typename... OtherColours>
        void PushColour(ImGuiCol colourID, ColourType colour, OtherColours&& ... otherColourPairs)
        {
            if constexpr (sizeof... (otherColourPairs) == 0)
            {
                ImGui::PushStyleColor(colourID, colour);
            }
            else
            {
                ImGui::PushStyleColor(colourID, colour);
                PushColour(std::forward<OtherColours>(otherColourPairs)...);
            }
        }
    };

    class ScopedStyleStack
    {
    public:
        ScopedStyleStack(const ScopedStyleStack&) = delete;
        ScopedStyleStack operator=(const ScopedStyleStack&) = delete;

        template <typename ValueType, typename... OtherStylePairs>
        ScopedStyleStack(ImGuiStyleVar firstStyleVar, ValueType firstValue, OtherStylePairs&& ... otherStylePairs)
            : m_Count((sizeof... (otherStylePairs) / 2) + 1)
        {
            static_assert ((sizeof... (otherStylePairs) & 1u) == 0,
                "ScopedStyleStack constructor expects a list of pairs of colour IDs and colours as its arguments");

            PushStyle(firstStyleVar, firstValue, std::forward<OtherStylePairs>(otherStylePairs)...);
        }

        ~ScopedStyleStack() { ImGui::PopStyleVar(m_Count); }

    private:
        int m_Count;

        template <typename ValueType, typename... OtherStylePairs>
        void PushStyle(ImGuiStyleVar styleVar, ValueType value, OtherStylePairs&& ... otherStylePairs)
        {
            if constexpr (sizeof... (otherStylePairs) == 0)
            {
                ImGui::PushStyleVar(styleVar, value);
            }
            else
            {
                ImGui::PushStyleVar(styleVar, value);
                PushStyle(std::forward<OtherStylePairs>(otherStylePairs)...);
            }
        }
    };


	ImU32 ColourWithValue(const ImColor& color, float value);
	ImU32 ColourWithSaturation(const ImColor& color, float saturation);
	ImU32 ColourWithHue(const ImColor& color, float hue);
	ImU32 ColourWithMultipliedValue(const ImColor& color, float multiplier);
	ImU32 ColourWithMultipliedSaturation(const ImColor& color, float multiplier);
	ImU32 ColourWithMultipliedHue(const ImColor& color, float multiplier);





	bool BeginPopup(const char* str_id, ImGuiWindowFlags flags = 0);
	void EndPopup();

	bool SearchWidget(std::string& searchString, const char* hint = "Search...", bool* grabFocus = nullptr);
	bool OptionsButton();




	// CustomTreeNode
	bool TreeNodeWithIcon(ImTextureID icon, ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end, ImColor iconTint = IM_COL32_WHITE);
	bool TreeNodeWithIcon(ImTextureID icon, const void* ptr_id, ImGuiTreeNodeFlags flags, ImColor iconTint, const char* fmt, ...);
	bool TreeNodeWithIcon(ImTextureID icon, const char* label, ImGuiTreeNodeFlags flags, ImColor iconTint = IM_COL32_WHITE);


}
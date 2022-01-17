
#include "apImGui.h"
#include "apGraphics.h"
#include "apRenderer.h"
#include "apGraphicsDevice_DX12.h"
#include "apTextureHelper.h"
#include "apHelper.h"
#include "AppleEngine.h"
#pragma warning(disable : 4996)



#include <filesystem>

namespace ap::imgui
{




	static int s_UIContextID = 0;
	static uint32_t s_Counter = 0;
	static char s_IDBuffer[16];

	static int s_CheckboxCount = 0;


	ap::Resource s_SearchIcon;
	ap::Resource s_ClearIcon;
	ap::Resource s_GearIcon;
	
	void Initialize()
	{
		s_SearchIcon = ap::resourcemanager::Load("Resources/images/icon_search_24px.png");
		s_ClearIcon = ap::resourcemanager::Load("Resources/images/close.png");
		s_GearIcon = ap::resourcemanager::Load("Resources/images/gear_icon.png");


	}

	const char* GenerateID()
	{
		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);

		return &s_IDBuffer[0];
	}


	void ShiftCursorX(float distance)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + distance);
	}

	void ShiftCursorY(float distance)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + distance);
	}

	void ShiftCursor(float x, float y)
	{
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(cursor.x + x, cursor.y + y));
	}

	inline void PushID()
	{
		ImGui::PushID(s_UIContextID++);
		s_Counter = 0;
	}
	inline void PopID()
	{
		ImGui::PopID();
		s_UIContextID--;
	}

	bool IsItemDisabled()
	{
		return ImGui::GetItemFlags() & ImGuiItemFlags_Disabled;
	}

	ImRect GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	ImRect RectExpanded(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x -= x;
		result.Min.y -= y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}


	void DrawItemActivityOutline(float rounding , bool drawWhenInactive , ImColor colourWhenActive )
	{
		auto* drawList = ImGui::GetWindowDrawList();
		const ImRect rect = RectExpanded(GetItemRect(), 1.0f, 1.0f);
		if (ImGui::IsItemHovered() && !ImGui::IsItemActive())
		{
			drawList->AddRect(rect.Min, rect.Max,
				ImColor(60, 60, 60), rounding, 0, 1.5f);
		}
		if (ImGui::IsItemActive())
		{
			drawList->AddRect(rect.Min, rect.Max,
				colourWhenActive, rounding, 0, 1.0f);
		}
		else if (!ImGui::IsItemHovered() && drawWhenInactive)
		{
			drawList->AddRect(rect.Min, rect.Max,
				ImColor(50, 50, 50), rounding, 0, 1.0f);
		}
	};











	bool PropertyGridHeader(const std::string& name, bool openByDefault )
	{
		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowItemOverlap
			| ImGuiTreeNodeFlags_FramePadding;

		if (openByDefault)
			treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

		bool open = false;
		const float framePaddingX = 6.0f;
		const float framePaddingY = 6.0f; // affects height of the header

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ framePaddingX, framePaddingY });


		ImGui::PushID(name.c_str());
		open = ImGui::TreeNodeEx("##dummy_id", treeNodeFlags, ap::helper::toUpper(name).c_str());
		ImGui::PopID();

		ImGui::PopStyleVar(2);


		return open;
	}



	void BeginCheckboxGroup(const char* label)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
	}


	void EndCheckboxGroup()
	{
		ImGui::PopItemWidth();
		ImGui::NextColumn();
		s_CheckboxCount = 0;
	}

	bool PropertyCheckboxGroup(const char* label, bool& value)
	{
		bool modified = false;

		if (++s_CheckboxCount > 1)
			ImGui::SameLine();

		ImGui::Text(label);
		ImGui::SameLine();

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);

		if (IsItemDisabled())
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

		if (ImGui::Checkbox(s_IDBuffer, &value))
			modified = true;

		if (IsItemDisabled())
			ImGui::PopStyleVar();

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		return modified;
	}


	void BeginPropertyGrid()
	{
		PushID();
		ImGui::Columns(2);
	}

	void EndPropertyGrid()
	{
		ImGui::Columns(1);
		PopID();
	}

	void PropertyGridSpacing()
	{
		ImGui::Spacing();
		ImGui::NextColumn();
		ImGui::NextColumn();
	}


	bool DrawInputText(const char* label, std::string& value, bool error )
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256];
		strcpy_s<256>(buffer, value.c_str());

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);

		if (error)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		if (ImGui::InputText(s_IDBuffer, buffer, 256))
		{
			value = buffer;
			modified = true;
		}
		if (error)
			ImGui::PopStyleColor();

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	void DrawInputText(const char* label, const std::string& value)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		itoa(s_Counter++, s_IDBuffer + 2, 16);

		ImGui::InputText(s_IDBuffer, (char*)value.c_str(), value.size(), ImGuiInputTextFlags_ReadOnly);

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void DrawInputText(const char* label, const char* value)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		ImGui::InputText(GenerateID() , (char*)value, 256, ImGuiInputTextFlags_ReadOnly);

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);


		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}



	bool DrawCombo(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected)
	{
		const char* current = options[*selected].c_str();
		ImGui::Text(label);
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		bool changed = false;

		const std::string id = "##" + std::string(label);
		if (ImGui::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				const bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i].c_str(), is_selected))
				{
					current = options[i].c_str();
					*selected = i;
					changed = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);


		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return changed;
	}


	bool DrawCheckbox(const char* label, bool& value, bool disable)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		if (disable)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}

		if (ImGui::Checkbox(GenerateID(), &value))
			modified = true;

		if (disable)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	bool DrawDragFloat(const char* label, float& value, float delta , float min , float max , bool readOnly )
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);

		if (!readOnly)
		{
			if (ImGui::DragFloat(s_IDBuffer, &value, delta, min, max))
				modified = true;
		}
		else
		{
			ImGui::InputFloat(s_IDBuffer, &value, 0.0F, 0.0F, "%.3f", ImGuiInputTextFlags_ReadOnly);
		}

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;

	}

	bool DrawDragFloat2(const char* label, XMFLOAT2& value, float delta )
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::DragFloat2(s_IDBuffer, (float*)&value, delta))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	bool DrawDragFloat3(const char* label, XMFLOAT3& value, float delta )
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::DragFloat3(s_IDBuffer, (float*)&value, delta))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	bool DrawDragFloat4(const char* label, XMFLOAT4& value, float delta )
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::DragFloat4(s_IDBuffer, (float*)&value, delta))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}


	bool DrawSliderInt(const char* label, int& value, int min, int max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::SliderInt(s_IDBuffer, &value, min, max))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	bool DrawSliderInt(const char* label, uint32_t& value, uint32_t min, uint32_t max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::SliderInt(s_IDBuffer, (int*)&value, min, max))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}


	bool DrawSliderFloat(const char* label, float& value, float min, float max, const char* format ,float power)
	{

		// 파워 삭제
		
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::SliderFloat(s_IDBuffer, &value, min, max, format))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}


	bool DrawSliderFloat2(const char* label, XMFLOAT2& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);



		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::SliderFloat2(s_IDBuffer, (float*)&value, min, max))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	bool DrawSliderFloat3(const char* label, XMFLOAT3& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::SliderFloat3(s_IDBuffer, (float*)&value, min, max))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	bool DrawSliderFloat4(const char* label, XMFLOAT4& value, float min, float max)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::SliderFloat4(s_IDBuffer, (float*)&value, min, max))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}


	bool DrawColorEdit3(const char* label, XMFLOAT3& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::ColorEdit3(s_IDBuffer, (float*)&value))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	bool DrawColorEdit4(const char* label, XMFLOAT4& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::ColorEdit4(s_IDBuffer, (float*)&value))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	bool DrawColorEdit3(const char* label, float* value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::ColorEdit3(s_IDBuffer, value))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}

	bool DrawButton(const char* label, const ImVec2& size )
	{
		bool result = ImGui::Button(label, size);
		ImGui::NextColumn();
		return result;
	}

	bool DrawButton2(const char* label, bool fill, const ImVec2& size_in)
	{
		bool modified = false;

		ImVec2 size = size_in;

		if (fill)
		{
			auto region = ImGui::GetContentRegionAvail();
			size.x = region.x;
		}

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button(label, size))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;

	}

	



	bool DrawVec3Control(const std::string& label, XMFLOAT3& values, float resetValue  , bool isScale , float columnWidth )
	{


		bool modified = false;

		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		float max = 0.0;
		float min = 0.0f;

		if (isScale)
		{
			min = 0.01;
			max = 50.0f;
		}


		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();


		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
		{
			values.x = resetValue;
			modified = true;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		if(ImGui::DragFloat("##X", &values.x, 0.1f, min, max, "%.2f"))
			modified = true;
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
		{
			values.y = resetValue;
			modified = true;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();

		if(ImGui::DragFloat("##Y", &values.y, 0.1f, min, max, "%.2f"))
			modified = true;
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
		{
			values.z = resetValue;
			modified = true;
		}
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		if(ImGui::DragFloat("##Z", &values.z, 0.1f, min, max, "%.2f"))
			modified = true;
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopID();

		return modified;
	}

	bool ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;

		// Default to using texture ID as ID. User can still push string/integer prefixes.

		PushID();
		const ImGuiID id = window->GetID("#image");
		PopID();

		const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : g.Style.FramePadding;
		return ImGui::ImageButtonEx(id, user_texture_id, size, uv0, uv1, padding, bg_col, tint_col);
	}




	bool DrawImage(const char* label, ap::ecs::Entity materialEntity, int textureIndex)
	{
		bool modified = false;

		ap::scene::Scene& scene = ap::scene::GetScene();

		ap::scene::MaterialComponent* material = scene.materials.GetComponent(materialEntity);

		if (material == nullptr)
			return false;

		ap::scene::MaterialComponent::TextureMap* textureMap = &material->textures[textureIndex];


		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		assert(textureMap != nullptr);

		const ap::graphics::Texture* texture = static_cast<const ap::graphics::Texture*>(textureMap->GetGPUResource());
		
		bool textureIsValid = texture;
		if (!textureIsValid)
		{
			texture = ap::texturehelper::getWhite();
		}
		

		uint64_t textureID = ap::graphics::GetDevice()->CopyDescriptorToImGui(texture);;
		ImGui::Image((void*)textureID, ImVec2(70.f, 70.0f));

		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted("Image");
			ImGui::PopTextWrapPos();
			ImGui::Image((void*)textureID, ImVec2(384.f, 384.0f));
			ImGui::EndTooltip();


			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (textureIsValid)
				{
					textureMap->resource = {};

				}
				else
				{
					ap::helper::FileDialogParams params;
					params.type = ap::helper::FileDialogParams::OPEN;
					params.description = "Texture";
					params.extensions.push_back("dds");
					params.extensions.push_back("png");
					params.extensions.push_back("jpg");
					params.extensions.push_back("jpeg");
					params.extensions.push_back("tga");
					params.extensions.push_back("bmp");

					
					ap::helper::FileDialog(params, [material, textureIndex](std::string fileName) {
						ap::eventhandler::Subscribe_Once(ap::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
							material->textures[textureIndex].name = fileName;
							material->textures[textureIndex].resource = ap::resourcemanager::Load(fileName, ap::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
							});
						});
				}


				modified = true;
			}

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{


			}




		}


		if (ImGui::BeginDragDropTarget())
		{
			auto data = ImGui::AcceptDragDropPayload("Asset");
			if (data)
			{
				std::filesystem::path assetPath = *((std::filesystem::path*)data->Data);
				if (assetPath.extension() == ".png" || assetPath.extension() == ".dds" || assetPath.extension() == ".jpg")
				{
					textureMap->resource = ap::resourcemanager::Load(assetPath.string().c_str(), ap::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
					textureMap->name = assetPath.string().c_str();
					modified = true;
				}

			}
			ImGui::EndDragDropTarget();
		}



		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;


	}


	bool DrawImage(const char* label, ImTextureID texture, const  XMFLOAT2& size )
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		ImGui::Image((void*)texture, ImVec2(size.x, size.y));

		float scale = 5.5f;
		if (size.x > 100.0f && size.y > 100.0f)
		{
			scale = 3.0f;
		}


		{
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted("Image");
				ImGui::PopTextWrapPos();
				ImGui::Image((void*)texture, ImVec2(size.x * scale, size.y * scale));
				ImGui::EndTooltip();
			}
		}


		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;


	}

	void DrawImage(ImTextureID image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		ImGui::Image(image, size, uv0, uv1, tint_col, border_col);

	}

	void DrawButtonImage(ImTextureID image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImRect rectangle)
	{
		DrawButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, rectangle.Min, rectangle.Max);
	}

	void DrawButtonImage(ImTextureID imageNormal, ImTextureID imageHovered, ImTextureID imagePressed, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed, ImVec2 rectMin, ImVec2 rectMax)
	{
		auto* drawList = ImGui::GetWindowDrawList();
		if (ImGui::IsItemActive())
			drawList->AddImage((imagePressed), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintPressed);
		else if (ImGui::IsItemHovered())
			drawList->AddImage((imageHovered), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintHovered);
		else
			drawList->AddImage((imageNormal), rectMin, rectMax, ImVec2(0, 0), ImVec2(1, 1), tintNormal);
	}


	 ImU32 ColourWithValue(const ImColor& color, float value)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, sat, std::min(value, 1.0f));
	}

	 ImU32 ColourWithSaturation(const ImColor& color, float saturation)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, std::min(saturation, 1.0f), val);
	}

	 ImU32 ColourWithHue(const ImColor& color, float hue)
	{
		const ImVec4& colRow = color.Value;
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, h, s, v);
		return ImColor::HSV(std::min(hue, 1.0f), s, v);
	}

	 ImU32 ColourWithMultipliedValue(const ImColor& color, float multiplier)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, sat, std::min(val * multiplier, 1.0f));
	}

	 ImU32 ColourWithMultipliedSaturation(const ImColor& color, float multiplier)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, std::min(sat * multiplier, 1.0f), val);
	}

	 ImU32 ColourWithMultipliedHue(const ImColor& color, float multiplier)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(std::min(hue * multiplier, 1.0f), sat, val);
	}

	bool BeginPopup(const char* str_id, ImGuiWindowFlags flags)
	{
		bool opened = false;
		if (ImGui::BeginPopup(str_id, flags))
		{
			opened = true;
			// Fill background wiht nice gradient
			const float padding = ImGui::GetStyle().WindowBorderSize;
			const ImRect windowRect = RectExpanded(ImGui::GetCurrentWindow()->Rect(), -padding, -padding);
			ImGui::PushClipRect(windowRect.Min, windowRect.Max, false);
			const ImColor col1 = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);// Colours::Theme::backgroundPopup;
			const ImColor col2 = ColourWithMultipliedValue(col1, 0.8f);
			ImGui::GetWindowDrawList()->AddRectFilledMultiColor(windowRect.Min, windowRect.Max, col1, col1, col2, col2);
			ImGui::GetWindowDrawList()->AddRect(windowRect.Min, windowRect.Max, ColourWithMultipliedValue(col1, 1.1f));
			ImGui::PopClipRect();

			// Popped in EndPopup()
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 80));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1.0f, 1.0f));
		}

		return opened;
	}

	void EndPopup()
	{
		ImGui::PopStyleVar(); // WindowPadding;
		ImGui::PopStyleColor(); // HeaderHovered;
		ImGui::EndPopup();
	}


	bool SearchWidget(std::string& searchString, const char* hint, bool* grabFocus )
	{

		PushID();

		ShiftCursorY(1.0f);
		
		const bool layoutSuspended = []
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->DC.CurrentLayout)
			{
				ImGui::SuspendLayout();
				return true;
			}
			return false;
		}();

		bool modified = false;
		bool searching = false;

		const float areaPosX = ImGui::GetCursorPosX();
		const float framePaddingY = ImGui::GetStyle().FramePadding.y;

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(28.0f, framePaddingY));


		constexpr uint32_t BuffSize = 256;
		char searchBuffer[BuffSize]{};
		strcpy_s<BuffSize>(searchBuffer, searchString.c_str());

		

		if (ImGui::InputText(GenerateID(), searchBuffer, BuffSize))
		{
			searchString = searchBuffer;
			modified = true;
		}
		else if (ImGui::IsItemDeactivatedAfterEdit())
		{
			searchString = searchBuffer;
			modified = true;
		}

		searching = searchBuffer[0] != 0;
		

		if (grabFocus && *grabFocus)
		{
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
				&& !ImGui::IsAnyItemActive()
				&& !ImGui::IsMouseClicked(0))
			{
				ImGui::SetKeyboardFocusHere(-1);
			}

			if (ImGui::IsItemFocused())
				*grabFocus = false;
		}

		DrawItemActivityOutline(3.0f, true, ap::imguicolor::accent);
		ImGui::SetItemAllowOverlap();

		ImGui::SameLine(areaPosX + 5.0f);

		if (layoutSuspended)
			ImGui::ResumeLayout();

		
		ImGui::BeginHorizontal(GenerateID(), ImGui::GetItemRectSize());
		const ImVec2 iconSize(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight());

		// Search icon
		{
			const float iconYOffset = framePaddingY - 3.0f;
			ShiftCursorY(iconYOffset);
			
			uint64_t textureid = ap::graphics::GetDevice()->CopyDescriptorToImGui(&s_SearchIcon.GetTexture());
			ImGui::Image((void*)textureid, iconSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(1.0f, 1.0f, 1.0f, 0.2f));
			ShiftCursorY(-iconYOffset);

			// Hint
			if (!searching)
			{
				ShiftCursorY(-framePaddingY + 1.0f);
				ImGui::PushStyleColor(ImGuiCol_Text, ap::imguicolor::textDarker);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, framePaddingY));
				ImGui::TextUnformatted(hint);
				ShiftCursorY(-1.0f);
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
			}
		}

		ImGui::Spring();

		 //Clear icon
		if (searching)
		{
			const float spacingX = 4.0f;
			const float lineHeight = ImGui::GetItemRectSize().y - framePaddingY / 2.0f;

			if (ImGui::InvisibleButton(GenerateID(), ImVec2{ lineHeight, lineHeight }))
			{
				searchString.clear();
				modified = true;
			}

			if (ImGui::IsMouseHoveringRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()))
				ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
			uint64_t textureid = ap::graphics::GetDevice()->CopyDescriptorToImGui(&s_ClearIcon.GetTexture());
			DrawButtonImage((void*)textureid, IM_COL32(160, 160, 160, 200),
				IM_COL32(170, 170, 170, 255),
				IM_COL32(160, 160, 160, 150),
				RectExpanded(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), -2.0f, -2.0f));

			ImGui::Spring(-1.0f, spacingX * 2.0f);
		}

		ImGui::EndHorizontal();
		ShiftCursorY(-1.0f);
		PopID();
		
		ImGui::PopStyleVar(); //ImGuiStyleVar_FramePadding
		ImGui::PopStyleVar(); //ImGuiStyleVar_FrameRounding

		return modified;

	}

	bool OptionsButton()
	{
		const bool clicked = ImGui::InvisibleButton("##options", ImVec2{ ImGui::GetFrameHeight(), ImGui::GetFrameHeight() });

		const float spaceAvail = std::min(ImGui::GetItemRectSize().x, ImGui::GetItemRectSize().y);
		const float desiredIconSize = 15.0f;
		const float padding = std::max((spaceAvail - desiredIconSize) / 2.0f, 0.0f);

		constexpr auto buttonColour = ap::imguicolor::text;
		const uint8_t value = uint8_t(ImColor(buttonColour).Value.x * 255);
		uint64_t textureid = ap::graphics::GetDevice()->CopyDescriptorToImGui(&s_GearIcon.GetTexture());
		DrawButtonImage((void*)textureid, IM_COL32(value, value, value, 200),
			IM_COL32(value, value, value, 255),
			IM_COL32(value, value, value, 150),
			RectExpanded(GetItemRect(), -padding, -padding));
		return clicked;
	}

	bool TreeNodeWithIcon(ImTextureID icon, ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end, ImColor iconTint)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		ImGuiLastItemData& lastItem = g.LastItemData;
		const ImGuiStyle& style = g.Style;
		const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
		const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

		if (!label_end)
			label_end = ImGui::FindRenderedTextEnd(label);
		const ImVec2 label_size = ImGui::CalcTextSize(label, label_end, false);

		// We vertically grow up to current line height up the typical widget height.
		const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
		ImRect frame_bb;
		frame_bb.Min.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
		frame_bb.Min.y = window->DC.CursorPos.y;
		frame_bb.Max.x = window->WorkRect.Max.x;
		frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
		if (display_frame)
		{
			// Framed header expand a little outside the default padding, to the edge of InnerClipRect
			// (FIXME: May remove this at some point and make InnerClipRect align with WindowPadding.x instead of WindowPadding.x*0.5f)
			frame_bb.Min.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
			frame_bb.Max.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
		}

		const float text_offset_x = g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2);           // Collapser arrow width + Spacing
		const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset);                    // Latch before ItemSize changes it
		const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);  // Include collapser
		ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
		ImGui::ItemSize(ImVec2(text_width, frame_height), padding.y);

		// For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
		ImRect interact_bb = frame_bb;
		if (!display_frame && (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth)) == 0)
			interact_bb.Max.x = frame_bb.Min.x + text_width + style.ItemSpacing.x * 2.0f;

		// Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
		// For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
		// This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
		const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
		bool is_open = ImGui::TreeNodeBehaviorIsOpen(id, flags);
		if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
			window->DC.TreeJumpToParentOnPopMask |= (1 << window->DC.TreeDepth);

		bool item_add = ImGui::ItemAdd(interact_bb, id);
		lastItem.StatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
		lastItem.DisplayRect = frame_bb;

		if (!item_add)
		{
			if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
				ImGui::TreePushOverrideID(id);
			IMGUI_TEST_ENGINE_ITEM_INFO(lastItem.ID, label, lastItem.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
			return is_open;
		}

		ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
		if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
			button_flags |= ImGuiButtonFlags_AllowItemOverlap;
		if (!is_leaf)
			button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

		// We allow clicking on the arrow section with keyboard modifiers held, in order to easily
		// allow browsing a tree while preserving selection with code implementing multi-selection patterns.
		// When clicking on the rest of the tree node we always disallow keyboard modifiers.
		const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
		const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
		const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
		if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
			button_flags |= ImGuiButtonFlags_NoKeyModifiers;

		// Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
		// Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
		// - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
		// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
		// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
		// - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
		// - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
		// It is rather standard that arrow click react on Down rather than Up.
		// We set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
		if (is_mouse_x_over_arrow)
			button_flags |= ImGuiButtonFlags_PressedOnClick;
		else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
			button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
		else
			button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

		bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
		const bool was_selected = selected;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
		bool toggled = false;
		if (!is_leaf)
		{
			if (pressed && g.DragDropHoldJustPressedId != id)
			{
				if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 || (g.NavActivateId == id))
					toggled = true;
				if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
					toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
				if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseDoubleClicked[0])
					toggled = true;
			}
			else if (pressed && g.DragDropHoldJustPressedId == id)
			{
				IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
				if (!is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
					toggled = true;
			}

			if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Left && is_open)
			{
				toggled = true;
				ImGui::NavMoveRequestCancel();
			}
			if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
			{
				toggled = true;
				ImGui::NavMoveRequestCancel();
			}

			if (toggled)
			{
				is_open = !is_open;
				window->DC.StateStorage->SetInt(id, is_open);
				lastItem.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
			}
		}
		if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
			ImGui::SetItemAllowOverlap();

		// In this branch, TreeNodeBehavior() cannot toggle the selection so this will never trigger.
		if (selected != was_selected) //-V547
			lastItem.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

		// Render

		const ImU32 arrow_col = selected ? ap::imguicolor::backgroundDark : ap::imguicolor::muted;

		ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
		if (display_frame)
		{
			// Framed type
			const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : (hovered && !selected && !held && !pressed && !toggled) ? ImGuiCol_HeaderHovered : ImGuiCol_Header);

			ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
			ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);
			if (flags & ImGuiTreeNodeFlags_Bullet)
				ImGui::RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f), arrow_col);
			else if (!is_leaf)
				ImGui::RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y), arrow_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
			else // Leaf without bullet, left-adjusted text
				text_pos.x -= text_offset_x;
			if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
				frame_bb.Max.x -= g.FontSize + style.FramePadding.x;

			//! Draw icon
			if (icon)
			{
				// Store item data
				auto itemId = lastItem.ID;
				auto itemFlags = lastItem.InFlags;
				auto itemStatusFlags = lastItem.StatusFlags;
				auto itemRect = lastItem.Rect;

				// Draw icon image which messes up last item data
				const float pad = 3.0f;
				const float arrowWidth = 20.0f + 1.0f;
				auto cursorPos = ImGui::GetCursorPos();
				ShiftCursorY(-frame_height + pad);
				ShiftCursorX(arrowWidth);
				DrawImage(icon, { frame_height - pad * 2.0f, frame_height - pad * 2.0f }, ImVec2(0, 0), ImVec2(1, 1), iconTint /*selected ? colourDark : tintFloat*/);

				// Restore itme data
				ImGui::SetLastItemData(itemId, itemFlags, itemStatusFlags, itemRect);

				text_pos.x += frame_height + 2.0f;
			}

			text_pos.y -= 1.0f;



			if (g.LogEnabled)
			{
				// NB: '##' is normally used to hide text (as a library-wide feature), so we need to specify the text range to make sure the ## aren't stripped out here.
				const char log_prefix[] = "\n##";
				const char log_suffix[] = "##";
				ImGui::LogRenderedText(&text_pos, log_prefix, log_prefix + 3);
				ImGui::RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
				ImGui::LogRenderedText(&text_pos, log_suffix, log_suffix + 2);
			}
			else
			{
				ImGui::RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
			}
		}
		else
		{
			// Unframed typed for tree nodes
			if (hovered || selected)
			{
				//if (held && hovered) HZ_CORE_WARN("held && hovered");
				//if(hovered && !selected && !held && !pressed && !toggled) HZ_CORE_WARN("hovered && !selected && !held");
				//else if(!selected) HZ_CORE_WARN("ImGuiCol_Header");

				const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : (hovered && !selected && !held && !pressed && !toggled) ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
				ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
				ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);
			}
			if (flags & ImGuiTreeNodeFlags_Bullet)
				ImGui::RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f), arrow_col);
			else if (!is_leaf)
				ImGui::RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f), arrow_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 0.70f);

			//! Draw icon
			if (icon)
			{
				// Store item data
				auto itemId = lastItem.ID;
				auto itemFlags = lastItem.InFlags;
				auto itemStatusFlags = lastItem.StatusFlags;
				auto itemRect = lastItem.Rect;

				// Draw icon image which messes up last item data
				const float pad = 3.0f;
				const float arrowWidth = 20.0f + 1.0f;
				auto cursorPos = ImGui::GetCursorPos();
				ShiftCursorY(-frame_height + pad);
				ShiftCursorX(arrowWidth);
				DrawImage(icon, { frame_height - pad * 2.0f, frame_height - pad * 2.0f }, ImVec2(0, 0), ImVec2(1, 1), iconTint /*selected ? colourDark : tintFloat*/);

				// Restore itme data
				ImGui::SetLastItemData(itemId, itemFlags, itemStatusFlags, itemRect);

				text_pos.x += frame_height + 2.0f;
			}

			text_pos.y -= 1.0f;


			if (g.LogEnabled)
				ImGui::LogRenderedText(&text_pos, ">");
			ImGui::RenderText(text_pos, label, label_end, false);
		}

		if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
			ImGui::TreePushOverrideID(id);
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
		return is_open;
	}

	bool TreeNodeWithIcon(ImTextureID icon, const void* ptr_id, ImGuiTreeNodeFlags flags, ImColor iconTint, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const char* label_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);

		bool is_open = TreeNodeWithIcon(icon, window->GetID(ptr_id), flags, g.TempBuffer, label_end, iconTint);

		va_end(args);
		return is_open;
	}

	bool TreeNodeWithIcon(ImTextureID icon, const char* label, ImGuiTreeNodeFlags flags, ImColor iconTint)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		return TreeNodeWithIcon(icon, window->GetID(label), flags, label, NULL, iconTint);
	}


	





}
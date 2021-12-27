
#include "apImGui.h"
#include "apGraphics.h"
#include "apRenderer.h"
#include "apGraphicsDevice_DX12.h"
#include "apTextureHelper.h"
#include "apHelper.h"
#pragma warning(disable : 4996)


#include <filesystem>

namespace ap::imgui
{

	static int s_UIContextID = 0;
	static uint32_t s_Counter = 0;
	static char s_IDBuffer[16];

	static int s_CheckboxCount = 0;

	inline void ShiftCursorX(float distance)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + distance);
	}

	inline void ShiftCursorY(float distance)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + distance);
	}

	inline void ShiftCursor(float x, float y)
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

	inline bool IsItemDisabled()
	{
		return ImGui::GetItemFlags() & ImGuiItemFlags_Disabled;
	}

	inline ImRect GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	inline ImRect RectExpanded(const ImRect& rect, float x, float y)
	{
		ImRect result = rect;
		result.Min.x -= x;
		result.Min.y -= y;
		result.Max.x += x;
		result.Max.y += y;
		return result;
	}


	inline void DrawItemActivityOutline(float rounding = 0.0f, bool drawWhenInactive = false, ImColor colourWhenActive = ImColor(80, 80, 80))
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

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		ImGui::InputText(s_IDBuffer, (char*)value, 256, ImGuiInputTextFlags_ReadOnly);

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


	bool DrawCheckbox(const char* label, bool& value)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::Checkbox(s_IDBuffer, &value))
			modified = true;

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


	bool DrawSliderFloat(const char* label, float& value, float min, float max, const char* format )
	{
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

	bool DrawButton2(const char* label, const ImVec2& size)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		s_IDBuffer[0] = '#';
		s_IDBuffer[1] = '#';
		memset(s_IDBuffer + 2, 0, 14);
		sprintf_s(s_IDBuffer + 2, 14, "%o", s_Counter++);
		if (ImGui::Button(label, size))
			modified = true;

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;

	}


	void DrawVec3Control(const std::string& label, XMFLOAT3& values, float resetValue  , bool isScale , float columnWidth )
	{
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
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, min, max, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, min, max, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, min, max, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopID();
	}




	bool DrawImage(const char* label, ap::scene::MaterialComponent::TextureMap* textureMap)
	{
		bool modified = false;


		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		assert(textureMap != nullptr);

		const ap::graphics::Texture* texture = static_cast<const ap::graphics::Texture*>(textureMap->GetGPUResource());
		
		if (!texture->IsValid())
		{
			texture = ap::texturehelper::getWhite();
		}

		uint64_t textureID = ap::graphics::GetDevice()->CopyDescriptorToImGui(texture);;
		ImGui::Image((void*)textureID, ImVec2(70.f, 70.0f));

		/*if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted("Image");
			ImGui::PopTextWrapPos();
			ImGui::Image((void*)textureID, ImVec2(384.f, 384.0f));
			ImGui::EndTooltip();


			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (textureMap->resource && textureMap->resource->texture.IsValid())
				{
					textureMap->resource = nullptr;

				}

				ap::helper::FileDialogParams params;
				params.type = ap::helper::FileDialogParams::OPEN;
				params.description = "Texture";
				params.extensions.push_back("dds");
				params.extensions.push_back("png");
				params.extensions.push_back("jpg");
				params.extensions.push_back("jpeg");
				params.extensions.push_back("tga");
				params.extensions.push_back("bmp");
				ap::helper::FileDialog(params, [&textureMap](std::string fileName) {
					ap::event::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
						textureMap->resource = ap::resourcemanager::Load(fileName, ap::resourcemanager::IMPORT_RETAIN_FILEDATA);
						textureMap->name = fileName;
						});
					});

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
					textureMap->resource = apResourceManager::Load(assetPath.string().c_str(), apResourceManager::IMPORT_RETAIN_FILEDATA);
					textureMap->name = assetPath.string().c_str();
					modified = true;
				}

			}
			ImGui::EndDragDropTarget();
		}*/



		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;


	}


	bool DrawImage(const char* label, const ap::graphics::Texture* texture, const  XMFLOAT2& size )
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);


		uint64_t textureID = ap::graphics::GetDevice()->CopyDescriptorToImGui(texture);;

		ImGui::Image((void*)textureID, ImVec2(size.x, size.y));

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
				ImGui::Image((void*)textureID, ImVec2(size.x * scale, size.y * scale));
				ImGui::EndTooltip();
			}
		}


		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;


	}




}
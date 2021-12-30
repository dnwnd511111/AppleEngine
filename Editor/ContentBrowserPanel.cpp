#include "pch.h"
#include "ContentBrowserPanel.h"
#include "Editor.h"
#include "apResourceManager.h"
#include "apImGui.h"



using namespace ap::ecs;
using namespace ap::scene;

using namespace ap::imgui;

namespace Panel
{
	static const std::filesystem::path s_AssetPath = "resources";

	ContentBrowserPanel::ContentBrowserPanel(Editor* editor)
		: 
		BasePanel(editor),
		currentDirectory(s_AssetPath)
	{
	}

	void  ContentBrowserPanel::IterateDirectroy(const std::filesystem::directory_entry& entry)
	{

		for (auto& directoryEntry : std::filesystem::directory_iterator(entry))
		{
			if (!directoryEntry.is_directory())
				continue;


			const auto& path = directoryEntry.path();
			std::string name = path.filename().string();
			bool isOpened = directries[path.string()];
			bool isActiveDirectory = currentDirectory == path;
			ImGuiTreeNodeFlags flags = ((isActiveDirectory || isOpened) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanFullWidth;
			std::string id = name + "_TreeNode";


			auto* window = ImGui::GetCurrentWindow();
			window->DC.CurrLineSize.y = 20.0f;
			window->DC.CurrLineTextBaseOffset = 3.0f;


			const ImRect itemRect = { window->WorkRect.Min.x, window->DC.CursorPos.y,
						  window->WorkRect.Max.x, window->DC.CursorPos.y + window->DC.CurrLineSize.y };

			const bool isItemClicked = [&itemRect, &id]
			{
				if (ImGui::ItemHoverable(itemRect, ImGui::GetID(id.c_str())))
				{
					return ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
				}
				return false;
			}();

			const bool isWindowFocused = ImGui::IsWindowFocused();


			auto fillWithColour = [&](const ImColor& colour)
			{
				const ImU32 bgColour = ImGui::ColorConvertFloat4ToU32(colour);
				ImGui::GetWindowDrawList()->AddRectFilled(itemRect.Min, itemRect.Max, bgColour);
			};


			const ap::graphics::Texture& tex = contentIcons[ICON_FOLDER].GetTexture();
			int mipmap = -1;
			uint64_t textureID = ap::graphics::GetDevice()->CopyDescriptorToImGui(&tex, mipmap);

			if (window->SkipItems)
				continue;



			// Fill background
			//----------------
			if (isActiveDirectory || isItemClicked)
			{
				if (isWindowFocused)
					fillWithColour(ap::imguicolor::selection);
				else
				{
					const ImColor col = ColourWithMultipliedValue(ap::imguicolor::selection, 0.8f);
					fillWithColour(ColourWithMultipliedSaturation(col, 0.7f));
				}

				currentDirectory = path;
				ImGui::PushStyleColor(ImGuiCol_Text, ap::imguicolor::backgroundDark);
			}

			
			bool open =TreeNodeWithIcon((ImTextureID)textureID, window->GetID(id.c_str()), flags,name.c_str(), nullptr);

			if (isActiveDirectory || isItemClicked)
				ImGui::PopStyleColor();

			// Fixing slight overlap
			ShiftCursorY(3.0f);

			if (open && directoryEntry.is_directory())
			{
				directries[path.string()] = true;
				IterateDirectroy(directoryEntry);
			}
			else if(directoryEntry.is_directory())
			{
				directries[path.string()] = false;
			}

			if (open)
				ImGui::TreePop();


		}

		
	}


	void ContentBrowserPanel::ImGuiRender(float dt)
	{

		static bool initialized = false;
		
		if (!initialized)
		{
			initialized = true;
			const std::vector<std::string> textures =
			{
				"resources\\images\\folder_closed.png",
				"resources\\images\\png.png",
				"resources\\images\\btn_back.png",
				"resources\\images\\btn_fwrd.png",
				"resources\\images\\refresh.png",
				"resources\\images\\file.png",
			};


			ap::jobsystem::context ctx;


			for (int i = 0; i < textures.size(); i++)
			{
				ap::jobsystem::Execute(ctx, [textures, i, this](ap::jobsystem::JobArgs args) { this->contentIcons[i] = ap::resourcemanager::Load(textures[i]); });
			}
			ap::jobsystem::Wait(ctx);
		}



		isContentBrowserHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

		{
			BeginPropertyGrid();

			static float padding = 16.0f;
			float cellSize = thumbnailSize + padding;

			ImGui::SetColumnOffset(1, 300.0f);
			ImGui::BeginChild("##folders_common");
			{
				if (ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
					ScopedColourStack itemBg(ImGuiCol_Header, IM_COL32_DISABLE,
						ImGuiCol_HeaderActive, IM_COL32_DISABLE);


					std::filesystem::directory_entry a(s_AssetPath);
					IterateDirectroy(a);


				}
			}
			ImGui::EndChild();

			ImGui::NextColumn();
			ImGui::BeginChild("##directory_structure", ImVec2(0, ImGui::GetWindowHeight() - 65));
			{
				//draw topbar
				RenderTopBar();

				ImGui::Separator();

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.35f));

				float panelWidth = ImGui::GetContentRegionAvail().x;
				int columnCount = (int)(panelWidth / cellSize);
				if (columnCount < 1)
					columnCount = 1;

				ImGui::Columns(columnCount, 0, false);

				std::function<void(ICON_TYPE)> renderDirectory = [&](ICON_TYPE type)
				{

					for (auto& directoryEntry : std::filesystem::directory_iterator(currentDirectory))
					{
						const auto& path = directoryEntry.path();
						std::string filenameString = path.filename().string();

						if (filenameString.find(searchStr) == std::string::npos)
							continue;


						ICON_TYPE iconType = ICON_COUNT;

						if (directoryEntry.is_directory())
						{
							iconType = ICON_FOLDER;
						}
						else if (directoryEntry.path().extension() == ".png")
						{
							iconType = ICON_PNG;
						}
						else if (directoryEntry.path().extension() == ".dds" || directoryEntry.path().extension() == ".jpg")
						{
							iconType = ICON_FILE;
						}


						if (iconType == type)
						{
							if (selectedPath == directoryEntry.path())
							{
								ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.25f, 0.75f));
							}

							const ap::graphics::Texture& tex = contentIcons[iconType].GetTexture();

							int mipmap = -1;

							if (0)//tex.desc.mip_levels > 1)
							{
								//tex.desc.width
								mipmap = tex.desc.mip_levels - std::floor(std::log2(thumbnailSize)+1) ;

							}
							uint64_t textureID = ap::graphics::GetDevice()->CopyDescriptorToImGui(&contentIcons[iconType].GetTexture(), mipmap);


							ImageButton((ImTextureID)textureID, { (float)thumbnailSize, (float)thumbnailSize });

							if (selectedPath == directoryEntry.path())
							{
								ImGui::PopStyleColor();
							}

							if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
							{
								selectedPath = directoryEntry.path();
							}

							if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
							{
								ImGui::SetDragDropPayload("Asset", (const void*)&selectedPath, sizeof(selectedPath));

								ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ 30.0f, 30.0f });
								ImGui::EndDragDropSource();

							}

							if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
							{
								if (directoryEntry.is_directory())
									currentDirectory /= path.filename();
							}

							ImGui::TextWrapped(filenameString.c_str());

							ImGui::NextColumn();
						}


					}
				};

				renderDirectory(ICON_FOLDER);
				renderDirectory(ICON_PNG);
				renderDirectory(ICON_FILE);


				ImGui::PopStyleColor(2);
				ImGui::Columns(1);
			}
			ImGui::EndChild();


			EndPropertyGrid();


		}




	}

	void ContentBrowserPanel::RenderTopBar()
	{
		ImGui::BeginChild("##top_bar", ImVec2(0, 40));
		{
			
			const ap::graphics::Texture& tex1 = contentIcons[ICON_BTN_BACK].GetTexture();
			const ap::graphics::Texture& tex2 = contentIcons[ICON_BTN_FORWRD].GetTexture();
			const ap::graphics::Texture& tex3 = contentIcons[ICON_BTN_REFRESH].GetTexture();

			int mipmap = -1;
			
			//if (tex1.desc.mip_levels > 1)
				//mipmap = tex1.desc.mip_levels - std::floor(std::log2(20) + 1);
			

			uint64_t backButton = ap::graphics::GetDevice()->CopyDescriptorToImGui(&tex1, mipmap);
			uint64_t forwardButton = ap::graphics::GetDevice()->CopyDescriptorToImGui(&tex2, mipmap);
			uint64_t refreshButton = ap::graphics::GetDevice()->CopyDescriptorToImGui(&tex3, 0);

			
			if (ImageButton((void*)backButton, ImVec2(20, 20), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0.0, 0.0, 0.0, 0.0f), ImVec4(0.7, 0.7, 0.7, 0.7f)))
			{
				if (currentDirectory != std::filesystem::path(s_AssetPath))
				{
					currentDirectory = currentDirectory.parent_path();
				}
				
			}
			

			ImGui::SameLine();

			if (ImageButton((void*)forwardButton, ImVec2(20, 20), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0.0, 0.0, 0.0, 0.0f), ImVec4(0.7, 0.7, 0.7, 0.7f)))
			{
				//추가해야함
			}


			ImGui::SameLine();

			if (ImageButton((void*)refreshButton, ImVec2(20, 20), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0.0, 0.0, 0.0, 0.0f), ImVec4(0.7, 0.7, 0.7, 0.7f)))
			{
				//추가해야함
			}

			ImGui::SameLine();
			
			ImGui::PushItemWidth(300);
			SearchWidget(searchStr);
			ImGui::PopItemWidth();
			
			
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 1.f);
			


			if (OptionsButton())
			{
				ImGui::OpenPopup("ContentBrowserSettings");
			}

			if (ap::imgui::BeginPopup("ContentBrowserSettings"))
			{


				ImGui::SliderFloat("##thumbnail_size", &thumbnailSize, 75.0f, 512.0f, "%.0f");
				ImGui::SetTooltip("Thumnail Size");

				EndPopup();
			}
			

		}
		ImGui::EndChild();


	}

}
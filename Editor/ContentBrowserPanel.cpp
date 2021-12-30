#include "pch.h"
#include "ContentBrowserPanel.h"
#include "Editor.h"
#include "apResourceManager.h"




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
			static int thumbnailSize = 75;
			float cellSize = thumbnailSize + padding;

			ImGui::SetColumnOffset(1, 300.0f);
			ImGui::BeginChild("##folders_common");
			{
				if (ImGui::CollapsingHeader("Content", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
				{
					//for (auto& [handle, directory] : m_BaseDirectory->SubDirectories)
						//RenderDirectoryHeirarchy(directory);
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
						//const auto& relativePath = std::filesystem::relative(path, s_AssetPath);
						//std::string filenameString = relativePath.filename().string();
						std::string filenameString = path.filename().string();



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

			//drawBottomBar

			ImGui::BeginChild("##panel_controls", ImVec2(ImGui::GetColumnWidth() - 12, 30), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				ImGui::Separator();
				ImGui::Columns(4, 0, false);
				//
				// 
				//
				ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::NextColumn();
				ImGui::SetNextItemWidth(ImGui::GetColumnWidth());
				ImGui::SliderInt("##column_count", &thumbnailSize, 50, 150);
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
			
			
		}
		ImGui::EndChild();


	}

}
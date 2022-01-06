#include "pch.h"
#include "HierarchyPanel.h"
#include "Editor.h"
#include "apECS.h"
#include "apScene.h"
#include "apOcean_waveworks.h"

using namespace ap::ecs;
using namespace ap::scene;

using namespace ap::imgui;

namespace Panel
{
	

	HierarchyPanel::HierarchyPanel(Editor* editor)
		:
		BasePanel(editor)
	{
	}

	
	bool IterateChildren(ap::ecs::Entity entity, std::string& input)
	{
		Scene& scene = GetScene();
		HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
		NameComponent* name = scene.names.GetComponent(entity);

		bool ret = false;
		if (name && name->name.find(input) != std::string::npos)
			ret = true;

		if (hier)
		{
			for (auto& child : hier->childrenID)
				ret |= IterateChildren(child, input);

		}

		return ret;
	}
	

	void HierarchyPanel::ImGuiRender(float dt)
	{
		
		if (ImGui::IsWindowHovered() && ImGui::IsWindowFocused())
			editor->renderComponent.DeleteSelectedEntities();

		Scene& scene = GetScene();

		static std::string inputStr;
		ImGui::PushItemWidth(-1);
		SearchWidget(inputStr);
		ImGui::PopItemWidth();



		for (int i = 0; i < scene.transforms.GetCount(); i++)
		{
			Entity entity = scene.transforms.GetEntity(i);
			HierarchyComponent* hierarchyComponent = scene.hierarchy.GetComponent(entity);
			NameComponent* name = scene.names.GetComponent(entity);

			if (hierarchyComponent == nullptr || hierarchyComponent->parentID == ap::ecs::INVALID_ENTITY)
			{

				if (inputStr.size() != 0 && !IterateChildren(entity,inputStr))
					continue;

				DrawEntityNode(entity);

			}

		}
		

		

		
	}

	void HierarchyPanel::ImGuiRenderProperties(float dt)
	{

		
		if (editor->renderComponent.translator.selected.size() == 1)
		{
			Entity entity = editor->renderComponent.translator.selected[0].entity;
			int& subsetIdx = editor->renderComponent.translator.selected[0].subsetIndex;
			DrawComponents(entity, subsetIdx);
		}

	}

	void HierarchyPanel::DrawEntityNode(ap::ecs::Entity entity) const
	{
		Scene& scene = GetScene();


		const char* name = "Unnamed Entity";

		NameComponent* nameComponent = scene.names.GetComponent(entity);
		if (nameComponent != nullptr)
			name = nameComponent->name.c_str();

	
		
		ImGuiTreeNodeFlags flags = ((editor->renderComponent.IsSelected(entity)) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanAvailWidth;


		HierarchyComponent* hierarchyComponent = scene.hierarchy.GetComponent(entity);

		if(hierarchyComponent == nullptr || hierarchyComponent->childrenID.size() ==0 )
			flags |= ImGuiTreeNodeFlags_Leaf;


		bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, name);
		
		if (ImGui::IsItemClicked() && ap::input::Down(ap::input::KEYBOARD_BUTTON_LSHIFT))
		{
			editor->renderComponent.AddSelected(entity);
		}
		else if (ImGui::IsItemClicked())
		{
			editor->renderComponent.ClearSelected();
			editor->renderComponent.AddSelected(entity);
			
		}



		if (opened)
		{
			if (hierarchyComponent != nullptr && hierarchyComponent->childrenID.size() > 0)
			{
				for (auto child : hierarchyComponent->childrenID)
				{
					DrawEntityNode(child);
				}
			}
			ImGui::TreePop();

		}
	}




	template<typename T, typename UIFunction>
	void DrawComponent(const std::string& name, T* component, UIFunction uiFunction, bool isSubComponent = true)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed
			| ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

		if (isSubComponent)
		{
			ImGui::Separator();
		}
		bool open = ImGui::TreeNodeEx(GenerateID(), treeNodeFlags, name.c_str());
		ImGui::PopStyleVar();

		if (isSubComponent)
		{

			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}
		}

		if (open)
		{
			uiFunction(*component);
			ImGui::TreePop();
		}

		//if (removeComponent)
			//entity.RemoveComponent<T>();
		

	}

	void InvalidateProbes()
	{
		Scene& scene = GetScene();

		for (int i = 0; i < scene.probes.GetCount(); i++)
		{
			Entity entity = scene.probes.GetEntity(i);
			EnvironmentProbeComponent* otherProbe = scene.probes.GetComponent(entity);
			otherProbe->SetDirty();
		}
	}



	void HierarchyPanel::DrawComponents(ap::ecs::Entity entity, int& subsetIdx)
	{

		Scene& scene = GetScene();

		HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
		ObjectComponent* object = scene.objects.GetComponent(entity);
		NameComponent* name = scene.names.GetComponent(entity);
		LayerComponent* layer = scene.layers.GetComponent(entity);
		TransformComponent* transform = scene.transforms.GetComponent(entity);
		ImpostorComponent* imposter = scene.impostors.GetComponent(entity);
		LightComponent* light = scene.lights.GetComponent(entity);
		CameraComponent* camera = scene.cameras.GetComponent(entity);
		EnvironmentProbeComponent* environmentprobe = scene.probes.GetComponent(entity);
		ForceFieldComponent* forcefield = scene.forces.GetComponent(entity);
		DecalComponent* decal = scene.decals.GetComponent(entity);
		ap::EmittedParticleSystem* emitter = scene.emitters.GetComponent(entity);
		ap::HairParticleSystem* hair = scene.hairs.GetComponent(entity);
		WeatherComponent* weather = &scene.weathers[0];
		SoundComponent* sound = scene.sounds.GetComponent(entity);
		InverseKinematicsComponent* kinematic = scene.inverse_kinematics.GetComponent(entity);
		SpringComponent* spring = scene.springs.GetComponent(entity);
		
		BeginPropertyGrid();
		PropertyGridSpacing();

		ImGui::Text("EntityID");
		ImGui::NextColumn();
		ImGui::PushItemWidth(50);

		ImGui::InputText(GenerateID(), (char*)std::to_string(entity).c_str(), 256, ImGuiInputTextFlags_ReadOnly);

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();

		ImGui::SameLine();

		auto region = ImGui::GetContentRegionAvail();
			
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component",ImVec2(region.x,20)))
			ImGui::OpenPopup("AddComponent");

		if (!IsItemDisabled())
			DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		ImGui::PopItemWidth();
		ImGui::NextColumn();


		if (ImGui::BeginPopup("AddComponent"))
		{
			//if (ImGui::MenuItem("Camera"))
			
			ImGui::EndPopup();
		}

	
		if (name)
		{
			std::string inputName = name->name;
			if (DrawInputText("Name", inputName))
				name->name = inputName;
			
		}

		EndPropertyGrid();




		if (transform)
		{
			if (hier)
			{
				DrawComponent("Hierarchy", hier, [](HierarchyComponent& hier)
					{
						BeginPropertyGrid();
						PropertyGridSpacing();


						DrawInputText("ParentID", hier.parentID ? std::to_string(hier.parentID).c_str() : "INVALID");
						DrawInputText("Children Count", std::to_string(hier.childrenID.size()).c_str());
						
						EndPropertyGrid();

					});


			}




			DrawComponent("Transform", transform, [](TransformComponent& transform)
				{

					PropertyGridSpacing();
					DrawVec3Control("Translation", transform.translation_local);
					XMFLOAT3 euler = ap::math::QuaternionToRollPitchYaw(transform.rotation_local);

					euler.x = XMConvertToDegrees(euler.x);
					euler.y = XMConvertToDegrees(euler.y);
					euler.z = XMConvertToDegrees(euler.z);

					DrawVec3Control("Rotation", euler);
					XMStoreFloat4(&transform.rotation_local, XMQuaternionRotationRollPitchYaw(XMConvertToRadians(euler.x), XMConvertToRadians(euler.y), XMConvertToRadians(euler.z)));
					DrawVec3Control("Scale", transform.scale_local, 1.0f, true);

					PropertyGridSpacing();
					transform.SetDirty();



				});
		}


		if (camera)
		{
			DrawComponent("Camera", camera, [](CameraComponent& camera) 
				{


					BeginPropertyGrid();
					PropertyGridSpacing();

					
					if (DrawButton2("Reset", true))
					{
						camera.zNearP = 0.1f;
						camera.zFarP = 800.0f;
						camera.fov = XM_PI / 3.0f;
						camera.focal_length = 1;
						camera.aperture_size = 0;
						camera.aperture_shape = XMFLOAT2(1, 1);
					}


					ImGui::Separator();
					PropertyGridSpacing();

					DrawSliderFloat("Far Plane", camera.zFarP, 300.0f, 2000.0f);
					DrawSliderFloat("Near Plane", camera.zNearP, 0.01f, 10.0f);
					DrawSliderFloat("Fov", camera.fov, 0.1f, 2.0f);
					DrawSliderFloat("Focal Length", camera.focal_length, 0.01f, 100.0f);
					DrawSliderFloat("Aperture Size", camera.aperture_size, 0.0f, 1.0f);
					DrawSliderFloat("Aperture Shape X", camera.aperture_shape.x, 0.0f, 2.0f);
					DrawSliderFloat("Aperture Shape Y", camera.aperture_shape.y, 0.0f, 2.0f);

					
					EndPropertyGrid();
				; 
				});


			DrawComponent("Weather", weather, [](WeatherComponent& weather)
				{
					BeginPropertyGrid();
					PropertyGridSpacing();

					

					{

						static int selectedIdx = 0;
						const std::vector<std::string> items =
						{
							"Default",
							"Daytime",
							"Sunset",
							"Cloudy",
							"Night"

						};
						if (DrawCombo("WeatherPreset", items, items.size(), &selectedIdx))
						{

							const float  fogStart = 0.10f;
							const float  fogEnd = 3000.0f;

							switch (selectedIdx)
							{
							case 0:
							{
								// Default
								weather = WeatherComponent();
								break;
							}
							case 1:
							{
								//Daytime
								weather.ambient = XMFLOAT3(33.0f / 255.0f, 47.0f / 255.0f, 127.0f / 255.0f);
								weather.horizon = XMFLOAT3(101.0f / 255.0f, 101.0f / 255.0f, 227.0f / 255.0f);
								weather.zenith = XMFLOAT3(99.0f / 255.0f, 133.0f / 255.0f, 255.0f / 255.0f);
								weather.cloudiness = 0.4f;
								weather.fogStart = 100;
								weather.fogEnd = 1000;
								weather.fogHeightSky = 0;
								break;
							}
							case 2:
							{
								//Sunset
								weather.ambient = XMFLOAT3(86.0f / 255.0f, 29.0f / 255.0f, 29.0f / 255.0f);
								weather.horizon = XMFLOAT3(121.0f / 255.0f, 28.0f / 255.0f, 22.0f / 255.0f);
								weather.zenith = XMFLOAT3(146.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f);
								weather.cloudiness = 0.36f;
								weather.fogStart = 50;
								weather.fogEnd = 600;
								weather.fogHeightSky = 0;
								break;
							}
							case 3:
							{
								//Cloudy
								weather.ambient = XMFLOAT3(0.1f, 0.1f, 0.1f);
								weather.horizon = XMFLOAT3(0.38f, 0.38f, 0.38f);
								weather.zenith = XMFLOAT3(0.42f, 0.42f, 0.42f);
								weather.cloudiness = 0.75f;
								weather.fogStart = 0;
								weather.fogEnd = 500;
								weather.fogHeightSky = 0;
								break;
							}
							case 4:
							{
								//Night
								weather.ambient = XMFLOAT3(12.0f / 255.0f, 21.0f / 255.0f, 77.0f / 255.0f);
								weather.horizon = XMFLOAT3(10.0f / 255.0f, 33.0f / 255.0f, 70.0f / 255.0f);
								weather.zenith = XMFLOAT3(4.0f / 255.0f, 20.0f / 255.0f, 51.0f / 255.0f);
								weather.cloudiness = 0.28f;
								weather.fogStart = 10;
								weather.fogEnd = 400;
								weather.fogHeightSky = 0;
								break;
							}
							default:
								break;
							}

							InvalidateProbes();

						}

					}
					
					PropertyGridSpacing();
					ImGui::Separator();
					PropertyGridSpacing();
					PropertyGridSpacing();

					DrawColorEdit3("Ambient Color", weather.ambient);
					DrawColorEdit3("Horizon Color", weather.horizon);
					DrawColorEdit3("Zenith Color", weather.zenith);
					
					


					PropertyGridSpacing();
					ImGui::Separator();
					PropertyGridSpacing();
					PropertyGridSpacing();

					//weather

					bool IsHeightFog = weather.IsHeightFog();
					if (DrawCheckbox("Height Fog", IsHeightFog))
					{
						weather.SetHeightFog(IsHeightFog);
					}

					bool IsSimpleSky = weather.IsSimpleSky();
					if (DrawCheckbox("Simple Sky", IsSimpleSky))
					{
						weather.SetSimpleSky(IsSimpleSky);
					}
					
					bool IsRealisticSky = weather.IsRealisticSky();
					if (DrawCheckbox("Realistic Sky", IsRealisticSky))
					{
						weather.SetRealisticSky(IsRealisticSky);
					}


					static float windDirection =0.0f;
					static float windMagnitude = 0.0f;
					


					DrawSliderFloat("Fog Start", weather.fogStart,0,5000);
					DrawSliderFloat("Fog End", weather.fogEnd, 1, 5000);
					DrawSliderFloat("Fog Height Start", weather.fogHeightStart,-100 ,100 );
					DrawSliderFloat("Fog Height End", weather.fogHeightEnd, -100,100 );
					DrawSliderFloat("Fog Height Sky", weather.fogHeightSky,0 , 1);
					DrawSliderFloat("Cloudiness", weather.cloudiness, 0, 1);
					DrawSliderFloat("Cloud Scale", weather.cloudScale, 0.00005f, 0.001f, "%.5f");
					DrawSliderFloat("Cloud Speed", weather.cloudSpeed, 0.001f, 0.2f);
					DrawSliderFloat("Wind Speed", weather.windSpeed,0 , 4);
					if (DrawSliderFloat("Wind Magnitude", windMagnitude, 0, 0.2f))
					{
						XMMATRIX rot = XMMatrixRotationY(windDirection * XM_PI * 2);
						XMVECTOR dir = XMVectorSet(1, 0, 0, 0);
						dir = XMVector3TransformNormal(dir, rot);
						dir *= windMagnitude;
						XMStoreFloat3(&weather.windDirection, dir);
					}
					if (DrawSliderFloat("Wind Direction", windDirection,0 , 1))
					{
						XMMATRIX rot = XMMatrixRotationY(windDirection * XM_PI * 2);
						XMVECTOR dir = XMVectorSet(1, 0, 0, 0);
						dir = XMVector3TransformNormal(dir, rot);
						dir *= windMagnitude;
						XMStoreFloat3(&weather.windDirection, dir);
					}
					DrawSliderFloat("Wind Wave Size", weather.windWaveSize, 0, 1);
					DrawSliderFloat("Wind Randomness", weather.windRandomness,0 , 10);
					DrawSliderFloat("Sky Exposure", weather.skyExposure, 0,4 );



					PropertyGridSpacing();
					ImGui::Separator();
					PropertyGridSpacing();
					PropertyGridSpacing();

					const char* skyLabel =  "Load SkyBox";

			
					if (DrawButton2(skyLabel,true))
					{
						if (!weather.skyMap.IsValid())
						{
							ap::helper::FileDialogParams params;
							params.type = ap::helper::FileDialogParams::OPEN;
							params.description = "Cubemap texture";
							params.extensions.push_back("dds");
							ap::helper::FileDialog(params, [=](std::string fileName) {
								ap::eventhandler::Subscribe_Once(ap::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
									auto& weather = GetScene().weathers[0];
									weather.skyMapName = fileName;
									weather.skyMap = ap::resourcemanager::Load(fileName, ap::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
									
									});
								});
						}
						else
						{
							weather.skyMap = {};
							weather.skyMapName.clear();
			
						}
					}
					

					const char* lutLabel =  "Load Color Grading LUT" ;

					
					if (DrawButton2(lutLabel,true))
					{
						if (!weather.colorGradingMap.IsValid())
						{
							ap::helper::FileDialogParams params;
							params.type = ap::helper::FileDialogParams::OPEN;
							params.description = "Texture";
							params.extensions = ap::resourcemanager::GetSupportedImageExtensions();
							ap::helper::FileDialog(params, [=](std::string fileName) {
								ap::eventhandler::Subscribe_Once(ap::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
									auto& weather = GetScene().weathers[0];
									weather.colorGradingMapName = fileName;
									weather.colorGradingMap = ap::resourcemanager::Load(fileName, ap::resourcemanager::Flags::IMPORT_COLORGRADINGLUT | ap::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
									
									});
								});
						}
						else
						{
							weather.colorGradingMap = {};
							weather.colorGradingMapName.clear();
						
						}
					}
					


					PropertyGridSpacing();
					ImGui::Separator();
					PropertyGridSpacing();
					PropertyGridSpacing();


					//v cloud
					bool IsVolumetricClouds = weather.IsVolumetricClouds();
					if (DrawCheckbox("Volumetric Cloud", IsVolumetricClouds))
					{
						weather.SetVolumetricClouds(IsVolumetricClouds);
					}
					DrawColorEdit3("V.Cloud Color", weather.volumetricCloudParameters.Albedo);
					DrawSliderFloat("Coverage Amount", weather.volumetricCloudParameters.CoverageAmount, 0.0f, 10.0f);
					DrawSliderFloat("Coverage Minimmum", weather.volumetricCloudParameters.CoverageMinimum, 1.0f, 2.0f);

					PropertyGridSpacing();
					ImGui::Separator();
					PropertyGridSpacing();


					ap::Ocean2& ocean2 = *GetScene().ocean2.get();

					//ocean
					bool IsOceanEnabled = weather.IsOceanEnabled();
					if (DrawCheckbox("Ocean Simulation Enable", IsOceanEnabled))
					{
						weather.SetOceanEnabled(IsOceanEnabled);
						if (!weather.IsOceanEnabled())
						{
							GetScene().ocean = {};
						}
					}

	

					if (DrawSliderFloat("UV Amplitude", weather.ocean2Parameters.OceanWindSimulationParameters.uv_warping_amplitude, 0.0f, 0.1f, "%.2f"))
						ocean2.bNeedToUpdateWindWavesSimulationProperties = true;
					if (DrawSliderFloat("UV Frequency", weather.ocean2Parameters.OceanWindSimulationParameters.uv_warping_frequency, 1.0f, 3.0f))
						ocean2.bNeedToUpdateWindWavesSimulationProperties = true;
					






					PropertyGridSpacing();
					ImGui::Separator();
					PropertyGridSpacing();
					PropertyGridSpacing();


					EndPropertyGrid();
					
				});

		}

		
		
		if (light)
		{

			DrawComponent("Light", light, [](LightComponent& light)
				{

					BeginPropertyGrid();
					PropertyGridSpacing();
					bool isCastingShadow = light.IsCastingShadow();
					if (DrawCheckbox("Cast Shadow", isCastingShadow))
					{
						light.SetCastShadow(isCastingShadow);
					}

					bool isVolumetricsEnabled = light.IsVolumetricsEnabled();
					if (DrawCheckbox("Volumetric", isVolumetricsEnabled))
					{
						light.SetVolumetricsEnabled(isVolumetricsEnabled);
					}
					bool VisualizerEnabled = light.IsVisualizerEnabled();
					if (DrawCheckbox("Visualizer", VisualizerEnabled))
					{
						light.SetVisualizerEnabled(VisualizerEnabled);
					}

					ImGui::Separator();
					PropertyGridSpacing();

					DrawSliderFloat("Range", light.range_local, 0.0f, 1000.0f);
					DrawColorEdit3("Color", light.color);

					DrawSliderFloat("Energy", light.energy, 0.0f, 64.0f);
					DrawSliderFloat("ConeAngleCosine", light.fov, 0.0f, 3.13f);


					PropertyGridSpacing();
					ImGui::Separator();
					PropertyGridSpacing();

					const int maxLensFlare = 7;

					const std::string lensBasePath = "resources/images/flare";
					
					if (DrawButton2("Set LensFlares", true))
					{
						for (int i = 0; i < maxLensFlare-1; i++)
						{
							std::string lensPath = (lensBasePath + std::to_string(i+1) + ".jpg").c_str();
							light.lensFlareNames[i] = lensPath;
							light.lensFlareRimTextures[i] = ap::resourcemanager::Load(lensPath, ap::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
						}
					}

					if (DrawButton2("Reset LensFlares", true))
					{
						for (int i = 0; i < maxLensFlare; i++)
						{

							light.lensFlareNames[i] = "";
							light.lensFlareRimTextures[i] = {};
						}
					}

					PropertyGridSpacing();
					ImGui::Separator();
					PropertyGridSpacing();
		

					for (int i = 0; i < maxLensFlare; i++)
					{

						std::string labelName = "LensFlare_" + std::to_string(i);
						ImGui::Text(labelName.c_str());
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);

		
						ap::graphics::Texture* texture = nullptr;
						if (light.lensFlareRimTextures[i].IsValid())
							texture = const_cast<ap::graphics::Texture*>(&light.lensFlareRimTextures[i].GetTexture());

						bool textureIsValid = texture;
						if (!textureIsValid)
						{
							texture = const_cast<ap::graphics::Texture*>(ap::texturehelper::getWhite());
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
									light.lensFlareRimTextures[i] = {};

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


									ap::helper::FileDialog(params, [&light,i](std::string fileName) {
										ap::eventhandler::Subscribe_Once(ap::eventhandler::EVENT_THREAD_SAFE_POINT, [fileName,&light,i](uint64_t userdata) {
											light.lensFlareNames[i] = fileName;
											light.lensFlareRimTextures[i] = ap::resourcemanager::Load(fileName, ap::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
											});
										});
								}

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
									
									light.lensFlareRimTextures[i] = ap::resourcemanager::Load(assetPath.string().c_str(), ap::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
									light.lensFlareNames[i] = assetPath.string();
	
								}

							}
							ImGui::EndDragDropTarget();
						}



						if (!IsItemDisabled())
							DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

						ImGui::PopItemWidth();
						ImGui::NextColumn();


						
					}


						

					


					EndPropertyGrid();


				});

		}



		if (environmentprobe)
		{
			DrawComponent("EnvProbe", environmentprobe, [](EnvironmentProbeComponent& probe)
				{

					BeginPropertyGrid();
					PropertyGridSpacing();
					bool isRealTime = probe.IsRealTime();
					if (DrawCheckbox("Real Time", isRealTime))
					{
						probe.SetRealTime(isRealTime);
					}

					if (DrawButton2("Refresh", true))
					{
						probe.SetDirty(true);
					}

					if (DrawButton2("Refresh ALL", true))
					{
						InvalidateProbes();
					}

					//DrawInputText("CubeMapIndex", std::to_string(probe.textureIndex));


					EndPropertyGrid();


				});
		}



		if (emitter)
		{
			DrawComponent("Emitter", emitter, [this](ap::EmittedParticleSystem& particle)
				{

					
					BeginPropertyGrid();
					PropertyGridSpacing();

					int selectedIdx = (int)particle.shaderType;

					const std::vector<std::string> items =
					{
						"SOFT",
						"SOFT_DISTORTION",
						"SIMPLE",
						"SOFT_LIGHTING"

					};
					DrawCombo("Shader Type", items, items.size(), &selectedIdx);
					particle.shaderType = (ap::EmittedParticleSystem::PARTICLESHADERTYPE)selectedIdx;

					if (DrawButton2("Restart", true))
						particle.Restart();

					//std::string meshName = particle.meshUUID ? scene->GetEntity(particle.meshUUID).GetComponent<NameComponent>().name : "NULL";
					//DrawInputText("MeshName", meshName.c_str());

					std::string memoryBudget = std::to_string(particle.GetMemorySizeInBytes());
					DrawInputText("Memory Budget", memoryBudget);

					auto data = particle.GetStatistics();
					DrawInputText("Alive Particle", std::to_string(data.aliveCount));
					DrawInputText("Dead Particle", std::to_string(data.deadCount));


					ImGui::Separator();
					PropertyGridSpacing();

					bool IsSorted = particle.IsSorted();
					if (DrawCheckbox("Sorting Enabled", IsSorted))
						particle.SetSorted(IsSorted);

					bool IsDepthCollisionEnabled = particle.IsDepthCollisionEnabled();
					if (DrawCheckbox("ZBuffer Collision Enabled", IsDepthCollisionEnabled))
						particle.SetDepthCollisionEnabled(IsDepthCollisionEnabled);

					bool IsSPHEnabled = particle.IsSPHEnabled();
					if (DrawCheckbox("SPH", IsSPHEnabled))
						particle.SetSPHEnabled(IsSPHEnabled);

					bool IsPaused = particle.IsPaused();
					if (DrawCheckbox("PAUSE", IsPaused))
						particle.SetPaused(IsPaused);

					bool volumeEnabled = particle.IsVolumeEnabled();
					if (DrawCheckbox("Volume", volumeEnabled))
						particle.SetVolumeEnabled(volumeEnabled);

					bool IsFrameBlendingEnabled = particle.IsFrameBlendingEnabled();
					if (DrawCheckbox("Frame Blending", IsFrameBlendingEnabled))
						particle.SetFrameBlendingEnabled(IsFrameBlendingEnabled);


					ImGui::Separator();
					PropertyGridSpacing();

					static uint32_t burstCount = 0;
					DrawSliderInt("Brust Count", burstCount, 0, 1000);

					if (DrawButton2("Burst", true))
						particle.Burst(burstCount);


					ImGui::Separator();
					PropertyGridSpacing();

					DrawSliderFloat("Frame Rate", particle.frameRate, 0.0f, 60.0f);
					DrawSliderInt("Frames X", particle.framesX, 1, 10);
					DrawSliderInt("Frames Y", particle.framesY, 1, 10);
					DrawSliderInt("Frame Count", particle.frameCount, 1, 100);
					DrawSliderInt("Frame Start", particle.frameStart, 0, 100);

					DrawDragFloat3("Velocity", particle.velocity);
					DrawDragFloat3("Gravity", particle.gravity);

					ImGui::Separator();
					PropertyGridSpacing();

					int maxParticle = particle.GetMaxParticleCount();
					if (DrawSliderInt("Max Particle", maxParticle, 1, 100000))
					{
						particle.SetMaxParticleCount(maxParticle);
						particle.Restart();

					};
					DrawSliderFloat("Count", particle.count, 0.0f, 10000.0f);
					DrawSliderFloat("Size", particle.size, 0.01f, 10.0f);
					DrawSliderFloat("Rotation", particle.rotation, 0.0f, 1.0f);
					DrawSliderFloat("Normal Factor", particle.normal_factor, 0.0f, 100.0f);
					DrawSliderFloat("ScaleX", particle.scaleX, 0.01f, 100.0f);
					//DrawSliderFloat("ScaleY", particle.scaleY, 0.01f, 10.0f);
					DrawSliderFloat("Life", particle.life, 0.0f, 100.0f);
					DrawSliderFloat("Random Factor", particle.random_factor, 0.0f, 1.0f);
					DrawSliderFloat("Random Life", particle.random_life, 0.0f, 2.0f);
					DrawSliderFloat("Random Color", particle.random_color, 0.0f, 2.0f);
					DrawSliderFloat("MotionBlur", particle.motionBlurAmount, 0.0f, 1.0f);
					DrawSliderFloat("Mass", particle.mass, 0.1f, 100.0f);
					//DrawSliderFloat("TimeStep", particle.FIXED_TIMESTEP, -1.f, 0.16f);
					DrawSliderFloat("Drag", particle.drag, -1.2f, 1.2f);


					PropertyGridSpacing();
					EndPropertyGrid();

				});


		}

		
		if (sound)
		{
			DrawComponent("Sound", sound, [](SoundComponent& sound)
				{
					BeginPropertyGrid();
					PropertyGridSpacing();


					{
						static int selectedItem = 0;
						const std::vector<std::string> items =
						{
							"DEFAULT",
							"GENERIC",
							"FOREST",
							"PADDEDCELL",
							"ROOM",
							"BATHROOM",
							"LIVINGROOM",
							"STONEROOM",
							"AUDITORIUM",
							"CONCERTHALL",
							"CAVE",
							"ARENA",
							"HANGAR",
							"CARPETEDHALLWAY",
							"HALLWAY",
							"STONECORRIDOR",
							"ALLEY",
							"CITY",
							"MOUNTAINS",
							"QUARRY",
							"PLAIN",
							"PARKINGLOT",
							"SEWERPIPE",
							"UNDERWATER",
							"SMALLROOM",
							"MEDIUMROOM",
							"LARGEROOM",
							"MEDIUMHALL",
							"LARGEHALL",
							"PLATE",


						};
						if (DrawCombo("Global Reverb", items, items.size(), &selectedItem))
							ap::audio::SetReverb((ap::audio::REVERB_PRESET)selectedItem);
					}

					{
						int selectedItem = (ap::audio::SUBMIX_TYPE)sound.soundinstance.type;
						const std::vector<std::string> items =
						{
							"SOUNDEFFECT",
							"MUSIC",
							"USER0",
							"USER1",

						};
						if (DrawCombo("Submix", items, items.size(), &selectedItem))
						{
							sound.soundinstance.type = (ap::audio::SUBMIX_TYPE)selectedItem;
							ap::audio::CreateSoundInstance(&sound.soundResource.GetSound(), &sound.soundinstance);
						}
					}

					const char* buttonLabel = sound.IsPlaying() ?  "Stop": "Play";


					if (DrawButton2(buttonLabel,true))
					{
						if (sound.IsPlaying())
							sound.Stop();
						else
							sound.Play();
						
					}

					bool IsLooped = sound.IsLooped();
					if (DrawCheckbox("Looped", IsLooped))
					{
						sound.SetLooped(IsLooped);
					}

					bool IsEnableReverb = sound.soundinstance.IsEnableReverb();
					if (DrawCheckbox("Reverb", IsEnableReverb))
					{
						sound.soundinstance.SetEnableReverb(IsEnableReverb);
						ap::audio::CreateSoundInstance(&sound.soundResource.GetSound(), &sound.soundinstance);
					}

					bool IsDisable3D = sound.IsDisable3D();
					if (DrawCheckbox("2D", IsDisable3D))
					{
						sound.SetDisable3D(IsDisable3D);
						ap::audio::CreateSoundInstance(&sound.soundResource.GetSound(), &sound.soundinstance);
					}


					DrawSliderFloat("Volume", sound.volume, 0.0f, 1.0f);

					

					EndPropertyGrid();
				});
		}


		if (object)
		{
			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			

			


			if (mesh)
			{
				RigidBodyPhysicsComponent* rigidbody = scene.rigidbodies.GetComponent(object->meshID);
				SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object->meshID);

				assert(mesh->subsets.size() > 0);
				if (subsetIdx == -1)
					subsetIdx = 0;


				if (mesh->armatureID != ap::ecs::INVALID_ENTITY)
				{
					DrawComponent("Animation", mesh, [&scene](MeshComponent& mesh)
						{
							BeginPropertyGrid();
							PropertyGridSpacing();

							
							static int selectedIdx = 0;

							if (scene.animations.GetCount() < selectedIdx)
								selectedIdx = 0;
							

							std::vector<std::string> items = {"NONE"};
							items.reserve(scene.animations.GetCount());

							for (int i = 0; i < scene.animations.GetCount(); i++)
							{
								Entity ent = scene.animations.GetEntity(i);
								
								NameComponent* name = scene.names.GetComponent(ent);
								items.push_back(name->name);
							}
							DrawCombo("Anim", items, items.size(), &selectedIdx);
							
							if (selectedIdx != 0)
							{

								AnimationComponent& anim = scene.animations[selectedIdx-1];

								
								bool IsLooped = anim.IsLooped();
								if (DrawCheckbox("Looped", IsLooped))
									anim.SetLooped(IsLooped);

								
								const char* buttonLabel = anim.IsPlaying() ? "Pause" : "Play";

								if (DrawButton2(buttonLabel, true))
								{
									if (anim.IsPlaying())
										anim.Pause();
									else
										anim.Play();
								}

								

								if (DrawButton2("Stop", true))
								{
									anim.Stop();
								}

								DrawSliderFloat("Timer", anim.timer, 0.0f, 30.0f);
								DrawSliderFloat("Amount", anim.amount, 0.0f, 1.0f);
								DrawSliderFloat("Speed", anim.speed, 0.0f, 4.0f);


							}

							


							PropertyGridSpacing();
							ImGui::Separator();
							PropertyGridSpacing();



							EndPropertyGrid();

						});

				}



				static float mass = 10;
				static float friction = 1;
				static float restitution = 1;


				DrawComponent("Mesh", mesh, [&subsetIdx,entity, object,name, &softbody, &scene](MeshComponent& mesh)
					{
						BeginPropertyGrid();
						PropertyGridSpacing();


						ImGui::Text("Mesh Data");
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);

						std::string MeshDatastr = "";

						MeshDatastr += "Mesh Name: " + name->name + "\n";
						MeshDatastr += "Vertex count: " + std::to_string(mesh.vertex_positions.size()) + "\n";
						MeshDatastr += "Index count: " + std::to_string(mesh.indices.size()) + "\n";
						MeshDatastr += "Subset count: " + std::to_string(mesh.subsets.size()) + "\n";
						if (mesh.vertexBuffer_POS.IsValid()) MeshDatastr += "position; ";
						if (mesh.vertexBuffer_UV0.IsValid()) MeshDatastr += "uvset_0; ";
						if (mesh.vertexBuffer_UV1.IsValid()) MeshDatastr += "uvset_1; ";
						if (mesh.vertexBuffer_ATL.IsValid()) MeshDatastr += "atlas; ";
						if (mesh.vertexBuffer_COL.IsValid()) MeshDatastr += "color; ";
						if (mesh.vertexBuffer_PRE.IsValid()) MeshDatastr += "previous_position; ";
						if (mesh.vertexBuffer_BON.IsValid()) MeshDatastr += "bone; ";
						if (mesh.vertexBuffer_TAN.IsValid()) MeshDatastr += "tangent; ";
						if (mesh.streamoutBuffer_POS.IsValid()) MeshDatastr += "streamout_position; ";
						if (mesh.streamoutBuffer_TAN.IsValid()) MeshDatastr += "streamout_tangents; ";
						if (mesh.subsetBuffer.IsValid()) MeshDatastr += "subset; ";

						ImGui::InputTextMultiline(GenerateID(), (char*)MeshDatastr.c_str(), MeshDatastr.size(),ImVec2(0,0) ,ImGuiInputTextFlags_ReadOnly);
						
						ImGui::PopItemWidth();
						ImGui::NextColumn();


						if (softbody != nullptr)
						{
							friction = softbody->friction;
							mass = softbody->mass;
							restitution = softbody->restitution;

						}


						PropertyGridSpacing();

						{
							std::vector<std::string> items;
							for (int i = 0; i < mesh.subsets.size(); i++)
							{
								items.push_back(std::to_string(i));
							}
							DrawCombo("Subset", items, items.size(), &subsetIdx);
						}

						{
							int selected = scene.materials.GetIndex(mesh.subsets[subsetIdx].materialID) + 1;

							std::vector<std::string> items = {"No Material"};
							items.reserve(scene.materials.GetCount()+1);

							for (int i = 0; i < scene.materials.GetCount(); i++)
							{
								Entity ent = scene.materials.GetEntity(i);
								NameComponent* name = scene.names.GetComponent(ent);
								items.push_back(name->name);

							}

							if (DrawCombo("Subset Material", items, items.size(), &selected))
							{
								if (selected == 0)
								{
									mesh.subsets[subsetIdx].materialID = INVALID_ENTITY;
								}
								else
								{
									mesh.subsets[subsetIdx].materialID = scene.materials.GetEntity(selected-1);
								}
							}


						}


						bool IsDoubleSided = mesh.IsDoubleSided();
						if (DrawCheckbox("Double Sided", IsDoubleSided))
							mesh.SetDoubleSided(IsDoubleSided);


						PropertyGridSpacing();
						ImGui::Separator();
						PropertyGridSpacing();

						bool isSoftBody = (softbody != nullptr);
						if (DrawCheckbox("Soft body", isSoftBody))
						{

							Scene& scene = ap::scene::GetScene();
							
							if (isSoftBody)
							{
								if (softbody == nullptr)
								{
									softbody = &scene.softbodies.Create(object->meshID);
									softbody->friction = friction;
									softbody->restitution = restitution;
									softbody->mass = mass;
								}
							}
							else
							{
								if (softbody != nullptr)
								{
									scene.softbodies.Remove(object->meshID);
									softbody = nullptr;
								}
							}
						}

						DrawSliderFloat("Mass", mass, 0.0f, 10.0f);
						DrawSliderFloat("Friction", friction, 0.0f, 1.0f);
						DrawSliderFloat("Restitution", restitution, 0.0f, 1.0f);


						PropertyGridSpacing();
						ImGui::Separator();
						PropertyGridSpacing();


						// Morph Target

						static float morphWeight = 0.0f;
						static int selectedMorphIdx = 0;

						if (selectedMorphIdx > mesh.targets.size())
							selectedMorphIdx = 0;

						{
							std::vector<std::string> items = {"NONE"};
							for (int i = 0; i < mesh.targets.size(); i++)
							{
								items.push_back(std::to_string(i));
							}
							DrawCombo("Morph Target", items, items.size(), &selectedMorphIdx);
							

						}

						if(selectedMorphIdx != 0 )
						{
							morphWeight = mesh.targets[selectedMorphIdx-1].weight;
						}
						else
						{
							morphWeight = 0.0f;
						}

						
						if (DrawSliderFloat("Morph Target Weight", morphWeight, 0.0f, 1.0f))
						{
							if (selectedMorphIdx != 0)
							{
								mesh.targets[selectedMorphIdx - 1].weight = morphWeight;
								mesh.dirty_morph = true;
							}
						}


						PropertyGridSpacing();
						ImGui::Separator();
						PropertyGridSpacing();



						EndPropertyGrid();

					});

				

			


				MaterialComponent*  material = scene.materials.GetComponent(mesh->subsets[subsetIdx].materialID);
				



				if (material)
				{
					DrawComponent("Material", material, [=](MaterialComponent& material)
						{


							BeginPropertyGrid();
							PropertyGridSpacing();


							{
								bool isCastingShadow = material.IsCastingShadow();
								if (DrawCheckbox("Cast shadow", isCastingShadow))
									material.SetCastShadow(isCastingShadow);


								bool IsReceiveShadow = material.IsReceiveShadow();
								if (DrawCheckbox("Receive shadow", IsReceiveShadow))
									material.SetReceiveShadow(IsReceiveShadow);

								bool IsUsingVertexColors = material.IsUsingVertexColors();
								if (DrawCheckbox("Use vertex colors", IsUsingVertexColors))
									material.SetUseVertexColors(IsUsingVertexColors);

								bool IsUsingSpecularGlossinessWorkflow = material.IsUsingSpecularGlossinessWorkflow();
								if (DrawCheckbox("Use Specular-Glossiness", IsUsingSpecularGlossinessWorkflow))
									material.SetUseSpecularGlossinessWorkflow(IsUsingSpecularGlossinessWorkflow);

								bool IsOcclusionEnabled_Primary = material.IsOcclusionEnabled_Primary();
								if (DrawCheckbox("Occlusion-Primary", IsOcclusionEnabled_Primary))
									material.SetOcclusionEnabled_Primary(IsOcclusionEnabled_Primary);

								bool IsOcclusionEnabled_Secondary = material.IsOcclusionEnabled_Secondary();
								if (DrawCheckbox("Occlusion-Secondary", IsOcclusionEnabled_Secondary))
									material.SetOcclusionEnabled_Secondary(IsOcclusionEnabled_Secondary);

								bool IsUsingWind = material.IsUsingWind();
								if (DrawCheckbox("Use wind", IsUsingWind))
									material.SetUseWind(IsUsingWind);

								bool IsDoubleSided = material.IsDoubleSided();
								if (DrawCheckbox("Double sided", IsDoubleSided))
									material.SetDoubleSided(IsDoubleSided);

							}

							PropertyGridSpacing();
							ImGui::Separator();
							{
								int selectedItem = material.userBlendMode;
								const std::vector<std::string> items =
								{
									"OPAQUE",
									"ALPHA",
									"PREMULTIPLIED",
									"ADDITIVE",
									"MULTIPLY"
								};
								if (DrawCombo("Blend Mode", items, items.size(), &selectedItem))
									material.userBlendMode = (ap::enums::BLENDMODE)selectedItem;
							}

							{
								int selectedItem = material.shaderType;
								std::vector<std::string> items =
								{
									"SHADERTYPE_PBR"						  ,
									"SHADERTYPE_PBR_PLANARREFLECTION"		  ,
									"SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING" ,
									"SHADERTYPE_PBR_ANISOTROPIC"			  ,
									"SHADERTYPE_WATER"						  ,
									"SHADERTYPE_CARTOON"					  ,
									"SHADERTYPE_UNLIT"						  ,
									"SHADERTYPE_PBR_CLOTH"					  ,
									"SHADERTYPE_PBR_CLEARCOAT"				  ,
									"SHADERTYPE_PBR_CLOTH_CLEARCOAT"		  ,
									"SHADERTYPE_RAIN"		                  ,

								};

								for (auto& x : ap::renderer::GetCustomShaders())
								{
									items.push_back(("Custom_" + x.name));
								}

								if (DrawCombo("Shader Type", items, items.size(), &selectedItem))
								{
									material.shaderType = (MaterialComponent::SHADERTYPE)selectedItem;

									if (selectedItem >= MaterialComponent::SHADERTYPE_COUNT)
									{
										material.SetCustomShaderID(selectedItem - MaterialComponent::SHADERTYPE_COUNT);
									}
									else
									{
										material.DisableCustomShader();
									}

									if (selectedItem == 4) //default water
									{
										material.baseColor = { 10.0f / 255.f, 63.f / 255.f, 168.f / 255.f, 5.0f / 255.f };
										material.specularColor = { 1.0f,1.0f,1.0f,1.0f };
										material.emissiveColor = { 1.0f,1.0f,1.0f,0.0f };
										material.roughness = 0.2f;
										material.reflectance = 0.10f;
										material.metalness = 0.0f;
										material.refraction = 0.027f;

										material.transmission = 0.0f;
										material.normalMapStrength = 1.0f;

										material.texAnimFrameRate = 60.f;
										material.texAnimDirection = { 0.0004, -0.0004 };
										material.texMulAdd = { 1.0f,1.0f,0.0f, 0.0f };

									}

								}


							}

							{
								int selectedItem = (int)material.shadingRate;
								const std::vector<std::string> items =
								{
									"1X1",
									"1X2",
									"2X1",
									"2X2",
									"2X4",
									"4X2",
									"4X4",
								};
								if (DrawCombo("Shading Rate", items, items.size(), &selectedItem))
									material.shadingRate = (ap::graphics::ShadingRate)selectedItem;
							}

							PropertyGridSpacing();
							ImGui::Separator();

							PropertyGridSpacing();
							DrawColorEdit4("Base Color", material.baseColor);
							DrawColorEdit4("Specular Color", material.specularColor);
							DrawColorEdit3("Emissive Color", (float*)&material.emissiveColor);
							DrawSliderFloat("EmissiveAlpha", material.emissiveColor.w, 0.0f, 10.0f);
							PropertyGridSpacing();
							ImGui::Separator();

							PropertyGridSpacing();
							DrawSliderFloat("Roughness", material.roughness, 0.0f, 1.0f);
							DrawSliderFloat("Reflectance", material.reflectance, 0.0f, 1.0f);
							DrawSliderFloat("Matalness", material.metalness, 0.0f, 1.0f);
							DrawSliderFloat("Refraction", material.refraction, 0.0f, 1.0f);
							PropertyGridSpacing();

							ImGui::Separator();
							PropertyGridSpacing();
							DrawSliderFloat("Transmission", material.transmission, 0.0f, 1.0f);
							DrawSliderFloat("Normal", material.normalMapStrength, 0.0f, 8.0f);
							DrawSliderFloat("Parallax Occlusion", material.parallaxOcclusionMapping, 0.0f,0.1f, "%.5f");
							DrawSliderFloat("Displacement", material.displacementMapping, 0.0f, 0.1f, "%.5f");
							DrawSliderFloat("AlphaRef", material.alphaRef, 0.0f, 1.0f - 1.0f / 256.0f);

							ImGui::Separator();
							PropertyGridSpacing();
							

							DrawColorEdit3("Subsurface Scattering Color", (float*)&material.subsurfaceScattering);
							DrawSliderFloat("Subsurface Scattering", material.subsurfaceScattering.w, 0.0f, 2);

							ImGui::Separator();
							PropertyGridSpacing();

							DrawColorEdit4("Sheen Color", material.sheenColor);
							DrawSliderFloat("Sheen Roughness", material.sheenRoughness, 0.0f, 1.0f );

							ImGui::Separator();
							PropertyGridSpacing();
							
							DrawSliderFloat("ClearCoat", material.clearcoat, 0.0f, 1.0f);
							DrawSliderFloat("ClearCoat Roughness", material.clearcoatRoughness, 0.0f, 1.f);


							PropertyGridSpacing();

							ImGui::Separator();
							PropertyGridSpacing();
							DrawSliderFloat("Texcoord anim FPS", material.texAnimFrameRate, 0, 60);
							DrawSliderFloat("Texcoord anim U", material.texAnimDirection.x, -0.02f, 0.02f, "%.6f");
							DrawSliderFloat("Texcoord anim V", material.texAnimDirection.y, -0.02f, 0.02f, "%.6f");
							DrawSliderFloat("Texture TileSize X", material.texMulAdd.x, 0.0f, 10.0f);
							DrawSliderFloat("Texture TileSize Y", material.texMulAdd.y, 0.0f, 10.0f);
							PropertyGridSpacing();



							const char* textureNames[] =
							{
								"BaseColorMap			   ",
								"NormalMap				   ",
								"SurfaceMap				   ",
								"EmissiveMap			   ",
								"DisplacementMap		   ",
								"OcclusionMap			   ",
								"TransmissionMap		   ",
								"SheenColorMap			   ",
								"SheenRoughnessMap		   ",
								"ClearcoatMap			   ",
								"ClearcoatRoughnessMap	   ",
								"ClearcoatNormalMap		   ",
								"SpecularMap			   "
							};



							for (int i = 0; i < MaterialComponent::TEXTURESLOT::TEXTURESLOT_COUNT; i++)
							{
								DrawImage(textureNames[i], mesh->subsets[subsetIdx].materialID, i);
								if (i != MaterialComponent::TEXTURESLOT::TEXTURESLOT_COUNT - 1)
								{
									ImGui::Separator();
									PropertyGridSpacing();

								}
							}


							PropertyGridSpacing();

							EndPropertyGrid();

							material.SetDirty();
						}, false);
				}
			}


		}

		

		






	}

}
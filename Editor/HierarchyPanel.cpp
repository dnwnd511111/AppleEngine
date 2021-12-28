#include "pch.h"
#include "HierarchyPanel.h"
#include "Editor.h"
#include "apECS.h"
#include "apScene.h"

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

	void HierarchyPanel::ImGuiRender(float dt)
	{
		
		if (ImGui::IsWindowHovered() && ImGui::IsWindowFocused())
			editor->renderComponent.DeleteSelectedEntities();

		Scene& scene = GetScene();

		for (int i = 0; i < scene.transforms.GetCount(); i++)
		{
			Entity entity = scene.transforms.GetEntity(i);
			HierarchyComponent* hierarchyComponent = scene.hierarchy.GetComponent(entity);
			if (hierarchyComponent == nullptr || hierarchyComponent->parentID == ap::ecs::INVALID_ENTITY)
			{
				DrawEntityNode(entity);
			}

		}
		

		

		
	}

	void HierarchyPanel::ImGuiRenderProperties(float dt)
	{

		
		if (editor->renderComponent.translator.selected.size() == 1)
		{
			Entity entity = editor->renderComponent.translator.selected[0].entity;
			DrawComponents(entity);
		}

	}

	void HierarchyPanel::DrawEntityNode(ap::ecs::Entity entity) const
	{
		Scene& scene = GetScene();


		const char* name = "Unnamed Entity";

		NameComponent* nameComponent = scene.names.GetComponent(entity);
		if (name != nullptr)
			name = nameComponent->name.c_str();

	
		
		ImGuiTreeNodeFlags flags = ((editor->renderComponent.IsSelected(entity)) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanAvailWidth;


		HierarchyComponent* hierarchyComponent = scene.hierarchy.GetComponent(entity);

		if(hierarchyComponent == nullptr || hierarchyComponent->childrenID.size() ==0 )
			flags |= ImGuiTreeNodeFlags_Leaf;


		bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, name);
		
		if (ImGui::IsItemClicked() && ap::input::Down(ap::input::KEYBOARD_BUTTON_LCONTROL))
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
		bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
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


	void HierarchyPanel::DrawComponents(ap::ecs::Entity entity)
	{

		Scene& scene = GetScene();

		
		NameComponent* name = scene.names.GetComponent(entity);
		LayerComponent* layer = scene.layers.GetComponent(entity);
		TransformComponent* transform = scene.transforms.GetComponent(entity);
		MaterialComponent* material = scene.materials.GetComponent(entity);
		MeshComponent* mesh =scene.meshes.GetComponent(entity);
		ImpostorComponent* imposter = scene.impostors.GetComponent(entity);
		ObjectComponent* object = scene.objects.GetComponent(entity);
		RigidBodyPhysicsComponent* rigidbody = scene.rigidbodies.GetComponent(entity);
		SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(entity);
		ArmatureComponent* armature =scene.armatures.GetComponent(entity);
		LightComponent* light = scene.lights.GetComponent(entity);
		CameraComponent* camera = scene.cameras.GetComponent(entity);
		EnvironmentProbeComponent* environmentprobe = scene.probes.GetComponent(entity);
		ForceFieldComponent* forcefield = scene.forces.GetComponent(entity);
		DecalComponent* decal = scene.decals.GetComponent(entity);
		AnimationComponent* animation =scene.animations.GetComponent(entity);
		ap::EmittedParticleSystem* emitter = scene.emitters.GetComponent(entity);
		ap::HairParticleSystem* hair = scene.hairs.GetComponent(entity);
		WeatherComponent* weather = scene.weathers.GetComponent(entity);
		SoundComponent* sound = scene.sounds.GetComponent(entity);
		InverseKinematicsComponent* kinematic = scene.inverse_kinematics.GetComponent(entity);
		SpringComponent* spring = scene.springs.GetComponent(entity);
		

		if (name)
		{
			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			std::strncpy(buffer, name->name.c_str(), sizeof(buffer));
			if (ImGui::InputText("##name", buffer, sizeof(buffer)))
			{
				name->name = std::string(buffer);
			}

			ImGui::SameLine();
			ImGui::PushItemWidth(-1);

			if (ImGui::Button("Add Component"))
				ImGui::OpenPopup("AddComponent");

			if (ImGui::BeginPopup("AddComponent"))
			{
				//if (ImGui::MenuItem("Camera"))
				//{

				//}


				ImGui::EndPopup();
			}
			ImGui::PopItemWidth();




		}


		if (camera)
		{
			DrawComponent("Camera", camera, [](CameraComponent& camera) 
				{


					BeginPropertyGrid();
					PropertyGridSpacing();

					
					if (DrawButton2("Reset"))
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

		}

		if (transform)
		{
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

					DrawSliderFloat("Range", light.range_local, 0.0f, 200.0f);
					DrawColorEdit3("Color", light.color);

					DrawSliderFloat("Energy", light.energy, 0.0f, 64.0f);
					DrawSliderFloat("ConeAngleCosine", light.fov, 0.0f, 3.13f);


					ImGui::Separator();
					PropertyGridSpacing();
					

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

					if (DrawButton2("Refresh"))
					{
						probe.SetDirty(true);
					}

					if (DrawButton2("Refresh ALL"))
					{
						Scene& scene = GetScene();

						for (int i = 0; i < scene.probes.GetCount(); i++)
						{
							Entity entity = scene.probes.GetEntity(i);
							EnvironmentProbeComponent* otherProbe = scene.probes.GetComponent(entity);
							otherProbe->SetDirty();
						}

						
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

					if (DrawButton2("Restart"))
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

					if (DrawButton2("Burst"))
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

		


	}

}
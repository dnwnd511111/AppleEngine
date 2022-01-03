#include "pch.h"
#include "PaintToolPanel.h"
#include "apImGui.h"
#include "Editor.h"
#include "shaders/ShaderInterop_Renderer.h"
#include <cmath>

using namespace ap::ecs;
using namespace ap::scene;
using namespace ap::graphics;

using namespace ap::imgui;


namespace Panel
{

	PaintToolPanel::PaintToolPanel(Editor* editor)
		:
		BasePanel(editor)
	{
	}



	void PaintToolPanel::ImGuiRender(float dt)
	{

		BeginPropertyGrid();
		PropertyGridSpacing();
		

		{
			{
				const std::vector<std::string> items =
				{
					"Disabled",
					"Texture",
					"Vertexcolor",
					"Sculpting - Add",
					"Sculpting - Subtract",
					"Softbody - Pinning",
					"Softbody - Physics",
					"Hairparticle - Add Triangle",
					"Hairparticle - Remove Triangle",
					"Hairparticle - Length (Alpha)",
					"Wind weight (Alpha)",
				};

				DrawCombo("Paint Mode", items, items.size(), &selectedMode);
			}

			{
				const std::vector<std::string> items =
				{
					"BASECOLORMAP",
					"NORMALMAP",
					"SURFACEMAP",
					"EMISSIVEMAP",
					"DISPLACEMENTMAP",
					"OCCLUSIONMAP",
					"TRANSMISSIONMAP",
					"SHEENCOLORMAP",
					"SHEENROUGHNESSMAP",
					"CLEARCOATMAP",
					"CLEARCOATROUGHNESSMAP",
					"CLEARCOATNORMALMAP",
					"SPECULARMAP",
				};

				DrawCombo("Texture Slot", items, items.size(), &selectedTextureIdx);
			}

			


			PropertyGridSpacing();
			ImGui::Separator();
			PropertyGridSpacing();

			DrawCheckbox("Backfaces", backfaces);
			DrawCheckbox("Wireframe", wireframe);
			DrawCheckbox("Pressure", pressure);



			PropertyGridSpacing();
			ImGui::Separator();
			PropertyGridSpacing();

			DrawSliderFloat("Brush Radius", brushRadius, 1, 500);
			DrawSliderFloat("Brush Amount", brushAmount, 0, 1);
			DrawSliderFloat("Brush Falloff", brushFalloff, 0, 16);
			DrawSliderInt("Brush Spacing", brushSpacing, 0, 500);
			DrawColorEdit4("Texture Color", textureColor);
			


			EndPropertyGrid();

		}

	}
	void PaintToolPanel::Update(float dt)
	{

		rot -= dt;


		// by default, paint tool is on center of screen, this makes it easy to tweak radius with GUI:
		XMFLOAT2 posNew;
		posNew.x = editor->renderComponent.GetLogicalWidth() * 0.5f;
		posNew.y = editor->renderComponent.GetLogicalHeight() * 0.5f;
		if (editor->renderComponent.GetGUI().HasFocus() || ap::backlog::isActive() || entity == INVALID_ENTITY)
		{
			pos = posNew;
			return;
		}

		const bool pointer_down = ap::input::Down(ap::input::MOUSE_BUTTON_LEFT);
		if (!pointer_down)
		{
			stroke_dist = FLT_MAX;
		}


		auto pointer = ap::input::GetPointer();
		float screenW = editor->renderComponent.GetLogicalWidth();
		float screenH = editor->renderComponent.GetLogicalHeight();
		float ratioX = screenW / (editor->viewportSize.x);
		float ratioY = screenH / (editor->viewportSize.y);

		POINT p;
		p.x = editor->viewportBounds[0].x;
		p.y = editor->viewportBounds[0].y;
		ScreenToClient(editor->window, &p);

		pointer.x = pointer.x - p.x;
		pointer.y = pointer.y - p.y;
		pointer.x *= ratioX;
		pointer.y *= ratioY;

		posNew = XMFLOAT2(pointer.x, pointer.y);
		stroke_dist += ap::math::Distance(pos, posNew);
		const float pressure_ = pressure ? pointer.w : 1.0f;

		const float spacing = brushSpacing;
		const bool pointer_moved = stroke_dist >= spacing;
		const bool painting = pointer_moved && pointer_down;

		if (painting)
		{
			stroke_dist = 0;
		}

		pos = posNew;

		const MODE mode = GetMode();
		const float radius = brushRadius;
		const float pressure_radius = radius * pressure_;
		const float amount = brushAmount;
		const float falloff = brushFalloff;
		const ap::Color color = ap::Color::fromFloat4(textureColor);
		const XMFLOAT4 color_float = textureColor;
	
		Scene& scene = ap::scene::GetScene();
		const CameraComponent& camera = ap::scene::GetCamera();
		const XMVECTOR C = XMLoadFloat2(&pos);
		const XMMATRIX VP = camera.GetViewProjection();
		const XMVECTOR MUL = XMVectorSet(0.5f, -0.5f, 1, 1);
		const XMVECTOR ADD = XMVectorSet(0.5f, 0.5f, 0, 0);
		const XMVECTOR SCREEN = XMVectorSet((float)editor->renderComponent.GetLogicalWidth(), (float)editor->renderComponent.GetLogicalHeight(), 1, 1);
		const XMVECTOR F = camera.GetAt();


		switch (mode)
		{
		case MODE_TEXTURE:
		{
			//const ap::scene::PickResult& intersect = editor->hovered;
			//if (intersect.entity != entity)
			//	break;

			//ObjectComponent* object = scene.objects.GetComponent(entity);
			//if (object == nullptr || object->meshID == INVALID_ENTITY)
			//	break;

			//MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			//if (mesh == nullptr || (mesh->vertex_uvset_0.empty() && mesh->vertex_uvset_1.empty()))
			//	break;

			//MaterialComponent* material = subset >= 0 && subset < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[subset].materialID) : nullptr;
			//if (material == nullptr)
			//	break;

			//int uvset = 0;
			//Texture editTexture = GetEditTextureSlot(*material, &uvset);
			//if (!editTexture.IsValid())
			//	break;
			//const TextureDesc& desc = editTexture.GetDesc();
			//auto& vertex_uvset = uvset == 0 ? mesh->vertex_uvset_0 : mesh->vertex_uvset_1;

			//const float u = intersect.bary.x;
			//const float v = intersect.bary.y;
			//const float w = 1 - u - v;
			//XMFLOAT2 uv;
			//uv.x = vertex_uvset[intersect.vertexID0].x * w +
			//	vertex_uvset[intersect.vertexID1].x * u +
			//	vertex_uvset[intersect.vertexID2].x * v;
			//uv.y = vertex_uvset[intersect.vertexID0].y * w +
			//	vertex_uvset[intersect.vertexID1].y * u +
			//	vertex_uvset[intersect.vertexID2].y * v;
			//uint2 center = XMUINT2(uint32_t(uv.x * desc.width), uint32_t(uv.y * desc.height));

			//if (painting)
			//{
			//	GraphicsDevice* device = ap::graphics::GetDevice();
			//	CommandList cmd = device->BeginCommandList();

			//	//RecordHistory(true, cmd);

			//	// Need to requery this because RecordHistory might swap textures on material:
			//	editTexture = GetEditTextureSlot(*material, &uvset);

			//	device->BindComputeShader(ap::renderer::GetShader(ap::enums::CSTYPE_PAINT_TEXTURE), cmd);

			//	device->BindResource(ap::texturehelper::getWhite(), 0, cmd);
			//	device->BindUAV(&editTexture, 0, cmd);

			//	PaintTextureCB cb;
			//	cb.xPaintBrushCenter = center;
			//	cb.xPaintBrushRadius = (uint32_t)pressure_radius;
			//	cb.xPaintBrushAmount = amount;
			//	cb.xPaintBrushFalloff = falloff;
			//	cb.xPaintBrushColor = color.rgba;
			//	device->PushConstants(&cb, sizeof(cb), cmd);

			//	const uint diameter = cb.xPaintBrushRadius * 2;
			//	const uint dispatch_dim = (diameter + PAINT_TEXTURE_BLOCKSIZE - 1) / PAINT_TEXTURE_BLOCKSIZE;
			//	device->Dispatch(dispatch_dim, dispatch_dim, 1, cmd);

			//	GPUBarrier barriers[] = {
			//		GPUBarrier::Memory(),
			//	};
			//	device->Barrier(barriers, arraysize(barriers), cmd);


			//	ap::renderer::GenerateMipChain(editTexture, ap::renderer::MIPGENFILTER::MIPGENFILTER_LINEAR, cmd);
			//}

			//ap::renderer::PaintRadius paintrad;
			//paintrad.objectEntity = entity;
			//paintrad.subset = subset;
			//paintrad.radius = radius;
			//paintrad.center = center;
			//paintrad.uvset = uvset;
			//paintrad.dimensions.x = desc.width;
			//paintrad.dimensions.y = desc.height;
			//ap::renderer::DrawPaintRadius(paintrad);
		}
		break;

		case MODE_VERTEXCOLOR:
		case MODE_WIND:
		{
			ObjectComponent* object = scene.objects.GetComponent(entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				break;

			MaterialComponent* material = subset >= 0 && subset < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[subset].materialID) : nullptr;
			if (material == nullptr)
			{
				for (auto& x : mesh->subsets)
				{
					material = scene.materials.GetComponent(x.materialID);
					if (material != nullptr)
					{
						switch (mode)
						{
						case MODE_VERTEXCOLOR:
							material->SetUseVertexColors(true);
							break;
						case MODE_WIND:
							material->SetUseWind(true);
							break;
						}
					}
				}
				material = nullptr;
			}
			else
			{
				switch (mode)
				{
				case MODE_VERTEXCOLOR:
					material->SetUseVertexColors(true);
					break;
				case MODE_WIND:
					material->SetUseWind(true);
					break;
				}
			}

			const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;

			const TransformComponent* transform = scene.transforms.GetComponent(entity);
			if (transform == nullptr)
				break;

			const XMMATRIX W = XMLoadFloat4x4(&transform->world);

			bool rebuild = false;

			switch (mode)
			{
			case MODE_VERTEXCOLOR:
				if (mesh->vertex_colors.empty())
				{
					mesh->vertex_colors.resize(mesh->vertex_positions.size());
					std::fill(mesh->vertex_colors.begin(), mesh->vertex_colors.end(), ap::Color::White().rgba); // fill white
					rebuild = true;
				}
				break;
			case MODE_WIND:
				if (mesh->vertex_windweights.empty())
				{
					mesh->vertex_windweights.resize(mesh->vertex_positions.size());
					std::fill(mesh->vertex_windweights.begin(), mesh->vertex_windweights.end(), 0xFF); // fill max affection
					rebuild = true;
				}
				break;
			}

			if (painting)
			{
				for (size_t j = 0; j < mesh->vertex_positions.size(); ++j)
				{
					XMVECTOR P, N;
					if (armature == nullptr)
					{
						P = XMLoadFloat3(&mesh->vertex_positions[j]);
						N = XMLoadFloat3(&mesh->vertex_normals[j]);
					}
					else
					{
						P = ap::scene::SkinVertex(*mesh, *armature, (uint32_t)j, &N);
					}
					P = XMVector3Transform(P, W);
					N = XMVector3Normalize(XMVector3TransformNormal(N, W));

					if (!backfaces && XMVectorGetX(XMVector3Dot(F, N)) > 0)
						continue;

					P = XMVector3TransformCoord(P, VP);
					P = P * MUL + ADD;
					P = P * SCREEN;

					if (subset >= 0 && mesh->vertex_subsets[j] != subset)
						continue;

					const float z = XMVectorGetZ(P);
					const float dist = XMVectorGetX(XMVector2Length(C - P));
					if (z >= 0 && z <= 1 && dist <= pressure_radius)
					{
						//RecordHistory(true);
						rebuild = true;
						const float affection = amount * std::pow(1.0f - (dist / pressure_radius), falloff);

						switch (mode)
						{
						case MODE_VERTEXCOLOR:
						{
							ap::Color vcol = mesh->vertex_colors[j];
							vcol = ap::Color::lerp(vcol, color, affection);
							mesh->vertex_colors[j] = vcol.rgba;
						}
						break;
						case MODE_WIND:
						{
							ap::Color vcol = ap::Color(0, 0, 0, mesh->vertex_windweights[j]);
							vcol = ap::Color::lerp(vcol, color, affection);
							mesh->vertex_windweights[j] = vcol.getA();
						}
						break;
						}
					}
				}
			}

			if (wireframe)
			{
				for (size_t j = 0; j < mesh->indices.size(); j += 3)
				{
					const uint32_t triangle[] = {
						mesh->indices[j + 0],
						mesh->indices[j + 1],
						mesh->indices[j + 2],
					};
					if (subset >= 0 && (mesh->vertex_subsets[triangle[0]] != subset || mesh->vertex_subsets[triangle[1]] != subset || mesh->vertex_subsets[triangle[2]] != subset))
						continue;

					const XMVECTOR P[arraysize(triangle)] = {
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[0]), W),
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[1]), W),
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[2]), W),
					};

					ap::renderer::RenderableTriangle tri;
					XMStoreFloat3(&tri.positionA, P[0]);
					XMStoreFloat3(&tri.positionB, P[1]);
					XMStoreFloat3(&tri.positionC, P[2]);
					if (mode == MODE_WIND)
					{
						tri.colorA = ap::Color(mesh->vertex_windweights[triangle[0]], 0, 0, 255);
						tri.colorB = ap::Color(mesh->vertex_windweights[triangle[1]], 0, 0, 255);
						tri.colorC = ap::Color(mesh->vertex_windweights[triangle[2]], 0, 0, 255);
					}
					else
					{
						tri.colorA.w = 0.8f;
						tri.colorB.w = 0.8f;
						tri.colorC.w = 0.8f;
					}
					ap::renderer::DrawTriangle(tri, true);
				}
			}

			if (rebuild)
			{
				mesh->CreateRenderData();
			}
		}
		break;

		case MODE_SCULPTING_ADD:
		case MODE_SCULPTING_SUBTRACT:
		{
			ObjectComponent* object = scene.objects.GetComponent(entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				break;

			const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;

			const TransformComponent* transform = scene.transforms.GetComponent(entity);
			if (transform == nullptr)
				break;

			const XMMATRIX W = XMLoadFloat4x4(&transform->world);

			bool rebuild = false;
			if (painting)
			{
				XMVECTOR averageNormal = XMVectorZero();
				struct PaintVert
				{
					size_t ind;
					float affection;
				};
				static ap::vector<PaintVert> paintindices;
				paintindices.clear();
				paintindices.reserve(mesh->vertex_positions.size());

				for (size_t j = 0; j < mesh->vertex_positions.size(); ++j)
				{
					XMVECTOR P, N;
					if (armature == nullptr)
					{
						P = XMLoadFloat3(&mesh->vertex_positions[j]);
						N = XMLoadFloat3(&mesh->vertex_normals[j]);
					}
					else
					{
						P = ap::scene::SkinVertex(*mesh, *armature, (uint32_t)j, &N);
					}
					P = XMVector3Transform(P, W);
					N = XMVector3Normalize(XMVector3TransformNormal(N, W));

					if (!backfaces && XMVectorGetX(XMVector3Dot(F, N)) > 0)
						continue;

					P = XMVector3TransformCoord(P, VP);
					P = P * MUL + ADD;
					P = P * SCREEN;

					const float z = XMVectorGetZ(P);
					const float dist = XMVectorGetX(XMVector2Length(C - P));
					if (z >= 0 && z <= 1 && dist <= pressure_radius)
					{
						averageNormal += N;
						const float affection = amount * std::pow(1.0f - (dist / pressure_radius), falloff);
						paintindices.push_back({ j, affection });
					}
				}

				if (!paintindices.empty())
				{
					//RecordHistory(true);
					rebuild = true;
					averageNormal = XMVector3Normalize(averageNormal);

					for (auto& x : paintindices)
					{
						XMVECTOR PL = XMLoadFloat3(&mesh->vertex_positions[x.ind]);
						switch (mode)
						{
						case MODE_SCULPTING_ADD:
							PL += averageNormal * x.affection;
							break;
						case MODE_SCULPTING_SUBTRACT:
							PL -= averageNormal * x.affection;
							break;
						}
						XMStoreFloat3(&mesh->vertex_positions[x.ind], PL);
					}
				}
			}

			if (wireframe)
			{
				for (size_t j = 0; j < mesh->indices.size(); j += 3)
				{
					const uint32_t triangle[] = {
						mesh->indices[j + 0],
						mesh->indices[j + 1],
						mesh->indices[j + 2],
					};
					const XMVECTOR P[arraysize(triangle)] = {
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[0]), W),
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[1]), W),
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[2]), W),
					};

					ap::renderer::RenderableTriangle tri;
					XMStoreFloat3(&tri.positionA, P[0]);
					XMStoreFloat3(&tri.positionB, P[1]);
					XMStoreFloat3(&tri.positionC, P[2]);
					tri.colorA.w = 0.8f;
					tri.colorB.w = 0.8f;
					tri.colorC.w = 0.8f;
					ap::renderer::DrawTriangle(tri, true);
				}
			}

			if (rebuild)
			{
				mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH_FAST);
			}
		}
		break;

		case MODE_SOFTBODY_PINNING:
		case MODE_SOFTBODY_PHYSICS:
		{
			ObjectComponent* object = scene.objects.GetComponent(entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				break;

			SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object->meshID);
			if (softbody == nullptr || softbody->vertex_positions_simulation.empty())
				break;

			// Painting:
			if (pointer_moved && ap::input::Down(ap::input::MOUSE_BUTTON_LEFT))
			{
				size_t j = 0;
				for (auto& ind : softbody->physicsToGraphicsVertexMapping)
				{
					XMVECTOR P = softbody->vertex_positions_simulation[ind].LoadPOS();
					P = XMVector3TransformCoord(P, VP);
					P = P * MUL + ADD;
					P = P * SCREEN;
					const float z = XMVectorGetZ(P);
					if (z >= 0 && z <= 1 && XMVectorGetX(XMVector2Length(C - P)) <= pressure_radius)
					{
						//RecordHistory(true);
						softbody->weights[j] = (mode == MODE_SOFTBODY_PINNING ? 0.0f : 1.0f);
						softbody->_flags |= SoftBodyPhysicsComponent::FORCE_RESET;
					}
					j++;
				}
			}

			// Visualizing:
			const XMMATRIX W = XMLoadFloat4x4(&softbody->worldMatrix);
			for (size_t j = 0; j < mesh->indices.size(); j += 3)
			{
				const uint32_t graphicsIndex0 = mesh->indices[j + 0];
				const uint32_t graphicsIndex1 = mesh->indices[j + 1];
				const uint32_t graphicsIndex2 = mesh->indices[j + 2];
				const uint32_t physicsIndex0 = softbody->graphicsToPhysicsVertexMapping[graphicsIndex0];
				const uint32_t physicsIndex1 = softbody->graphicsToPhysicsVertexMapping[graphicsIndex1];
				const uint32_t physicsIndex2 = softbody->graphicsToPhysicsVertexMapping[graphicsIndex2];
				const float weight0 = softbody->weights[physicsIndex0];
				const float weight1 = softbody->weights[physicsIndex1];
				const float weight2 = softbody->weights[physicsIndex2];
				ap::renderer::RenderableTriangle tri;
				if (softbody->vertex_positions_simulation.empty())
				{
					XMStoreFloat3(&tri.positionA, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[graphicsIndex0]), W));
					XMStoreFloat3(&tri.positionB, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[graphicsIndex1]), W));
					XMStoreFloat3(&tri.positionC, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[graphicsIndex2]), W));
				}
				else
				{
					tri.positionA = softbody->vertex_positions_simulation[graphicsIndex0].pos;
					tri.positionB = softbody->vertex_positions_simulation[graphicsIndex1].pos;
					tri.positionC = softbody->vertex_positions_simulation[graphicsIndex2].pos;
				}
				if (weight0 == 0)
					tri.colorA = XMFLOAT4(1, 1, 0, 1);
				else
					tri.colorA = XMFLOAT4(1, 1, 1, 1);
				if (weight1 == 0)
					tri.colorB = XMFLOAT4(1, 1, 0, 1);
				else
					tri.colorB = XMFLOAT4(1, 1, 1, 1);
				if (weight2 == 0)
					tri.colorC = XMFLOAT4(1, 1, 0, 1);
				else
					tri.colorC = XMFLOAT4(1, 1, 1, 1);
				if (wireframe)
				{
					ap::renderer::DrawTriangle(tri, true);
				}
				if (weight0 == 0 && weight1 == 0 && weight2 == 0)
				{
					tri.colorA = tri.colorB = tri.colorC = XMFLOAT4(1, 0, 0, 0.8f);
					ap::renderer::DrawTriangle(tri);
				}
			}
		}
		break;

		case MODE_HAIRPARTICLE_ADD_TRIANGLE:
		case MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
		case MODE_HAIRPARTICLE_LENGTH:
		{
			ap::HairParticleSystem* hair = scene.hairs.GetComponent(entity);
			if (hair == nullptr || hair->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(hair->meshID);
			if (mesh == nullptr)
				break;

			const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;

			const TransformComponent* transform = scene.transforms.GetComponent(entity);
			if (transform == nullptr)
				break;

			const XMMATRIX W = XMLoadFloat4x4(&transform->world);

			if (painting)
			{
				for (size_t j = 0; j < mesh->vertex_positions.size(); ++j)
				{
					XMVECTOR P, N;
					if (armature == nullptr)
					{
						P = XMLoadFloat3(&mesh->vertex_positions[j]);
						N = XMLoadFloat3(&mesh->vertex_normals[j]);
					}
					else
					{
						P = ap::scene::SkinVertex(*mesh, *armature, (uint32_t)j, &N);
					}
					P = XMVector3Transform(P, W);
					N = XMVector3Normalize(XMVector3TransformNormal(N, W));

					if (!backfaces && XMVectorGetX(XMVector3Dot(F, N)) > 0)
						continue;

					P = XMVector3TransformCoord(P, VP);
					P = P * MUL + ADD;
					P = P * SCREEN;

					const float z = XMVectorGetZ(P);
					const float dist = XMVectorGetX(XMVector2Length(C - P));
					if (z >= 0 && z <= 1 && dist <= pressure_radius)
					{
						//RecordHistory(true);
						switch (mode)
						{
						case MODE_HAIRPARTICLE_ADD_TRIANGLE:
							hair->vertex_lengths[j] = 1.0f;
							break;
						case MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
							hair->vertex_lengths[j] = 0;
							break;
						case MODE_HAIRPARTICLE_LENGTH:
							if (hair->vertex_lengths[j] > 0) // don't change distribution
							{
								const float affection = amount * std::pow(1.0f - (dist / pressure_radius), falloff);
								hair->vertex_lengths[j] = ap::math::Lerp(hair->vertex_lengths[j], color_float.w, affection);
								// don't let it "remove" the vertex by keeping its length above zero:
								//	(because if removed, distribution also changes which might be distracting)
								hair->vertex_lengths[j] = ap::math::Clamp(hair->vertex_lengths[j], 1.0f / 255.0f, 1.0f);
							}
							break;
						}
						hair->_flags |= ap::HairParticleSystem::REBUILD_BUFFERS;
					}
				}
			}

			if (wireframe)
			{
				for (size_t j = 0; j < mesh->indices.size(); j += 3)
				{
					const uint32_t triangle[] = {
						mesh->indices[j + 0],
						mesh->indices[j + 1],
						mesh->indices[j + 2],
					};
					const XMVECTOR P[arraysize(triangle)] = {
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[0]), W),
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[1]), W),
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[2]), W),
					};

					ap::renderer::RenderableTriangle tri;
					XMStoreFloat3(&tri.positionA, P[0]);
					XMStoreFloat3(&tri.positionB, P[1]);
					XMStoreFloat3(&tri.positionC, P[2]);
					ap::renderer::DrawTriangle(tri, true);
				}

				for (size_t j = 0; j < hair->indices.size() && wireframe; j += 3)
				{
					const uint32_t triangle[] = {
						hair->indices[j + 0],
						hair->indices[j + 1],
						hair->indices[j + 2],
					};
					const XMVECTOR P[arraysize(triangle)] = {
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[0]), W),
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[1]), W),
						XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : ap::scene::SkinVertex(*mesh, *armature, triangle[2]), W),
					};

					ap::renderer::RenderableTriangle tri;
					XMStoreFloat3(&tri.positionA, P[0]);
					XMStoreFloat3(&tri.positionB, P[1]);
					XMStoreFloat3(&tri.positionC, P[2]);
					tri.colorA = tri.colorB = tri.colorC = XMFLOAT4(1, 0, 1, 0.9f);
					ap::renderer::DrawTriangle(tri, false);
				}
			}
		}
		break;

		}

	}
	void PaintToolPanel::DrawBrush() const
	{

		const MODE mode = GetMode();
		if (mode == MODE_DISABLED || mode == MODE_TEXTURE || entity == INVALID_ENTITY || ap::backlog::isActive())
			return;

		const int segmentcount = 36;
		const float radius = brushRadius;

		for (int i = 0; i < segmentcount; i += 1)
		{
			const float angle0 = rot + (float)i / (float)segmentcount * XM_2PI;
			const float angle1 = rot + (float)(i + 1) / (float)segmentcount * XM_2PI;
			ap::renderer::RenderableLine2D line;
			line.start.x = pos.x + sinf(angle0) * radius;
			line.start.y = pos.y + cosf(angle0) * radius;
			line.end.x = pos.x + sinf(angle1) * radius;
			line.end.y = pos.y + cosf(angle1) * radius;
			line.color_end = line.color_start = i % 2 == 0 ? XMFLOAT4(0, 0, 0, 0.8f) : XMFLOAT4(1, 1, 1, 1);
			ap::renderer::DrawLine(line);
		}
	}

	PaintToolPanel::MODE PaintToolPanel::GetMode() const
	{
		return (MODE)selectedMode;
	}
	void PaintToolPanel::SetEntity(ap::ecs::Entity value, int subsetindex)
	{
		entity = value;
		subset = subsetindex;

		if (entity == INVALID_ENTITY)
		{
			
		}
		else if (GetMode() == MODE_TEXTURE)
		{
			
		}
	}
}

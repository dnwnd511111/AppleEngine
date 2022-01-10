
#include "pch.h"
#include "Editor.h"
#include "apRenderer.h"

#include "ModelImporter.h"
#include "Translator.h"
#include "Utility/stb_image.h"

#include "apImGui.h"

#include <string>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <limits>

using namespace ap::graphics;
using namespace ap::primitive;
using namespace ap::rectpacker;
using namespace ap::scene;
using namespace ap::ecs;

using namespace ap::imgui;


void EditorLoadingScreen::Load()
{

	font = ap::SpriteFont("Loading...", ap::font::Params(0, 0, 36, ap::font::APFALIGN_CENTER, ap::font::APFALIGN_CENTER));
	AddFont(&font);

	sprite = ap::Sprite("resources/images/apple.png");
	sprite.anim.opa = 1;
	sprite.anim.repeatable = true;
	sprite.params.siz = XMFLOAT2(128, 128);
	sprite.params.pivot = XMFLOAT2(0.5f, 1.0f);
	sprite.params.quality = ap::image::QUALITY_LINEAR;
	sprite.params.blendFlag = ap::enums::BLENDMODE_ALPHA;
	AddSprite(&sprite);


	LoadingScreen::Load();
	
}

void EditorLoadingScreen::Update(float dt)
{
	font.params.posX = GetLogicalWidth() * 0.5f;
	font.params.posY = GetLogicalHeight() * 0.5f;
	sprite.params.pos = XMFLOAT3(GetLogicalWidth() * 0.5f, GetLogicalHeight() * 0.5f - font.TextHeight(), 0);

	LoadingScreen::Update(dt);

}


void Editor::Initialize()
{

	Application::Initialize();

	graphicsDevice->InitImGui(window);
	ap::imgui::Initialize();



	// With this mode, file data for resources will be kept around. This allows serializing embedded resource data inside scenes
	ap::resourcemanager::SetMode(ap::resourcemanager::Mode::ALLOW_RETAIN_FILEDATA);

	if (1)
	{
		infoDisplay.active = true;
		infoDisplay.watermark = false;
		infoDisplay.resolution = true;
		infoDisplay.colorspace = true;
		infoDisplay.fpsinfo = true;
		infoDisplay.heap_allocation_counter = true;
	}
	else
	{
		infoDisplay.active = true;
		infoDisplay.watermark = true;
		infoDisplay.fpsinfo = true;
		infoDisplay.resolution = true;
		//infoDisplay.logical_size = true;
		infoDisplay.colorspace = true;
		infoDisplay.heap_allocation_counter = true;

	}



	ap::renderer::SetOcclusionCullingEnabled(true);

	loader.Load();

	renderComponent.main = this;

	loader.addLoadingComponent(&renderComponent, this, 0.2f);

	ActivatePath(&loader, 0.2f);

	
}

void Editor::ImGuiRender()
{
	//::ImGuiRender();
	// 
	// Note: Switch this to true to enable dockspace
	static bool dockspaceOpen = true;
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}
	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
		ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	float minWinSizeX = style.WindowMinSize.x;
	style.WindowMinSize.x = 250.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}
	style.WindowMinSize.x = minWinSizeX;



	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			
			if (ImGui::MenuItem("New", "Ctrl+N"))
			{
				renderComponent.translator.selected.clear();
				ap::renderer::ClearWorld(ap::scene::GetScene());
				paintToolPanel.SetEntity(INVALID_ENTITY);
			}

			if (ImGui::MenuItem("Open...", "Ctrl+O"))
			{
				ap::helper::FileDialogParams params;
				params.type = ap::helper::FileDialogParams::OPEN;
				params.description = "Model formats (.apscene, .obj, .gltf, .glb)";
				params.extensions.push_back("apscene");
				params.extensions.push_back("obj");
				params.extensions.push_back("gltf");
				params.extensions.push_back("glb");
				ap::helper::FileDialog(params, [&](std::string fileName) {
					ap::eventhandler::Subscribe_Once(ap::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

						size_t camera_count_prev = ap::scene::GetScene().cameras.GetCount();

						loader.addLoadingFunction([=](ap::jobsystem::JobArgs args) {
							std::string extension = ap::helper::toUpper(ap::helper::GetExtensionFromFileName(fileName));

							if (!extension.compare("APSCENE")) // engine-serialized
							{
								ap::scene::LoadModel(fileName); //XMMatrixIdentity(), true);
							}
							else if (!extension.compare("OBJ")) // wavefront-obj
							{
								Scene scene;
								ImportModel_OBJ(fileName, scene);
								ap::scene::GetScene().Merge(scene);
							}
							else if (!extension.compare("GLTF")) // text-based gltf
							{
								Scene scene;
								ImportModel_GLTF(fileName, scene);
								ap::scene::GetScene().Merge(scene);
							}
							else if (!extension.compare("GLB")) // binary gltf
							{
								Scene scene;
								ImportModel_GLTF(fileName, scene);
								ap::scene::GetScene().Merge(scene);
							}
							});
						loader.onFinished([=] {

							// Detect when the new scene contains a new camera, and snap the camera onto it:
							size_t camera_count = ap::scene::GetScene().cameras.GetCount();
							if (camera_count > 0 && camera_count > camera_count_prev)
							{
								Entity entity = ap::scene::GetScene().cameras.GetEntity(camera_count_prev);
								if (entity != INVALID_ENTITY)
								{
									TransformComponent* camera_transform = ap::scene::GetScene().transforms.GetComponent(entity);
									if (camera_transform != nullptr)
									{
										//cameraWnd.camera_transform = *camera_transform;
									}

									CameraComponent* cam = ap::scene::GetScene().cameras.GetComponent(entity);
									if (cam != nullptr)
									{
										ap::scene::GetCamera() = *cam;
										// camera aspect should be always for the current screen
										ap::scene::GetCamera().width = (float)renderComponent.renderPath->GetInternalResolution().x;
										ap::scene::GetCamera().height = (float)renderComponent.renderPath->GetInternalResolution().y;
									}
								}
							}

							ActivatePath(&renderComponent, 0.2f, ap::Color::Black());
							});

						renderComponent.ResetHistory();
						ActivatePath(&loader, 0.2f, ap::Color::Black());
						});
					});

			}

			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
			{
				ap::helper::FileDialogParams params;
				params.type = ap::helper::FileDialogParams::SAVE;
				params.description = "Apple Scene (.apscene)";
				params.extensions.push_back("apscene");

				ap::helper::FileDialog(params, [params,this](std::string fileName) {
					ap::eventhandler::Subscribe_Once(ap::eventhandler::EVENT_THREAD_SAFE_POINT, [fileName, params, this](uint64_t userdata) {
						std::string filename = ap::helper::ReplaceExtension(fileName, params.extensions.front());
						ap::Archive archive =  ap::Archive(filename, false);
						if (archive.IsOpen())
						{
							Scene& scene = ap::scene::GetScene();

							//ap::resourcemanager::Mode embed_mode = (ap::resourcemanager::GetMode());
							//ap::resourcemanager::SetMode(embed_mode);

							scene.Serialize(archive);
							
							this->renderComponent.ResetHistory();
						}
						else
						{
							ap::helper::messageBox("Could not create " + fileName + "!");
						}
						});
					});

			}

			if (ImGui::MenuItem("Exit"))
			{
				DestroyWindow(window);

			}

			ImGui::EndMenu();
		}


		if (ImGui::BeginMenu("Edit"))
		{
			

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
		
			ImGui::EndMenu();
		}



		ImGui::EndMenuBar();
	}


	//viewport
	
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto windowOffset = ImGui::GetWindowPos();
		
		float dpi = canvas.GetDPIScaling();
		
		viewportBounds[0] = { (viewportMinRegion.x + windowOffset.x)/ dpi, (viewportMinRegion.y + windowOffset.y)/ dpi };
		viewportBounds[1] = { (viewportMaxRegion.x + windowOffset.x)/ dpi, (viewportMaxRegion.y + windowOffset.y)/ dpi };

		viewportFocused = ImGui::IsWindowFocused();
		viewportHovered = ImGui::IsWindowHovered();

		ImGui::GetIO().WantCaptureKeyboard = !viewportFocused;

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		viewportSize = { (viewportPanelSize.x)/ dpi, (viewportPanelSize.y)/ dpi };

		uint64_t textureID;


		textureID = graphicsDevice->CopyDescriptorToImGui(&rendertarget_imgui);
		ImGui::Image((ImTextureID)(textureID), ImVec2{ viewportSize.x, viewportSize.y });




		// ImGizmos

		renderComponent.translator.dragStarted = false;
		renderComponent.translator.dragEnded = false;

		if (renderComponent.translator.selected.size() > 0 && (renderComponent.translator.imGizmoType != (ImGuizmo::OPERATION)-1))
		{


			renderComponent.translator.PreTranslate();

			static XMMATRIX matrixDragged = XMMatrixIdentity();
			static XMMATRIX matrixStarted = XMMatrixIdentity();
			
			if (ImGuizmo::IsUsing())
			{
				if (!renderComponent.translator.dragging)
				{
					renderComponent.translator.dragStarted = true;
				}
				renderComponent.translator.dragging = true;
			}
			else
			{
				if (renderComponent.translator.dragging)
				{
					renderComponent.translator.dragEnded = true;
				}
				renderComponent.translator.dragging = false;
				

				if (renderComponent.translator.dragEnded)
				{
					XMFLOAT4X4 temp;
					XMStoreFloat4x4(&temp, XMMatrixInverse(nullptr, matrixStarted)* matrixDragged);
					ap::Archive& archive = renderComponent.AdvanceHistory();
					archive << EditorComponent::HISTORYOP_TRANSLATOR;
					archive << temp;
				}



				matrixDragged = renderComponent.translator.transform.GetLocalMatrix();
				matrixStarted = renderComponent.translator.transform.GetLocalMatrix();
			}
			


			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			
			ImGuizmo::SetRect(viewportBounds[0].x* dpi, viewportBounds[0].y* dpi, viewportSize.x* dpi, viewportSize.y* dpi);

			Scene& scene = GetScene();
			CameraComponent* cameraComponent = scene.cameras.GetComponent(renderComponent.mainCamera);
			XMMATRIX view;
			XMMATRIX projection;
			XMMATRIX preMatrix = matrixDragged;
			XMMATRIX delta = XMMatrixIdentity();
			if (cameraComponent != nullptr)
			{
				view = cameraComponent->GetView();
				projection = XMMatrixPerspectiveFovLH(cameraComponent->fov, cameraComponent->width / cameraComponent->height, cameraComponent->zNearP, cameraComponent->zFarP);
			}
			ImGuizmo::Manipulate(&view.r[0].m128_f32[0], &projection.r[0].m128_f32[0],
				(ImGuizmo::OPERATION)renderComponent.translator.imGizmoType, ImGuizmo::LOCAL, &matrixDragged.r[0].m128_f32[0],
				&delta.r[0].m128_f32[0]);


			if (ImGuizmo::IsUsing())
			{


				if (renderComponent.translator.imGizmoType == (ImGuizmo::OPERATION::SCALE))
				{

					XMVECTOR S, R, T;
					XMMatrixDecompose(&S, &R, &T, preMatrix);
					XMVECTOR S1, R1, T1;
					XMMatrixDecompose(&S1, &R1, &T1, matrixDragged);

					XMVECTOR scale = XMVectorReciprocal(S) * S1;
					XMFLOAT3 scale1;
					XMStoreFloat3(&scale1, scale);
					renderComponent.translator.transform.Scale(scale1);

				}
				else
				{
					renderComponent.translator.transform.MatrixTransform(delta);
				}
				renderComponent.translator.transform.UpdateTransform();
			}
			
			renderComponent.translator.PostTranslate();



	
		}
		else
		{
			if (renderComponent.translator.dragging)
			{
				renderComponent.translator.dragEnded = true;
			}
			renderComponent.translator.dragging = false;
			
		}
		

		ImGui::End(); //Viewport
		ImGui::PopStyleVar();

	}


	float dt = ap::scene::GetScene().dt;

	bool isLoadingModel = activePath != &renderComponent;
	// Render panels 
	{
		ImGui::Begin("Scene Hierarchy");
		if (!isLoadingModel)
			hierarchyPanel.ImGuiRender(dt);
		ImGui::End();

		ImGui::Begin("Properties");
		if (!isLoadingModel)
			hierarchyPanel.ImGuiRenderProperties(dt);
		ImGui::End();


		ImGui::Begin("Content Browser", NULL, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
		contentBrowserPanel.ImGuiRender(dt);
		ImGui::End();

		ImGui::Begin("Paint Tool");
		paintToolPanel.ImGuiRender(dt);
		ImGui::End();


		ImGui::Begin("PostProcess");
		ImGuiRender_PostProcess();
		ImGui::End();


		ImGui::Begin("Place Actors");
		ImGuiRender_PlaceActors();
		ImGui::End();

		//
		ImGuiRender_Terrain();
		ImGuiRender_ToolBar();
		ImGuiRender_Renderer();

	}
	
	


	ImGui::End(); // DockSpace Demo


}

void Editor::ImGuiRender_PlaceActors()
{

	Scene& scene = ap::scene::GetScene();
	CameraComponent& camera = ap::scene::GetCamera();


	XMFLOAT3 lightColor = { 1,1,1 };
	XMFLOAT3 frontCamPos;
	XMStoreFloat3(&frontCamPos,(XMLoadFloat3(&camera.Eye) + (XMLoadFloat3(&camera.At) * 10.0f)));

	static std::string entityType;

	ImGui::Text("Entity");
	PropertyGridSpacing();

	BeginPropertyGrid();
	ImGui::Columns(3);
	if (DrawButton("Point", ImVec2(70, 20)))
	{
		scene.Entity_CreateLight("Point", frontCamPos, lightColor, 2.0f, 60.0f);
	}

	if (DrawButton("Spot", ImVec2(70, 20)))
	{
		scene.Entity_CreateLight("Spot", frontCamPos, lightColor, 2.0f, 60.0f,ap::scene::LightComponent::LightType::SPOT);

	}

	if (DrawButton("Directional", ImVec2(70, 20)))
	{
		scene.Entity_CreateLight("Directional", frontCamPos, lightColor, 2.0f, 60.0f, ap::scene::LightComponent::LightType::DIRECTIONAL);
	}

	if (DrawButton("EnvProbe", ImVec2(70, 20)))
	{
		scene.Entity_CreateEnvironmentProbe("EnvProbe", frontCamPos);
	}

	if (DrawButton("Emitter", ImVec2(70, 20)))
	{
		scene.Entity_CreateEmitter("Emitter", frontCamPos);

	}

	if (DrawButton("Hair", ImVec2(70, 20)))
	{
		scene.Entity_CreateHair("editorHair", frontCamPos);

	}

	if (DrawButton("Decal", ImVec2(70, 20)))
	{
		scene.Entity_CreateDecal("Decal", frontCamPos);

	}

	if (DrawButton("Sound", ImVec2(70, 20)))
	{
		ap::helper::FileDialogParams params;
		params.type = ap::helper::FileDialogParams::OPEN;
		params.description = "Sound";
		params.extensions = ap::resourcemanager::GetSupportedSoundExtensions();
		ap::helper::FileDialog(params, [=](std::string fileName) {
			ap::eventhandler::Subscribe_Once(ap::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				Entity entity = GetScene().Entity_CreateSound("Sound", fileName);
				});
			});


	}


	EndPropertyGrid();



	PropertyGridSpacing();
	PropertyGridSpacing();
	PropertyGridSpacing();
	ImGui::Separator();

	float geoWidth = 60;
	float geoHeight = 20;


	{
		static std::string data;


		float scale = 3.0f;
		ImGui::Text("Geometry");
		PropertyGridSpacing();
		BeginPropertyGrid();
		ImGui::Columns(4);


		const std::string basePath = "resources/models/Geometry/";

		if (DrawButton("Cube", ImVec2(geoWidth, geoHeight)))
		{
			ImportModel_OBJ(basePath+"cube.obj", scene);
		}

		if (DrawButton("Sphere", ImVec2(geoWidth, geoHeight)))
		{
			ImportModel_OBJ(basePath + "uvsphere.obj", scene);
		}
		if (DrawButton("Cylinder", ImVec2(geoWidth, geoHeight)))
		{
			ImportModel_OBJ(basePath + "cylinder.obj", scene);
		}
		if (DrawButton("Torus", ImVec2(geoWidth, geoHeight)))
		{
			ImportModel_OBJ(basePath + "Torus.obj", scene);
		}

		//if (DrawButton("teapot", ImVec2(geoWidth, geoHeight)))
		//{
		//	ImportModel_OBJ("assets/models/Geometry/teapot.obj", scene);
		//}

		if (DrawButton("Suzanne", ImVec2(geoWidth, geoHeight)))
		{
			ImportModel_OBJ(basePath + "suzanne.obj", scene);
		}

		if (DrawButton("Plane", ImVec2(geoWidth, geoHeight)))
		{
			ImportModel_OBJ(basePath + "plane.obj", scene);
		}



		EndPropertyGrid();
	}

}


void Editor::ImGuiRender_PostProcess()
{
	if (renderComponent.renderPath == nullptr)
		return;


	BeginPropertyGrid();
	PropertyGridSpacing();

	float exposure = renderComponent.renderPath->getExposure();
	if (DrawSliderFloat("Exposure", exposure, 0, 3))
		renderComponent.renderPath->setExposure(exposure);


	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();

	{
		int selectedIdx = renderComponent.renderPath->getAO();
		std::vector<std::string> items =
		{
			"Disabled",
			"SSAO",
			"HBAO",
			"MSAO",
		};
		if (ap::graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			items.push_back("RTAO");

		if (DrawCombo("AO", items, items.size(), &selectedIdx))
		{
			renderComponent.renderPath->setAO((ap::RenderPath3D::AO)selectedIdx);

			switch (renderComponent.renderPath->getAO())
			{
			case ap::RenderPath3D::AO_SSAO:
				renderComponent.renderPath->setAORange(2.0f);
				renderComponent.renderPath->setAOSampleCount(9.0f);
				break;
			case ap::RenderPath3D::AO_RTAO:
				renderComponent.renderPath->setAORange(10.0f);
				break;
			default:
				break;
			}
		}

	}

	float AOPower = renderComponent.renderPath->getAOPower();
	if (DrawSliderFloat("AO Power", AOPower, 0.25f, 8))
		renderComponent.renderPath->setAOPower(AOPower);
	
	float AORange = renderComponent.renderPath->getAORange();
	if (DrawSliderFloat("AO Range", AORange, 1, 100))
		renderComponent.renderPath->setAORange(AORange);

	int AOSampleCount = renderComponent.renderPath->getAOSampleCount();
	if (DrawSliderInt("AO Sample Count", AOSampleCount, 1, 16))
		renderComponent.renderPath->setAOSampleCount(AOSampleCount);


	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();


	bool SSREnabled = renderComponent.renderPath->getSSREnabled();
	if (DrawCheckbox("SSR", SSREnabled))
	{
		renderComponent.renderPath->setSSREnabled(SSREnabled);
	}

	bool RaytracedReflectionEnabled = renderComponent.renderPath->getRaytracedReflectionEnabled();
	if (DrawCheckbox("Ray Traced Reflections", RaytracedReflectionEnabled))
	{
		renderComponent.renderPath->setRaytracedReflectionsEnabled(RaytracedReflectionEnabled);
	}

	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();


	bool EyeAdaptionEnabled = renderComponent.renderPath->getEyeAdaptionEnabled();
	if (DrawCheckbox("EyeAdaption", EyeAdaptionEnabled))
	{
		renderComponent.renderPath->setEyeAdaptionEnabled(EyeAdaptionEnabled);
	}

	float EyeAdaptionKey = renderComponent.renderPath->getEyeAdaptionKey();
	if (DrawSliderFloat("EyeAdaption Key", EyeAdaptionKey, 0.01f, 0.5f))
		renderComponent.renderPath->setEyeAdaptionKey(EyeAdaptionKey);

	float EyeAdaptionRate = renderComponent.renderPath->getEyeAdaptionRate();
	if (DrawSliderFloat("EyeAdaption Rate", EyeAdaptionRate, 0.01f, 4.0f))
		renderComponent.renderPath->setEyeAdaptionRate(EyeAdaptionRate);


	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();

	bool MotionBlurEnabled = renderComponent.renderPath->getMotionBlurEnabled();
	if (DrawCheckbox("MotionBlur", MotionBlurEnabled))
	{
		renderComponent.renderPath->setMotionBlurEnabled(MotionBlurEnabled);
	}

	float MotionBlurStrength = renderComponent.renderPath->getMotionBlurStrength();
	if (DrawSliderFloat("MotionBlur Strength", MotionBlurStrength, 0.1f, 400.0f))
		renderComponent.renderPath->setMotionBlurStrength(MotionBlurStrength);


	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();

	bool DepthOfFieldEnabled = renderComponent.renderPath->getDepthOfFieldEnabled();
	if (DrawCheckbox("DepthOfField", DepthOfFieldEnabled))
	{
		renderComponent.renderPath->setDepthOfFieldEnabled(DepthOfFieldEnabled);
	}

	float DepthOfFieldStrength = renderComponent.renderPath->getDepthOfFieldStrength();
	if (DrawSliderFloat("DepthOfField Strength", DepthOfFieldStrength, 1.0f, 20))
		renderComponent.renderPath->setDepthOfFieldStrength(DepthOfFieldStrength);




	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();

	bool BloomEnabled = renderComponent.renderPath->getBloomEnabled();
	if (DrawCheckbox("Bloom", BloomEnabled))
	{
		renderComponent.renderPath->setBloomEnabled(BloomEnabled);
	}


	float BloomThreshold = renderComponent.renderPath->getBloomThreshold();
	if (DrawSliderFloat("Bloom Threshold", BloomThreshold, 0.0f, 10))
		renderComponent.renderPath->setBloomThreshold(BloomThreshold);



	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();

	bool LensFlareEnabled = renderComponent.renderPath->getLensFlareEnabled();
	if (DrawCheckbox("LensFlare", LensFlareEnabled))
		renderComponent.renderPath->setLensFlareEnabled(LensFlareEnabled);

	bool FXAAEnabled = renderComponent.renderPath->getFXAAEnabled();
	if (DrawCheckbox("FXAA", FXAAEnabled))
		renderComponent.renderPath->setFXAAEnabled(FXAAEnabled);

	bool ColorGradingEnabled = renderComponent.renderPath->getColorGradingEnabled();
	if (DrawCheckbox("Color Grading", ColorGradingEnabled))
		renderComponent.renderPath->setColorGradingEnabled(ColorGradingEnabled);

	bool DitherEnabled = renderComponent.renderPath->getDitherEnabled();
	if (DrawCheckbox("Dithering", DitherEnabled))
		renderComponent.renderPath->setDitherEnabled(DitherEnabled);


	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();


	bool SharpenFilterEnabled = renderComponent.renderPath->getSharpenFilterEnabled();
	if (DrawCheckbox("Sharpen Filter", SharpenFilterEnabled))
		renderComponent.renderPath->setSharpenFilterEnabled(SharpenFilterEnabled);

	float SharpenFilterAmount = renderComponent.renderPath->getSharpenFilterAmount();
	if (DrawSliderFloat("Sharpen Amount", SharpenFilterAmount, 0.0f, 4))
		renderComponent.renderPath->setSharpenFilterAmount(SharpenFilterAmount);



	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();

	bool OutlineEnabled = renderComponent.renderPath->getOutlineEnabled();
	if (DrawCheckbox("Cartoon Outline", OutlineEnabled))
		renderComponent.renderPath->setOutlineEnabled(OutlineEnabled);

	float OutlineThreshold = renderComponent.renderPath->getOutlineThreshold();
	if (DrawSliderFloat("Cartoon Threshold", OutlineThreshold, 0.0f, 1))
		renderComponent.renderPath->setOutlineThreshold(OutlineThreshold);

	float OutlineThickness = renderComponent.renderPath->getOutlineThickness();
	if (DrawSliderFloat("Cartoon Thickness", OutlineThickness, 0.0f, 4))
		renderComponent.renderPath->setOutlineThickness(OutlineThickness);

	PropertyGridSpacing();
	PropertyGridSpacing();
	ImGui::Separator();


	bool ChromaticAberrationEnabled = renderComponent.renderPath->getChromaticAberrationEnabled();
	if (DrawCheckbox("Chromatic Aberration", ChromaticAberrationEnabled))
		renderComponent.renderPath->setChromaticAberrationEnabled(ChromaticAberrationEnabled);

	float ChromaticAberrationAmount = renderComponent.renderPath->getChromaticAberrationAmount();
	if (DrawSliderFloat("Chromatic Amount", ChromaticAberrationAmount, 0.0f, 4))
		renderComponent.renderPath->setChromaticAberrationAmount(ChromaticAberrationAmount);



	PropertyGridSpacing();
	PropertyGridSpacing();
	ImGui::Separator();

	bool FSREnabled = renderComponent.renderPath->getFSREnabled();
	if (DrawCheckbox("FSR", FSREnabled))
		renderComponent.renderPath->setFSREnabled(FSREnabled);

	float FSRSharpness = renderComponent.renderPath->getFSRSharpness();
	if (DrawSliderFloat("FSR Sharpness", FSRSharpness, 0.0f, 2))
		renderComponent.renderPath->setFSRSharpness(FSRSharpness);



	EndPropertyGrid();

}

void Editor::ImGuiRender_ToolBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 0));
	ImGui::Begin("ToolBar", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);




	
	ShiftCursorX(20);
	ImGui::Text("Profiler Enabled");
	ImGui::SameLine();

	ShiftCursorY(-3);
	bool IsEnabled = ap::profiler::IsEnabled();
	if (ImGui::Checkbox(GenerateID(), &IsEnabled))
	{
		ap::profiler::SetEnabled(IsEnabled);
	}

	if (!IsItemDisabled())
		DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

	ImGui::SameLine();


	ShiftCursorX(20);
	ImGui::Text("Physics Simulation");
	ImGui::SameLine();

	
	bool IsSimulationEnabled = ap::physics::IsSimulationEnabled();
	if (ImGui::Checkbox(GenerateID(), &IsSimulationEnabled))
	{
		ap::physics::SetSimulationEnabled(IsSimulationEnabled);
	}

	if (!IsItemDisabled())
		DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);


	ImGui::SameLine();
	ShiftCursorX(20);
	ImGui::Text("Cinema Mode");
	ImGui::SameLine();

	
	if (ImGui::Checkbox(GenerateID(), &isCinema))
	{
		isCinema = true;
		if (renderComponent.renderPath != nullptr)
		{
			renderComponent.renderPath->GetGUI().SetVisible(false);
		}
		renderComponent.GetGUI().SetVisible(false);
		ap::profiler::SetEnabled(false);
		infoDisplay.active = false;
		isEditor = false;
	}

	if (!IsItemDisabled())
		DrawItemActivityOutline(2.0f, true, ap::imguicolor::accent);

		
	





	ImGui::PopStyleVar();
	ImGui::End();
	ImGui::PopStyleVar(2);

}

void Editor::ImGuiRender_Renderer()
{
	ImGui::Begin("Renderer");

	BeginPropertyGrid();
	PropertyGridSpacing();

	


	static int selectedSwapChainIdx =0;

	float gameSpeed = ap::renderer::GetGameSpeed();
	if(DrawSliderFloat("Speed", gameSpeed, 0.00f, 4.0f))
		ap::renderer::SetGameSpeed(gameSpeed);

	bool vsync = swapChain.desc.vsync;
	if (DrawCheckbox("VSync", vsync))
		ap::eventhandler::SetVSync(vsync);

	{

		std::vector<std::string> items =
		{ 
			"SDR 8bit",
			"SDR 10bit",
		};

		if (ap::graphics::GetDevice()->IsSwapChainSupportsHDR(&swapChain))
		{
			items.push_back("HDR 10bit");
			items.push_back("HDR 16bit");

			switch (swapChain.desc.format)
			{
			default:
			case ap::graphics::Format::R8G8B8A8_UNORM:
				selectedSwapChainIdx = 0;
				break;
			case ap::graphics::Format::R10G10B10A2_UNORM:
				if (swapChain.desc.allow_hdr)
				{
					selectedSwapChainIdx = 2;
				}
				else
				{
					selectedSwapChainIdx = 1;
				}
				break;
			case ap::graphics::Format::R16G16B16A16_FLOAT:
				selectedSwapChainIdx = 4;
				break;
			}
		}
		else
		{
			switch (swapChain.desc.format)
			{
			default:
			case ap::graphics::Format::R8G8B8A8_UNORM:
				selectedSwapChainIdx = 0;
				break;
			case ap::graphics::Format::R10G10B10A2_UNORM:
				selectedSwapChainIdx = 1;
				break;
			case ap::graphics::Format::R16G16B16A16_FLOAT:
				selectedSwapChainIdx = 1;
				break;
			}
		}

		if (DrawCombo("Swapchain Format", items, items.size(), &selectedSwapChainIdx))
		{


			switch (selectedSwapChainIdx)
			{
			default:
			case 0:
				swapChain.desc.format = ap::graphics::Format::R8G8B8A8_UNORM;
				swapChain.desc.allow_hdr = false;
				break;
			case 1:
				swapChain.desc.format = ap::graphics::Format::R10G10B10A2_UNORM;
				swapChain.desc.allow_hdr = false;
				break;
			case 2:
				swapChain.desc.format = ap::graphics::Format::R10G10B10A2_UNORM;
				swapChain.desc.allow_hdr = true;
				break;
			case 3:
				swapChain.desc.format = ap::graphics::Format::R16G16B16A16_FLOAT;
				swapChain.desc.allow_hdr = true;
				break;
			}

			bool success = ap::graphics::GetDevice()->CreateSwapChain(&swapChain.desc, nullptr, &swapChain);
			assert(success);

		}




			
	}


	bool OcclusionCullingEnabled = ap::renderer::GetOcclusionCullingEnabled();
	if (DrawCheckbox("Occlusion Culling", OcclusionCullingEnabled))
		ap::renderer::SetOcclusionCullingEnabled(OcclusionCullingEnabled);

	
	float resolutionScale = renderComponent.resolutionScale;
	if (DrawSliderFloat("Resolution Scale", resolutionScale, 0.25f, 2.0f,"%.2f"))
	{
		renderComponent.renderPath->resolutionScale = resolutionScale;
		renderComponent.resolutionScale = resolutionScale;
		renderComponent.ResizeBuffers();
	}

	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();


	bool VoxelRadianceEnabled = ap::renderer::GetVoxelRadianceEnabled();
	if (DrawCheckbox("Voxel GI", VoxelRadianceEnabled))
		ap::renderer::SetVoxelRadianceEnabled(VoxelRadianceEnabled);

	bool VoxelHelper = ap::renderer::GetToDrawVoxelHelper();
	if (DrawCheckbox("Voxel DEBUG", VoxelHelper))
		ap::renderer::SetToDrawVoxelHelper(VoxelHelper);

	bool VoxelRadianceSecondaryBounceEnabled = ap::renderer::GetVoxelRadianceSecondaryBounceEnabled();
	if (DrawCheckbox("Voxel Secondary Bounce", VoxelRadianceSecondaryBounceEnabled))
		ap::renderer::SetVoxelRadianceSecondaryBounceEnabled(VoxelRadianceSecondaryBounceEnabled);

	bool VoxelRadianceReflectionsEnabled = ap::renderer::GetVoxelRadianceReflectionsEnabled();
	if (DrawCheckbox("Voxel Reflections", VoxelRadianceReflectionsEnabled))
		ap::renderer::SetVoxelRadianceReflectionsEnabled(VoxelRadianceReflectionsEnabled);


	float VoxelRadianceVoxelSize = ap::renderer::GetVoxelRadianceVoxelSize();
	if (DrawSliderFloat("Voxel GI Voxel Size", VoxelRadianceVoxelSize, 0.25f, 2.0f))
		ap::renderer::SetVoxelRadianceVoxelSize(VoxelRadianceVoxelSize);

	int VoxelRadianceNumCones = ap::renderer::GetVoxelRadianceNumCones();
	if (DrawSliderInt("Voxel GI NumCones", VoxelRadianceNumCones,1, 16))
		ap::renderer::SetVoxelRadianceNumCones(VoxelRadianceNumCones);

	float VoxelRadianceRayStepSize = ap::renderer::GetVoxelRadianceRayStepSize();
	if (DrawSliderFloat("Voxel GI Ray Step Size", VoxelRadianceRayStepSize, 0.5f, 2.0f))
		ap::renderer::SetVoxelRadianceRayStepSize(VoxelRadianceRayStepSize);

	float VoxelRadianceMaxDistance = ap::renderer::GetVoxelRadianceMaxDistance();
	if (DrawSliderFloat("Voxel GI Max Distance", VoxelRadianceMaxDistance, 0.0f, 100.0f))
		ap::renderer::SetVoxelRadianceMaxDistance(VoxelRadianceMaxDistance);


	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();

	bool WireRender = ap::renderer::IsWireRender();
	if (DrawCheckbox("Render Wireframe", WireRender))
		ap::renderer::SetWireRender(WireRender);

	bool VariableRateShadingClassification = ap::renderer::GetVariableRateShadingClassification();
	if (DrawCheckbox("VRS Classification", VariableRateShadingClassification))
		ap::renderer::SetVariableRateShadingClassification(VariableRateShadingClassification);

	bool VariableRateShadingClassificationDebug = ap::renderer::GetVariableRateShadingClassificationDebug();
	if (DrawCheckbox("VRS DEBUG", VariableRateShadingClassificationDebug))
		ap::renderer::SetVariableRateShadingClassificationDebug(VariableRateShadingClassificationDebug);

	bool AdvancedLightCulling = ap::renderer::GetAdvancedLightCulling();
	if (DrawCheckbox("2.5D Light Culling", AdvancedLightCulling))
		ap::renderer::SetAdvancedLightCulling(AdvancedLightCulling);

	bool DebugLightCulling = ap::renderer::GetDebugLightCulling();
	if (DrawCheckbox("2.5D Light Culling DEBUG", DebugLightCulling))
		ap::renderer::SetDebugLightCulling(DebugLightCulling);

	bool TessellationEnabled = ap::renderer::GetTessellationEnabled();
	if (DrawCheckbox("Tessellation Enabled", TessellationEnabled))
		ap::renderer::SetTessellationEnabled(TessellationEnabled);

	bool TransparentShadowsEnabled = ap::renderer::GetTransparentShadowsEnabled();
	if (DrawCheckbox("Transparent Shadows", TransparentShadowsEnabled))
		ap::renderer::SetTransparentShadowsEnabled(TransparentShadowsEnabled);

	{

		int selectedIdx = ap::renderer::GetRaytracedShadowsEnabled() ?  1:0;

		std::vector<std::string> items =
		{
			"Shadowmaps",
		};
		if(ap::graphics::GetDevice()->CheckCapability(ap::graphics::GraphicsDeviceCapability::RAYTRACING))
			items.push_back("Ray Traced");

		if (DrawCombo("Shadow type", items, items.size(), &selectedIdx))
		{
			switch (selectedIdx)
			{
			case 0:
				ap::renderer::SetRaytracedShadowsEnabled(false);
				break;
			case 1:
				ap::renderer::SetRaytracedShadowsEnabled(true);
				break;
			default:
				break;
			}
		}

	}


	{
		static int selectedIdx = 4;

		const std::vector<std::string> items =
		{
			"Off",
			"128",
			"256",
			"512",
			"1024",
			"2048",
			"4096",
		};

		if (DrawCombo("2D Shadowmap resolution", items, items.size(), &selectedIdx))
		{
			switch (selectedIdx)
			{
			case 0:
				ap::renderer::SetShadowProps2D(0, -1);
				break;
			case 1:
				ap::renderer::SetShadowProps2D(128, -1);
				break;
			case 2:
				ap::renderer::SetShadowProps2D(256, -1);
				break;
			case 3:
				ap::renderer::SetShadowProps2D(512, -1);
				break;
			case 4:
				ap::renderer::SetShadowProps2D(1024, -1);
				break;
			case 5:
				ap::renderer::SetShadowProps2D(2048, -1);
				break;
			case 6:
				ap::renderer::SetShadowProps2D(4096, -1);
				break;
			default:
				break;
			}
		}

	}

	{
		static int selectedIdx = 3;

		const std::vector<std::string> items =
		{
			"Off",
			"128",
			"256",
			"512",
			"1024",
			"2048",
			"4096",
		};

		if (DrawCombo("Cube Shadowmap resolution", items, items.size(), &selectedIdx))
		{
			switch (selectedIdx)
			{
			case 0:
				ap::renderer::SetShadowPropsCube(0, -1);
				break;
			case 1:
				ap::renderer::SetShadowPropsCube(128, -1);
				break;
			case 2:
				ap::renderer::SetShadowPropsCube(256, -1);
				break;
			case 3:
				ap::renderer::SetShadowPropsCube(512, -1);
				break;
			case 4:
				ap::renderer::SetShadowPropsCube(1024, -1);
				break;
			case 5:
				ap::renderer::SetShadowPropsCube(2048, -1);
				break;
			case 6:
				ap::renderer::SetShadowPropsCube(4096, -1);
				break;
			default:
				break;
			}
		}

	}

	{


		int selectedIdx = (renderComponent.renderPath && renderComponent.renderPath->getMSAASampleCount()) ? std::log2(renderComponent.renderPath->getMSAASampleCount()) : 0;

		const std::vector<std::string> items =
		{
			"Off",
			"2",
			"4",
			"8",
			
		};

		if (DrawCombo("MSAA", items, items.size(), &selectedIdx))
		{
			switch (selectedIdx)
			{
			case 0:
				renderComponent.renderPath->setMSAASampleCount(1);
				break;
			case 1:
				renderComponent.renderPath->setMSAASampleCount(2);
				break;
			case 2:
				renderComponent.renderPath->setMSAASampleCount(4);
				break;
			case 3:
				renderComponent.renderPath->setMSAASampleCount(8);
				break;
			default:
				break;
			}
			renderComponent.ResizeBuffers();
		}

	}



	bool TemporalAAEnabled = ap::renderer::GetTemporalAAEnabled();
	if (DrawCheckbox("Temporal AA", TemporalAAEnabled))
		ap::renderer::SetTemporalAAEnabled(TemporalAAEnabled);

	bool TemporalAADebugEnabled = ap::renderer::GetTemporalAADebugEnabled();
	if (DrawCheckbox("Temporal AA DEBUG", TemporalAADebugEnabled))
		ap::renderer::SetTemporalAADebugEnabled(TemporalAADebugEnabled);

	{
		static int selectedIdx = 3;

		const std::vector<std::string> items =
		{
			"Nearest",
			"Bilinear",
			"Trilinear",
			"Anisotropic",

		};

		if (DrawCombo("Texture Quality", items, items.size(), &selectedIdx))
		{

			ap::graphics::SamplerDesc desc = ap::renderer::GetSampler(ap::enums::SAMPLER_OBJECTSHADER)->GetDesc();

			switch (selectedIdx)
			{
			case 0:
				desc.filter = ap::graphics::Filter::MIN_MAG_MIP_POINT;
				break;
			case 1:
				desc.filter = ap::graphics::Filter::MIN_MAG_LINEAR_MIP_POINT;
				break;
			case 2:
				desc.filter = ap::graphics::Filter::MIN_MAG_MIP_LINEAR;
				break;
			case 3:
				desc.filter = ap::graphics::Filter::ANISOTROPIC;
				break;
			default:
				break;
			}
		}

	}

	static float MipBias = 0;
	if (DrawSliderFloat("MipLOD Bias", MipBias, -2, 2.0f))
	{
		ap::graphics::SamplerDesc desc = ap::renderer::GetSampler(ap::enums::SAMPLER_OBJECTSHADER)->GetDesc();
		desc.mip_lod_bias = ap::math::Clamp(MipBias, -15.9f, 15.9f);
		ap::renderer::ModifyObjectSampler(desc);
	}

	int RaytraceBounceCount = ap::renderer::GetRaytraceBounceCount();
	if (DrawSliderInt("Raytrace Bounces", RaytraceBounceCount, 1, 10))
		ap::renderer::SetRaytraceBounceCount(RaytraceBounceCount);

	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();

	{
		bool DebugDrawEnabled = ap::physics::IsDebugDrawEnabled();
		if (DrawCheckbox("Physics visualizer", DebugDrawEnabled))
			ap::physics::SetDebugDrawEnabled(DebugDrawEnabled);

		bool DrawDebugPartitionTree = ap::renderer::GetToDrawDebugPartitionTree();
		if (DrawCheckbox("SPTree visualizer", DrawDebugPartitionTree))
			ap::renderer::SetToDrawDebugPartitionTree(DrawDebugPartitionTree);

		bool DrawDebugBoneLines = ap::renderer::GetToDrawDebugBoneLines();
		if (DrawCheckbox("Bone line visualizer", DrawDebugBoneLines))
			ap::renderer::SetToDrawDebugBoneLines(DrawDebugBoneLines);

		bool DrawDebugEmitters = ap::renderer::GetToDrawDebugEmitters();
		if (DrawCheckbox("Emitter visualizer", DrawDebugEmitters))
			ap::renderer::SetToDrawDebugEmitters(DrawDebugEmitters);

		bool DrawDebugForceFields = ap::renderer::GetToDrawDebugForceFields();
		if (DrawCheckbox("Force Field visualizer", DrawDebugForceFields))
			ap::renderer::SetToDrawDebugForceFields(DrawDebugForceFields);

		bool RaytraceDebugBVHVisualizerEnabled = ap::renderer::GetRaytraceDebugBVHVisualizerEnabled();
		if (DrawCheckbox("Raytrace BVH  visualizer", RaytraceDebugBVHVisualizerEnabled))
			ap::renderer::SetRaytraceDebugBVHVisualizerEnabled(RaytraceDebugBVHVisualizerEnabled);

		bool DrawDebugEnvProbes = ap::renderer::GetToDrawDebugEnvProbes();
		if (DrawCheckbox("Env probe visualizer", DrawDebugEnvProbes))
			ap::renderer::SetToDrawDebugEnvProbes(DrawDebugEnvProbes);

		bool DrawDebugCameras = ap::renderer::GetToDrawDebugCameras();
		if (DrawCheckbox("Camera Proxy visualizer", DrawDebugCameras))
			ap::renderer::SetToDrawDebugCameras(DrawDebugCameras);

		bool DrawGridHelper = ap::renderer::GetToDrawGridHelper();
		if (DrawCheckbox("Grid helper", DrawGridHelper))
			ap::renderer::SetToDrawGridHelper(DrawGridHelper);

	}

	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();

	
	{

		int pickType = renderComponent.pickType;



		const std::vector<std::string> items =
		{
			"Pick Objects",
			"Pick EnvProbes",
			"Pick Lights",
			"Pick Decals",
			"Pick Force Fields",
			"Pick Emitters",
			"Pick Hairs",
			"Pick Cameras",
			"Pick Armatures",
			"Pick Sounds",
		};

		const std::vector<PICKTYPE> items2 =
		{
			PICK_OBJECT       ,
			PICK_LIGHT		  ,
			PICK_DECAL		  ,
			PICK_ENVPROBE	  ,
			PICK_FORCEFIELD	  ,
			PICK_EMITTER	  ,
			PICK_HAIR		  ,
			PICK_CAMERA 	  ,
			PICK_ARMATURE	  ,
			PICK_SOUND		  ,

		};
		assert((items.size() == items2.size()));


		for (int i = 0; i < items.size(); i++)
		{
			bool picked = pickType & items2[i];
			if (DrawCheckbox(items[i].c_str(), picked))
				pickType = picked ? pickType | items2[i] : pickType & ~items2[i];

		}


		renderComponent.pickType = pickType;

	}


	PropertyGridSpacing();
	ImGui::Separator();
	PropertyGridSpacing();


	bool FreezeCullingCameraEnabled = ap::renderer::GetFreezeCullingCameraEnabled();
	if (DrawCheckbox("Freeze culling camera", FreezeCullingCameraEnabled))
		ap::renderer::SetFreezeCullingCameraEnabled(FreezeCullingCameraEnabled);

	bool DisableAlbedoMaps = ap::renderer::IsDisableAlbedoMaps();
	if (DrawCheckbox("Disable albedo maps", DisableAlbedoMaps))
		ap::renderer::SetDisableAlbedoMaps(DisableAlbedoMaps);

	bool ForceDiffuseLighting = ap::renderer::IsForceDiffuseLighting();
	if (DrawCheckbox("Force diffuse lighting", ForceDiffuseLighting))
		ap::renderer::SetForceDiffuseLighting(ForceDiffuseLighting);
	
	EndPropertyGrid();

	ImGui::End();
}

void Editor::ImGuiRender_Terrain()
{
	ImGui::Begin("Terrain");

	static int terrainX = 128;
	static float terrainY = 0.5;
	static int terrainZ = 128;
	const int channelCount = 4;

	MeshComponent* mesh = nullptr;

	static unsigned char* rgb = nullptr;
	static std::string textureName = "";

	BeginPropertyGrid();
	PropertyGridSpacing();


	auto generate_mesh = [](MeshComponent* mesh, int width, int height, unsigned char* rgb = nullptr,
		int channelCount = 4, float heightmap_scale = 1)
	{

		if (mesh == nullptr)
			return;
		mesh->vertex_positions.resize(width * height);
		mesh->vertex_normals.resize(width * height);
		mesh->vertex_colors.resize(width * height);
		mesh->vertex_uvset_0.resize(width * height);
		mesh->vertex_uvset_1.resize(width * height);
		mesh->vertex_atlas.resize(width * height);
		for (int i = 0; i < width; ++i)
		{
			for (int j = 0; j < height; ++j)
			{
				size_t index = size_t(i + j * width);
				mesh->vertex_positions[index] = XMFLOAT3((float)i - (float)width * 0.5f, 0, (float)j - (float)height * 0.5f);
				if (rgb != nullptr)
					mesh->vertex_positions[index].y = ((float)rgb[index * channelCount] - 127.0f) * heightmap_scale;
				mesh->vertex_colors[index] =  ap::Color(255, 0, 0, 0).rgba;
				XMFLOAT2 uv = XMFLOAT2((float)i / (float)width, (float)j / (float)height);
				mesh->vertex_uvset_0[index] = uv;
				mesh->vertex_uvset_1[index] = uv;
				mesh->vertex_atlas[index] = uv;
			}
		}
		mesh->indices.resize((width - 1) * (height - 1) * 6);
		size_t counter = 0;
		for (int x = 0; x < width - 1; x++)
		{
			for (int y = 0; y < height - 1; y++)
			{
				int lowerLeft = x + y * width;
				int lowerRight = (x + 1) + y * width;
				int topLeft = x + (y + 1) * width;
				int topRight = (x + 1) + (y + 1) * width;

				mesh->indices[counter++] = topLeft;
				mesh->indices[counter++] = lowerLeft;
				mesh->indices[counter++] = lowerRight;

				mesh->indices[counter++] = topLeft;
				mesh->indices[counter++] = lowerRight;
				mesh->indices[counter++] = topRight;
			}
		}
		mesh->subsets.back().indexCount = (uint32_t)mesh->indices.size();

		mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH_FAST);
	};

	//terrain
	
	
	
	if (renderComponent.translator.selected.size() == 1)
	{
		Entity ent = renderComponent.translator.selected[0].entity;
		Scene& scene = ap::scene::GetScene();
		ObjectComponent* object =scene.objects.GetComponent(ent);
		if (object)
		{
			mesh =  scene.meshes.GetComponent(object->meshID);
			if (!(mesh && mesh->IsTerrain()))
			{
				mesh == nullptr;
			}
		}
		
	}

	
	
	if (DrawSliderInt("Terrain X", terrainX, 16, 1024))
	{
		generate_mesh(mesh, terrainX, terrainZ, rgb, channelCount, terrainY);
	}
	if (DrawSliderInt("Terrain Z", terrainZ, 16, 1024))
	{
		generate_mesh(mesh, terrainX, terrainZ, rgb, channelCount, terrainY);
	}

	if (DrawSliderFloat("Terrain Y", terrainY, 0, 1.0f))
	{
		generate_mesh(mesh, terrainX, terrainZ, rgb, channelCount, terrainY);
	}

	

	if (DrawButton2("Generate Terrain", true))
	{

		Scene& scene = ap::scene::GetScene();
		Entity entity = scene.Entity_CreateObject("editorTerrain");
		ObjectComponent& object = *scene.objects.GetComponent(entity);
		object.meshID = scene.Entity_CreateMesh("terrainMesh");
		mesh = scene.meshes.GetComponent(object.meshID);
		mesh->SetTerrain(true);
		mesh->subsets.emplace_back();
		mesh->subsets.back().materialID = scene.Entity_CreateMaterial("terrainMaterial");
		mesh->subsets.back().indexOffset = 0;
		MaterialComponent* material = scene.materials.GetComponent(mesh->subsets.back().materialID);
		material->SetUseVertexColors(true);


		ap::scene::PickResult pick;
		pick.entity = entity;
		pick.subsetIndex = 0;
		renderComponent.ClearSelected();
		renderComponent.AddSelected(pick);

		generate_mesh(mesh,terrainX, terrainZ);


	}


	const char* buttonTitle =  "Load HeightMap";

	if (DrawButton2(buttonTitle, true))
	{

		ap::helper::FileDialogParams params;
		params.type = ap::helper::FileDialogParams::OPEN;
		params.description = "Texture";
		params.extensions = ap::resourcemanager::GetSupportedImageExtensions();
		ap::helper::FileDialog(params, [&](std::string fileName) {

				if (rgb != nullptr)
				{
					stbi_image_free(rgb);
					rgb = nullptr;
				}

				int bpp;
				rgb = stbi_load(fileName.c_str(), &terrainX, &terrainZ, &bpp, channelCount);
				textureName = fileName;

				if (mesh == nullptr)
				{
					Scene& scene = ap::scene::GetScene();
					Entity entity = scene.Entity_CreateObject("editorTerrain");
					ObjectComponent& object = *scene.objects.GetComponent(entity);
					object.meshID = scene.Entity_CreateMesh("terrainMesh");
					mesh = scene.meshes.GetComponent(object.meshID);
					mesh->SetTerrain(true);
					mesh->subsets.emplace_back();
					mesh->subsets.back().materialID = scene.Entity_CreateMaterial("terrainMaterial");
					mesh->subsets.back().indexOffset = 0;
					MaterialComponent* material = scene.materials.GetComponent(mesh->subsets.back().materialID);
					material->SetUseVertexColors(true);

					ap::scene::PickResult pick;
					pick.entity = entity;
					pick.subsetIndex = 0;
					renderComponent.ClearSelected();
					renderComponent.AddSelected(pick);

					
				}
				
				generate_mesh(mesh, terrainX, terrainZ, rgb, channelCount, terrainY);
			

	
			});



	}


	EndPropertyGrid();
	ImGui::End();

}



void EditorComponent::ChangeRenderPath(RENDERPATH path)
{
	switch (path)
	{
	case EditorComponent::RENDERPATH_DEFAULT:
		renderPath = std::make_unique<ap::RenderPath3D>();
		break;
	case EditorComponent::RENDERPATH_PATHTRACING:
		renderPath = std::make_unique<ap::RenderPath3D_PathTracing>();
		break;
	default:
		assert(0);
		break;
	}

	renderPath->resolutionScale = resolutionScale;

	renderPath->Load();


}

void EditorComponent::ResizeBuffers()
{
	
	init(main->canvas);
	RenderPath2D::ResizeBuffers();

	GraphicsDevice* device = ap::graphics::GetDevice();

	renderPath->init(*this);
	renderPath->ResizeBuffers();

	if (renderPath->GetDepthStencil() != nullptr)
	{
		bool success = false;

		XMUINT2 internalResolution = GetInternalResolution();

		TextureDesc desc;
		desc.width = internalResolution.x;
		desc.height = internalResolution.y;

		desc.format = Format::R8_UNORM;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
		if (renderPath->getMSAASampleCount() > 1)
		{
			desc.sample_count = renderPath->getMSAASampleCount();
			success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline_MSAA);
			assert(success);
			desc.sample_count = 1;
		}
		success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[0]);
		assert(success);
		success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[1]);
		assert(success);

		{
			RenderPassDesc desc;
			desc.attachments.push_back(RenderPassAttachment::RenderTarget(&rt_selectionOutline[0], RenderPassAttachment::LoadOp::CLEAR));
			if (renderPath->getMSAASampleCount() > 1)
			{
				desc.attachments[0].texture = &rt_selectionOutline_MSAA;
				desc.attachments.push_back(RenderPassAttachment::Resolve(&rt_selectionOutline[0]));
			}
			desc.attachments.push_back(
				RenderPassAttachment::DepthStencil(
					renderPath->GetDepthStencil(),
					RenderPassAttachment::LoadOp::LOAD,
					RenderPassAttachment::StoreOp::STORE,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY,
					ResourceState::DEPTHSTENCIL_READONLY
				)
			);
			success = device->CreateRenderPass(&desc, &renderpass_selectionOutline[0]);
			assert(success);

			if (renderPath->getMSAASampleCount() == 1)
			{
				desc.attachments[0].texture = &rt_selectionOutline[1]; // rendertarget
			}
			else
			{
				desc.attachments[1].texture = &rt_selectionOutline[1]; // resolve
			}
			success = device->CreateRenderPass(&desc, &renderpass_selectionOutline[1]);
			assert(success);
		}
	}

}


void EditorComponent::ResizeLayout()
{
	RenderPath2D::ResizeLayout();
}

void EditorComponent::Load()
{
#ifdef PLATFORM_UWP
	uwp_copy_assets();
#endif // PLATFORM_UWP

	ap::jobsystem::context ctx;
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { pointLightTex = ap::resourcemanager::Load("resources/images/pointlight.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { spotLightTex = ap::resourcemanager::Load("resources/images/spotlight.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { dirLightTex = ap::resourcemanager::Load("resources/images/directional_light.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { areaLightTex = ap::resourcemanager::Load("resources/images/arealight.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { decalTex = ap::resourcemanager::Load("resources/images/decal.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { forceFieldTex = ap::resourcemanager::Load("resources/images/forcefield.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { emitterTex = ap::resourcemanager::Load("resources/images/emitter.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { hairTex = ap::resourcemanager::Load("resources/images/hair.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { cameraTex = ap::resourcemanager::Load("resources/images/camera.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { armatureTex = ap::resourcemanager::Load("resources/images/armature.dds"); });
	ap::jobsystem::Execute(ctx, [this](ap::jobsystem::JobArgs args) { soundTex = ap::resourcemanager::Load("resources/images/sound.dds"); });
	// wait for ctx is at the end of this function!

	translator.Create();
	translator.enabled = false;

	ap::jobsystem::Wait(ctx);


	//
	ChangeRenderPath(RENDERPATH_DEFAULT);

	ap::renderer::SetToDrawDebugEnvProbes(true);
	ap::renderer::SetToDrawGridHelper(true);
	//ap::renderer::SetToDrawDebugCameras(true);



	RenderPath2D::Load();
}

void EditorComponent::Start()
{
	RenderPath2D::Start();
}

void EditorComponent::PreUpdate()
{
	RenderPath2D::PreUpdate();

	renderPath->PreUpdate();
}

void EditorComponent::FixedUpdate()
{
	RenderPath2D::FixedUpdate();

	renderPath->FixedUpdate();
}


void EditorComponent::Update(float dt)
{
	ap::profiler::range_id profrange = ap::profiler::BeginRangeCPU("Editor Update");

	
	

	

	if (mainCamera == ap::ecs::INVALID_ENTITY || !ap::scene::GetScene().cameras.Contains(mainCamera))
	{
		if (ap::scene::GetScene().cameras.GetCount() > 0)
		{
			mainCamera = ap::scene::GetScene().cameras.GetEntity(0);
		}
		else
		{
			mainCamera = ap::scene::GetScene().Entity_CreateCamera("camera", GetLogicalWidth(), GetLogicalHeight());
			ap::scene::GetScene().transforms.GetComponent(mainCamera)->Translate(XMFLOAT3(0, 2, -10));

		}

	}

	


	Scene& scene = ap::scene::GetScene();
	CameraComponent& camera = *scene.cameras.GetComponent(mainCamera);
	TransformComponent* cameraTransform = scene.transforms.GetComponent(mainCamera);
	assert(cameraTransform != nullptr);


	if (scene.weathers.GetCount() == 0)
	{
		WeatherComponent& weather = scene.weathers.Create(CreateEntity());

		weather.ambient = XMFLOAT3(33.0f / 255.0f, 47.0f / 255.0f, 127.0f / 255.0f);
		weather.horizon = XMFLOAT3(101.0f / 255.0f, 101.0f / 255.0f, 227.0f / 255.0f);
		weather.zenith = XMFLOAT3(99.0f / 255.0f, 133.0f / 255.0f, 255.0f / 255.0f);
		weather.cloudiness = 0.4f;
		weather.fogStart = 100;
		weather.fogEnd = 1000;
		weather.fogHeightSky = 0;
	}



	main->paintToolPanel.Update(dt);

	selectionOutlineTimer += dt;

	if (main->viewportHovered || main->isCinema)
	{


		bool clear_selected = false;
		if (ap::input::Press(ap::input::KEYBOARD_BUTTON_ESCAPE))
		{
			if (main->isCinema)
			{
				// Exit cinema mode:
				if (renderPath != nullptr)
				{
					renderPath->GetGUI().SetVisible(true);
				}
				GetGUI().SetVisible(true);
				main->infoDisplay.active = true;

				main->isCinema = false;
				main->isEditor = true;
			}
			else
			{
				clear_selected = true;
			}
		}



		//임시
		static float rotationSpeed = 1.0f;



		// Camera control:
		XMFLOAT4 editorMouse = ap::input::GetPointer();
		float screenW = GetLogicalWidth();
		float screenH = GetLogicalHeight();
		float ratioX = screenW / (main->viewportSize.x );
		float ratioY = screenH / (main->viewportSize.y );

		POINT p;
		p.x = main->viewportBounds[0].x;
		p.y = main->viewportBounds[0].y;
		ScreenToClient(main->window, &p);

		editorMouse.x = editorMouse.x - p.x;
		editorMouse.y = editorMouse.y - p.y;
		editorMouse.x *= ratioX;
		editorMouse.y *= ratioY;

		
		XMFLOAT4 currentMouse = editorMouse;



		if (!ap::backlog::isActive() && !GetGUI().HasFocus())
		{
			static XMFLOAT4 originalMouse = XMFLOAT4(0, 0, 0, 0);
			static bool camControlStart = true;
			if (camControlStart)
			{
				originalMouse = ap::input::GetPointer();
			}

			float xDif = 0, yDif = 0;

			if (ap::input::Down(ap::input::MOUSE_BUTTON_MIDDLE))
			{
				camControlStart = false;
#if 0
				// Mouse delta from previous frame:
				xDif = currentMouse.x - originalMouse.x;
				yDif = currentMouse.y - originalMouse.y;
#else
				// Mouse delta from hardware read:
				xDif = ap::input::GetMouseState().delta_position.x;
				yDif = ap::input::GetMouseState().delta_position.y;
#endif
				xDif = 0.1f * xDif * (1.0f / 60.0f);
				yDif = 0.1f * yDif * (1.0f / 60.0f);
				ap::input::SetPointer(originalMouse);
				ap::input::HidePointer(true);
			}
			else
			{
				camControlStart = true;
				ap::input::HidePointer(false);
			}

			const float buttonrotSpeed = 2.0f * dt;
			if (ap::input::Down(ap::input::KEYBOARD_BUTTON_LEFT))
			{
				xDif -= buttonrotSpeed;
			}
			if (ap::input::Down(ap::input::KEYBOARD_BUTTON_RIGHT))
			{
				xDif += buttonrotSpeed;
			}
			if (ap::input::Down(ap::input::KEYBOARD_BUTTON_UP))
			{
				yDif -= buttonrotSpeed;
			}
			if (ap::input::Down(ap::input::KEYBOARD_BUTTON_DOWN))
			{
				yDif += buttonrotSpeed;
			}

			const XMFLOAT4 leftStick = ap::input::GetAnalog(ap::input::GAMEPAD_ANALOG_THUMBSTICK_L, 0);
			const XMFLOAT4 rightStick = ap::input::GetAnalog(ap::input::GAMEPAD_ANALOG_THUMBSTICK_R, 0);
			const XMFLOAT4 rightTrigger = ap::input::GetAnalog(ap::input::GAMEPAD_ANALOG_TRIGGER_R, 0);

			const float jostickrotspeed = 0.05f;
			xDif += rightStick.x * jostickrotspeed;
			yDif += rightStick.y * jostickrotspeed;

			xDif *= rotationSpeed;
			yDif *= rotationSpeed;


			//camera 업데이트
			if (1)
			{
				//임시
				static XMFLOAT3 _move = {};
				static float acceleration = 0.18f;
				static float movementSpeed = 10.0f;

				// FPS Camera
				const float clampedDT = std::min(dt, 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...

				const float speed = ((ap::input::Down(ap::input::KEYBOARD_BUTTON_LSHIFT) ? 10.0f : 1.0f) + rightTrigger.x * 10.0f) * clampedDT * movementSpeed;
				XMVECTOR move = XMLoadFloat3(&_move);
				XMVECTOR moveNew = XMVectorSet(leftStick.x, 0, leftStick.y, 0);

				if (!ap::input::Down(ap::input::KEYBOARD_BUTTON_LCONTROL) && ap::input::Down(ap::input::MOUSE_BUTTON_MIDDLE))
				{
					// Only move camera if control not pressed
					if (ap::input::Down((ap::input::BUTTON)'A') || ap::input::Down(ap::input::GAMEPAD_BUTTON_LEFT)) { moveNew += XMVectorSet(-1, 0, 0, 0); }
					if (ap::input::Down((ap::input::BUTTON)'D') || ap::input::Down(ap::input::GAMEPAD_BUTTON_RIGHT)) { moveNew += XMVectorSet(1, 0, 0, 0); }
					if (ap::input::Down((ap::input::BUTTON)'W') || ap::input::Down(ap::input::GAMEPAD_BUTTON_UP)) { moveNew += XMVectorSet(0, 0, 1, 0); }
					if (ap::input::Down((ap::input::BUTTON)'S') || ap::input::Down(ap::input::GAMEPAD_BUTTON_DOWN)) { moveNew += XMVectorSet(0, 0, -1, 0); }
					if (ap::input::Down((ap::input::BUTTON)'E') || ap::input::Down(ap::input::GAMEPAD_BUTTON_2)) { moveNew += XMVectorSet(0, 1, 0, 0); }
					if (ap::input::Down((ap::input::BUTTON)'Q') || ap::input::Down(ap::input::GAMEPAD_BUTTON_1)) { moveNew += XMVectorSet(0, -1, 0, 0); }
					moveNew += XMVector3Normalize(moveNew);
				}
				else if(!ap::input::Down(ap::input::KEYBOARD_BUTTON_LCONTROL))
				{
					if (ap::input::Down((ap::input::BUTTON)'Q'))
						translator.imGizmoType = (ImGuizmo::OPERATION)-1;
					if (ap::input::Down((ap::input::BUTTON)'W'))
						translator.imGizmoType = ImGuizmo::OPERATION::TRANSLATE;
					if (ap::input::Down((ap::input::BUTTON)'E'))
						translator.imGizmoType = ImGuizmo::OPERATION::ROTATE;
					if (ap::input::Down((ap::input::BUTTON)'R'))
						translator.imGizmoType = ImGuizmo::OPERATION::SCALE;
				}

				moveNew *= speed;

				move = XMVectorLerp(move, moveNew, acceleration * clampedDT / 0.0166f); // smooth the movement a bit
				float moveLength = XMVectorGetX(XMVector3Length(move));

				if (moveLength < 0.0001f)
				{
					move = XMVectorSet(0, 0, 0, 0);
				}

				if (abs(xDif) + abs(yDif) > 0 || moveLength > 0.0001f)
				{
					XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&cameraTransform->rotation_local));
					XMVECTOR move_rot = XMVector3TransformNormal(move, camRot);
					XMFLOAT3 _move;
					XMStoreFloat3(&_move, move_rot);
					cameraTransform->Translate(_move);
					cameraTransform->RotateRollPitchYaw(XMFLOAT3(yDif, xDif, 0));
					camera.SetDirty();
				}

				cameraTransform->UpdateTransform();
				XMStoreFloat3(&_move, move);
			}
			else
			{
				// Orbital Camera

				if (ap::input::Down(ap::input::KEYBOARD_BUTTON_LSHIFT))
				{
					XMVECTOR V = XMVectorAdd(camera.GetRight() * xDif, camera.GetUp() * yDif) * 10;
					XMFLOAT3 vec;
					XMStoreFloat3(&vec, V);
					cameraTransform->Translate(vec);
				}
				else if (ap::input::Down(ap::input::KEYBOARD_BUTTON_LCONTROL) || currentMouse.z != 0.0f)
				{
					cameraTransform->Translate(XMFLOAT3(0, 0, yDif * 4 + currentMouse.z));
					cameraTransform->translation_local.z = std::min(0.0f, cameraTransform->translation_local.z);
					camera.SetDirty();
				}
				else if (abs(xDif) + abs(yDif) > 0)
				{
					cameraTransform->RotateRollPitchYaw(XMFLOAT3(yDif * 2, xDif * 2, 0));
					camera.SetDirty();
				}

				cameraTransform->UpdateTransform();
			}



			// Begin picking:
			unsigned int pickMask = pickType;
			Ray pickRay = ap::renderer::GetPickRay((long)currentMouse.x, (long)currentMouse.y, *this);
			{
				hovered = ap::scene::PickResult();

				if (pickMask & PICK_LIGHT)
				{
					for (size_t i = 0; i < scene.lights.GetCount(); ++i)
					{
						Entity entity = scene.lights.GetEntity(i);
						const TransformComponent& transform = *scene.transforms.GetComponent(entity);

						XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
						float dis = XMVectorGetX(disV);
						if (dis > 0.01f && dis < ap::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
						{
							hovered = ap::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
				if (pickMask & PICK_DECAL)
				{
					for (size_t i = 0; i < scene.decals.GetCount(); ++i)
					{
						Entity entity = scene.decals.GetEntity(i);
						const TransformComponent& transform = *scene.transforms.GetComponent(entity);

						XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
						float dis = XMVectorGetX(disV);
						if (dis > 0.01f && dis < ap::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
						{
							hovered = ap::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
				if (pickMask & PICK_FORCEFIELD)
				{
					for (size_t i = 0; i < scene.forces.GetCount(); ++i)
					{
						Entity entity = scene.forces.GetEntity(i);
						const TransformComponent& transform = *scene.transforms.GetComponent(entity);

						XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
						float dis = XMVectorGetX(disV);
						if (dis > 0.01f && dis < ap::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
						{
							hovered = ap::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
				if (pickMask & PICK_EMITTER)
				{
					for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
					{
						Entity entity = scene.emitters.GetEntity(i);
						const TransformComponent& transform = *scene.transforms.GetComponent(entity);

						XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
						float dis = XMVectorGetX(disV);
						if (dis > 0.01f && dis < ap::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
						{
							hovered = ap::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
				if (pickMask & PICK_HAIR)
				{
					for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
					{
						Entity entity = scene.hairs.GetEntity(i);
						const TransformComponent& transform = *scene.transforms.GetComponent(entity);

						XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
						float dis = XMVectorGetX(disV);
						if (dis > 0.01f && dis < ap::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
						{
							hovered = ap::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
				if (pickMask & PICK_ENVPROBE)
				{
					for (size_t i = 0; i < scene.probes.GetCount(); ++i)
					{
						Entity entity = scene.probes.GetEntity(i);
						const TransformComponent& transform = *scene.transforms.GetComponent(entity);

						if (Sphere(transform.GetPosition(), 1).intersects(pickRay))
						{
							float dis = ap::math::Distance(transform.GetPosition(), pickRay.origin);
							if (dis < hovered.distance)
							{
								hovered = ap::scene::PickResult();
								hovered.entity = entity;
								hovered.distance = dis;
							}
						}
					}
				}
				if (pickMask & PICK_CAMERA)
				{
					for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
					{
						Entity entity = scene.cameras.GetEntity(i);

						const TransformComponent& transform = *scene.transforms.GetComponent(entity);

						XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
						float dis = XMVectorGetX(disV);
						if (dis > 0.01f && dis < ap::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
						{
							hovered = ap::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
				if (pickMask & PICK_ARMATURE)
				{
					for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
					{
						Entity entity = scene.armatures.GetEntity(i);
						const TransformComponent& transform = *scene.transforms.GetComponent(entity);

						XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
						float dis = XMVectorGetX(disV);
						if (dis > 0.01f && dis < ap::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
						{
							hovered = ap::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
				if (pickMask & PICK_SOUND)
				{
					for (size_t i = 0; i < scene.sounds.GetCount(); ++i)
					{
						Entity entity = scene.sounds.GetEntity(i);
						const TransformComponent& transform = *scene.transforms.GetComponent(entity);

						XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
						float dis = XMVectorGetX(disV);
						if (dis > 0.01f && dis < ap::math::Distance(transform.GetPosition(), pickRay.origin) * 0.05f && dis < hovered.distance)
						{
							hovered = ap::scene::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}

				if (pickMask & PICK_OBJECT && hovered.entity == INVALID_ENTITY)
				{
					// Object picking only when mouse button down, because it can be slow with high polycount
					if (
						ap::input::Down(ap::input::MOUSE_BUTTON_LEFT) ||
						ap::input::Down(ap::input::MOUSE_BUTTON_RIGHT) ||
						main->paintToolPanel.GetMode()  !=   0
						)
					{
						hovered = ap::scene::Pick(pickRay, pickMask);
					}
				}
			}

			// Interactions only when paint tool is disabled:
			if (main->paintToolPanel.GetMode() == 0)
			{
				// Interact:
				if (hovered.entity != INVALID_ENTITY)
				{
					//const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
					//if (object != nullptr)
					//{
					//	if (translator.selected.empty() && object->GetRenderTypes() & ap::enums::RENDERTYPE_WATER)
					//	{
					//		if (ap::input::Down(ap::input::MOUSE_BUTTON_LEFT))
					//		{
					//			// if water, then put a water ripple onto it:
					//			scene.PutWaterRipple("images/ripple.png", hovered.position);
					//		}
					//	}
					//	else if (decalWnd.placementCheckBox.GetCheck() && ap::input::Press(ap::input::MOUSE_BUTTON_LEFT))
					//	{
					//		// if not water or softbody, put a decal on it:
					//		static int decalselector = 0;
					//		decalselector = (decalselector + 1) % 2;
					//		Entity entity = scene.Entity_CreateDecal("editorDecal", (decalselector == 0 ? "images/leaf.dds" : "images/blood1.png"));
					//		TransformComponent& transform = *scene.transforms.GetComponent(entity);
					//		transform.MatrixTransform(hovered.orientation);
					//		transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
					//		transform.Scale(XMFLOAT3(2, 2, 2));
					//		scene.Component_Attach(entity, hovered.entity);

					//		RefreshSceneGraphView();
					//	}
					//}

				}
			}

			// Select...
			static bool selectAll = false;
			if (ap::input::Press(ap::input::MOUSE_BUTTON_RIGHT) || selectAll || clear_selected)
			{

				ap::Archive& archive = AdvanceHistory();
				archive << HISTORYOP_SELECTION;
				// record PREVIOUS selection state...
				archive << translator.selected.size();
				for (auto& x : translator.selected)
				{
					archive << x.entity;
					archive << x.position;
					archive << x.normal;
					archive << x.subsetIndex;
					archive << x.distance;
				}

				if (selectAll)
				{
					// Add everything to selection:
					selectAll = false;
					ClearSelected();

					for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
					{
						Entity entity = scene.transforms.GetEntity(i);
						auto* hier = scene.hierarchy.GetComponent(entity);
						if ((hier && hier->parentID != ap::ecs::INVALID_ENTITY) || entity == mainCamera)
						{
							
							// Parented objects won't be attached, but only the parents instead. Otherwise it would cause "double translation"
							continue;
						}
						ap::scene::PickResult picked;
						picked.entity = entity;
						AddSelected(picked);
					}
				}
				else if (hovered.entity != INVALID_ENTITY)
				{
					// Add the hovered item to the selection:

					if (!translator.selected.empty() && ap::input::Down(ap::input::KEYBOARD_BUTTON_LSHIFT))
					{
						// Union selection:
						ap::vector<ap::scene::PickResult> saved = translator.selected;
						translator.selected.clear();
						for (const ap::scene::PickResult& picked : saved)
						{
							AddSelected(picked);
						}
						AddSelected(hovered);
					}
					else
					{
						// Replace selection:
						translator.selected.clear();
						AddSelected(hovered);
					}
				}
				else
				{
					clear_selected = true;
				}

				if (clear_selected)
				{
					ClearSelected();
				}


				// record NEW selection state...
				archive << translator.selected.size();
				for (auto& x : translator.selected)
				{
					archive << x.entity;
					archive << x.position;
					archive << x.normal;
					archive << x.subsetIndex;
					archive << x.distance;
				}


			}



			if(main->viewportHovered)
			{

				// Control operations...
				if (ap::input::Down(ap::input::KEYBOARD_BUTTON_LCONTROL))
				{
					// Select All
					if (ap::input::Press((ap::input::BUTTON)'A'))
					{
						selectAll = true;
					}
					// Copy
					if (ap::input::Press((ap::input::BUTTON)'C'))
					{
						auto prevSel = translator.selected;

						clipboard.SetReadModeAndResetPos(false);
						clipboard << prevSel.size();
						for (auto& x : prevSel)
						{
							scene.Entity_Serialize(clipboard, x.entity);
						}
					}
					// Paste
					if (ap::input::Press((ap::input::BUTTON)'V'))
					{
						auto prevSel = translator.selected;
						translator.selected.clear();

						clipboard.SetReadModeAndResetPos(true);
						size_t count;
						clipboard >> count;
						for (size_t i = 0; i < count; ++i)
						{
							ap::scene::PickResult picked;
							picked.entity = scene.Entity_Serialize(clipboard);
							AddSelected(picked);
						}


					}
					// Duplicate Instances
					if (ap::input::Press((ap::input::BUTTON)'D'))
					{
						auto prevSel = translator.selected;
						translator.selected.clear();
						for (auto& x : prevSel)
						{
							ap::scene::PickResult picked;
							picked.entity = scene.Entity_Duplicate(x.entity);
							AddSelected(picked);
						}


					}
					// Put Instances
					if (clipboard.IsOpen() && hovered.subsetIndex >= 0 && ap::input::Down(ap::input::KEYBOARD_BUTTON_LSHIFT) && ap::input::Press(ap::input::MOUSE_BUTTON_LEFT))
					{
						clipboard.SetReadModeAndResetPos(true);
						size_t count;
						clipboard >> count;
						for (size_t i = 0; i < count; ++i)
						{
							Entity entity = scene.Entity_Serialize(clipboard);
							TransformComponent* transform = scene.transforms.GetComponent(entity);
							if (transform != nullptr)
							{
								transform->translation_local = {};
								//transform->MatrixTransform(hovered.orientation);
								transform->Translate(hovered.position);
							}
						}


					}
					// Undo
					if (ap::input::Press((ap::input::BUTTON)'Z'))
					{
						ConsumeHistoryOperation(true);


					}
					// Redo
					if (ap::input::Press((ap::input::BUTTON)'Y'))
					{
						ConsumeHistoryOperation(false);


					}
				}


				// Delete
				DeleteSelectedEntities();

				
				if (translator.selected.empty())
				{
					main->paintToolPanel.SetEntity(INVALID_ENTITY);
				}
				else
				{
					const ap::scene::PickResult& picked = translator.selected.back();

					main->paintToolPanel.SetEntity(picked.entity, picked.subsetIndex);
				}


			}

		}


		



	}

	camera = *scene.cameras.GetComponent(mainCamera);
	cameraTransform = scene.transforms.GetComponent(mainCamera);

	// Update MainCamera
	camera.TransformCamera(*cameraTransform);
	camera.UpdateCamera();
	ap::scene::GetCamera() = camera;



	// Clear highlite state:
	for (size_t i = 0; i < scene.materials.GetCount(); ++i)
	{
		scene.materials[i].SetUserStencilRef(EDITORSTENCILREF_CLEAR);
	}
	for (size_t i = 0; i < scene.objects.GetCount(); ++i)
	{
		scene.objects[i].SetUserStencilRef(EDITORSTENCILREF_CLEAR);
	}
	for (auto& x : translator.selected)
	{
		ObjectComponent* object = scene.objects.GetComponent(x.entity);
		if (object != nullptr) // maybe it was deleted...
		{
			object->SetUserStencilRef(EDITORSTENCILREF_HIGHLIGHT_OBJECT);
			if (x.subsetIndex >= 0)
			{
				const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh != nullptr && (int)mesh->subsets.size() > x.subsetIndex)
				{
					MaterialComponent* material = scene.materials.GetComponent(mesh->subsets[x.subsetIndex].materialID);
					if (material != nullptr)
					{
						material->SetUserStencilRef(EDITORSTENCILREF_HIGHLIGHT_MATERIAL);
					}
				}
			}
		}
	}

	
	

	//camera.TransformCamera(cameraWnd.camera_transform);
	//camera.UpdateCamera();


	

	ap::profiler::EndRange(profrange);

	RenderPath2D::Update(dt);

	renderPath->colorspace = colorspace;
	renderPath->Update(dt);
}

void EditorComponent::PostUpdate()
{
	RenderPath2D::PostUpdate();

	renderPath->PostUpdate();
}

void EditorComponent::Render() const
{
	Scene& scene = ap::scene::GetScene();

	// Hovered item boxes:
	if (1)
	{
		if (hovered.entity != INVALID_ENTITY)
		{
			const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
			if (object != nullptr)
			{
				const AABB& aabb = *scene.aabb_objects.GetComponent(hovered.entity);

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				ap::renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const LightComponent* light = scene.lights.GetComponent(hovered.entity);
			if (light != nullptr)
			{
				const AABB& aabb = *scene.aabb_lights.GetComponent(hovered.entity);

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				ap::renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0, 0.5f));
			}

			const DecalComponent* decal = scene.decals.GetComponent(hovered.entity);
			if (decal != nullptr)
			{
				ap::renderer::DrawBox(decal->world, XMFLOAT4(0.5f, 0, 0.5f, 0.5f));
			}

			const EnvironmentProbeComponent* probe = scene.probes.GetComponent(hovered.entity);
			if (probe != nullptr)
			{
				const AABB& aabb = *scene.aabb_probes.GetComponent(hovered.entity);

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				ap::renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const ap::HairParticleSystem* hair = scene.hairs.GetComponent(hovered.entity);
			if (hair != nullptr)
			{
				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, hair->aabb.getAsBoxMatrix());
				ap::renderer::DrawBox(hoverBox, XMFLOAT4(0, 0.5f, 0, 0.5f));
			}
		}

		
	}

	// Selected items box:
	if (!translator.selected.empty())
	{
		AABB selectedAABB = AABB(
			XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
			XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()));
		for (auto& picked : translator.selected)
		{
			if (picked.entity != INVALID_ENTITY)
			{
				const ObjectComponent* object = scene.objects.GetComponent(picked.entity);
				if (object != nullptr)
				{
					const AABB& aabb = *scene.aabb_objects.GetComponent(picked.entity);
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const LightComponent* light = scene.lights.GetComponent(picked.entity);
				if (light != nullptr)
				{
					const AABB& aabb = *scene.aabb_lights.GetComponent(picked.entity);
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const DecalComponent* decal = scene.decals.GetComponent(picked.entity);
				if (decal != nullptr)
				{
					const AABB& aabb = *scene.aabb_decals.GetComponent(picked.entity);
					selectedAABB = AABB::Merge(selectedAABB, aabb);

					// also display decal OBB:
					XMFLOAT4X4 selectionBox;
					selectionBox = decal->world;
					ap::renderer::DrawBox(selectionBox, XMFLOAT4(1, 0, 1, 1));
				}

				const EnvironmentProbeComponent* probe = scene.probes.GetComponent(picked.entity);
				if (probe != nullptr)
				{
					const AABB& aabb = *scene.aabb_probes.GetComponent(picked.entity);
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const ap::HairParticleSystem* hair = scene.hairs.GetComponent(picked.entity);
				if (hair != nullptr)
				{
					selectedAABB = AABB::Merge(selectedAABB, hair->aabb);
				}

			}
		}

		XMFLOAT4X4 selectionBox;
		XMStoreFloat4x4(&selectionBox, selectedAABB.getAsBoxMatrix());
		ap::renderer::DrawBox(selectionBox, XMFLOAT4(1, 1, 1, 1));
	}

	main->paintToolPanel.DrawBrush();


	renderPath->Render();

	// Selection outline:
	if (renderPath->GetDepthStencil() != nullptr && !translator.selected.empty())
	{
		GraphicsDevice* device = ap::graphics::GetDevice();
		CommandList cmd = device->BeginCommandList();

		device->EventBegin("Editor - Selection Outline Mask", cmd);

		Viewport vp;
		vp.width = (float)rt_selectionOutline[0].GetDesc().width;
		vp.height = (float)rt_selectionOutline[0].GetDesc().height;
		device->BindViewports(1, &vp, cmd);

		ap::image::Params fx;
		fx.enableFullScreen();
		fx.stencilComp = ap::image::STENCILMODE::STENCILMODE_EQUAL;

		// We will specify the stencil ref in user-space, don't care about engine stencil refs here:
		//	Otherwise would need to take into account engine ref and draw multiple permutations of stencil refs.
		fx.stencilRefMode = ap::image::STENCILREFMODE_USER;

		// Materials outline:
		{
			device->RenderPassBegin(&renderpass_selectionOutline[0], cmd);

			// Draw solid blocks of selected materials
			fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_MATERIAL;
			ap::image::Draw(ap::texturehelper::getWhite(), fx, cmd);

			device->RenderPassEnd(cmd);
		}

		// Objects outline:
		{
			device->RenderPassBegin(&renderpass_selectionOutline[1], cmd);

			// Draw solid blocks of selected objects
			fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT;
			ap::image::Draw(ap::texturehelper::getWhite(), fx, cmd);

			device->RenderPassEnd(cmd);
		}

		device->EventEnd(cmd);
	}

	RenderPath2D::Render();

}

void EditorComponent::Compose(CommandList cmd) const
{
	renderPath->Compose(cmd);

	if (main->isCinema)
	{
		return;
	}

	// Draw selection outline to the screen:
	const float selectionColorIntensity = std::sin(selectionOutlineTimer * XM_2PI * 0.8f) * 0.5f + 0.5f;
	if (renderPath->GetDepthStencil() != nullptr && !translator.selected.empty())
	{
		GraphicsDevice* device = ap::graphics::GetDevice();
		device->EventBegin("Editor - Selection Outline", cmd);
		ap::renderer::BindCommonResources(cmd);
		float opacity = ap::math::Lerp(0.4f, 1.0f, selectionColorIntensity);
		XMFLOAT4 col = selectionColor2;
		col.w *= opacity;
		ap::renderer::Postprocess_Outline(rt_selectionOutline[0], cmd, 0.1f, 1, col);
		col = selectionColor;
		col.w *= opacity;
		ap::renderer::Postprocess_Outline(rt_selectionOutline[1], cmd, 0.1f, 1, col);
		device->EventEnd(cmd);
	}

	const CameraComponent& camera = ap::scene::GetCamera();

	Scene& scene = ap::scene::GetScene();

	const ap::Color inactiveEntityColor = ap::Color::fromFloat4(XMFLOAT4(1, 1, 1, 0.5f));
	const ap::Color hoveredEntityColor = ap::Color::fromFloat4(XMFLOAT4(1, 1, 1, 1));
	const XMFLOAT4 glow = ap::math::Lerp(ap::math::Lerp(XMFLOAT4(1, 1, 1, 1), selectionColor, 0.4f), selectionColor, selectionColorIntensity);
	const ap::Color selectedEntityColor = ap::Color::fromFloat4(glow);

	// remove camera jittering
	CameraComponent cam = *renderPath->camera;
	cam.jitter = XMFLOAT2(0, 0);
	cam.UpdateCamera();
	const XMMATRIX VP = cam.GetViewProjection();

	const XMMATRIX R = XMLoadFloat3x3(&cam.rotationMatrix);

	ap::image::Params fx;
	fx.customRotation = &R;
	fx.customProjection = &VP;

	if (pickType & PICK_LIGHT)
	{
		for (size_t i = 0; i < scene.lights.GetCount(); ++i)
		{
			const LightComponent& light = scene.lights[i];
			Entity entity = scene.lights.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = ap::math::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}

			switch (light.GetType())
			{
			case LightComponent::POINT:
				ap::image::Draw(&pointLightTex.GetTexture(), fx, cmd);
				break;
			case LightComponent::SPOT:
				ap::image::Draw(&spotLightTex.GetTexture(), fx, cmd);
				break;
			case LightComponent::DIRECTIONAL:
				ap::image::Draw(&dirLightTex.GetTexture(), fx, cmd);
				break;
			default:
				ap::image::Draw(&areaLightTex.GetTexture(), fx, cmd);
				break;
			}
		}
	}


	if (pickType & PICK_DECAL)
	{
		for (size_t i = 0; i < scene.decals.GetCount(); ++i)
		{
			Entity entity = scene.decals.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = ap::math::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			ap::image::Draw(&decalTex.GetTexture(), fx, cmd);

		}
	}

	if (pickType & PICK_FORCEFIELD)
	{
		for (size_t i = 0; i < scene.forces.GetCount(); ++i)
		{
			Entity entity = scene.forces.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = ap::math::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			ap::image::Draw(&forceFieldTex.GetTexture(), fx, cmd);
		}
	}

	if (pickType & PICK_CAMERA)
	{
		for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
		{
			Entity entity = scene.cameras.GetEntity(i);

			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = ap::math::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			ap::image::Draw(&cameraTex.GetTexture(), fx, cmd);
		}
	}

	if (pickType & PICK_ARMATURE)
	{
		for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
		{
			Entity entity = scene.armatures.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = ap::math::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			ap::image::Draw(&armatureTex.GetTexture(), fx, cmd);
		}
	}

	if (pickType & PICK_EMITTER)
	{
		for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
		{
			Entity entity = scene.emitters.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = ap::math::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			ap::image::Draw(&emitterTex.GetTexture(), fx, cmd);
		}
	}

	if (pickType & PICK_HAIR)
	{
		for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
		{
			Entity entity = scene.hairs.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = ap::math::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			ap::image::Draw(&hairTex.GetTexture(), fx, cmd);
		}
	}

	if (pickType & PICK_SOUND)
	{
		for (size_t i = 0; i < scene.sounds.GetCount(); ++i)
		{
			Entity entity = scene.sounds.GetEntity(i);
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);

			float dist = ap::math::Distance(transform.GetPosition(), camera.Eye) * 0.08f;

			fx.pos = transform.GetPosition();
			fx.siz = XMFLOAT2(dist, dist);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			fx.color = inactiveEntityColor;

			if (hovered.entity == entity)
			{
				fx.color = hoveredEntityColor;
			}
			for (auto& picked : translator.selected)
			{
				if (picked.entity == entity)
				{
					fx.color = selectedEntityColor;
					break;
				}
			}


			ap::image::Draw(&soundTex.GetTexture(), fx, cmd);
		}
	}


	RenderPath2D::Compose(cmd);
}


void EditorComponent::ClearSelected()
{
	translator.selected.clear();
	
}
void EditorComponent::AddSelected(Entity entity)
{
	ap::scene::PickResult res;
	res.entity = entity;
	AddSelected(res);
}
void EditorComponent::AddSelected(const PickResult& picked)
{
	bool removal = false;
	for (size_t i = 0; i < translator.selected.size(); ++i)
	{
		if (translator.selected[i] == picked)
		{
			// If already selected, it will be deselected now:
			translator.selected[i] = translator.selected.back();
			translator.selected.pop_back();
			removal = true;
			break;
		}
	}

	if (!removal)
	{
		translator.selected.push_back(picked);
	}
}
bool EditorComponent::IsSelected(Entity entity) const
{
	for (auto& x : translator.selected)
	{
		if (x.entity == entity)
		{
			return true;
		}
	}
	return false;
}

void EditorComponent::ResetHistory()
{
	historyPos = -1;
	history.clear();
}
ap::Archive& EditorComponent::AdvanceHistory()
{
	historyPos++;

	while (static_cast<int>(history.size()) > historyPos)
	{
		history.pop_back();
	}

	history.emplace_back();
	history.back().SetReadModeAndResetPos(false);

	return history.back();
}
void EditorComponent::ConsumeHistoryOperation(bool undo)
{
	if ((undo && historyPos >= 0) || (!undo && historyPos < (int)history.size() - 1))
	{
		if (!undo)
		{
			historyPos++;
		}

		Scene& scene = ap::scene::GetScene();

		ap::Archive& archive = history[historyPos];
		archive.SetReadModeAndResetPos(true);

		int temp;
		archive >> temp;
		HistoryOperationType type = (HistoryOperationType)temp;

		switch (type)
		{
		case HISTORYOP_TRANSLATOR:
		{
			XMFLOAT4X4 delta;
			archive >> delta;
			translator.enabled = true;

			translator.PreTranslate();
			XMMATRIX W = XMLoadFloat4x4(&delta);
			if (undo)
			{
				W = XMMatrixInverse(nullptr, W);
			}
			W = W * XMLoadFloat4x4(&translator.transform.world);
			XMStoreFloat4x4(&translator.transform.world, W);
			translator.PostTranslate();
		}
		break;
		case HISTORYOP_DELETE:
		{
			size_t count;
			archive >> count;
			ap::vector<Entity> deletedEntities(count);
			for (size_t i = 0; i < count; ++i)
			{
				archive >> deletedEntities[i];
			}

			if (undo)
			{
				for (size_t i = 0; i < count; ++i)
				{
					scene.Entity_Serialize(archive);
				}
			}
			else
			{
				for (size_t i = 0; i < count; ++i)
				{
					scene.Entity_Remove(deletedEntities[i]);
				}
			}

		}
		break;
		case HISTORYOP_SELECTION:
		{
			// Read selections states from archive:

			ap::vector<ap::scene::PickResult> selectedBEFORE;
			size_t selectionCountBEFORE;
			archive >> selectionCountBEFORE;
			for (size_t i = 0; i < selectionCountBEFORE; ++i)
			{
				ap::scene::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedBEFORE.push_back(sel);
			}

			ap::vector<ap::scene::PickResult> selectedAFTER;
			size_t selectionCountAFTER;
			archive >> selectionCountAFTER;
			for (size_t i = 0; i < selectionCountAFTER; ++i)
			{
				ap::scene::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedAFTER.push_back(sel);
			}


			// Restore proper selection state:
			if (undo)
			{
				translator.selected = selectedBEFORE;
			}
			else
			{
				translator.selected = selectedAFTER;
			}
		}
		break;
		case HISTORYOP_PAINTTOOL:
			//paintToolWnd.ConsumeHistoryOperation(archive, undo);
			break;
		case HISTORYOP_NONE:
			assert(0);
			break;
		default:
			break;
		}

		if (undo)
		{
			historyPos--;
		}
	}

	
}

void EditorComponent::DeleteSelectedEntities()
{
	Scene& scene = GetScene();

	if (ap::input::Press(ap::input::KEYBOARD_BUTTON_DELETE))
	{
		ap::Archive& archive = AdvanceHistory();
		archive << HISTORYOP_DELETE;

		archive << translator.selected.size();
		for (auto& x : translator.selected)
		{
			archive << x.entity;
		}
		for (auto& x : translator.selected)
		{
			scene.Entity_Serialize(archive, x.entity);
		}
		for (auto& x : translator.selected)
		{
			scene.Entity_Remove(x.entity);
		}

		translator.selected.clear();

	}
}




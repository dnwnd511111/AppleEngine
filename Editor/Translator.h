#pragma once
#include "CommonInclude.h"
#include "apCanvas.h"
#include "apVector.h"
#include "ImGuizmo.h"

class Translator
{
private:
public:
	XMFLOAT4 prevPointer = XMFLOAT4(0, 0, 0, 0);
	bool dragging = false;
	bool dragStarted = false;
	bool dragEnded = false;
	XMFLOAT4X4 dragDeltaMatrix = ap::math::IDENTITY_MATRIX;

	void Create();

	void Update(const ap::Canvas& canvas);
	void Draw(const ap::scene::CameraComponent& camera, ap::graphics::CommandList cmd) const;

	// Attach selection to translator temporarily
	void PreTranslate();
	// Apply translator to selection
	void PostTranslate();

	void RefreshTopParent();

	ap::scene::TransformComponent transform;
	XMMATRIX archiveMatrix;
	XMMATRIX deltaMatrix;

	
	ap::vector<ap::scene::PickResult> selected;
	std::unordered_map<ap::ecs::Entity, bool> topParents;

	bool enabled = false;

	enum TRANSLATOR_STATE
	{
		TRANSLATOR_IDLE,
		TRANSLATOR_X,
		TRANSLATOR_Y,
		TRANSLATOR_Z,
		TRANSLATOR_XY,
		TRANSLATOR_XZ,
		TRANSLATOR_YZ,
		TRANSLATOR_XYZ,
	} state = TRANSLATOR_IDLE;

	float dist = 1;

	bool isTranslator = true, isScalator = false, isRotator = false;

	ImGuizmo::OPERATION imGizmoType = (ImGuizmo::OPERATION)-1;

	// Check if the drag started in this exact frame
	bool IsDragStarted() const { return dragStarted; };
	// Check if the drag ended in this exact frame
	bool IsDragEnded() const { return dragEnded; };
	// Delta matrix from beginning to end of drag operation
	XMFLOAT4X4 GetDragDeltaMatrix() const { return dragDeltaMatrix; }
};


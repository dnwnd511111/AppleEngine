#pragma once
#include "CommonInclude.h"
#include "apCanvas.h"
#include "apVector.h"

class Translator
{
private:
	XMFLOAT4 prevPointer = XMFLOAT4(0, 0, 0, 0);
	XMFLOAT4X4 dragDeltaMatrix = ap::math::IDENTITY_MATRIX;
	bool dragging = false;
	bool dragStarted = false;
	bool dragEnded = false;
public:
	void Create();

	void Update(const ap::Canvas& canvas);
	void Draw(const ap::scene::CameraComponent& camera, ap::graphics::CommandList cmd) const;

	// Attach selection to translator temporarily
	void PreTranslate();
	// Apply translator to selection
	void PostTranslate();

	ap::scene::TransformComponent transform;
	ap::vector<ap::scene::PickResult> selected;

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


	// Check if the drag started in this exact frame
	bool IsDragStarted() const { return dragStarted; };
	// Check if the drag ended in this exact frame
	bool IsDragEnded() const { return dragEnded; };
	// Delta matrix from beginning to end of drag operation
	XMFLOAT4X4 GetDragDeltaMatrix() const { return dragDeltaMatrix; }
};


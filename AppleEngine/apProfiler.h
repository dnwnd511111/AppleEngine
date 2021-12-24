#pragma once
#include "apGraphicsDevice.h"
#include "apCanvas.h"

namespace ap::profiler
{
	typedef size_t range_id;

	// Begin collecting profiling data for the current frame
	void BeginFrame();

	// Finalize collecting profiling data for the current frame
	void EndFrame(ap::graphics::CommandList cmd);

	// Start a CPU profiling range
	range_id BeginRangeCPU(const char* name);

	// Start a GPU profiling range
	range_id BeginRangeGPU(const char* name, ap::graphics::CommandList cmd);

	// End a profiling range
	void EndRange(range_id id);

	// Renders a basic text of the Profiling results to the (x,y) screen coordinate
	void DrawData(const ap::Canvas& canvas, float x, float y, ap::graphics::CommandList cmd);

	// Enable/disable profiling
	void SetEnabled(bool value);

	bool IsEnabled();
};


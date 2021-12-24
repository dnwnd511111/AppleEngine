#pragma once
#include "CommonInclude.h"
#include "apInput.h"

#ifdef SDL2
#include <SDL2/SDL.h>
#endif

namespace ap::input::sdlinput
{
	// Call this once to register raw input devices
	void Initialize();

	// Updates the state of raw input devices, call once per frame
	void Update();

	// Writes the keyboard state into state parameter
	void GetKeyboardState(ap::input::KeyboardState* state);

	// Writes the mouse state into state parameter
	void GetMouseState(ap::input::MouseState* state);

	// Returns how many controller devices have received input ever. This doesn't correlate with which ones are currently available
	int GetMaxControllerCount();

	// Returns whether the controller identified by index parameter is available or not
	//	Id state parameter is not nullptr, and the controller is available, the state will be written into it
	bool GetControllerState(ap::input::ControllerState* state, int index);

	// Sends feedback data for the controller identified by index parameter to output
	void SetControllerFeedback(const ap::input::ControllerFeedback& data, int index);
}

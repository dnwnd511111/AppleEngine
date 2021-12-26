#ifndef APPLEENGINE
#define APPLEENGINE
#define APPLE_ENGINE

// NOTE:
// The purpose of this file is to expose all engine features.
// It should be included in the engine's implementing application not the engine itself!
// It should be included in the precompiled header if available.

#include "CommonInclude.h"

// High-level interface:
#include "apApplication.h"
#include "apRenderPath.h"
#include "apRenderPath2D.h"
#include "apRenderPath3D.h"
#include "apRenderPath3D_PathTracing.h"
#include "apLoadingScreen.h"

// Engine-level systems
#include "apVersion.h"
#include "apPlatform.h"
#include "apBacklog.h"
#include "apPrimitive.h"
#include "apImage.h"
#include "apFont.h"
#include "apSprite.h"
#include "apSpriteFont.h"
#include "apScene.h"
#include "apECS.h"
#include "apEmittedParticle.h"
#include "apHairParticle.h"
#include "apRenderer.h"
#include "apMath.h"
#include "apAudio.h"
#include "apResourceManager.h"
#include "apTimer.h"
#include "apHelper.h"
#include "apInput.h"
#include "apRawInput.h"
#include "apXInput.h"
#include "apSDLInput.h"
#include "apTextureHelper.h"
#include "apRandom.h"
#include "apColor.h"
#include "apPhysics.h"
#include "apEnums.h"
#include "apInitializer.h"
#include "apGraphics.h"
#include "apGraphicsDevice.h"
#include "apGUI.h"
#include "apArchive.h"
#include "apSpinLock.h"
#include "apRectPacker.h"
#include "apProfiler.h"
#include "apOcean.h"
#include "apFFTGenerator.h"
#include "apArguments.h"
#include "apGPUBVH.h"
#include "apGPUSortLib.h"
#include "apJobSystem.h"
#include "apEventHandler.h"
#include "apShaderCompiler.h"
#include "apCanvas.h"
#include "apUnorderedMap.h"
#include "apUnorderedSet.h"
#include "apVector.h"


#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_internal.h"

#ifdef _WIN32
#ifdef PLATFORM_UWP
//#pragma comment(lib,"AppleEngine_UWP.lib")
#else
#pragma comment(lib,"AppleEngine.lib")
#endif // PLATFORM_UWP
#endif // _WIN32



#endif // APPLEENGINE

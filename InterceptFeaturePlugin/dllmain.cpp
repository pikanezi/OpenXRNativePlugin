#include "IUnityInterface.h"
#include "IUnityLog.h"
#include "openxr/openxr.h"
#include <cstring>
#include <string>
#include <chrono>
#include <format>

typedef void (*SendOpenXrFrameTime_pfn)(double openXrFrameTime);

static IUnityLog* unityLogPtr = nullptr;
static PFN_xrGetInstanceProcAddr s_xrGetInstanceProcAddr = nullptr;
static PFN_xrBeginFrame s_xrBeginFrame = nullptr;
static PFN_xrEndFrame s_xrEndFrame = nullptr;

std::chrono::high_resolution_clock::time_point beginTime;

static SendOpenXrFrameTime_pfn send_frametime = nullptr;


static XrResult XRAPI_PTR intercepted_xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo)
{
	XrResult result = s_xrBeginFrame(session, frameBeginInfo);
	beginTime = std::chrono::high_resolution_clock::now();

	return result;
}

static XrResult XRAPI_PTR intercepted_xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo)
{
	XrResult result = s_xrEndFrame(session, frameEndInfo);
	auto now = std::chrono::high_resolution_clock::now();
	auto frame_time = std::chrono::duration_cast<std::chrono::nanoseconds>(now - beginTime).count();
	send_frametime(frame_time);

	return result;
}

static XrResult XRAPI_PTR intercept_functions_xrGetInstanceProcAddr(XrInstance instance, const char* name,
	PFN_xrVoidFunction* function)
{
	XrResult result = s_xrGetInstanceProcAddr(instance, name, function);
	if (strcmp("xrBeginFrame", name) == 0)
	{
		s_xrBeginFrame = (PFN_xrBeginFrame)*function;
		*function = (PFN_xrVoidFunction)intercepted_xrBeginFrame;
	}
	if (strcmp("xrEndFrame", name) == 0)
	{
		s_xrEndFrame = (PFN_xrEndFrame)*function;
		*function = (PFN_xrVoidFunction)intercepted_xrEndFrame;
	}
	return result;
}

extern "C"
{
	void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API script_set_callback(SendOpenXrFrameTime_pfn callback)
	{
		send_frametime = callback;
	}

	PFN_xrGetInstanceProcAddr UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API script_intercept_xrCreateSession_xrGetInstanceProcAddr(PFN_xrGetInstanceProcAddr old)
	{
		s_xrGetInstanceProcAddr = old;
		return intercept_functions_xrGetInstanceProcAddr;
	}

	void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
	{
		unityLogPtr = unityInterfaces->Get<IUnityLog>();
	}

	void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
	{
		unityLogPtr = nullptr;
	}
}

#include "IUnityInterface.h"
#include "IUnityLog.h"
#include "openxr/openxr.h"
#include <cstring>
#include <string>
#include <chrono>
#include <format>

typedef void (*SendOpenXrGpuTime_pfn)(double openXrGpuTime);
typedef void (*SendOpenXrCpuTime_pfn)(double openXrCpuTime);

static IUnityLog* unityLogPtr = nullptr;
static PFN_xrGetInstanceProcAddr s_xrGetInstanceProcAddr = nullptr;
static PFN_xrBeginFrame s_xrBeginFrame = nullptr;
static PFN_xrWaitFrame s_xrWaitFrame= nullptr;

std::chrono::high_resolution_clock::time_point last_gpu_time;
std::chrono::high_resolution_clock::time_point last_cpu_time;

static SendOpenXrGpuTime_pfn send_gpu_time = nullptr;
static SendOpenXrCpuTime_pfn send_cpu_time = nullptr;


static XrResult XRAPI_PTR intercepted_xrBeginFrame(XrSession session, const XrFrameBeginInfo* frameBeginInfo)
{
	XrResult result = s_xrBeginFrame(session, frameBeginInfo);
	auto now = std::chrono::high_resolution_clock::now();
	auto gpu_time = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_gpu_time).count();
	last_gpu_time = now;
	send_gpu_time((double)gpu_time);

	return result;
}

static XrResult XRAPI_PTR intercepted_xrWaitFrame(XrSession session, const XrFrameWaitInfo* frameWaitInfo, XrFrameState* frameState)
{
	XrResult result = s_xrWaitFrame(session, frameWaitInfo, frameState);
	auto now = std::chrono::high_resolution_clock::now();
	auto cpu_time = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_cpu_time).count();
	last_cpu_time = now;
	send_cpu_time((double)cpu_time);

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
	if (strcmp("xrWaitFrame", name) == 0)
	{
		s_xrWaitFrame = (PFN_xrWaitFrame)*function;
		*function = (PFN_xrVoidFunction)intercepted_xrWaitFrame;
	}
	return result;
}

extern "C"
{
	void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API script_set_gpu_time_callback(SendOpenXrGpuTime_pfn callback)
	{
		send_gpu_time = callback;
	}

	void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API script_set_cpu_time_callback(SendOpenXrCpuTime_pfn callback)
	{
		send_cpu_time = callback;
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

using AOT;
using System;
using UnityEditor;
using UnityEngine.XR.OpenXR.Features;
using System.Runtime.InteropServices;

#if UNITY_EDITOR
[UnityEditor.XR.OpenXR.Features.OpenXRFeature(UiName = "Get GPU and CPU Frame Times",
    BuildTargetGroups = new[] { BuildTargetGroup.Standalone },
    Desc = "Get an accurate GPU and CPU frame time",
    Version = "0.0.1",
    FeatureId = featureId)]
#endif
public class OpenXRFeatureFrameTime : OpenXRFeature {
    public const string featureId = "com.vneel.openxr.feature.gpuframetime";
    
    public static double gpuFrameTime { get; private set; }
    public static double cpuFrameTime { get; private set; }
    
    protected override IntPtr HookGetInstanceProcAddr(IntPtr func) {
        return intercept_xrCreateSession_xrGetInstanceProcAddr(func);
    }

    protected override bool OnInstanceCreate(ulong xrInstance) {
        SetGpuTimeCallback(OnGpuFrameTime);
        SetCpuTimeCallback(OnCpuFrameTime);
        return true;
    }


    private delegate void OnGpuFrameTimeDelegate(double frameTime);
    [MonoPInvokeCallback(typeof(OnGpuFrameTimeDelegate))]
    private static void OnGpuFrameTime(double frameTime) {
        gpuFrameTime = frameTime;
    }

    private delegate void OnCpuFrameTimeDelegate(double frameTime);
    [MonoPInvokeCallback(typeof(OnCpuFrameTimeDelegate))]
    private static void OnCpuFrameTime(double frameTime) {
        cpuFrameTime = frameTime;
    }

    private const string ExtLib = "InterceptFeaturePlugin";

    [DllImport(ExtLib, EntryPoint = "script_intercept_xrCreateSession_xrGetInstanceProcAddr")]
    private static extern IntPtr intercept_xrCreateSession_xrGetInstanceProcAddr(IntPtr func);

    private delegate void ReceiveGpuFrameTimeDelegate(double frameTime);
    [DllImport(ExtLib, EntryPoint = "script_set_gpu_time_callback")]
    private static extern void SetGpuTimeCallback(ReceiveGpuFrameTimeDelegate callback);
    
    private delegate void ReceiveCpuFrameTimeDelegate(double frameTime);
    [DllImport(ExtLib, EntryPoint = "script_set_Cpu_time_callback")]
    private static extern void SetCpuTimeCallback(ReceiveCpuFrameTimeDelegate callback);
}
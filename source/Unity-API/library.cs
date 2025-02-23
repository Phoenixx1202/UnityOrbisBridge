using System;
using System.Runtime.InteropServices;

namespace UnityOrbisBridge
{
    internal static class PrivateImports
    {
        #region System Information
        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint GetCPUTemperature();

        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint GetSOCTemperature();

        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetDiskInfo([MarshalAs(UnmanagedType.LPStr)] string infoType);
        #endregion



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void DownloadWebFile([MarshalAs(UnmanagedType.LPStr)] string url,
            [MarshalAs(UnmanagedType.LPStr)] string pathWithFile, [MarshalAs(UnmanagedType.I1)]
        bool bgDL = false, [MarshalAs(UnmanagedType.LPStr)] string name = "NULL");
    }

    public static partial class UOB
    {
        #region cURL Web Interactions
        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void ResetDownloadVars();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetDownloadInfo(string info);



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern long GetDownloadError();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void CancelDownload();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr DownloadAsBytes(string url, out int size);
        #endregion



        #region Logging and Notifications
        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void PrintToConsole([MarshalAs(UnmanagedType.LPStr)] string message, int type);



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void PrintAndLog([MarshalAs(UnmanagedType.LPStr)] string message,
            int type, [MarshalAs(UnmanagedType.LPStr)] string filePath = "/data/UnityOrbisBridge.log");



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void TextNotify(int type, [MarshalAs(UnmanagedType.LPStr)] string message);



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImageNotify([MarshalAs(UnmanagedType.LPStr)]
        string imageURL, [MarshalAs(UnmanagedType.LPStr)] string message);
        #endregion



        #region System Information
        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetFWVersion();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetConsoleType();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern int GetSystemLanguageID();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetSystemLanguage();
        #endregion



        #region System Control
        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetTemperatureLimit(byte limit);



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetKeyboardInput([MarshalAs(UnmanagedType.LPStr)] string title = "",
            [MarshalAs(UnmanagedType.LPStr)] string initialText = "");



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AlarmBuzzer(int type);



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void RunCMDAsRoot(IntPtr function, IntPtr arg, int cwdMode);
        #endregion



        #region Filesystem Operations
        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WriteFile([MarshalAs(UnmanagedType.LPStr)] string content,
            [MarshalAs(UnmanagedType.LPStr)] string filePath = "/data/UnityOrbisBridge.log");



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AppendFile([MarshalAs(UnmanagedType.LPStr)] string content,
            [MarshalAs(UnmanagedType.LPStr)] string filePath = "/data/UnityOrbisBridge.log");



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void MountRootDirectories();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void InstallLocalPackage(string uri, string name, bool deleteAfter);



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void DownloadAndInstallPKG(string url, string name, string iconURL);
        #endregion



        #region Application Operations
        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void InitializeNativeDialogs();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool IsFreeOfSandbox();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void EnterSandbox();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void BreakFromSandbox();



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void MountInSandbox([MarshalAs(UnmanagedType.LPStr)]
        string systemPath, [MarshalAs(UnmanagedType.LPStr)] string mountName);



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void UnmountFromSandbox([MarshalAs(UnmanagedType.LPStr)] string mountName);



        [DllImport("UnityOrbisBridge", CallingConvention = CallingConvention.Cdecl)]
        public static extern void ExitApplication();
        #endregion
    }
}

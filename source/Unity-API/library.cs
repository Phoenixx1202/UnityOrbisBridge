using System;
using System.Runtime.InteropServices;

namespace UnityOrbisBridge
{
    internal static class PrivateImports
    {
        #region System Information
        [DllImport("UnityOrbisBridge")] public static extern uint GetCPUTemperature();
        [DllImport("UnityOrbisBridge")] public static extern uint GetSOCTemperature();
        [DllImport("UnityOrbisBridge")] public static extern IntPtr GetDiskInfo(string infoType);
        #endregion

        [DllImport("UnityOrbisBridge")] public static extern void DownloadWebFile(string url, string pathWithFile, bool bgDL = false, string name = "NULL");
    }

    public static partial class UOB
    {
        #region cURL Web Interactions
        [DllImport("UnityOrbisBridge")] public static extern IntPtr GetDownloadInfo(string info);
        [DllImport("UnityOrbisBridge")] public static extern bool HasDownloadCompleted();
        [DllImport("UnityOrbisBridge")] public static extern bool HasDownloadErrorOccured();
        [DllImport("UnityOrbisBridge")] public static extern void ResetDownloadVars();
        [DllImport("UnityOrbisBridge")] public static extern void CancelDownload();
        [DllImport("UnityOrbisBridge")] public static extern IntPtr DownloadAsBytes(string url, out int size);
        [DllImport("UnityOrbisBridge")] public static extern bool IsInternetAvailable();

        #endregion

        #region Logging and Notifications
        [DllImport("UnityOrbisBridge")] public static extern void PrintToConsole(string message, int type);
        [DllImport("UnityOrbisBridge")] public static extern void PrintAndLog(string message, int type, string filePath = "/data/UnityOrbisBridge.log");
        [DllImport("UnityOrbisBridge")] public static extern void TextNotify(int type, string message);
        [DllImport("UnityOrbisBridge")] public static extern void ImageNotify(string imageURL, string message);

        #endregion

        #region System Information
        [DllImport("UnityOrbisBridge")] public static extern bool IsPlayStation5(); 
        [DllImport("UnityOrbisBridge")] public static extern IntPtr GetFWVersion();
        [DllImport("UnityOrbisBridge")] public static extern IntPtr GetConsoleType();
        [DllImport("UnityOrbisBridge")] public static extern int GetSystemLanguageID();
        [DllImport("UnityOrbisBridge")] public static extern IntPtr GetSystemLanguage();
       
        #endregion

        #region System Control
        [DllImport("UnityOrbisBridge")] public static extern void SetTemperatureLimit(byte limit);
        [DllImport("UnityOrbisBridge")] public static extern IntPtr GetKeyboardInput(string title = "", string initialText = "");
        [DllImport("UnityOrbisBridge")] public static extern void AlarmBuzzer(int type);
        [DllImport("UnityOrbisBridge")] public static extern void RunCMDAsRoot(IntPtr function, IntPtr arg, int cwdMode);
        
        #endregion

        #region Filesystem Operations
        [DllImport("UnityOrbisBridge")] public static extern void WriteFile(string content, string filePath = "/user/data/UnityOrbisBridge.log");
        [DllImport("UnityOrbisBridge")] public static extern void AppendFile(string content, string filePath = "/user/data/UnityOrbisBridge.log");
        [DllImport("UnityOrbisBridge")] public static extern void MountRootDirectories();
        [DllImport("UnityOrbisBridge")] public static extern void InstallLocalPackage(string filePath, string name, bool deleteAfter);
        [DllImport("UnityOrbisBridge")] public static extern void InstallWebPackage(string url, string name, string iconURL);
        [DllImport("UnityOrbisBridge")] public static extern bool CheckIfAppExists(string titleId);

        #endregion

        #region Application Operations
        [DllImport("UnityOrbisBridge")] public static extern void InitializeNativeDialogs();
        [DllImport("UnityOrbisBridge")] public static extern bool IsFreeOfSandbox();
        [DllImport("UnityOrbisBridge")]public static extern void EnterSandbox();
        [DllImport("UnityOrbisBridge")] public static extern void BreakFromSandbox();
        [DllImport("UnityOrbisBridge")] public static extern void MountInSandbox(string systemPath, string mountName);
        [DllImport("UnityOrbisBridge")] public static extern void UnmountFromSandbox(string mountName);
        [DllImport("UnityOrbisBridge")] public static extern void ExitApplication();
        [DllImport("UnityOrbisBridge")] public static extern void UpdateViaHomebrewStore(string query = "");
        
        #endregion
    }
}

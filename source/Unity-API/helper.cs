using System;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;

namespace UnityOrbisBridge
{
    internal static class PrivateHelpers
    {
        public static void CurlDownloadFile(string url, string path, string fileWithExtension, bool bgDL, string name)
        {
            if (UOB.IsFreeOfSandbox()) PrivateImports.DownloadWebFile(url, path.EndsWith("/") ? path + fileWithExtension : path + "/" + fileWithExtension, bgDL, name);
        }
    }

    public static partial class UOB
    {
        public static Dictionary<DiskInfo, string> lastDiskInfo = new Dictionary<DiskInfo, string>
        {
            { DiskInfo.Used, "" }, { DiskInfo.Free, "" }, { DiskInfo.Total, "" }, { DiskInfo.Percent, "" }
        };

        public static string GetDiskInfo(DiskInfo info, string mountPoint = "/user")
        {
            var diskInfo = new Dictionary<DiskInfo, string>
            {
                { DiskInfo.Free, Marshal.PtrToStringAnsi(PrivateImports.GetDiskInfo("freeSpace", mountPoint)) },
                { DiskInfo.Used, Marshal.PtrToStringAnsi(PrivateImports.GetDiskInfo("usedSpace", mountPoint)) },
                { DiskInfo.Total, Marshal.PtrToStringAnsi(PrivateImports.GetDiskInfo("totalSpace", mountPoint)) },
                { DiskInfo.Percent, Marshal.PtrToStringAnsi(PrivateImports.GetDiskInfo("percentUsed", mountPoint)) }
            };

            return diskInfo.TryGetValue(info, out var value) ? value : string.Empty;
        }

        public static string GetDiskInfoAsFormattedText(DiskInfo info, string mountPoint = "/user")
        {
            var cultureInfo = CultureInfo.CurrentCulture;
            var diskInfo = new Dictionary<DiskInfo, string>
            {
                { DiskInfo.Free, Marshal.PtrToStringAnsi(PrivateImports.GetDiskInfo("freeSpace", mountPoint)) },
                { DiskInfo.Used, Marshal.PtrToStringAnsi(PrivateImports.GetDiskInfo("usedSpace", mountPoint)) },
                { DiskInfo.Total, Marshal.PtrToStringAnsi(PrivateImports.GetDiskInfo("totalSpace", mountPoint)) },
                { DiskInfo.Percent, Marshal.PtrToStringAnsi(PrivateImports.GetDiskInfo("percentUsed", mountPoint)) }
            };

            switch (info)
            {
                case DiskInfo.Free:
                    return string.Format(cultureInfo, "Free: {0}", diskInfo[DiskInfo.Free]);
                case DiskInfo.Used:
                    return string.Format(cultureInfo, "Used: {0}", diskInfo[DiskInfo.Used]);
                case DiskInfo.Percent:
                    return string.Format(cultureInfo, "Used: {0} of {1}", diskInfo[DiskInfo.Percent], diskInfo[DiskInfo.Total]);
                case DiskInfo.AllFormatted:
                    return string.Format(cultureInfo, "Used: {0} of {1} ({2})", diskInfo[DiskInfo.Used], diskInfo[DiskInfo.Total], diskInfo[DiskInfo.Percent]);
                default:
                    return string.Empty;
            }
        }

        public static float? GetTemperature(Temperature temperature = Temperature.CPU, bool fahrenheit = false)
        {
            uint tempCelsius = temperature == Temperature.CPU
                ? PrivateImports.GetCPUTemperature() : PrivateImports.GetSOCTemperature();

            return fahrenheit ? tempCelsius * 9 / 5 + 32 : tempCelsius;
        }

        public static void DownloadWebFile(string url, string path, string file, string extension, bool bgDL = false, string name = "NULL")
            => PrivateHelpers.CurlDownloadFile(url, path, file + extension, bgDL, name);

        public static void DownloadPkgFile(string url, string path, string file, bool bgDL = false, string name = "NULL")
            => PrivateHelpers.CurlDownloadFile(url, path, file + ".pkg", bgDL, name);

        [Obsolete("This method is deprecated. Use the method with interpolated strings instead.")]
        public static void PrintToConsole(int type, string format, params object[] args)
            => PrintToConsole(string.Format(format, args), type);
    }
}

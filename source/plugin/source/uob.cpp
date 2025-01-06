#include "../headers/includes.hpp"

void InitializeNativeDialogs();

extern "C"
{
#pragma region cURL Web Interactions
    void ResetDownloadVars();
    void DownloadWebFile(const char *url, const char *pathWithFile, bool bgDL, const char *name);
    char *GetDownloadInfo(const char *info);
    long GetDownloadError();
    void CancelDownload();
    char *DownloadAsBytes(const char *url, size_t *out_size);
#pragma endregion

#pragma region Logging and Notifications
    void PrintToConsole(const char *message, int type);
    void PrintAndLog(const char *message, int type, const char *file);
    void TextNotify(int type, const char *msg);
    void ImageNotify(const char *IconUri, const char *text);
#pragma endregion

#pragma region System Information
    const char *GetFWVersion();
    const char *GetConsoleType();
    int32_t GetSystemLanguageID();
    const char *GetSystemLanguage();
    uint32_t GetCPUTemperature();
    uint32_t GetSOCTemperature();
#pragma endregion

#pragma region System Control
    void SetTemperatureLimit(uint8_t limit);
    const char *GetKeyboardInput(const char *title, const char *initialText);
    void AlarmBuzzer(int type);
    void RunCMDAsRoot(void (*function)(void *arg), void *arg, int cwd_mode);
#pragma endregion

#pragma region Filesystem Operations
    const char *GetDiskInfo(const char *infoType);
    void WriteFile(const char *content, const char *file);
    void AppendFile(const char *content, const char *file);
    void MountRootDirectories();
    void InstallLocalPackage(const char *uri, const char *name, bool deleteAfter);
    void DownloadAndInstallPKG(const char *url, const char *name, const char *iconURL);
#pragma endregion

#pragma region Application Operations
    bool IsFreeOfSandbox();
    void EnterSandbox();
    void BreakFromSandbox();
    void MountInSandbox(const char *systemPath, const char *mountName);
    void UnmountFromSandbox(const char *mountName);
    void ExitApplication();
#pragma endregion
}

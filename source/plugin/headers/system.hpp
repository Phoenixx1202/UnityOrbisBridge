#pragma once

extern std::string consoleType;

extern "C"
{
#pragma region Logging and Notifications
    void PrintToConsole(const char *message, int type = 0);
    void PrintAndLog(const char *message, int type = 0, const char *file = "/user/data/UnityOrbisBridge.log");
    void TextNotify(int type, const char *msg);
    void ImageNotify(const char *iconUri, const char *text);
#pragma endregion

#pragma region System Information
    bool IsPlayStation5();
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
    const char *GetDiskInfo(const char *infoType, const char *mountPoint);
    void CreateDirectory(const char *dirPath);
    void WriteFile(const char *content, const char *file = "/user/data/UnityOrbisBridge.log");
    void AppendFile(const char *content, const char *file = "/user/data/UnityOrbisBridge.log");
    void MountRootDirectories();
    void UnmountFromSandbox(const char *mountName);
    void InstallLocalPackage(const char *file, const char *name, const char *iconURI, bool deleteAfter);
    void InstallWebPackage(const char *url, const char *name, const char *titleId, const char *iconURL);
    void ExtractZipFile(const char *filePath, const char *outPath);
    bool CheckIfAppExists(const char *titleId);
#pragma endregion
}

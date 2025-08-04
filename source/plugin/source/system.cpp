#include "../headers/includes.hpp"

std::string consoleType;

#pragma region Logging and Notifications
void PrintToConsole(const char *message, int type)
{
    if (type < 0 || type > 4)
        type = 0;

    std::string logMessage = std::string(logPrefixes[type]) + " " + message;
    sceKernelDebugOutText(0, (logMessage + "\n").c_str());
}

void PrintAndLog(const char *message, int type, const char *file)
{
    if (type < 0 || type > 4)
        type = 0;

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char timestamp[100];
    std::strftime(timestamp, sizeof(timestamp), "%m/%d/%Y @ %I:%M:%S%p", std::localtime(&now));
    std::string fileString = std::string(timestamp) + " " + logPrefixes[type] + " " + message;

    PrintToConsole(message, type);
    AppendFile(fileString.c_str(), file);
}

void TextNotify(int type, const char *msg)
{
    sceSysUtilSendSystemNotificationWithText(type, msg);
}

void ImageNotify(const char *iconUri, const char *text)
{
    OrbisNotificationRequest Buffer;
    Buffer.type = NotificationRequest;
    Buffer.useIconImageUri = 1;
    Buffer.targetId = -1;
    strcpy(Buffer.message, text);
    strcpy(Buffer.iconUri, iconUri ? iconUri : "cxml://psnotification/tex_icon_system");
    sceKernelSendNotificationRequest(0, &Buffer, sizeof(Buffer), 0);
}
#pragma endregion

#pragma region System Information
bool IsPlayStation5()
{
    const char *goldHENPath = "/data/GoldHEN";
    const char *etaHENPath = "/data/etaHEN";
    const char *devA53mmPath = "/dev/a53mm";
    const char *devA53mmsysPath = "/dev/a53mmsys";
    const char *devA53ioPath = "/dev/a53io";

    auto pathExists = [](const char *path) -> bool
    {
        struct stat buffer;
        return (stat(path, &buffer) == 0);
    };

    if (pathExists(goldHENPath))
        return false;

    return pathExists(etaHENPath) ||
           ((pathExists(devA53mmPath) ||
             pathExists(devA53mmsysPath)) ||
            pathExists(devA53ioPath));
}

const char *GetFWVersion()
{
    static char versionString[32];
    OrbisKernelSwVersion versionInfo;

    versionInfo.Size = sizeof(OrbisKernelSwVersion);

    if (sceKernelGetSystemSwVersion(&versionInfo) < 0)
        return "00.00";

    strncpy(versionString, versionInfo.VersionString, sizeof(versionString) - 1);
    versionString[sizeof(versionString) - 1] = '\0';

    return versionString[0] ? versionString : "00.00";
}

const char *GetConsoleType()
{
    static std::string consoleType;
    if (sceKernelIsCEX())
        consoleType = "CEX";
    else if (sceKernelIsDevKit())
        consoleType = "KIT";
    else if (sceKernelIsTestKit())
        consoleType = "TEST";
    return consoleType.c_str();
}

int32_t GetSystemLanguageID()
{
    int32_t languageID = -1;
    sceSystemServiceParamGetInt(ORBIS_SYSTEM_SERVICE_PARAM_ID_LANG, &languageID);
    return languageID;
}

const char *GetSystemLanguage()
{
    static const char *languages[] = {
        "jp", "en-US", "fr", "es", "de", "it", "nl", "pt-PT", "ru", "ko",
        "zh-TW", "zh-CN", "fi", "sv", "da", "no", "pl", "pt-BR", "en-GB",
        "tr", "es-LA", "ar", "fr-CA", "cs", "hu", "el", "ro", "th", "vi", "id"};

    int32_t langID = GetSystemLanguageID();
    return (langID >= ORBIS_SYSTEM_PARAM_LANG_JAPANESE && langID <= ORBIS_SYSTEM_PARAM_LANG_INDONESIAN) ? languages[langID] : "NULL";
}

uint32_t GetCPUTemperature()
{
    uint32_t celsius;
    sceKernelGetCpuTemperature(&celsius);
    return celsius;
}

uint32_t GetSOCTemperature()
{
    uint32_t celsius;
    sceKernelGetSocSensorTemperature(0, &celsius);
    return celsius;
}

#pragma endregion

#pragma region System Control
void SetTemperatureLimit(uint8_t limit)
{
    int fd = open("/dev/icc_fan", O_RDONLY);
    if (fd < 0)
        return;
    char data[10] = {0, 0, 0, 0, 0, static_cast<char>(limit), 0, 0, 0, 0};
    ioctl(fd, 0xC01C8F07, data);
    close(fd);
}

const char *GetKeyboardInput(const char *title, const char *initialText)
{
    char userInput[512] = {0};
    if (getKeyboardInput(title, initialText, userInput))
    {
        char *output = new char[strlen(userInput) + 1];
        strcpy(output, userInput);
        return output;
    }
    return strdup("NULL");
}

void AlarmBuzzer(int type)
{
    sceKernelIccSetBuzzer(type);
}

void RunCMDAsRoot(void (*function)(void *arg), void *arg, int cwd_mode)
{
    jbc_run_as_root(function, arg, cwd_mode);
}
#pragma endregion

#pragma region Filesystem Operations
const char *GetDiskInfo(const char *infoType)
{
    static std::string result;

    long percentUsed = -1;
    double totalSpace = 0;
    double usedSpace = 0;
    double freeSpace = 0;

    df("/user", percentUsed, totalSpace, usedSpace, freeSpace);

    auto formatSize = [](double size) -> std::string
    {
        std::string unit = "GB";
        double displaySize = size;

        if (size >= 1024.0)
        {
            unit = "TB";
            displaySize = size / 1024.0;
        }
        else if (size < 1.0)
        {
            unit = "MB";
            displaySize = size * 1024.0;
        }

        int integerPart = static_cast<int>(displaySize);
        double fractionalPart = displaySize - integerPart;
        int fractionalPartRounded = static_cast<int>(
            fractionalPart * 100.0 + 0.5);
        std::string result = std::to_string(integerPart) + "." +
                             (fractionalPartRounded < 10 ? "0" : "") +
                             std::to_string(fractionalPartRounded) + " " + unit;

        return result;
    };

    if (strcmp(infoType, "percentUsed") == 0)
        result = std::to_string(percentUsed) + "%";
    else if (strcmp(infoType, "totalSpace") == 0)
        result = formatSize(totalSpace);
    else if (strcmp(infoType, "usedSpace") == 0)
        result = formatSize(usedSpace);
    else if (strcmp(infoType, "freeSpace") == 0)
        result = formatSize(freeSpace);

    return result.c_str();
}

void CreateDirectory(const char *dirPath)
{
    struct stat info;
    if (stat(dirPath, &info) == 0 && (info.st_mode & S_IFDIR))
        return;

    if (mkdir(dirPath, 0777) != 0)
        PrintToConsole(("Failed to create directory: " + std::string(strerror(errno))).c_str(), 2);
}

void WriteFile(const char *content, const char *file)
{
    int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1)
    {
        PrintToConsole("WriteFile(): Error opening file", 2);

        return;
    }
    std::string _content = std::string(content) + "\n";
    if (write(fd, _content.c_str(), _content.length()) == -1)
        PrintToConsole("WriteFile(): Error writing to file", 2);

    close(fd);
}

void AppendFile(const char *content, const char *file)
{
    int fd = open(file, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (fd == -1)
    {
        PrintToConsole("AppendFile(): Error opening file", 2);
        return;
    }

    std::string _content = std::string(content) + "\n";
    if (write(fd, _content.c_str(), _content.length()) == -1)
        PrintToConsole("AppendFile(): Error writing to file", 2);

    close(fd);
}

void MountRootDirectories()
{
    static const char *devices[] = {
        "/dev/da0x0.crypt",
        "/dev/da0x1.crypt",
        "/dev/da0x4.crypt",
        "/dev/da0x5.crypt"};

    static const char *mount_points[] = {
        "/preinst",
        "/preinst2",
        "/system",
        "/system_ex"};

    for (int i = 0; i < 4; ++i)
        mount_large_fs(devices[i], mount_points[i], "exfatfs", "511", 0x0000000000010000ULL);
}

void InstallLocalPackage(const char *file, const char *name, bool deleteAfter)
{
    bool status = false;

    PrintToConsole("Starting package installation...", 0);

    InitializeNativeDialogs();

    if (!IsPlayStation5())
        status = installPKG(file, name, deleteAfter) == 0;
    else
        status = SendInstallRequestForPS5(file);

    printAndLog(status ? 1 : 3, status ? "Package installation succeeded." : "Package installation has failed.");
}

void InstallWebPackage(const char *url, const char *name, const char *iconURL)
{
    bool status = false;

    PrintToConsole("Starting package installation...", 0);

    InitializeNativeDialogs();

    const char *redirected_url = FollowRedirects(url);
    if (strcmp(redirected_url, url) != 0)
    {
        url = redirected_url;
        printAndLog(1, "Redirected URL: %s", url);
    }

    if (!IsPlayStation5())
        status = installWebPKG(url, name, iconURL) != 0;
    else
        status = SendInstallRequestForPS5(url);

    printAndLog(status ? 1 : 3, status ? "Package installation succeeded." : "Package installation has failed.");
}

void ExtractZipFile(const char *filePath, const char *outPath)
{
    mz_zip_archive zip_archive = {0};
    if (!mz_zip_reader_init_file(&zip_archive, filePath, 0))
        return;

    char cleaned_out_path[1024];
    strncpy(cleaned_out_path, outPath, sizeof(cleaned_out_path) - 1);
    cleaned_out_path[sizeof(cleaned_out_path) - 1] = 0;

    char *ext = strrchr(cleaned_out_path, '.');
    if (ext && (strcmp(ext, ".zip") == 0 || strcmp(ext, ".pkg") == 0))
        *ext = 0;

    int file_count = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int file_index = 0; file_index < file_count; file_index++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, file_index, &file_stat))
            continue;

        if (mz_zip_reader_is_file_a_directory(&zip_archive, file_index))
        {
            char dir_path[1024];
            snprintf(dir_path, sizeof(dir_path), "%s/%s", cleaned_out_path, file_stat.m_filename);
            mkdir(dir_path, 0755);
            continue;
        }

        char output_file_path[1024];
        snprintf(output_file_path, sizeof(output_file_path), "%s/%s", cleaned_out_path, file_stat.m_filename);

        char dir_path_buffer[1024];
        strncpy(dir_path_buffer, output_file_path, sizeof(dir_path_buffer) - 1);
        dir_path_buffer[sizeof(dir_path_buffer) - 1] = 0;

        size_t output_path_len = strlen(output_file_path);
        size_t base_dir_len = strlen(cleaned_out_path) + 1;

        for (size_t pos = base_dir_len; pos < output_path_len; pos++)
        {
            if (dir_path_buffer[pos] == '/')
            {
                dir_path_buffer[pos] = 0;
                if (!if_exists(dir_path_buffer))
                    mkdir(dir_path_buffer, 0755);
                dir_path_buffer[pos] = '/';
            }
        }

        size_t uncompressed_size = 0;
        void *extracted_data = mz_zip_reader_extract_to_heap(&zip_archive, file_index, &uncompressed_size, 0);
        if (!extracted_data)
            continue;

        FILE *output_file = fopen(output_file_path, "wb");
        if (output_file)
        {
            fwrite(extracted_data, 1, uncompressed_size, output_file);
            fclose(output_file);
        }

        mz_free(extracted_data);
    }

    mz_zip_reader_end(&zip_archive);
}

bool CheckIfAppExists(const char *titleId)
{
    struct stat info;
    std::string paths[] = {"/user/app/", "/mnt/ext0/user/app/"};

    for (const auto &path : paths)
    {
        if (stat((path + titleId).c_str(), &info) == 0 && (info.st_mode & S_IFDIR))
            return true;
    }

    return false;
}

#pragma endregion

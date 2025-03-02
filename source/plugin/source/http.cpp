#include "../headers/includes.hpp"

bool threadDownload = true;
char *fileName = nullptr;
int downloadProgress = 0;
int64_t totalFileSize = 0;
int64_t currentSize = 0;
double downloadSpeed = 0.0;
char *dataBuffer = nullptr;
size_t bufferSize = 0;
char *filePath = nullptr;
bool hasDownloadCompleted = false;
bool downloadErrorOccured = false;
std::atomic<bool> cancelDownload(false);
CURL *curl = nullptr;

char *GetDownloadInfo(const char *info)
{
    if (strcmp(info, "speed") == 0)
        return strdup(std::to_string(downloadSpeed).c_str());
    else if (strcmp(info, "downloaded") == 0)
        return strdup(std::to_string(currentSize).c_str());
    else if (strcmp(info, "filesize") == 0)
        return strdup(std::to_string(totalFileSize).c_str());
    else if (strcmp(info, "progress") == 0)
        return strdup(std::to_string(downloadProgress).c_str());
    return nullptr;
}

bool HasDownloadCompleted()
{
    return hasDownloadCompleted;
}

bool HasDownloadErrorOccured()
{
    return downloadErrorOccured;
}

void ResetDownloadVars()
{
    cancelDownload.store(false);
    downloadProgress = 0;
    totalFileSize = 0;
    currentSize = 0;
    downloadSpeed = 0.0;

    if (fileName)
    {
        free(fileName);
        fileName = nullptr;
    }

    free(dataBuffer);
    dataBuffer = nullptr;
    bufferSize = 0;

    if (curl)
    {
        curl_easy_cleanup(curl);
        curl = nullptr;
    }

    hasDownloadCompleted = false;
    downloadErrorOccured = false;
}

void CancelDownload()
{
    if (cancelDownload.load())
        cancelDownload.store(false);

    cancelDownload.store(true);
}

static int UpdateDownloadProgress(void *, int64_t totalSize, int64_t downloadedSize, int64_t, int64_t)
{
    if (cancelDownload.load())
        return CURLE_ABORTED_BY_CALLBACK;

    if (curl)
    {
        double currentDownloadSpeed = 0.0;
        CURLcode speedResult = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &currentDownloadSpeed);

        if (speedResult == CURLE_OK)
        {
            downloadSpeed = currentDownloadSpeed;
            totalFileSize = totalSize;
            currentSize = downloadedSize;
            downloadProgress = totalSize < 0 ? 0 : static_cast<int>((static_cast<float>(downloadedSize) / totalSize) * 100.f);
        }

        if (!threadDownload)
        {
            if (!fileName || strcmp(fileName, "NULL") == 0)
                setProgressMsgText(downloadProgress, "Downloading...");
            else
                setProgressMsgText(downloadProgress, "Downloading \"%s\"", fileName);
        }
    }

    return 0;
}

void BeginDownload(const char *url, const char *pathWithFile)
{
    ResetDownloadVars();

    if (!pathWithFile)
    {
        PrintAndLog("Download path is null.", 3);
        downloadErrorOccured = true;
        return;
    }

    FILE *file = fopen(pathWithFile, "wb");
    if (!file)
    {
        PrintAndLog(("Failed to open file: " + std::string(pathWithFile) + " due to: " + strerror(errno)).c_str(), 3);
        downloadErrorOccured = true;
        return;
    }

    filePath = strdup(pathWithFile);
    if (!filePath)
    {
        PrintAndLog("Memory allocation failed for filePath.", 3);
        downloadErrorOccured = true;
        fclose(file);
        return;
    }

    if (!threadDownload)
        initiateProgressDialog((char *)"Starting download...");

    curl = curl_easy_init();
    if (!curl)
    {
        PrintAndLog("CURL failed to initialize.", 3);
        downloadErrorOccured = true;
        fclose(file);
        unlink(filePath);
        free(filePath);
        filePath = nullptr;
        if (!threadDownload)
            sceMsgDialogTerminate();
        return;
    }

    std::string userAgent = "UnityOrbisBridge | FW: " + std::string(GetFWVersion());
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 20);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, UpdateDownloadProgress);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, nullptr);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK)
    {
        if (result == CURLE_ABORTED_BY_CALLBACK)
            PrintAndLog(std::string(curl_easy_strerror(result)).c_str(), 2);
        else
        {
            PrintAndLog(("Download failed with error: " + std::string(curl_easy_strerror(result))).c_str(), 3);
            downloadErrorOccured = true;
        }

        unlink(filePath);
    }
    else
        PrintAndLog("Download completed successfully.", 1);

    hasDownloadCompleted = true;

    curl_easy_cleanup(curl);
    curl = nullptr;

    fclose(file);
    free(filePath);
    filePath = nullptr;

    if (!threadDownload)
        sceMsgDialogTerminate();
}

void DownloadWebFile(const char *url, const char *pathWithFile, bool bgDL, const char *name)
{
    threadDownload = bgDL;

    if (!url || !pathWithFile)
    {
        PrintAndLog("Invalid URL or file path.", 3);
        return;
    }

    if (fileName)
        free(fileName);

    fileName = name ? strdup(name) : nullptr;

    if (!bgDL)
        BeginDownload(url, pathWithFile);
    else
    {
        std::string urlCopy(url);
        std::string filePathCopy(pathWithFile);
        std::thread downloadThread([urlCopy, filePathCopy]()
                                   { BeginDownload(urlCopy.c_str(), filePathCopy.c_str()); });

        downloadThread.detach();
    }

    PrintAndLog(bgDL ? "Background download has begun." : "Foreground download has begun.", 1);
    PrintAndLog(("Downloading file and writing to: " + std::string(pathWithFile)).c_str(), 1);
}

static size_t DownloadDataCallback(void *ptr, size_t size, size_t count)
{
    size_t totalSize = size * count;

    char *newBuffer = (char *)realloc(dataBuffer, bufferSize + totalSize + 1);
    if (!newBuffer)
    {
        printAndLog(3, "Memory allocation failed during download.");

        return 0;
    }

    dataBuffer = newBuffer;

    memcpy(&(dataBuffer[bufferSize]), ptr, totalSize);
    bufferSize += totalSize;
    dataBuffer[bufferSize] = '\0';

    return totalSize;
}

char *DownloadThread(const char *url, size_t *out_size)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        *out_size = 0;
        return NULL;
    }

    free(dataBuffer);
    dataBuffer = NULL;
    bufferSize = 0;

    std::string userAgent = "UnityOrbisBridge | FW: " + std::string(GetFWVersion());

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadDataCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK)
    {
        printAndLog(3, "Download failed with error: %s", curl_easy_strerror(result));
        free(dataBuffer);
        dataBuffer = NULL;
        bufferSize = 0;
        curl_easy_cleanup(curl);
        return NULL;
    }

    curl_easy_cleanup(curl);
    *out_size = bufferSize;
    return dataBuffer;
}

char *DownloadAsBytes(const char *url, size_t *out_size)
{
    std::future<char *> future = std::async(std::launch::async,
                                            [url, out_size]()
                                            {
                                                return DownloadThread(url, out_size);
                                            });

    return future.get();
}

#include "../headers/includes.hpp"

bool threadDownload = true;
const char *fileName = NULL;
int downloadProgress = 0;
int64_t totalFileSize = 0;
int64_t currentSize = 0;
char *dataBuffer = NULL;
size_t bufferSize = 0;
long httpStatusCode = 0;

std::atomic<bool> cancelDownload(false);

void ResetDownloadVars()
{
    cancelDownload.store(false);

    downloadProgress = 0;
    totalFileSize = 0;
    currentSize = 0;
    fileName = nullptr;

    if (dataBuffer)
    {
        free(dataBuffer);
        dataBuffer = nullptr;
    }

    bufferSize = 0;
    httpStatusCode = 0;
}

static int UpdateDownloadProgress(void *bar, int64_t totalSize, int64_t downloadedSize, int64_t, int64_t)
{
    if (cancelDownload.load())
        return CURLE_ABORTED_BY_CALLBACK; // Signal to CURL to stop the download

    int progress = 0;
    if (totalSize > 0)
        progress = static_cast<int>((static_cast<float>(downloadedSize) / totalSize) * 100.f);

    downloadProgress = progress;
    totalFileSize = totalSize;
    currentSize = downloadedSize;

    if (!threadDownload)
    {
        if (fileName == NULL || strcmp(fileName, "NULL") == 0)
            setProgressMsgText(downloadProgress, "Downloading...");
        else
            setProgressMsgText(downloadProgress, "Downloading \"%s\"", fileName);
    }

    return 0;
}

void BeginDownload(const char *url, const char *pathWithFile)
{
    ResetDownloadVars();

    FILE *file = fopen(pathWithFile, "wb");
    if (!file)
    {
        printAndLog(3, "Failed to open file... %s due to: %s", pathWithFile, strerror(errno));

        fclose(file);

        return;
    }

    if (!threadDownload)
        initiateProgressDialog((char *)"Starting download...");

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        printAndLog(3, "CURL has failed to initialize.");

        fclose(file); // Close the file if CURL initialization fails

        httpStatusCode = -1;
        unlink(pathWithFile); // Delete the file after closing

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
        printAndLog(result == 42 ? 2 : 3, curl_easy_strerror(result));

        httpStatusCode = -1;
        unlink(pathWithFile);
    }
    else
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);

        if (httpStatusCode == 200)
            printAndLog(1, "Download completed with HTTP response code: %ld", httpStatusCode);
        else
        {
            printAndLog(1, "Download failed with HTTP response code: %ld, deleting file...", httpStatusCode);

            httpStatusCode = -1;
            unlink(pathWithFile);
        }
    }

    curl_easy_cleanup(curl);

    fclose(file);

    if (!threadDownload)
        sceMsgDialogTerminate();
}

void DownloadWebFile(const char *url, const char *pathWithFile, bool bgDL, const char *name)
{
    threadDownload = bgDL;
    fileName = name;

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

    if (bgDL)
        printAndLog(1, "Background download has begun.", url, pathWithFile, bgDL);
    else
        printAndLog(1, "Foreground download has begun.", url, pathWithFile, bgDL);

    printAndLog(0, "Downloading file and writing to: %s", pathWithFile);
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
        httpStatusCode = -1;
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
        httpStatusCode = static_cast<long>(result);
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

char *GetDownloadInfo(const char *info)
{
    static char buffer[128];

    struct
    {
        const char *key;
        int64_t *value;
        const char *format;
    } infoMap[] = {
        {"downloaded", &currentSize, "%" PRId64},
        {"filesize", &totalFileSize, "%" PRId64},
        {"progress", (int64_t *)&downloadProgress, "%d"},
    };

    for (const auto &entry : infoMap)
    {
        if (strcmp(info, entry.key) == 0)
        {
            snprintf(buffer, sizeof(buffer), entry.format, *entry.value);

            return buffer;
        }
    }

    return NULL;
}

long GetDownloadError()
{
    return httpStatusCode;
}

void CancelDownload()
{
    cancelDownload.store(true);
}

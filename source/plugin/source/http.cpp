#include "../headers/includes.hpp"

bool threadDownload = true;
const char *fileName = nullptr;
const char *filePath = nullptr;
int downloadProgress = 0;
int64_t totalFileSize = 0;
int64_t currentSize = 0;
double downloadSpeed = 0.0;
char *dataBuffer = nullptr;
size_t bufferSize = 0;
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
    fileName = nullptr;
    filePath = nullptr;
    downloadProgress = 0;
    totalFileSize = 0;
    currentSize = 0;
    downloadSpeed = 0.0;

    free(dataBuffer);
    dataBuffer = nullptr;

    bufferSize = 0;

    hasDownloadCompleted = false;
    downloadErrorOccured = false;

    if (curl)
    {
        curl_easy_cleanup(curl);
        curl = nullptr;
    }
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
            downloadProgress = (totalSize < 0) ? 0 : static_cast<int>((static_cast<float>(downloadedSize) / totalSize) * 100.f);
        }

        if (!threadDownload)
        {
            if (!fileName || strcmp(fileName, "NULL") == 0)
                setProgressMsgText(downloadProgress, "Downloading...");
            else
                setProgressMsgText(downloadProgress, "Downloading \"%s\"...", fileName);
        }
    }

    return 0;
}

size_t HeaderCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    std::string header(static_cast<char *>(ptr), size * nmemb);
    if (header.find("Content-Disposition: attachment") != std::string::npos)
    {
        PrintAndLog("Server is sending the file as an attachment.", 1);
        size_t filenamePos = header.find("filename=");
        if (filenamePos != std::string::npos)
        {
            size_t start = header.find_first_not_of(" \t", filenamePos + 9);
            if (start != std::string::npos)
            {
                std::string filename;
                if (header[start] == '"')
                {
                    size_t end = header.find('"', start + 1);
                    if (end != std::string::npos)
                        filename = header.substr(start + 1, end - start - 1);
                }
                else
                {
                    size_t end = header.find_first_of(" \r\n", start);
                    filename = header.substr(start, end - start);
                }
                if (!filename.empty())
                    PrintAndLog(("Filename from header: " + filename).c_str(), 1);
            }
        }
    }
    return size * nmemb;
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

    std::string finalPath(pathWithFile);
    std::string resumePath = finalPath + ".resume";
    FILE *file = nullptr;
    long resume_offset = 0;
    bool fileClosed = false;

    if (access(resumePath.c_str(), F_OK) == 0)
    {
        file = fopen(resumePath.c_str(), "r+b");
        if (file)
        {
            fseek(file, 0, SEEK_END);
            resume_offset = ftell(file);
        }
        else
        {
            PrintAndLog("Failed to open resume file.", 3);

            downloadErrorOccured = true;

            return;
        }
    }
    else
    {
        file = fopen(resumePath.c_str(), "w+b");
        if (!file)
        {
            PrintAndLog("Failed to open file for writing.", 3);

            downloadErrorOccured = true;

            return;
        }
    }

    filePath = strdup(resumePath.c_str());
    if (!filePath)
    {
        PrintAndLog("Memory allocation failed for filePath.", 3);

        fclose(file);

        downloadErrorOccured = true;

        return;
    }

    if (!threadDownload)
        initiateProgressDialog((char *)"Starting download...");

    curl = curl_easy_init();
    if (!curl)
    {
        PrintAndLog("CURL failed to initialize.", 3);

        fclose(file);
        fileClosed = true;
        unlink(filePath);
        filePath = nullptr;

        if (!threadDownload)
            sceMsgDialogTerminate();

        downloadErrorOccured = true;
        return;
    }

    std::string userAgent = "UnityOrbisBridge | FW: " + std::string(GetFWVersion());
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, UpdateDownloadProgress);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);

    if (resume_offset > 0)
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM, resume_offset);

    CURLcode result = curl_easy_perform(curl);
    if (result == CURLE_OK)
    {
        PrintAndLog(("The download (" + std::string(url) +
                     ") completed and has been saved to \"" + finalPath + "\".")
                        .c_str(),
                    1);

        fclose(file);
        fileClosed = true;

        if (rename(resumePath.c_str(), finalPath.c_str()) != 0)
            PrintAndLog("Failed to rename resume file to final filename.", 3);
    }
    else if (result == CURLE_ABORTED_BY_CALLBACK)
    {
        PrintAndLog("Download cancelled; partial file saved as resume file.", 1);

        fclose(file);
        fileClosed = true;
    }
    else
    {
        PrintAndLog(("Download failed with error: " +
                     std::string(curl_easy_strerror(result)))
                        .c_str(),
                    3);

        downloadErrorOccured = true;

        unlink(filePath);
    }

    curl_easy_cleanup(curl);
    curl = nullptr;

    if (!fileClosed && file)
    {
        fclose(file);
        fileClosed = true;
    }

    filePath = nullptr;

    if (!threadDownload)
        sceMsgDialogTerminate();

    hasDownloadCompleted = true;
}

void DownloadWebFile(const char *url, const char *pathWithFile, bool bgDL, const char *name)
{
    threadDownload = bgDL;

    if (!url || !pathWithFile)
    {
        PrintAndLog("Invalid URL or file path.", 3);
        return;
    }

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

    PrintAndLog(bgDL ? "Background download has begun." : "Foreground download has begun.", 1);
    PrintAndLog(("Downloading file and writing to: " + std::string(pathWithFile)).c_str(), 1);
}

static size_t DownloadAsBytesCallback(void *ptr, size_t size, size_t count)
{
    size_t totalSize = size * count;
    char *newBuffer = static_cast<char *>(realloc(dataBuffer, bufferSize + totalSize + 1));
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

char *DownloadAsBytesThread(const char *url, size_t *out_size)
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
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadAsBytesCallback);

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
                                                return DownloadAsBytesThread(url, out_size);
                                            });
    return future.get();
}

char *FollowRedirects(const char *url)
{
    CURL *curl;
    CURLcode curl_res;
    const char *finalUrl = nullptr;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl)
    {
        std::string userAgent = "UnityOrbisBridge | FW: " + std::string(GetFWVersion());
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

        curl_res = curl_easy_perform(curl);
        if (curl_res == CURLE_OK)
            curl_res = curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &finalUrl);

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    if (finalUrl)
    {
        size_t len = strlen(finalUrl) + 1;
        char *result = new char[len];
        memcpy(result, finalUrl, len);
        return result;
    }

    return nullptr;
}

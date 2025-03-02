#pragma once

extern bool threadDownload;
extern char *fileName;
extern int downloadProgress;
extern int64_t totalFileSize;
extern int64_t currentSize;
extern double downloadSpeed;
extern char *dataBuffer;
extern size_t bufferSize;
extern char *filePath;
extern bool hasDownloadCompleted;
extern bool downloadErrorOccured;
extern std::atomic<bool> cancelDownload;
extern CURL *curl;

static int UpdateDownloadProgress(void *bar, int64_t totalSize, int64_t downloadedSize, int64_t, int64_t);
void BeginDownload(const char *url, const char *pathWithFile);
static size_t DownloadDataCallback(void *ptr, size_t size, size_t nmemb);
char *DownloadThread(const char *url, size_t *out_size);

extern "C"
{
    char *GetDownloadInfo(const char *info);
    bool HasDownloadCompleted();
    bool HasDownloadErrorOccured();
    void ResetDownloadVars();
    void CancelDownload();
    void DownloadWebFile(const char *url, const char *pathWithFile, bool bgDL, const char *name);
    char *DownloadAsBytes(const char *url, size_t *out_size);
}

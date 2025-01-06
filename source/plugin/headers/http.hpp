#pragma once

extern bool threadDownload;
extern const char *fileName;
extern int downloadProgress;
extern int64_t totalFileSize;
extern int64_t currentSize;
extern char *dataBuffer;
extern size_t bufferSize;
extern long httpStatusCode;

extern std::atomic<bool> cancelDownload;

static int UpdateDownloadProgress(void *bar, int64_t totalSize, int64_t downloadedSize, int64_t, int64_t);
void BeginDownload(const char *url, const char *pathWithFile);
static size_t DownloadDataCallback(void *ptr, size_t size, size_t nmemb);

extern "C"
{
    void ResetDownloadVars();
    void DownloadWebFile(const char *url, const char *pathWithFile, bool bgDL, const char *name);
    char *DownloadAsBytes(const char *url, size_t *out_size);
    char *GetDownloadInfo(const char *info);
    long GetDownloadError();
    void CancelDownload();
}

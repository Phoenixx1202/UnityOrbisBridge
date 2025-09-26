#pragma once

extern bool threadDownload;
extern const char *fileName;
extern const char *filePath;
extern int downloadProgress;
extern int64_t totalFileSize;
extern int64_t currentSize;
extern double downloadSpeed;
extern char *dataBuffer;
extern size_t bufferSize;
extern bool hasDownloadCompleted;
extern bool downloadErrorOccured;
extern std::atomic<bool> cancelDownload;
extern CURL *curl;

static int UpdateDownloadProgress(void *bar, int64_t totalSize, int64_t downloadedSize, int64_t, int64_t);
size_t HeaderCallback(void *ptr, size_t size, size_t nmemb, void *data);
void BeginDownload(const char *url, const char *pathWithFile);
static size_t DownloadAsBytesCallback(void *ptr, size_t size, size_t nmemb);
char *DownloadAsBytesThread(const char *url, size_t *out_size);
char *DownloadAsBytesRangeThread(const char *url, uint32_t offset, uint32_t size, size_t *out_size);
const char *FollowRedirects(const char *url);

extern "C"
{
    char *GetDownloadInfo(const char *info);
    bool HasDownloadCompleted();
    bool HasDownloadErrorOccured();
    void ResetDownloadVars();
    void CancelDownload();
    void DownloadWebFile(const char *url, const char *pathWithFile, bool bgDL, const char *name);
    char *DownloadAsBytes(const char *url, size_t *out_size);
    char *DownloadAsBytesRange(const char *url, uint32_t offset, uint32_t size, size_t *out_size);
}

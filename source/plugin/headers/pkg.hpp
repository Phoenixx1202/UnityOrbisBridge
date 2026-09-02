#pragma once

#define BGFT_HEAP_SIZE (1 * 1024 * 1024)

extern bool sceAppInst_done;

int PKG_ERROR(const char *name, int ret);

bool app_inst_util_init(void);
void app_inst_util_fini(void);

bool bgft_init(void);
void bgft_fini(void);

int initiateProgressDialog(const char *format, ...);
void setProgressMsgText(int prog, const char *fmt, ...);
void *displayDownloadProgress(void *argument);

uint32_t installPKG(const char *fullpath, const char *name, const char *iconURI, bool deleteAfter = false);
uint32_t installWebPKG(const char *url, const char *name, const char *title_id, const char *iconURI);
int installManifestPKG(const char *manifestUrl, const char *name, const char *contentId, const char *iconURI, unsigned long packageSize, const char *packageType);
int installManifestPKGFromJson(const char *manifestJson, const char *localIp, const char *name, const char *contentId, const char *iconURI, unsigned long packageSize, const char *packageType);
int getLastPackageInstallError(void);

bool SendInstallRequestForPS5(const char *url);

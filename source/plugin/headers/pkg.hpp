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

uint32_t installPKG(const char *fullpath, const char *filename, bool deleteAfter = false);
uint32_t installWebPKG(const char *url, const char *name, const char *icon_url);

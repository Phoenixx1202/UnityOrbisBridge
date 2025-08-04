#pragma once

#pragma region Constants and Defines
#define SCE_SYSMODULE_INTERNAL_COMMON_DIALOG 0x80000018
#define SCE_SYSMODULE_INTERNAL_SYSUTIL 0x80000018

#define YES 1
#define NO 2

#define ORBIS_COMMON_DIALOG_MAGIC_NUMBER 0xC0D1A109
#define ORBIS_MSG_DIALOG_BUTTON_ID_INVALID (0)
#define ORBIS_MSG_DIALOG_BUTTON_ID_OK (1)
#define ORBIS_MSG_DIALOG_BUTTON_ID_YES (1)
#define ORBIS_MSG_DIALOG_BUTTON_ID_NO (2)
#define ORBIS_MSG_DIALOG_BUTTON_ID_BUTTON1 (1)
#define ORBIS_MSG_DIALOG_BUTTON_ID_BUTTON2 (2)
#pragma endregion

enum bgft_task_option_t
{
    BGFT_TASK_OPTION_NONE = 0x0,
    BGFT_TASK_OPTION_DELETE_AFTER_UPLOAD = 0x1,
    BGFT_TASK_OPTION_INVISIBLE = 0x2,
    BGFT_TASK_OPTION_ENABLE_PLAYGO = 0x4,
    BGFT_TASK_OPTION_FORCE_UPDATE = 0x8,
    BGFT_TASK_OPTION_REMOTE = 0x10,
    BGFT_TASK_OPTION_COPY_CRASH_REPORT_FILES = 0x20,
    BGFT_TASK_OPTION_DISABLE_INSERT_POPUP = 0x40,
    BGFT_TASK_OPTION_DISABLE_CDN_QUERY_PARAM = 0x10000,
};

#pragma region Struct Definitions
struct bgft_download_param
{
    int user_id;
    int entitlement_type;
    const char *id;
    const char *content_url;
    const char *content_ex_url;
    const char *content_name;
    const char *icon_path;
    const char *sku_id;
    enum bgft_task_option_t option;
    const char *playgo_scenario_id;
    const char *release_date;
    const char *package_type;
    const char *package_sub_type;
    unsigned long package_size;
};

struct bgft_download_param_ex
{
    struct bgft_download_param param;
    unsigned int slot;
};

struct bgft_task_progress_internal
{
    unsigned int bits;
    int error_result;
    unsigned long length;
    unsigned long transferred;
    unsigned long length_total;
    unsigned long transferred_total;
    unsigned int num_index;
    unsigned int num_total;
    unsigned int rest_sec;
    unsigned int rest_sec_total;
    int preparing_percent;
    int local_copy_percent;
};

struct bgft_init_params
{
    void *heap;
    size_t heapSize;
};

typedef struct install_args
{
    char *title_id;
    char *fname;
    int task_id;
    char *path;
    bool is_thread;
    bool delete_pkg;
    void *bgft_heap;
} install_args;

struct bgft_download_task_progress_info
{
    unsigned int bits;
    int error_result;
    unsigned long length;
    unsigned long transferred;
    unsigned long length_total;
    unsigned long transferred_total;
    unsigned int num_index;
    unsigned int num_total;
    unsigned int rest_sec;
    unsigned int rest_sec_total;
    int preparing_percent;
    int local_copy_percent;
};

struct _SceBgftTaskProgress
{
    unsigned int bits;
    int error_result;
    unsigned long length;
    unsigned long transferred;
    unsigned long lengthTotal;
    unsigned long transferredTotal;
    unsigned int numIndex;
    unsigned int numTotal;
    unsigned int restSec;
    unsigned int restSecTotal;
    int preparingPercent;
    int localCopyPercent;
};

typedef struct _SceBgftTaskProgress SceBgftTaskProgress;
typedef int SceBgftTaskId;
#pragma endregion

extern "C"
{
    void sceSysUtilSendSystemNotificationWithText(int messageType, const char *message);

    uint32_t sceKernelGetCpuTemperature(uint32_t *celsius);
    uint32_t sceKernelGetSocSensorTemperature(uint32_t *unk, uint32_t *celsius);

    int sceBgftServiceInit(struct bgft_init_params *params);
    int sceBgftServiceDownloadStartTask(SceBgftTaskId taskId);
    int sceBgftServiceDownloadStartTaskAll(void);
    int sceBgftServiceDownloadPauseTask(SceBgftTaskId taskId);
    int sceBgftServiceDownloadPauseTaskAll(void);
    int sceBgftServiceDownloadResumeTask(SceBgftTaskId taskId);
    int sceBgftServiceDownloadResumeTaskAll(void);
    int sceBgftServiceDownloadStopTask(SceBgftTaskId taskId);
    int sceBgftServiceDownloadStopTaskAll(void);
    int sceBgftServiceDownloadGetProgress(SceBgftTaskId taskId, SceBgftTaskProgress *progress);

    int sceBgftServiceIntDownloadRegisterTaskByStorageEx(struct bgft_download_param_ex *params, int *task_id);
    int sceBgftServiceIntDebugDownloadRegisterPkg(struct bgft_download_param *params, int *task_id);
    int sceBgftServiceIntDownloadRegisterTask(struct bgft_download_param *params, int *task_id);
    int sceBgftServiceDownloadStartTask(int task_id);

    int sceBgftServiceTerm(void);

    int sceAppInstUtilTerminate();
    int sceAppInstUtilAppExists(const char *tid, int *flag);
    int sceAppInstUtilInitialize(void);
    int sceAppInstUtilAppInstallPkg(const char *file_path, void *reserved);
    int sceAppInstUtilAppUnInstall(const char *title_id);
    int sceAppInstUtilGetTitleIdFromPkg(const char *pkg_path, char *title_id, int *is_app);
    int sceAppInstUtilCheckAppSystemVer(const char *title_id, uint64_t buf, uint64_t bufs);
    int sceAppInstUtilAppPrepareOverwritePkg(const char *pkg_path);
    int sceAppInstUtilAppGetSize(const char *title_id, uint64_t *buf);

    int sceLncUtilGetAppTitleId(int appid, char *tid_out);

    static inline void _sceCommonDialogSetMagicNumber(uint32_t *magic, const OrbisCommonDialogBaseParam *param)
    {
        *magic = (uint32_t)(ORBIS_COMMON_DIALOG_MAGIC_NUMBER + (uint64_t)param);
    }

    static inline void _sceCommonDialogBaseParamInit(OrbisCommonDialogBaseParam *param)
    {
        memset(param, 0x0, sizeof(OrbisCommonDialogBaseParam));
        param->size = (uint32_t)sizeof(OrbisCommonDialogBaseParam);
        _sceCommonDialogSetMagicNumber(&(param->magic), param);
    }

    static inline void OrbisMsgDialogParamInitialize(OrbisMsgDialogParam *param)
    {
        memset(param, 0x0, sizeof(OrbisMsgDialogParam));

        _sceCommonDialogBaseParamInit(&param->baseParam);
        param->size = sizeof(OrbisMsgDialogParam);
    }
}
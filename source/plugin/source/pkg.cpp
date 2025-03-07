#include "../headers/includes.hpp"

bool sceAppInst_done = false;
static bool s_bgft_initialized = false;
static struct bgft_init_params s_bgft_init_params;

int PKG_ERROR(const char *name, int ret)
{
    printAndLog(3, "%s error: %x", name, ret);
    return ret;
}

bool app_inst_util_init(void)
{
    int ret;

    if (sceAppInst_done)
        goto done;

    printAndLog(1, "Initializing AppInstUtil...");

    ret = sceAppInstUtilInitialize();
    if (ret)
    {
        printAndLog(3, "sceAppInstUtilInitialize failed: 0x%08X", ret);
        goto err;
    }

    sceAppInst_done = true;

done:
    return true;

err:
    sceAppInst_done = false;
    return false;
}

void app_inst_util_fini(void)
{
    int ret;

    if (!sceAppInst_done)
        return;

    ret = sceAppInstUtilTerminate();
    if (ret)
        printAndLog(3, "sceAppInstUtilTerminate failed: 0x%08X", ret);

    sceAppInst_done = false;
}

bool bgft_init(void)
{
    int ret;

    if (s_bgft_initialized)
        goto done;

    memset(&s_bgft_init_params, 0, sizeof(s_bgft_init_params));
    s_bgft_init_params.heapSize = BGFT_HEAP_SIZE;
    s_bgft_init_params.heap = (uint8_t *)malloc(s_bgft_init_params.heapSize);

    if (!s_bgft_init_params.heap)
    {
        printAndLog(3, "No memory for BGFT heap.");
        goto err;
    }

    memset(s_bgft_init_params.heap, 0, s_bgft_init_params.heapSize);

    printAndLog(1, "Initializing BGFT...");
    ret = sceBgftServiceInit(&s_bgft_init_params);
    if (ret)
    {
        printAndLog(3, "sceBgftInitialize failed: 0x%08X", ret);
        goto err_bgft_heap_free;
    }

    s_bgft_initialized = true;

done:
    return true;

err_bgft_heap_free:
    if (s_bgft_init_params.heap)
    {
        free(s_bgft_init_params.heap);
        s_bgft_init_params.heap = NULL;
    }

    memset(&s_bgft_init_params, 0, sizeof(s_bgft_init_params));

err:
    s_bgft_initialized = false;
    return false;
}

void bgft_fini(void)
{
    int ret;

    if (!s_bgft_initialized)
        return;

    ret = sceBgftServiceTerm();
    if (ret)
        printAndLog(3, "sceBgftServiceTerm failed: 0x%08X", ret);

    if (s_bgft_init_params.heap)
    {
        free(s_bgft_init_params.heap);
        s_bgft_init_params.heap = NULL;
    }

    memset(&s_bgft_init_params, 0, sizeof(s_bgft_init_params));
    s_bgft_initialized = false;
}

void *displayDownloadProgress(void *arguments)
{
    struct install_args *args = (install_args *)arguments;
    SceBgftTaskProgress progress_info;

    std::string msg = fmt::format("Installing... [{}] {}", args->title_id, args->fname);

    printAndLog(1, msg.c_str());

    initiateProgressDialog(args->fname);

    int prog = 0;
    while (prog < 99)
    {
        memset(&progress_info, 0, sizeof(progress_info));

        int ret = sceBgftServiceDownloadGetProgress(args->task_id, &progress_info);
        if (ret)
            return (void *)(intptr_t)PKG_ERROR("sceBgftDownloadGetProgress", ret);

        if (progress_info.transferred > 0 && progress_info.error_result != 0)
            return (void *)(intptr_t)PKG_ERROR("BGFT_ERROR", progress_info.error_result);

        prog = (uint32_t)(((float)progress_info.transferred / progress_info.length) * 100.f);

        setProgressMsgText(prog, msg.c_str());
    }

    if (progress_info.error_result == 0)
    {
        if (args->delete_pkg)
            unlink(args->path);
    }
    else
        printAndLog(3, "Installation of %s has failed with code: 0x%x", args->title_id, progress_info.error_result);

    sceMsgDialogTerminate();

    free(args->title_id);
    free(args->path);
    free(args->fname);
    free(args);
    bgft_fini();

    return NULL;
}

uint32_t installPKG(const char *fullpath, const char *filename, bool deleteAfter)
{
    char title_id[16];
    int is_app, ret = -1;
    int task_id = -1;

    if (if_exists(fullpath))
    {
        if (sceAppInst_done)
        {
            if (!app_inst_util_init())
                return PKG_ERROR("AppInstUtil", ret);
        }

        if (!bgft_init())
            return PKG_ERROR("BGFT initialization", ret);

        ret = sceAppInstUtilGetTitleIdFromPkg(fullpath, title_id, &is_app);
        if (ret)
            return PKG_ERROR("sceAppInstUtilGetTitleIdFromPkg", ret);

        struct bgft_download_param_ex download_params;
        memset(&download_params, 0, sizeof(download_params));
        download_params.param.entitlement_type = 5;
        download_params.param.id = "";
        download_params.param.content_url = fullpath;
        download_params.param.content_name = filename;
        download_params.param.icon_path = "https://t4.ftcdn.net/jpg/01/25/36/71/360_F_125367167_JnrCHTqtZhAbWS3doG4tt631usPHiPnr.jpg";
        download_params.param.playgo_scenario_id = "0";
        download_params.param.option = BGFT_TASK_OPTION_DISABLE_CDN_QUERY_PARAM;
        download_params.slot = 0;

    retry:
        ret = sceBgftServiceIntDownloadRegisterTaskByStorageEx(&download_params, &task_id);
        if (ret == 0x80990088 || ret == 0x80990015)
        {
            ret = sceAppInstUtilAppUnInstall(&title_id[0]);
            if (ret != 0)
                return PKG_ERROR("sceAppInstUtilAppUnInstall", ret);

            goto retry;
        }
        else if (ret)
            return PKG_ERROR("sceBgftServiceIntDownloadRegisterTaskByStorageEx", ret);

        ret = sceBgftServiceDownloadStartTask(task_id);
        if (ret)
            return PKG_ERROR("sceBgftDownloadStartTask", ret);
    }
    else
    {
        printAndLog(3, "Failed to open file: %s", fullpath);
        return ret;
    }

    struct install_args *args = (struct install_args *)malloc(sizeof(struct install_args));
    args->title_id = strdup(title_id);
    args->task_id = task_id;
    args->path = strdup(fullpath);
    args->fname = strdup(filename);
    args->is_thread = false;
    args->delete_pkg = deleteAfter;
    displayDownloadProgress((void *)args);

    return 0;
}

uint32_t installWebPKG(const char *url, const char *name, const char *icon_url)
{
    char title_id[16];
    int ret = -1, task_id = -1;

    const char *redirected_url = FollowRedirects(url);
    if (redirected_url != nullptr)
        url = redirected_url; 

    if (!bgft_init())
        return PKG_ERROR("BGFT initialization", ret);

    struct bgft_download_param download_params;
    memset(&download_params, 0, sizeof(download_params));
    download_params.entitlement_type = 5;
    download_params.id = "";
    download_params.content_url = url;
    download_params.content_name = name;
    download_params.icon_path = icon_url;
    download_params.playgo_scenario_id = "0";
    download_params.option = BGFT_TASK_OPTION_DISABLE_CDN_QUERY_PARAM;

retry:
    ret = sceBgftServiceIntDebugDownloadRegisterPkg(&download_params, &task_id);
    if (ret == 0x80F00633)
    {
        TextNotify(0, "Please change NP Environment\nin \"Debug Settings\" from\n \"NP\" to \"SP-INT\".");

        ret = 0;
    }

    if (ret == 0x80990088 || ret == 0x80990015)
    {
        ret = sceAppInstUtilAppUnInstall(title_id);

        if (ret != 0)
            return PKG_ERROR("sceAppInstUtilAppUnInstall", ret);

        goto retry;
    }

    if (ret)
        return PKG_ERROR("sceBgftServiceIntDebugDownloadRegisterPkg", ret);

    ret = sceBgftServiceDownloadStartTask(task_id);

    if (ret)
        return PKG_ERROR("sceBgftDownloadStartTask", ret);

    return 0;
}

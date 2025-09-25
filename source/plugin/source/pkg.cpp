#include "../headers/includes.hpp"

bool sceAppInst_done = false;
static bool s_bgft_initialized = false;
static struct bgft_init_params s_bgft_init_params;

int PKG_ERROR(const char *name, int ret)
{
    printAndLogFmt(3, "%s error: %x", name, ret);
    return ret;
}

bool app_inst_util_init(void)
{
    int ret;

    if (sceAppInst_done)
        goto done;

    printAndLogFmt(0, "Initializing AppInstUtil...");

    ret = sceAppInstUtilInitialize();
    if (ret)
    {
        printAndLogFmt(4, "sceAppInstUtilInitialize failed: 0x%08X", ret);
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
        printAndLogFmt(4, "sceAppInstUtilTerminate failed: 0x%08X", ret);

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
        printAndLogFmt(4, "No memory for BGFT heap.");
        goto err;
    }

    memset(s_bgft_init_params.heap, 0, s_bgft_init_params.heapSize);

    printAndLogFmt(0, "Initializing BGFT...");
    ret = sceBgftServiceInit(&s_bgft_init_params);
    if (ret)
    {
        printAndLogFmt(4, "sceBgftInitialize failed: 0x%08X", ret);
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
        printAndLogFmt(4, "sceBgftServiceTerm failed: 0x%08X", ret);

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

    std::string msg = std::string("Installing... [") + args->title_id + "] " + args->fname;

    printAndLogFmt(1, msg.c_str());

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
        printAndLogFmt(4, "Installation of %s has failed with code: 0x%x", args->title_id, progress_info.error_result);

    sceMsgDialogTerminate();

    free(args->title_id);
    free(args->path);
    free(args->fname);
    free(args);
    bgft_fini();

    return NULL;
}

uint32_t installPKG(const char *fullpath, const char *name, const char *iconURI, bool deleteAfter)
{
    char title_id[16];
    int is_app, ret = -1;
    int task_id = -1;
    uint64_t pkg_size;

    printAndLogFmt(0, "Checking if file exists: %s", fullpath);
    if (if_exists(fullpath))
    {
        printAndLogFmt(0, "File found. Proceeding with installation.");
        if (sceAppInst_done)
        {
            printAndLogFmt(0, "Initializing AppInstUtil...");
            if (!app_inst_util_init())
                return PKG_ERROR("AppInstUtil initialization failed", ret);
        }

        if (!bgft_init())
            return PKG_ERROR("BGFT initialization failed", ret);

        printAndLogFmt(0, "Retrieving Title ID from package...");
        ret = sceAppInstUtilGetTitleIdFromPkg(fullpath, title_id, &is_app);
        if (ret)
            return PKG_ERROR("sceAppInstUtilGetTitleIdFromPkg failed", ret);

        if (!iconURI || !*iconURI)
            iconURI = "https://t4.ftcdn.net/jpg/01/25/36/71/360_F_125367167_JnrCHTqtZhAbWS3doG4tt631usPHiPnr.jpg";

        ret = sceAppInstUtilAppGetSize(title_id, &pkg_size);
        if (ret)
            return PKG_ERROR("sceAppInstUtilAppGetSize failed", ret);

        struct bgft_download_param_ex download_params;
        memset(&download_params, 0, sizeof(download_params));
        download_params.param.user_id = 0;
        download_params.param.entitlement_type = 5;
        download_params.param.id = "";
        download_params.param.content_url = fullpath;
        download_params.param.content_ex_url = "";
        download_params.param.content_name = name;
        download_params.param.icon_path = iconURI;
        download_params.param.sku_id = "";
        download_params.param.option = BGFT_TASK_OPTION_DISABLE_CDN_QUERY_PARAM;
        download_params.param.playgo_scenario_id = "0";
        download_params.param.release_date = "";
        download_params.param.package_type = "";
        download_params.param.package_sub_type = "";
        download_params.param.package_size = pkg_size;
        download_params.slot = 0;

    retry:
        printAndLogFmt(0, "Registering download task...");
        ret = sceBgftServiceIntDownloadRegisterTaskByStorageEx(&download_params, &task_id);
        if (ret == 0x80990088 || ret == 0x80990015)
        {
            printAndLogFmt(2, "Conflicting installation detected. Uninstalling existing title: [%s] %s!", title_id, name);
            ret = sceAppInstUtilAppUnInstall(&title_id[0]);
            if (ret != 0)
                return PKG_ERROR("sceAppInstUtilAppUnInstall failed", ret);
            goto retry;
        }

        if (ret)
            return PKG_ERROR("sceBgftServiceIntDownloadRegisterTaskByStorageEx failed", ret);

        if (ret == 0x80990086)
        {
            printAndLogFmt(2, "Installation already queued in notifcations, prompting user to cancel it.");
            TextNotify(222, "Already queued in notifcations\nplease cancel it and retry.");
            return ret;
        }

        ret = sceBgftServiceDownloadStartTask(task_id);
        if (ret)
            return PKG_ERROR("sceBgftDownloadStartTask failed", ret);
    }
    else
    {
        printAndLogFmt(3, "Failed to open file: %s", fullpath);
        return ret;
    }

    printAndLogFmt(0, "Allocating memory for install arguments...");
    struct install_args *args = (struct install_args *)malloc(sizeof(struct install_args));
    if (!args)
        return PKG_ERROR("Memory allocation failed", -1);

    args->title_id = strdup(title_id);
    args->task_id = task_id;
    args->path = strdup(fullpath);
    args->fname = strdup(name);
    args->is_thread = false;
    args->delete_pkg = deleteAfter;

    printAndLogFmt(0, "Starting download progress display thread...");
    displayDownloadProgress((void *)args);

    return 0;
}

uint32_t installWebPKG(const char *url, const char *name, const char *title_id, const char *iconURI)
{
    int ret = -1, task_id = -1;

    if (!bgft_init())
        return PKG_ERROR("BGFT initialization failed", ret);

    struct bgft_download_param download_params;
    memset(&download_params, 0, sizeof(download_params));

    download_params.user_id = 0;
    download_params.entitlement_type = 5;
    download_params.id = "";
    download_params.content_url = url;
    download_params.content_ex_url = "";
    download_params.content_name = name;
    download_params.icon_path = iconURI;
    download_params.sku_id = "";
    download_params.option = BGFT_TASK_OPTION_DISABLE_CDN_QUERY_PARAM;
    download_params.playgo_scenario_id = "0";
    download_params.release_date = "";
    download_params.package_type = "";
    download_params.package_sub_type = "";
    download_params.package_size = NULL;

retry:
    printAndLogFmt(0, "Registering web download task...");
    ret = sceBgftServiceIntDebugDownloadRegisterPkg(&download_params, &task_id);
    if (ret == 0x80F00633)
    {
        printAndLogFmt(2, "Incorrect NP environment setting detected. Prompting user to change it.");
        TextNotify(222, "Please change NP Environment\nin \"Debug Settings\" from\n \"NP\" to \"SP-INT\".");
        ret = 0;
    }

    if (ret == 0x80990088 || ret == 0x80990015)
    {
        printAndLogFmt(2, "Conflicting installation detected. Uninstalling existing title: [%s] %s!", title_id, name);
        ret = sceAppInstUtilAppUnInstall(title_id);
        if (ret != 0)
            return PKG_ERROR("sceAppInstUtilAppUnInstall failed", ret);
        goto retry;
    }

    if (ret == 0x80990086)
    {
        printAndLogFmt(2, "Installation already queued in notifcations, prompting user to cancel it.");
        TextNotify(222, "Already queued in notifcations\nplease cancel it and retry.");
        return ret;
    }

    if (ret)
        return PKG_ERROR("sceBgftServiceIntDebugDownloadRegisterPkg failed", ret);

    printAndLogFmt(0, "Starting web download task: %d", task_id);
    ret = sceBgftServiceDownloadStartTask(task_id);
    if (ret)
        return PKG_ERROR("sceBgftDownloadStartTask failed", ret);

    return 0;
}

bool SendInstallRequestForPS5(const char *url)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printAndLogFmt(3, "Socket server failed to creation for DPI.");
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9090);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printAndLogFmt(3, "Failed to connect to the DPI server.");
        close(sock);
        return false;
    }

    std::string request = std::string("{ \"url\" : \"") + url + "\" }";
    printAndLogFmt(0, "Sending install request: %s\n", request.c_str());

    if (send(sock, request.c_str(), request.size(), 0) < 0)
    {
        printAndLogFmt(3, "Failed to send install request to DPI.");
        close(sock);
        return false;
    }

    char buffer[1024] = {0};
    int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
    close(sock);

    if (bytesRead <= 0)
    {
        printAndLogFmt(3, "No response or failed to read response from DPI.");
        return false;
    }

    buffer[bytesRead] = '\0';
    printAndLogFmt(0, "Response: %s", buffer);

    return std::string(buffer).find("\"res\" : \"0\"") != std::string::npos;
}

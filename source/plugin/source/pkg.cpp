#include "../headers/includes.hpp"

bool sceAppInst_done = false;
static bool s_bgft_done = false;
static void *s_bgft_heap = nullptr;
static int s_lastPackageInstallError = 0;

typedef char playgo_scenario_id_t[3];
typedef char content_id_t[0x30];
typedef char language_t[8];

typedef struct
{
    content_id_t content_id;
    int content_type;
    int content_platform;
} SceAppInstallPkgInfo;

typedef struct
{
    const char *uri;
    const char *ex_uri;
    const char *playgo_scenario_id;
    const char *content_id;
    const char *content_name;
    const char *icon_url;
} MetaInfo;

typedef struct
{
    language_t languages[30];
    playgo_scenario_id_t playgo_scenario_ids[64];
    content_id_t content_ids[64];
    long unknown[810];
} PlayGoInfo;

typedef int (*SceAppInstUtilInstallByPackageFn)(MetaInfo *arg1, SceAppInstallPkgInfo *pkg_info, PlayGoInfo *arg2);

static SceAppInstUtilInstallByPackageFn ResolveAppInstUtilInstallByPackage()
{
    static SceAppInstUtilInstallByPackageFn installFn = nullptr;
    if (installFn != nullptr)
        return installFn;

    int handle = sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx", 0, 0, 0, 0, 0);
    if (handle < 0)
        handle = sceKernelLoadStartModule("libSceAppInstUtil.sprx", 0, 0, 0, 0, 0);
    
    if (handle >= 0)
    {
        void *symbolAddress = nullptr;
        if (sceKernelDlsym(handle, "sceAppInstUtilInstallByPackage", &symbolAddress) == 0 && symbolAddress != nullptr)
        {
            installFn = reinterpret_cast<SceAppInstUtilInstallByPackageFn>(symbolAddress);
            return installFn;
        }
    }
    return nullptr;
}

int PKG_ERROR(const char *name, int ret)
{
    s_lastPackageInstallError = ret;
    printAndLogFmt(3, "%s error: %x", name, ret);
    return ret;
}

bool app_inst_util_init(void)
{
    if (sceAppInst_done)
        return true;

    printAndLogFmt(0, "Initializing AppInstUtil...");

    int ret = sceAppInstUtilInitialize();
    if (ret)
    {
        printAndLogFmt(4, "sceAppInstUtilInitialize failed: 0x%08X", ret);
        sceAppInst_done = false;
        return false;
    }

    sceAppInst_done = true;
    return true;
}

void app_inst_util_fini(void)
{
    if (!sceAppInst_done)
        return;

    int ret = sceAppInstUtilTerminate();
    if (ret)
        printAndLogFmt(4, "sceAppInstUtilTerminate failed: 0x%08X", ret);

    sceAppInst_done = false;
}

bool bgft_init(void)
{
    if (s_bgft_done)
        return true;

    if (s_bgft_heap == nullptr)
    {
        s_bgft_heap = malloc(BGFT_HEAP_SIZE);
        if (s_bgft_heap == nullptr)
        {
            s_lastPackageInstallError = -1;
            printAndLogFmt(4, "Failed to allocate BGFT heap.");
            return false;
        }

        memset(s_bgft_heap, 0, BGFT_HEAP_SIZE);
    }

    bgft_init_params params = {};
    params.heap = s_bgft_heap;
    params.heapSize = BGFT_HEAP_SIZE;

    printAndLogFmt(0, "Initializing BGFT...");
    int ret = sceBgftServiceInit(&params);
    if (ret != 0)
    {
        s_lastPackageInstallError = ret;
        printAndLogFmt(4, "sceBgftServiceInit failed: 0x%08X", ret);
        free(s_bgft_heap);
        s_bgft_heap = nullptr;
        s_bgft_done = false;
        return false;
    }

    s_bgft_done = true;
    s_lastPackageInstallError = 0;
    return true;
}

void bgft_fini(void)
{
    if (s_bgft_done)
    {
        int ret = sceBgftServiceTerm();
        if (ret != 0)
            printAndLogFmt(4, "sceBgftServiceTerm failed: 0x%08X", ret);
    }

    if (s_bgft_heap != nullptr)
    {
        free(s_bgft_heap);
        s_bgft_heap = nullptr;
    }

    s_bgft_done = false;
}

void *displayDownloadProgress(void *arguments)
{
    return arguments;
}

typedef int (*BgftRegisterPackageTaskFn)(bgft_download_param *params, int *task_id);

static BgftRegisterPackageTaskFn ResolveBgftRegisterPackageTask()
{
    static BgftRegisterPackageTaskFn registerTask = nullptr;
    if (registerTask != nullptr)
        return registerTask;

    int handle = sceKernelLoadStartModule("/system/common/lib/libSceBgft.sprx", 0, 0, 0, 0, 0);
    if (handle < 0)
        handle = sceKernelLoadStartModule("libSceBgft.sprx", 0, 0, 0, 0, 0);

    if (handle < 0)
    {
        s_lastPackageInstallError = handle;
        printAndLogFmt(4, "Failed to load libSceBgft.sprx: 0x%08X", handle);
        return nullptr;
    }

    const char *symbols[] = {
        "sceBgftServiceIntDownloadRegisterTask", // Flatz uses this for base games!
        "sceBgftServiceIntDebugDownloadRegisterPkg",
        "sceBgftDebugDownloadRegisterPkg",
        "sceBgftServiceDownloadRegisterTask"
    };

    for (size_t i = 0; i < sizeof(symbols) / sizeof(symbols[0]); i++)
    {
        void *symbolAddress = nullptr;
        int ret = sceKernelDlsym(handle, symbols[i], &symbolAddress);
        if (ret == 0 && symbolAddress != nullptr)
        {
            registerTask = reinterpret_cast<BgftRegisterPackageTaskFn>(symbolAddress);
            printAndLogFmt(0, "Resolved BGFT register function: %s", symbols[i]);
            return registerTask;
        }
    }

    s_lastPackageInstallError = -5;
    printAndLogFmt(4, "Failed to resolve BGFT register function.");
    return nullptr;
}

static uint32_t InstallByPackageUri(const char *uri, const char *name, const char *iconURI)
{
    if (uri == nullptr || uri[0] == '\0')
        return PKG_ERROR("InstallByPackageUri", -1);

    if (!app_inst_util_init())
        return PKG_ERROR("AppInstUtil initialization failed", -1);

    MetaInfo meta = {};
    meta.uri = uri;
    meta.ex_uri = "";
    meta.playgo_scenario_id = "";
    meta.content_id = "";
    meta.content_name = name != nullptr && name[0] != '\0' ? name : "Package";
    meta.icon_url = iconURI != nullptr ? iconURI : "";

    SceAppInstallPkgInfo pkgInfo = {};
    PlayGoInfo playGo = {};

    SceAppInstUtilInstallByPackageFn installFn = ResolveAppInstUtilInstallByPackage();
    if (installFn == nullptr)
        return PKG_ERROR("ResolveAppInstUtilInstallByPackage failed", -5);

    printAndLogFmt(0, "Requesting install by package: %s", uri);
    int ret = installFn(&meta, &pkgInfo, &playGo);
    if (ret != 0)
        return PKG_ERROR("sceAppInstUtilInstallByPackage failed", ret);

    return 0;
}

uint32_t installPKG(const char *fullpath, const char *name, const char *iconURI, bool deleteAfter)
{
    uint32_t result = InstallByPackageUri(fullpath, name, iconURI);
    if (result == 0 && deleteAfter && fullpath != nullptr && fullpath[0] != '\0')
    {
        printAndLogFmt(1, "Delete-after-install requested; leaving package cleanup to the caller.");
    }
    return result;
}

uint32_t installWebPKG(const char *url, const char *name, const char *title_id, const char *iconURI)
{
    (void)title_id;
    return InstallByPackageUri(url, name, iconURI);
}

int installManifestPKG(const char *manifestUrl, const char *name, const char *contentId, const char *iconURI, unsigned long packageSize, const char *packageType)
{
    (void)iconURI;

    if (manifestUrl == nullptr || manifestUrl[0] == '\0')
        return PKG_ERROR("installManifestPKG missing manifest URL", -1);

    if (contentId == nullptr || contentId[0] == '\0')
        return PKG_ERROR("installManifestPKG missing content id", -2);

    if (!app_inst_util_init())
        return PKG_ERROR("AppInstUtil initialization failed", -3);

    if (!bgft_init())
        return PKG_ERROR("BGFT initialization failed", s_lastPackageInstallError != 0 ? s_lastPackageInstallError : -4);

    int user_id = -1;
    int ret = sceUserServiceGetForegroundUser(&user_id);
    if (ret != 0)
        return PKG_ERROR("sceUserServiceGetForegroundUser failed", ret);

    bgft_download_param params = {};
    params.user_id = user_id;
    params.entitlement_type = 5;
    params.id = contentId;
    params.content_url = manifestUrl;
    params.content_ex_url = "";
    params.content_name = name != nullptr && name[0] != '\0' ? name : "Package";
    params.icon_path = "";
    params.sku_id = "";
    params.option = BGFT_TASK_OPTION_DISABLE_CDN_QUERY_PARAM;
    params.playgo_scenario_id = "0";
    params.release_date = "";
    params.package_type = packageType != nullptr && packageType[0] != '\0' ? packageType : "PS4GD";
    params.package_sub_type = "";
    params.package_size = packageSize;

    int task_id = -1;
    BgftRegisterPackageTaskFn registerTask = ResolveBgftRegisterPackageTask();
    if (registerTask == nullptr)
        return PKG_ERROR("ResolveBgftRegisterPackageTask failed", s_lastPackageInstallError != 0 ? s_lastPackageInstallError : -5);

    printAndLogFmt(0, "Registering manifest BGFT task: %s", manifestUrl);
    ret = registerTask(&params, &task_id);
    if (ret != 0)
        return PKG_ERROR("BGFT register function failed", ret);

    s_lastPackageInstallError = 0;
    printAndLogFmt(1, "Manifest BGFT task registered: %d", task_id);
    return task_id;
}

int getLastPackageInstallError(void)
{
    return s_lastPackageInstallError;
}

bool SendInstallRequestForPS5(const char *url)
{
    if (url == nullptr || url[0] == '\0')
        return false;

    printAndLogFmt(0, "Sending install request through AppInstUtil: %s", url);
    return installWebPKG(url, "PS5 Download", "", "") == 0;
}

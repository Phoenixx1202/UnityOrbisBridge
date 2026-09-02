#include "../headers/includes.hpp"
#include <mutex>
#include <string>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

bool sceAppInst_done = false;
static bool s_bgft_done = false;
static void *s_bgft_heap = nullptr;
static int s_lastPackageInstallError = 0;
static const int DUSKARYON_MANIFEST_SERVER_PORT = 9998;
static std::atomic<bool> s_manifest_server_running(false);
static int s_manifest_server_socket = -1;
static std::mutex s_manifest_mutex;
static std::string s_manifest_json;

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

typedef int (*BgftInitFn)(bgft_init_params *params);
typedef int (*BgftTermFn)(void);
typedef int (*BgftRegisterPackageTaskFn)(bgft_download_param *params, int *task_id);

static int LoadBgftModule()
{
    int handle = sceKernelLoadStartModule("/system/common/lib/libSceBgft.sprx", 0, 0, 0, 0, 0);
    if (handle < 0)
        handle = sceKernelLoadStartModule("libSceBgft.sprx", 0, 0, 0, 0, 0);

    if (handle < 0)
    {
        s_lastPackageInstallError = handle;
        printAndLogFmt(4, "Failed to load libSceBgft.sprx: 0x%08X", handle);
    }

    return handle;
}

template <typename T>
static T ResolveBgftSymbol(const char *const *symbols, size_t symbolCount)
{
    int handle = LoadBgftModule();
    if (handle < 0)
        return nullptr;

    for (size_t i = 0; i < symbolCount; i++)
    {
        void *symbolAddress = nullptr;
        int ret = sceKernelDlsym(handle, symbols[i], &symbolAddress);
        if (ret == 0 && symbolAddress != nullptr)
        {
            printAndLogFmt(0, "Resolved BGFT symbol: %s", symbols[i]);
            return reinterpret_cast<T>(symbolAddress);
        }
    }

    s_lastPackageInstallError = -5;
    printAndLogFmt(4, "Failed to resolve BGFT symbol.");
    return nullptr;
}

static BgftInitFn ResolveBgftInit()
{
    static BgftInitFn initFn = nullptr;
    if (initFn != nullptr)
        return initFn;

    const char *symbols[] = {
        "sceBgftServiceIntInit",
        "sceBgftInitialize",
        "sceBgftServiceInit"
    };

    initFn = ResolveBgftSymbol<BgftInitFn>(symbols, sizeof(symbols) / sizeof(symbols[0]));
    return initFn;
}

static BgftTermFn ResolveBgftTerm()
{
    static BgftTermFn termFn = nullptr;
    if (termFn != nullptr)
        return termFn;

    const char *symbols[] = {
        "sceBgftServiceIntTerm",
        "sceBgftFinalize",
        "sceBgftServiceTerm"
    };

    termFn = ResolveBgftSymbol<BgftTermFn>(symbols, sizeof(symbols) / sizeof(symbols[0]));
    return termFn;
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

    BgftInitFn initFn = ResolveBgftInit();
    if (initFn == nullptr)
    {
        free(s_bgft_heap);
        s_bgft_heap = nullptr;
        s_bgft_done = false;
        return false;
    }

    printAndLogFmt(0, "Initializing BGFT...");
    int ret = initFn(&params);
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
        BgftTermFn termFn = ResolveBgftTerm();
        if (termFn != nullptr)
        {
            int ret = termFn();
            if (ret != 0)
                printAndLogFmt(4, "BGFT term failed: 0x%08X", ret);
        }
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

static BgftRegisterPackageTaskFn ResolveBgftRegisterPackageTask()
{
    static BgftRegisterPackageTaskFn registerTask = nullptr;
    if (registerTask != nullptr)
        return registerTask;

    const char *symbols[] = {
        "sceBgftServiceIntDownloadRegisterTask", // Flatz uses this for base games!
        "sceBgftServiceIntDebugDownloadRegisterPkg",
        "sceBgftDebugDownloadRegisterPkg",
        "sceBgftServiceDownloadRegisterTask"
    };

    registerTask = ResolveBgftSymbol<BgftRegisterPackageTaskFn>(symbols, sizeof(symbols) / sizeof(symbols[0]));
    return registerTask;
}

static uint32_t InstallByPackageUri(const char *uri, const char *name, const char *iconURI)
{
    s_lastPackageInstallError = 0;

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

    s_lastPackageInstallError = 0;
    return 0;
}

static uint32_t InstallLocalPackageFile(const char *fullpath, bool deleteAfter)
{
    s_lastPackageInstallError = 0;

    if (fullpath == nullptr || fullpath[0] == '\0')
        return PKG_ERROR("InstallLocalPackageFile", -1);

    if (!app_inst_util_init())
        return PKG_ERROR("AppInstUtil initialization failed", -1);

    printAndLogFmt(0, "Requesting local package install: %s", fullpath);
    int ret = sceAppInstUtilAppInstallPkg(fullpath, nullptr);
    if (ret != 0)
        return PKG_ERROR("sceAppInstUtilAppInstallPkg failed", ret);

    if (deleteAfter)
        printAndLogFmt(1, "Delete-after-install requested; leaving package cleanup to the caller.");

    s_lastPackageInstallError = 0;
    return 0;
}

uint32_t installPKG(const char *fullpath, const char *name, const char *iconURI, bool deleteAfter)
{
    (void)name;
    (void)iconURI;
    return InstallLocalPackageFile(fullpath, deleteAfter);
}

uint32_t installWebPKG(const char *url, const char *name, const char *title_id, const char *iconURI)
{
    (void)title_id;
    return InstallByPackageUri(url, name, iconURI);
}

static bool SendAll(int fd, const char *data, size_t size)
{
    size_t sent = 0;
    while (sent < size)
    {
        ssize_t ret = send(fd, data + sent, size - sent, MSG_NOSIGNAL);
        if (ret <= 0)
            return false;

        sent += static_cast<size_t>(ret);
    }

    return true;
}

static void HandleManifestClient(int client)
{
    char request[1024] = {};
    ssize_t requestSize = recv(client, request, sizeof(request) - 1, 0);
    bool headOnly = requestSize > 0 && strncmp(request, "HEAD ", 5) == 0;

    std::string body;
    {
        std::lock_guard<std::mutex> lock(s_manifest_mutex);
        body = s_manifest_json;
    }

    if (body.empty())
        body = "{}";

    char header[512] = {};
    snprintf(header,
             sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Cache-Control: no-store\r\n"
             "Connection: close\r\n"
             "\r\n",
             body.size());

    SendAll(client, header, strlen(header));
    if (!headOnly)
        SendAll(client, body.c_str(), body.size());

    close(client);
}

static void ManifestServerLoop(int serverSocket)
{
    while (s_manifest_server_running.load())
    {
        struct sockaddr_in clientAddress = {};
        socklen_t clientAddressLength = sizeof(clientAddress);
        int client = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddressLength);
        if (client < 0)
            continue;

        HandleManifestClient(client);
    }

    close(serverSocket);
}

static bool StartManifestJsonServer(const char *manifestJson, const char *localIp, char *outUrl, size_t outUrlSize)
{
    if (manifestJson == nullptr || manifestJson[0] == '\0')
        return false;

    {
        std::lock_guard<std::mutex> lock(s_manifest_mutex);
        s_manifest_json = manifestJson;
    }

    const char *hostIp = (localIp != nullptr && localIp[0] != '\0') ? localIp : "127.0.0.1";
    snprintf(outUrl, outUrlSize, "http://%s:%d/manifest.json", hostIp, DUSKARYON_MANIFEST_SERVER_PORT);

    if (s_manifest_server_running.load())
        return true;

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        s_lastPackageInstallError = serverSocket;
        printAndLogFmt(4, "Manifest server socket failed: 0x%08X", serverSocket);
        return false;
    }

    int enabled = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

    struct sockaddr_in address = {};
    address.sin_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(DUSKARYON_MANIFEST_SERVER_PORT);

    int ret = bind(serverSocket, reinterpret_cast<struct sockaddr *>(&address), sizeof(address));
    if (ret < 0)
    {
        s_lastPackageInstallError = ret;
        printAndLogFmt(4, "Manifest server bind failed: 0x%08X", ret);
        close(serverSocket);
        return false;
    }

    ret = listen(serverSocket, 4);
    if (ret < 0)
    {
        s_lastPackageInstallError = ret;
        printAndLogFmt(4, "Manifest server listen failed: 0x%08X", ret);
        close(serverSocket);
        return false;
    }

    s_manifest_server_socket = serverSocket;
    s_manifest_server_running.store(true);

    try
    {
        std::thread(ManifestServerLoop, serverSocket).detach();
    }
    catch (...)
    {
        s_manifest_server_running.store(false);
        close(serverSocket);
        s_manifest_server_socket = -1;
        s_lastPackageInstallError = -6;
        printAndLogFmt(4, "Manifest server thread failed.");
        return false;
    }

    printAndLogFmt(1, "Manifest JSON server listening: %s", outUrl);
    return true;
}

int installManifestPKG(const char *manifestUrl, const char *name, const char *contentId, const char *iconURI, unsigned long packageSize, const char *packageType)
{
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
    params.icon_path = iconURI != nullptr && iconURI[0] != '\0' ? iconURI : "";
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

int installManifestPKGFromJson(const char *manifestJson, const char *localIp, const char *name, const char *contentId, const char *iconURI, unsigned long packageSize, const char *packageType)
{
    char manifestUrl[256] = {};
    if (!StartManifestJsonServer(manifestJson, localIp, manifestUrl, sizeof(manifestUrl)))
        return PKG_ERROR("StartManifestJsonServer failed", s_lastPackageInstallError != 0 ? s_lastPackageInstallError : -1);

    return installManifestPKG(manifestUrl, name, contentId, iconURI, packageSize, packageType);
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

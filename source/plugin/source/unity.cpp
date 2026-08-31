#include "../headers/includes.hpp"

jbc_cred g_Creds, g_RootCreds;

static bool nativeDialogInitialized = false;
static bool hasBrokenFromTheSandbox = false;

bool PS5_Jailbreak()
{
    struct sockaddr_in address;
    address.sin_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons(9028);
    memset(&address.sin_zero, 0, sizeof(address.sin_zero));
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr.s_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return false;

    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        puts("Failed to connect to daemon");
        close(sock);
        return false;
    }

    HijackerCommand cmd;
    cmd.PID = getpid();
    cmd.cmd = JAILBREAK_CMD;

    if (send(sock, (void *)&cmd, sizeof(cmd), MSG_NOSIGNAL) == -1)
    {
        puts("failed to send command");
        close(sock);
        return false;
    }

    recv(sock, reinterpret_cast<void *>(&cmd), sizeof(cmd), MSG_NOSIGNAL);
    close(sock);

    if (cmd.ret != 0 && cmd.ret != -1337)
    {
        puts("Jailbreak has failed");
        return false;
    }

    return true;
}

bool PS5_WhitelistJailbreak()
{
    std::string json = "{\"PID\":\"" + std::to_string(getpid()) + "\"}";

    remove("/download0/etahen_jailbreak");

    sceKernelUsleep(500);

    std::ofstream outFile("/download0/etahen_jailbreak");
    if (!outFile.is_open())
        return false;

    outFile << json;
    outFile.close();

    showDialogMessage((char *)"Waiting for etaHEN... (MAX 30 secs)");
    auto start_time = std::chrono::high_resolution_clock::now();

    void *addr = nullptr;

    while (true)
    {
        int handle = sceKernelLoadStartModule("libSceSystemService.sprx", 0, 0, 0, 0, 0);
        sceKernelDlsym(handle, "sceSystemServiceLaunchApp", (void **)&addr);

        if (addr != nullptr)
            break;

        auto time_now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(time_now - start_time).count() > 30)
        {
            sceMsgDialogTerminate();
            return false;
        }

        sceKernelUsleep(500);
    }

    sceMsgDialogTerminate();
    return true;
}

bool IsFreeOfSandbox()
{
    if (hasBrokenFromTheSandbox)
        return true;

    FILE *filePtr = fopen("/user/.jailbreak", "w");
    if (!filePtr)
        return false;

    fclose(filePtr);
    remove("/user/.jailbreak");

    return true;
}

void EnterSandbox()
{
    if (IsPlayStation5())
        return;

    if (IsFreeOfSandbox())
        jbc_set_cred(&g_Creds);

    hasBrokenFromTheSandbox = false;
}

void BreakFromSandbox()
{
    if (IsPlayStation5())
    {
        if (hasBrokenFromTheSandbox)
            return;

        if (!PS5_WhitelistJailbreak())
            PS5_Jailbreak();

        hasBrokenFromTheSandbox = true;
        return;
    }
    else
    {
        if (IsFreeOfSandbox())
            return;

        jbc_get_cred(&g_Creds);
        g_RootCreds = g_Creds;
        jbc_jailbreak_cred(&g_RootCreds);
        jbc_set_cred(&g_RootCreds);
    }

    hasBrokenFromTheSandbox = true;
}

void MountInSandbox(const char *systemPath, const char *mountName)
{
    if (IsPlayStation5())
        return;

    if (jbc_mount_in_sandbox(systemPath, mountName) == 0)
        PrintToConsole("Mount: Passed", 0);
    else
        PrintToConsole("Mount: Failed", 2);
}

void UnmountFromSandbox(const char *mountName)
{
    if (IsPlayStation5())
        return;

    if (jbc_unmount_in_sandbox(mountName) == 0)
        PrintToConsole("Unmount: Passed", 0);
    else
        PrintToConsole("Unmount: Failed", 2);
}

void InitializeNativeDialogs()
{
    if (!nativeDialogInitialized)
    {
        nativeDialogInitialized = true;

        printAndLogFmt(0, "Initiating native message dialogs...");

        sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);
        sceCommonDialogInitialize();
        sceMsgDialogInitialize();

        sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx", 0, 0, 0, 0, 0);
        sceKernelLoadStartModule("/system/common/lib/libSceBgft.sprx", 0, 0, 0, 0, 0);
    }
}

void ExitApplication()
{
    EnterSandbox();

    sceSystemServiceLoadExec("exit", 0);
}

void UpdateViaHomebrewStore(const char *query)
{
    BreakFromSandbox();
    (void)query;
    printAndLogFmt(1, "UpdateViaHomebrewStore is not enabled in this bridge build.");
}

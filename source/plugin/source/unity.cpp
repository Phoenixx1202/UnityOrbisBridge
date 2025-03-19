#include "../headers/includes.hpp"

jbc_cred g_Creds, g_RootCreds;

static bool nativeDialogInitialized = false;
static bool hasBrokenFromTheSandbox = false;

int HJOpenConnectionforBC()
{
    struct sockaddr_in address;
    address.sin_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons(9028); // command server port
    memset(&address.sin_zero, 0, sizeof(address.sin_zero));
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr.s_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        close(sock);
        return -1;
    }

    return sock;
}

bool HJJailbreakforBC(int &sock)
{
    // send jailbreak IPC command
    HijackerCommand cmd;
    cmd.PID = getpid();
    cmd.cmd = JAILBREAK_CMD;

    if (send(sock, (void *)&cmd, sizeof(cmd), MSG_NOSIGNAL) == -1)
    {
        puts("failed to send command");
        return false;
    }
    else
    {
        // get ret value from daemon
        recv(sock, reinterpret_cast<void *>(&cmd), sizeof(cmd), MSG_NOSIGNAL);
        close(sock);
        sock = -1;

        if (cmd.ret != 0 && cmd.ret != -1337)
        {
            puts("Jailbreak has failed");
            return false;
        }
        return true;
    }

    return false;
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
    if (IsFreeOfSandbox())
        return;

    if (IsPlayStation5())
    {
        int ret = HJOpenConnectionforBC();
        if (ret < 0)
        {
            puts("Failed to connect to daemon");
            return;
        }

        if (!HJJailbreakforBC(ret))
        {
            puts("Jailbreak failed");
            return;
        }
    }
    else
    {
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

        bool wasAlreadyFree = false;

        if (IsFreeOfSandbox())
            wasAlreadyFree = true;

        BreakFromSandbox();

        printAndLog(1, "Initiating native dialogs...");

        sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG);
        sceCommonDialogInitialize();
        sceMsgDialogInitialize();

        sceKernelLoadStartModule("/system/common/lib/libSceAppInstUtil.sprx", 0, NULL, 0, NULL, NULL);
        sceKernelLoadStartModule("/system/common/lib/libSceBgft.sprx", 0, NULL, 0, NULL, NULL);

        if (!wasAlreadyFree)
            EnterSandbox();
    }
}

void ExitApplication()
{
    EnterSandbox();

    sceSystemServiceLoadExec("exit", 0);
}

void UpdateViaHomebrewStore(const char *query)
{
    int32_t userId = 0;
    char *titleId = nullptr;

    BreakFromSandbox();

    while (!IsFreeOfSandbox())
        sceKernelSleep(1);

    InitializeNativeDialogs();

    if (!CheckIfAppExists("NPXS39041"))
    {
        printAndLog(0, "The Homebrew Store currently isnt installed...");

        const char *storeUrl = IsPlayStation5()
                                   ? "https://pkg-zone.com/update/Store-R2-PS5.pkg"
                                   : "https://pkg-zone.com/update/Store-R2.pkg";

        printAndLog(0, "Attempting to download the Homebrew Store...");

        DownloadWebFile(storeUrl, "/user/app/store_download.pkg", false, "Homebrew Store");

        while (downloadProgress < 99 && !hasDownloadCompleted && !downloadErrorOccured)
            sceKernelSleep(1);

        if (hasDownloadCompleted && !downloadErrorOccured)
        {
            printAndLog(0, "Now installing the Homebrew Store...");

            InstallLocalPackage("/user/app/store_download.pkg", "Homebrew Store", true);

            if (IsPlayStation5())
            {
                while (!CheckIfAppExists("NPXS39041"))
                    sceKernelSleep(1);
            }

            ResetDownloadVars();
        }
        else
        {
            printAndLog(0, "Failed to download the Homebrew Store!");

            return;
        }
    }
    else
        printAndLog(0, "Homebrew Store is installed, launching...");

    if (!IsPlayStation5())
    {
        int storeId = sceSystemServiceGetAppIdOfMiniApp();
        if ((storeId & ~0xFFFFFF) == 0x60000000 && if_exists("/mnt/sandbox/pfsmnt/NPXS39041-app0/"))
        {
            printAndLog(0, "Closing Homebrew Store for re-launch!...");

            sceSystemServiceKillApp(storeId, -1, 0, 0);
        }
    }


    if (!IsPlayStation5())
    {
        if (query != nullptr && query[0] != '\0')
            titleId = strdup(query);
        else
            sceLncUtilGetAppTitleId(sceSystemServiceGetAppIdOfBigApp(), titleId);
    }
    else
    {
        if (query != nullptr && query[0] != '\0')
            titleId = strdup(query);
        else
        {
            printAndLog(0, "No query provided to launch Homebrew Store...");

            return;
        }
    }

    const char *argv[] = {titleId, nullptr};

    LncAppParam param;
    param.size = sizeof(LncAppParam);
    param.user_id = userId;
    param.app_opt = 0;
    param.crash_report = 0;
    param.LaunchAppCheck_flag = LaunchApp_SkipSystemUpdate;
  
    printAndLog(0, "Attempting to launch Homebrew Store with \"%s\" query...", titleId);

    uint32_t res = sceLncUtilLaunchApp("NPXS39041", argv, &param);
    if ((res & 0x80000000) && res != 2157182993)
        printAndLog(0, "App launch failed with error code: %u", res);

    if (res == 2157182993)
        printAndLog(0, "Homebrew Store has launched successfully!");

    if (titleId)
        free(titleId);
}
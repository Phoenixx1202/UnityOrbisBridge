#include "../headers/includes.hpp"

jbc_cred g_Creds, g_RootCreds;

bool IsFreeOfSandbox()
{
  FILE *filePtr =
      fopen("/user/.jailbreak", "w");

  if (!filePtr)
    return false;

  fclose(filePtr);

  return (remove("/user/.jailbreak") == 0);
}

void EnterSandbox()
{
  if (IsFreeOfSandbox())
    jbc_set_cred(&g_Creds);
}

void BreakFromSandbox()
{
  if (IsFreeOfSandbox())
    return;

  jbc_get_cred(&g_Creds);
  g_RootCreds = g_Creds;
  jbc_jailbreak_cred(&g_RootCreds);
  jbc_set_cred(&g_RootCreds);
}

void MountInSandbox(const char *systemPath, const char *mountName)
{
  if (jbc_mount_in_sandbox(systemPath, mountName) == 0)
    PrintToConsole("Mount: Passed", 0);
  else
    PrintToConsole("Mount: Failed", 2);
}

void UnmountFromSandbox(const char *mountName)
{
  if (jbc_unmount_in_sandbox(mountName) == 0)
    PrintToConsole("Unmount: Passed", 0);
  else
    PrintToConsole("Unmount: Failed", 2);
}

void ExitApplication()
{
  EnterSandbox();
  sceSystemServiceLoadExec("exit", 0);
}

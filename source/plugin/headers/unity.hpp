#pragma once

enum Commands : int
{
  INVALID_CMD = -1,
  ACTIVE_CMD = 0,
  LAUNCH_CMD,
  PROCLIST_CMD,
  KILL_CMD,
  KILL_APP_CMD,
  JAILBREAK_CMD
};

struct HijackerCommand
{
  int magic = 0xDEADBEEF;
  Commands cmd = INVALID_CMD;
  int PID = -1;
  int ret = -1337;
  char msg1[0x500];
  char msg2[0x500];
};

extern jbc_cred g_Creds, g_RootCreds;

bool PS5_Jailbreak();
bool PS5_WhitelistJailbreak();

extern "C"
{
  bool IsFreeOfSandbox();
  void EnterSandbox();
  void BreakFromSandbox();
  void MountInSandbox(const char *systemPath, const char *mountName);
  void UnmountFromSandbox(const char *mountName);
  void InitializeNativeDialogs();
  void ExitApplication();
  void UpdateViaHomebrewStore(const char *query);
}

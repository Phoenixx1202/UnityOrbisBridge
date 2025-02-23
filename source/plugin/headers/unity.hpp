#pragma once

extern jbc_cred g_Creds, g_RootCreds;

extern "C"
{
  void InitializeNativeDialogs();
  bool IsFreeOfSandbox();
  void EnterSandbox();
  void BreakFromSandbox();
  void MountInSandbox(const char *systemPath, const char *mountName);
  void UnmountFromSandbox(const char *mountName);
  void ExitApplication();
}

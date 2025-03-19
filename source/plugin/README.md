# UnityOrbisBridge (.PRX)
A library with various PS4 SDK functions to be called by the matching [.DLL for Unity](/source/Unity-API) for homebrew development.

## Code Snippets / Usage

### Web Interaction
<details>
  <summary>Click to expand</summary>

  ```c
  string[] infoTypes = { "downloaded", "filesize", "progress" };
  char *info = GetDownloadInfo(infoType[0].c_str());
  ```

  ```c
  bool finished = HasDownloadCompleted();
  ```

  ```c
  bool error = HasDownloadErrorOccured();
  ```

  ```c
  ResetDownloadVars();
  ```

  ```c
  CancelDownload();
  ```

  ```c
  bool isBackgroundDL = true;
  bool displayName = "Content Title"; // used for displaying download progress on foreground downloads
  DownloadWebFile("http://example.com/file.txt", "/user/data/directory", isBackgroundDL, displayName);
  ```

  ```c
  size_t out_size;
  char *bytes = DownloadAsBytes("http://example.com/file.png", &out_size);
  ```

</details>

### Logging and Notifications
<details>
  <summary>Click to expand</summary>

  ```c
  PrintToConsole("This is a log message", 0);
  ```

  ```c
  PrintAndLog("Error occurred", 1, "/user/data/log.txt");
  ```

  ```c
  TextNotify(1, "Hello, World!");
  ```

  ```c
  ImageNotify("/user/data/directory/image.png", "Notification message");
  ```

</details>

### System Information
<details>
  <summary>Click to expand</summary>

  ```c
  bool isPS5 = IsPlayStation5();
  ```

  ```c
  const char *firmwareVersion = GetFWVersion();
  ```

  ```c
  const char *consoleType = GetConsoleType();
  ```

  ```c
  int32_t languageID = GetSystemLanguageID();
  ```

  ```c
  const char *language = GetSystemLanguage();
  ```

  ```c
  uint32_t cpuTemp = GetCPUTemperature();
  ```

  ```c
  uint32_t socTemp = GetSOCTemperature();
  ```

</details>

### System Control
<details>
  <summary>Click to expand</summary>

  ```c
  SetTemperatureLimit(75);
  ```

  ```c
  const char *input = GetKeyboardInput("Title", "Default Text");
  ```

  ```c
  AlarmBuzzer(1);
  ```

  ```c
  RunCMDAsRoot(functionPointer, argumentPointer, 0);
  ```

</details>

### File System Operations
<details>
  <summary>Click to expand</summary>

  ```c
  string[] infoTypes = { "percentUsed", "totalSpace", "usedSpace", "freeSpace" };
  const char *diskInfo = GetDiskInfo(infoTypes[3].c_str());
  ```

  ```c
  WriteFile("Hello, World!", "/user/data/file.txt");
  ```

  ```c
  AppendFile("Appended text", "/user/data/file.txt");
  ```

  ```c
  MountRootDirectories();
  ```

  ```c
  bool deleteAfter = true;
  bool displayName = "Content Title"; // used for displaying download progress on foreground downloads
  InstallLocalPackage("/user/data/pkg", displayName, deleteAfter);
  ```

  ```c
  InstallWebPackage("http://example.com/content.pkg", "Content Title", "http://example.com/icon.png");
  ```

  ```c
  CheckIfAppExists("CUSA00000");
  ```

</details>

### Application Operations
<details>
  <summary>Click to expand</summary>
  
  ```c
  InitializeNativeDialogs();
  ```
  
  ```c
  bool isFree = IsFreeOfSandbox();
  ```

  ```c
  EnterSandbox();
  ```

  ```c
  BreakFromSandbox();
  ```

  ```c
  MountInSandbox("/mnt/ext0/system", "externalDrive");
  ```

  ```c
  UnmountFromSandbox("externalDrive");
  ```

  ```c
  ExitApplication();
  ```

  ```c
  UpdateViaHomebrewStore("CUSA00000"); // this can be title ID, app name, or left empty to auto-pass title ID
  ```
</details>

## License
This project is licensed under the GNU General Public License v2.0 - see the [LICENSE](../../LICENSE) file for details.

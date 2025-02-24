# UnityOrbisBridge (.DLL) API for Unity
A library to import all the the functions from the [plugin](/source/plugin/) / `.prx` for quick and easy usage in your Unity projects; best if used with [UOBWrapper](/source/wrapper/UOBWrapper.cs).

## Features / Functions
### Web Interaction
<details>
  <summary>Click to expand</summary>
  
  - ```csharp
    void ResetDownloadVars()
    IntPtr GetDownloadInfo(string info)
    long GetDownloadError()
    void CancelDownload()
    IntPtr DownloadAsBytes(string url, out int size)
    ```
    
</details>

### Logging and Notifications
<details>
  <summary>Click to expand</summary>
  
  - ```csharp
    void PrintToConsole(const char *message, int type)
    void PrintAndLog(const char *message, int type, const char *file)
    void TextNotify(int type, string message)
    void ImageNotify(string imageURL, string message)
    ```

</details>

### System Information
<details>
  <summary>Click to expand</summary>
  
  - ```csharp
       IntPtr GetFWVersion()
       IntPtr GetConsoleType()
       int GetSystemLanguageID()
       IntPtr GetSystemLanguage()
    ```

</details>

### System Control
<details>
  <summary>Click to expand</summary>

  - ```csharp
    void SetTemperatureLimit(byte limit)
    IntPtr GetKeyboardInput(string title = "", string initialText = "")
    void AlarmBuzzer(int type)
    void RunCMDAsRoot(IntPtr function, IntPtr arg, int cwdMode)
    ```

</details>

### File System Operations
<details>
  <summary>Click to expand</summary>

  - ```csharp
    void WriteFile(string content, string file)
    void AppendFile(string content, string file)
    void MountRootDirectories()
    void InstallLocalPackage(string uri, string name, bool deleteAfter)
    void DownloadAndInstallPKG(string url, string name, string icon)
    ```

</details>

### Application Operations
<details>
  <summary>Click to expand</summary>

  - ```csharp
    void InitializeNativeDialogs()
    bool IsFreeOfSandbox()
    void EnterSandbox()
    void BreakFromSandbox()
    void MountInSandbox(string systemPath, string mountName)
    void UnmountFromSandbox()
    void ExitApplication()
    ```

</details>

### Helper Methods
<details>
  <summary>Click to expand</summary>
  
  - ```csharp
    string GetDiskInfo(DiskInfo info)
    string GetDiskInfoAsFormattedText(DiskInfo info, Dictionary<DiskInfo, string> diskInfo)
    float? GetTemperature(Temperature temperature = Temperature.CPU, bool fahrenheit = false)
    void DownloadWebFile(string url, string path, string file, string extension, bool bgDL = false, string name = "NULL")
    void DownloadPkgFile(string url, string path, string file, bool bgDL = false, string name = "NULL")
    ```

</details> 
  
## Notes
- **Memory Management**  
  Automatic marshaling is provided for most types. However, native pointers (IntPtr) require manual cleanup.

- **Error Handling**  
  Basic error handling is implemented. Add exceptions and fallbacks as needed for robustness.

- **Private Functions**  
  `PrivateImports` is intended for internal use only. Avoid calling directly unless necessary for custom scenarios.

- **Helper Functions**  
  Utilize helper functions to streamline your code and handle common use cases more efficiently.

## Example Usage of Intptr
Several functions return IntPtr, which you can use to interact with unmanaged data.

```csharp
// Get the firmware version as IntPtr
IntPtr fwVersionPtr = uob.GetFWVersion();

// Marshal the IntPtr to a C# string
string fwVersion = Marshal.PtrToStringAnsi(fwVersionPtr);

// Display the firmware version in the Unity console
Debug.Log("Firmware Version: " + fwVersion);
```

## License
This project is licensed under the GNU General Public License v2.0 - see the [LICENSE](../../LICENSE) file for details.

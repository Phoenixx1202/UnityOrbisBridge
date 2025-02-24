# UOBWrapper 

A wrapper for interacting with the **`UnityOrbisBridge`** (`.sprx / .dll`) plugin for Unity and is used by [FPKGi](https://www.github.com/ItsJokerZz/FPKGi). 

## Features

### Temperature Management
- **Update Temperature**:
  ```csharp
  UOBWrapper.UpdateTemperature(textObject, UOB.Temperature.CPU);
  ```
  Updates the temperature display for the specified temperature sensor (CPU/SoC).

- **Update Temperature with Custom Colors**:
  ```csharp
  UOBWrapper.UpdateTemperature(textObject, coldColor, normalColor, hotColor, UOB.Temperature.CPU);
  ```
  Updates the temperature display with custom colors for different temperature ranges.

- **Update Temperature with Custom Min/Max Ranges**:
  ```csharp
  UOBWrapper.UpdateTemperature(textObject, coldColor, normalColor, hotColor, UOB.Temperature.CPU, 50f, 85f);
  ```
  Updates the temperature display with custom min/max temperature values.

### Disk Information
- **Update Disk Info**:
  ```csharp
  UOBWrapper.UpdateDiskInfo(textObject, UOB.DiskInfo.Used);
  ```
  Displays updated disk information (used, free, total, or percentage) in the specified text object.

### Logging
- **Log Message**:
  ```csharp
  UOBWrapper.Print("This is a log message", UOBWrapper.PrintType.Default);
  ```
  Logs a message to the console based on the specified log type.

- **Log Message with File Save**:
  ```csharp
  UOBWrapper.Print(true, UOBWrapper.PrintType.Error, "Error message", "/path/to/log.txt");
  ```
  Logs a message and optionally saves it to a file.

### Image Downloading
- **Download Image from URL**:
  ```csharp
  UOBWrapper.SetImageFromURL("http://example.com/image.png", ref image);
  ```
  Downloads an image from the specified URL and assigns it to a `RawImage` component.

- **Parse File as String**:
  ```csharp
  string content = await UOBWrapper.DownloadAsBytes("http://example.com/file.txt");
  ```
  Downloads content from a URL as a string.

## License
This project is licensed under the GNU General Public License v2.0 - see the [LICENSE](../../LICENSE) file for details.

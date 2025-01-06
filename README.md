# UnityOrbisBridge - A Unity OOSDK Integration Library

## Setup & Usage
You have two options to get started with UnityOrbisBridge:

### 1. Download Precompiled Binaries
- Visit the **Actions** tab above to download the latest compiled `.prx` and `.dll` files. <br>
  These binaries are regularly updated automatically via GitHub Actions. <br>
- Optionally, you can also download the [UOBWrapper](/source/wrapper/UOBWrapper.cs) for simplified function usage.

### 2. Clone or Download the Repo & Build Locally
- Clone or download the repository to build the library yourself.
- You can build it manually or leverage GitHub Actions for an automatic build process in your cloned repository.
  
### 3. Set Up in Your Unity Project
- In your Unity project, navigate to the root folder and go to the `Assets` directory.
- Create a folder named `Plugins` if it doesn't already exist, and another within named `PS4`
- Inside the `PS4` folder, move or copy the `UnityOrbisBridge.prx` file into this subfolder.
- Similarly, copy or move the `UnityOrbisBridge.dll` file into the `Plugins` folder.

### 4. Inlcuding for Usage
- Now open you Unity projects `.csproj` file.
- Navigate/create a script and then open it.
- At the top of the file type `using UnityOrbisBridge;`

### 5. Using the Library
- Simpily type `UOB.` then a function from the library as per [this](/source/Unity-API/README.md). <br>
  For example:
  ```csharp
  float currentTemp = UOB.GetTemperature(Temperature.CPU, false);
  ```
- You can also use the [UOBWrapper](/source/wrapper/UOBWrapper.cs) for more simplifed usages of functions. <br>
  For example:
  ```csharp
  UOBWrapper.Print("Hello, world!", PrintType.Warning);
  ```
  <br> if use the wrapper, ensure to either type type `UOBWrapper.` before each function call. <br>
  or you can simpily just type `using static UOBWrapper;` per each file-usage where needed.

## How to Build?
<details>
  <summary><strong>Prerequisites</strong></summary>
  <ul>
    <li>Linux device</li>
    <li>LLVM/Clang 12</li>
    <li><a href="https://github.com/OpenOrbis/OpenOrbis">OpenOrbis SDK</a></li> 
    <li><a href="https://github.com/sleirsgoevy/ps4-libjbc">ps4-libjbc</a></li>
    <li><a href="https://github.com/bucanero/oosdk_libraries">oosdk_libraries</a></li>
  </ul>
</details>

1. Navigate to `/opt/` and clone the required repositories:

   ```
   cd /opt/
   sudo git clone https://github.com/bucanero/oosdk_libraries.git /opt/oosdk_libraries
   sudo git clone https://github.com/bucanero/ps4-libjbc.git /opt/ps4-libjbc
   ```

2. Set the OpenOrbis Toolchain environment variable:
   
   ```
   export OO_PS4_TOOLCHAIN="/opt/OpenOrbis/PS4Toolchain"
   ```

3. Navigate to the `ps4-libjbc` directory and install it:

   ```
   cd /opt/ps4-libjbc
   sudo make install
   ```

4. Navigate to the `oosdk_libraries` directory and install the required libraries:

   ```
   cd /opt/oosdk_libraries/zlib_partial && sudo make install
   cd /opt/oosdk_libraries/polarssl-1.3.9 && sudo make install
   cd /opt/oosdk_libraries/curl-7.64.1 && sudo make install
   ```

5. Install LLVM and Clang (version 12) if you haven't already:

   ```
   sudo apt update
   sudo apt install llvm-12 clang-12
   ```

6. **Install libcurl from source:**

   ```
   cd /opt/oosdk_libraries/curl-7.64.1
   mkdir orbis && cd orbis
   wget https://raw.githubusercontent.com/bucanero/SDL-PS4/ps4/cmake/openorbis.cmake
   cmake --toolchain openorbis.cmake .. -DCMAKE_USE_POLARSSL=1 -DUSE_UNIX_SOCKETS=0 -DENABLE_THREADED_RESOLVER=0 -DENABLE_IPV6=0
   make libcurl
   cp lib/libcurl.a "${OO_PS4_TOOLCHAIN}/lib"
   cp -R ../include/curl "${OO_PS4_TOOLCHAIN}/include/"
   ```

7. Finally, build the project:
  navigate to wherever you've cloned/download the repo and simply run: `make`

## Credits
### Open Source References Used:
- [PS4 Fan ICC by Zer0xFF](https://gist.github.com/Zer0xFF/4aa38d836a5696ed1b6486bb8e782b4a#file-ps4-fan_icc)
- [ps4_unjail by PSTools](https://github.com/PSTools/ps4_unjail)
- [ps4-notifi by Al-Azif](https://github.com/Al-Azif/ps4-notifi)
- [ps4-libjbc by sleirsgoevy](https://github.com/sleirsgoevy/ps4-libjbc)
- [ps4-ipi by 0x199](https://github.com/0x199/ps4-ipi)
- [ps4_remote_pkg_installer by flatz](https://github.com/flatz/ps4_remote_pkg_installer)
- [PS4RPI by illusion0001](https://github.com/illusion0001/PS4RPI)
- [store-api by LightningMods](https://github.com/LightningMods/store-api)
- [itemzflow by LightningMods](https://github.com/LightningMods/itemzflow)

### Special Thanks
A huge thank you to the following members of the [OOSDK Discord](https://www.discord.com/invite/GQr8ydn) for their support:
- **[TheMagicalBlob](https://github.com/TheMagicalBlob)**, [LightningMods](https://github.com/LightningMods), [Al-Azif](https://github.com/Al-Azif), Da Puppeh, Kernel Panic, lainofthewired, and others.

## License
This project is licensed under the GNU General Public License v2.0 - see the [LICENSE](https://github.com/ItsJokerZz/uob-testing/blob/main/LICENSE) file for details.

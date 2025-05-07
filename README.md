# DX11-ImGui-HookKit 🎮 GUI Overlay for DirectX 11 Games

easy-to-use C++ starter kit for creating DirectX 11 game overlays using **Dear ImGui**. This project provides a solid foundation for injecting a customizable ImGui interface into DirectX 11 applications games, leveraging **MinHook** for function hooking and supporting ImGui's awesome **docking** and **viewport** features.

This means you can have ImGui windows that dock together within the game, or even drag them out as separate windows on your desktop

## ✨ Features

* **DirectX 11 Hooking**: Targets `Present` and `ResizeBuffers` for seamless ImGui rendering and window resize handling.
* **MinHook Integration**: Uses the powerful MinHook library for x86 and x64 function hooking.
* **Dear ImGui with Docking & Viewports**:
    * Full docking support (`IMGUI_HAS_DOCK`).
    * Multi-viewport support (`IMGUI_HAS_VIEWPORT`) allows ImGui windows to be moved outside the main game window.
* **CMake Build System**: Easy to configure and build.
* **x86 & x64 Support**: CMake configuration handles both architectures.
* **Basic Menu Example**: Includes a simple `menu.cpp` to get you started with your own UI.
* **Enhanced Stability**: Includes error handling, logging (`OutputDebugStringW`), and careful resource management to minimize crashes.
* **Clear Code Structure**: Separated logic for DLL entry (`dllmain.cpp`), hooking (`hooks.cpp`), and UI (`menu.cpp`).

## ⚙️ Dependencies

You'll need to grab these libraries and place them in a `libs` folder (or adjust paths in `CMakeLists.txt`):

1.  **Dear ImGui (Docking Branch)**:
    * **Source**: [https://github.com/ocornut/imgui](https://github.com/ocornut/imgui)
    * **Branch**: Make sure you're using the `docking` branch.
    * **Setup**: Place the ImGui source directory (e.g., `imgui-docking`) in your `C:/libs/` folder or update `IMGUI_PATH` in `CMakeLists.txt`. The current setup expects `C:/libs/imgui-docking`.

2.  **MinHook**:
    * **Source**: [https://github.com/TsudaKageyu/minhook](https://github.com/TsudaKageyu/minhook)
    * **Version**: The project was set up with v1.3.3 (the "bin" release was used in later iterations of the prompt, meaning `MinHook.dll` and its import `.lib`).
    * **Setup**:
        * If using the **binary release** (contains `.dll` and `.lib` in a `bin` folder): Place the MinHook directory (e.g., `MinHook_134_bin`) in `C:/libs/` so the structure is like `C:/libs/MinHook_134_bin/include/MinHook.h` and `C:/libs/MinHook_134_bin/bin/MinHook.x64.lib`. Update `MINHOOK_PATH` in `CMakeLists.txt` if needed.
        * If compiling MinHook from **source**: Build MinHook as a static library (`.lib`) using the same compiler toolset as this project to avoid mismatches. Then update `CMakeLists.txt` to link against your compiled static library.

## 🛠️ Building

This project uses CMake to generate build files for your preferred compiler (e.g., Visual Studio).

1.  **Prerequisites**:
    * A C++ compiler that supports C++17 (e.g., Visual Studio 2019 or newer).
    * CMake (version 3.10 or newer).
2.  **Configure Paths**:
    * Open `CMakeLists.txt`.
    * Verify that `MINHOOK_PATH` and `IMGUI_PATH` point to the correct locations of your MinHook and ImGui libraries.
3.  **Generate Build Files**:
    * Create a `build` directory in the project root.
    * Open a terminal or CMake GUI, navigate to the `build` directory.
    * Run CMake to generate project files:
        ```bash
        # from the build directory
        cmake .. 
        # or specify a generator e.g. for visual studio 2022 x64
        # cmake .. -G "Visual Studio 17 2022" -A x64
        ```
4.  **Compile**:
    * Open the generated solution file (e.g., `.sln` for Visual Studio) or use the command line:
        ```bash
        cmake --build . --config Release
        ```
    * This will produce `ImGuiDX11Hook.dll` (or a similar name based on `PROJECT_NAME` in CMake) in your build output directory (e.g., `build/Release`). The MinHook DLL (`MinHook.x64.dll` or `MinHook.x86.dll`) will also be copied to this directory.

## 🚀 How to Use

1.  **Build the DLL**: Follow the build instructions above. You will get `YourProjectName.dll` and the corresponding `MinHook.dll`.
2.  **Inject the DLL**: Use your favorite DLL injector to inject 2 dlls which is project name dll and minhook dll but FIRST MINHOOK DLL into the target DirectX 11 game process (either x86 or x64, matching your build).
    * Make sure both `YourProjectName.dll` and the `MinHook.dll` (e.g., `MinHook.x64.dll`) are in a location accessible by the game, or ideally, in the same directory. The CMake script copies MinHook's DLL to the output directory of your hook DLL.
3.  **Toggle Menu**:
    * Press the `INSERT` key to show/hide the ImGui menu.
4.  **Unload/Eject**:
    * Press the `DELETE` key. This will trigger the shutdown sequence in the DLL (unhooking functions, cleaning up ImGui) and then the DLL will call `FreeLibraryAndExitThread` on itself. Your injector might then fully detach it.

## 📝 Notes & Disclaimer

* **VTable Indexes**: The current method for finding `Present` and `ResizeBuffers` function addresses relies on hardcoded VTable indexes (8 and 13). While common, these *can* change with different DirectX SDK versions or game updates, potentially breaking the hook. A more robust (but significantly more complex) solution would involve signature scanning.
* **Error Logging**: The DLL uses `OutputDebugStringW` for logging. Use a tool like DebugView (from Sysinternals) to see these logs, which can be super helpful for troubleshooting.
* **Game Compatibility**: This is a template. Specific games might have their own anti-cheat mechanisms or quirks that could detect or interfere with this type of hook.

## 💡 Customization

* **UI**: Edit `src/menu.cpp` and `src/menu.h` to create your own ImGui interface.
* **Hooks**: Add more hooks in `src/hooks.cpp` and `src/hooks.h` if you need to interact with other game functions.
* **ImGui Configuration**: Modify `src/imgui_config.h` for custom ImGui backend flags or features.

---

Happy Hacking! ✨

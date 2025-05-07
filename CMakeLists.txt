
cmake_minimum_required(VERSION 3.10)

project(ImGuiDX11Hook CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


set(MINHOOK_PATH "C:/libs/MinHook_134_bin")
set(IMGUI_PATH "C:/libs/imgui-docking")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)


add_library(${PROJECT_NAME} SHARED
    src/dllmain.cpp   
    src/hooks.cpp       
    src/menu.cpp        

    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp
    ${IMGUI_PATH}/imgui_demo.cpp

    ${IMGUI_PATH}/backends/imgui_impl_win32.cpp
    ${IMGUI_PATH}/backends/imgui_impl_dx11.cpp
)


target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${MINHOOK_PATH}/include    
    ${IMGUI_PATH}             
    ${IMGUI_PATH}/backends    
)


target_compile_definitions(${PROJECT_NAME} PRIVATE IMGUI_USER_CONFIG="imgui_config.h")


if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 8 bytes for pointer means 64-bit
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MINHOOK_PATH}/bin/MinHook.x64.lib)
    set(MINHOOK_DLL_TO_COPY ${MINHOOK_PATH}/bin/MinHook.x64.dll)
else() # otherwise 32-bit
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MINHOOK_PATH}/bin/MinHook.x86.lib)
    set(MINHOOK_DLL_TO_COPY ${MINHOOK_PATH}/bin/MinHook.x86.dll)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${MINHOOK_DLL_TO_COPY}"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMENT "Copying MinHook DLL to output directory"
)

target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 dxgi)


if(WIN32) # if we're compiling on windows

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

    target_compile_options(${PROJECT_NAME} PRIVATE "/wd4244" "/wd4267") 

endif()

# optional: set output directory for the dll
# makes it easier to find the compiled dll
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # for dlls place in bin for convenience
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # for executables (and DLLs on Windows)

# Demon-Realm 桌面端构建脚本，结构参照 cocos2d-x 官方 cpp 模板。
#
# 引擎位置默认为仓库内的本地检出 `cocos2d/`（不纳入版本库），可用
# -DCOCOS2DX_ROOT_PATH=<engine path> 覆盖。
#
# 典型用法（macOS）：
#   cmake -S . -B build -G Ninja
#   cmake --build build
cmake_minimum_required(VERSION 3.6)

# 目标架构：cocos2d-x 4.0 自带的 macOS 预编译第三方库只有 i386/x86_64，没有 arm64，
# 因此在 Apple Silicon 上必须按 x86_64 构建，运行时依赖 Rosetta。
# 这里设为默认值而不是依赖命令行参数，保证 IDE、CMake Tools 和命令行的行为一致；
# 必须写在 project() 之前才能作用于编译器与 ABI 检测。
# 如果将来把第三方库换成含 arm64 的版本，配置时显式传入 -DCMAKE_OSX_ARCHITECTURES=arm64 即可覆盖。
if(CMAKE_HOST_APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for macOS" FORCE)
endif()

set(APP_NAME DemonRealm)
project(${APP_NAME})

if(NOT DEFINED COCOS2DX_ROOT_PATH)
    set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d)
endif()

if(NOT EXISTS ${COCOS2DX_ROOT_PATH}/cocos/CMakeLists.txt)
    message(FATAL_ERROR
        "cocos2d-x engine was not found at ${COCOS2DX_ROOT_PATH}. "
        "See README.md for the engine setup steps.")
endif()

if(NOT DEFINED BUILD_ENGINE_DONE)
    set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)
    include(CocosBuildSet)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${ENGINE_BINARY_PATH}/cocos/core)
endif()

# 引擎工具链兼容：cocos2d-x 4.0 依赖旧版 libc++ 的传递包含，新版 Xcode 已移除。
# 只对引擎目标强制预包含标准库兼容头，本项目代码仍须显式包含自己的依赖。
if(APPLE AND TARGET cocos2d)
    target_compile_options(cocos2d
        PRIVATE "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EngineToolchainCompat.hpp")
endif()

# 运行时资源目录，与 Classes 同级，由引擎打包进应用资源。
set(GAME_RES_FOLDER
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
    )

if(APPLE OR WINDOWS)
    cocos_mark_multi_resources(common_res_files RES_TO "Resources" FOLDERS ${GAME_RES_FOLDER})
endif()

# 跨平台业务代码
set(GAME_HEADER
    Classes/App/GameLauncher.hpp
    Classes/Presentation/MainSceneView.hpp
    Classes/Presentation/EnterGameView.hpp
    )

set(GAME_SOURCE
    Classes/App/GameLauncher.cpp
    Classes/Presentation/MainSceneView.cpp
    Classes/Presentation/EnterGameView.cpp
    )

# 平台入口与打包资源
if(WINDOWS)
    list(APPEND GAME_HEADER
         proj.win32/main.h
         proj.win32/resource.h
         )
    list(APPEND GAME_SOURCE
         proj.win32/main.cpp
         proj.win32/game.rc
         ${common_res_files}
         )
elseif(MACOSX)
    set(APP_UI_RES
        proj.ios_mac/mac/Icon.icns
        proj.ios_mac/mac/Info.plist
        )
    list(APPEND GAME_SOURCE
         proj.ios_mac/mac/main.cpp
         proj.ios_mac/mac/Prefix.pch
         ${APP_UI_RES}
         ${common_res_files}
         )
else()
    message(FATAL_ERROR "Demon-Realm currently targets macOS and Windows desktop only.")
endif()

set(all_code_files
    ${GAME_HEADER}
    ${GAME_SOURCE}
    )

add_executable(${APP_NAME} ${all_code_files})

# 资源文件名使用中文，源码里的窄字符串字面量必须按 UTF-8 解析。
# Clang 默认即为 UTF-8；MSVC 默认按系统代码页解析，需要显式指定。
if(MSVC)
    target_compile_options(${APP_NAME} PRIVATE /utf-8)
endif()

target_link_libraries(${APP_NAME} cocos2d)
target_include_directories(${APP_NAME}
    PRIVATE Classes
    PRIVATE ${COCOS2DX_ROOT_PATH}/cocos/audio/include/
)

setup_cocos_app_config(${APP_NAME})

if(MACOSX)
    set_target_properties(${APP_NAME} PROPERTIES
        RESOURCE "${APP_UI_RES}"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/mac/Info.plist"
    )
    if(XCODE)
        set_xcode_property(${APP_NAME} INFOPLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/mac/Info.plist")
    endif()
elseif(WINDOWS)
    cocos_copy_target_dll(${APP_NAME})
    cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
    cocos_copy_target_res(${APP_NAME} LINK_TO ${APP_RES_DIR} FOLDERS ${GAME_RES_FOLDER})
endif()

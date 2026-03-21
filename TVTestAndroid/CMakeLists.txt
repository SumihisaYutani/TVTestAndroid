cmake_minimum_required(VERSION 3.21)

project(TVTestAndroid VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6パッケージを探す
find_package(Qt6 REQUIRED COMPONENTS Core Network Multimedia MultimediaWidgets Widgets)

# FFmpeg executable detection (external process approach)
# 開発ライブラリが不要なシンプルな外部プロセス方式

# FFmpeg executable detection (simplified for external process use)
if(WIN32)
    # 複数の候補からFFmpegを検索
    set(FFMPEG_CANDIDATES
        "C:/ffmpeg/bin/ffmpeg.exe"
        "C:/Users/Yutani Sumihisa/AppData/Local/Microsoft/WinGet/Packages/Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-8.1-full_build/bin/ffmpeg.exe"
    )
    set(FFPLAY_CANDIDATES
        "C:/ffmpeg/bin/ffplay.exe"
        "C:/Users/Yutani Sumihisa/AppData/Local/Microsoft/WinGet/Packages/Gyan.FFmpeg_Microsoft.Winget.Source_8wekyb3d8bbwe/ffmpeg-8.1-full_build/bin/ffplay.exe"
    )
    
    foreach(CANDIDATE ${FFMPEG_CANDIDATES})
        if(EXISTS "${CANDIDATE}")
            set(FFMPEG_EXECUTABLE "${CANDIDATE}")
            break()
        endif()
    endforeach()
    
    foreach(CANDIDATE ${FFPLAY_CANDIDATES})
        if(EXISTS "${CANDIDATE}")
            set(FFPLAY_EXECUTABLE "${CANDIDATE}")
            break()
        endif()
    endforeach()
else()
    find_program(FFMPEG_EXECUTABLE ffmpeg)
    find_program(FFPLAY_EXECUTABLE ffplay)
endif()

# Check if FFmpeg executables are found
if(EXISTS "${FFMPEG_EXECUTABLE}")
    set(FFMPEG_FOUND TRUE)
    message(STATUS "FFmpeg executables found")
    message(STATUS "  FFMPEG: ${FFMPEG_EXECUTABLE}")
    if(EXISTS "${FFPLAY_EXECUTABLE}")
        message(STATUS "  FFPLAY: ${FFPLAY_EXECUTABLE}")
    endif()
else()
    set(FFMPEG_FOUND FALSE)
    message(WARNING "FFmpeg executable not found - TSストリーム再生機能は無効になります")
endif()

# Qt6の標準設定
qt6_standard_project_setup()

#自動MOC、UIC、RCCを有効化
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# VLCライブラリパス設定
set(VLC_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vlc-3.0.23")
set(VLC_INCLUDE_DIR "${VLC_ROOT_DIR}/sdk/include")
set(VLC_LIB_DIR "${VLC_ROOT_DIR}/sdk/lib")

# VLCライブラリ検出
if(EXISTS "${VLC_INCLUDE_DIR}/vlc/libvlc.h")
    set(VLC_FOUND TRUE)
    message(STATUS "VLCライブラリが見つかりました")
    message(STATUS "  VLC_INCLUDE: ${VLC_INCLUDE_DIR}")
    message(STATUS "  VLC_LIB: ${VLC_LIB_DIR}")
else()
    set(VLC_FOUND FALSE)
    message(WARNING "VLCライブラリが見つかりません - ストリーミング機能は無効になります")
endif()

# ソースファイル
set(SOURCES
    main.cpp
    src/network/BonDriverNetwork.cpp
    src/network/VLCStreamingPlayer.cpp
    src/network/TSStreamingServer.cpp
    src/ui/MainWindow.cpp
    src/utils/Logger.cpp
)

# ヘッダーファイル
set(HEADERS
    src/network/BonDriverNetwork.h
    src/network/VLCStreamingPlayer.h
    src/network/TSStreamingServer.h
    src/ui/MainWindow.h
    src/utils/Logger.h
)

# FFmpeg外部プロセス使用のため、FFmpeg関連ソースファイルは不要

# 実行ファイルの作成（Qt6の新しい方式）
qt6_add_executable(TVTestAndroid)

# ソースファイルをターゲットに追加
target_sources(TVTestAndroid PRIVATE ${SOURCES} ${HEADERS})

# リソースファイル
qt6_add_resources(TVTestAndroid "resources"
    PREFIX "/"
    FILES
        resources/channels.json
)

# インクルードディレクトリ
target_include_directories(TVTestAndroid PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${VLC_INCLUDE_DIR}
)
add_definitions(-DUSE_VLC_MEDIA_PLAYER)

# Qt6ライブラリとリンク
target_link_libraries(TVTestAndroid PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
)

# VLCライブラリとリンク
if(VLC_FOUND)
    target_link_libraries(TVTestAndroid PRIVATE
        "${VLC_LIB_DIR}/libvlc.lib"
        "${VLC_LIB_DIR}/libvlccore.lib"
    )
    
    # VLC DLLとプラグインディレクトリを出力ディレクトリにコピー
    add_custom_command(TARGET TVTestAndroid POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VLC_ROOT_DIR}/libvlc.dll"
            $<TARGET_FILE_DIR:TVTestAndroid>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VLC_ROOT_DIR}/libvlccore.dll"
            $<TARGET_FILE_DIR:TVTestAndroid>
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VLC_ROOT_DIR}/plugins"
            $<TARGET_FILE_DIR:TVTestAndroid>/plugins
        COMMENT "VLC DLLとプラグインをコピー中..."
    )
endif()

# VLCストリーミング設定
if(VLC_FOUND)
    message(STATUS "VLCストリーミング有効")
    message(STATUS "リアルタイムTSストリーミング対応")
else()
    message(WARNING "VLCなし - ストリーミング機能無効")
endif()

# Android固有の設定
if(ANDROID)
    set_target_properties(TVTestAndroid PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
    )
    
    # Android用権限設定
    # AndroidManifest.xmlで以下を追加:
    # - <uses-permission android:name="android.permission.INTERNET" />
    # - <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
    
    message(STATUS "Android build: FFmpeg無効、TSファイル保存モード")
else()
    message(STATUS "Desktop build: FFmpeg対応予定")
endif()

# 構成サマリー
if(VLC_FOUND)
    message(STATUS "構成完了: Qt6 + VLCライブラリによるリアルタイムストリーミング")
else()
    message(STATUS "構成完了: Qt6のみ（ストリーミング機能制限）")
endif()

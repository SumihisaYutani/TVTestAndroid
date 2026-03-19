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

# ソースファイル
set(SOURCES
    main.cpp
    src/network/BonDriverNetwork.cpp
    src/network/HighPerformanceStreamProcessor.cpp
    src/ui/MainWindow.cpp
    src/media/FfmpegPipePlayer.cpp
    src/media/TsStreamDevice.cpp
    src/utils/Logger.cpp
)

# ヘッダーファイル
set(HEADERS
    src/network/BonDriverNetwork.h
    src/network/HighPerformanceStreamProcessor.h
    src/ui/MainWindow.h
    src/media/FfmpegPipePlayer.h
    src/media/TsBuffer.h
    src/media/TsStreamDevice.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/media
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

# Qt6ライブラリとリンク
target_link_libraries(TVTestAndroid PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Widgets
)

# FFmpeg外部プロセス使用（ライブラリリンク不要）
if(FFMPEG_FOUND)
    message(STATUS "FFmpeg external process enabled")
    add_definitions(-DUSE_FFMPEG_EXTERNAL)
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

# Qt6とFFmpeg外部プロセスのシンプル構成
message(STATUS "LibVLC依存を除去、Qt6 + FFmpeg外部プロセス構成")

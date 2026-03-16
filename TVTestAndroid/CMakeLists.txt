cmake_minimum_required(VERSION 3.21)

project(TVTestAndroid VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6パッケージを探す
find_package(Qt6 REQUIRED COMPONENTS Core Network Multimedia MultimediaWidgets Widgets)

# FFmpeg library detection
# Windows固有パス設定
if(WIN32)
    set(FFMPEG_ROOT "C:/ffmpeg" CACHE PATH "FFmpeg installation root")
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${FFMPEG_ROOT}")
    
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h 
        PATHS "${FFMPEG_ROOT}/include" 
        NO_DEFAULT_PATH)
    find_library(AVCODEC_LIBRARY 
        NAMES avcodec libavcodec avcodec.dll.a
        PATHS "${FFMPEG_ROOT}/lib" "${FFMPEG_ROOT}/bin"
        NO_DEFAULT_PATH)
        
    find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h 
        PATHS "${FFMPEG_ROOT}/include"
        NO_DEFAULT_PATH)
    find_library(AVFORMAT_LIBRARY 
        NAMES avformat libavformat avformat.dll.a
        PATHS "${FFMPEG_ROOT}/lib" "${FFMPEG_ROOT}/bin"
        NO_DEFAULT_PATH)
        
    find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h 
        PATHS "${FFMPEG_ROOT}/include"
        NO_DEFAULT_PATH)
    find_library(AVUTIL_LIBRARY 
        NAMES avutil libavutil avutil.dll.a
        PATHS "${FFMPEG_ROOT}/lib" "${FFMPEG_ROOT}/bin"
        NO_DEFAULT_PATH)
        
    find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h 
        PATHS "${FFMPEG_ROOT}/include"
        NO_DEFAULT_PATH)
    find_library(SWSCALE_LIBRARY 
        NAMES swscale libswscale swscale.dll.a
        PATHS "${FFMPEG_ROOT}/lib" "${FFMPEG_ROOT}/bin"
        NO_DEFAULT_PATH)
else()
    # Linux/Android用のデフォルト検索
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
    find_library(AVCODEC_LIBRARY avcodec)
    find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h) 
    find_library(AVFORMAT_LIBRARY avformat)
    find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
    find_library(AVUTIL_LIBRARY avutil)
    find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
    find_library(SWSCALE_LIBRARY swscale)
endif()

# Check if all FFmpeg components are found
if(AVCODEC_INCLUDE_DIR AND AVCODEC_LIBRARY AND AVFORMAT_INCLUDE_DIR AND AVFORMAT_LIBRARY AND AVUTIL_INCLUDE_DIR AND AVUTIL_LIBRARY AND SWSCALE_INCLUDE_DIR AND SWSCALE_LIBRARY)
    set(FFMPEG_FOUND TRUE)
    message(STATUS "FFmpeg found")
    message(STATUS "  AVCODEC: ${AVCODEC_LIBRARY}")
    message(STATUS "  AVFORMAT: ${AVFORMAT_LIBRARY}")
    message(STATUS "  AVUTIL: ${AVUTIL_LIBRARY}")
    message(STATUS "  SWSCALE: ${SWSCALE_LIBRARY}")
else()
    set(FFMPEG_FOUND FALSE)
    message(WARNING "FFmpeg not found - TSストリーム再生機能は無効になります")
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
    src/ui/MainWindow.cpp
    src/utils/Logger.cpp
)

# ヘッダーファイル
set(HEADERS
    src/network/BonDriverNetwork.h
    src/ui/MainWindow.h
    src/utils/Logger.h
)

# FFmpeg関連ソースファイル（FFmpegが見つかった場合のみ追加）
if(FFMPEG_FOUND)
    list(APPEND SOURCES
        src/media/FFmpegDecoder.cpp
        src/ui/FFmpegVideoWidget.cpp
    )
    list(APPEND HEADERS
        src/media/FFmpegDecoder.h
        src/ui/FFmpegVideoWidget.h
    )
    add_definitions(-DUSE_FFMPEG)
endif()

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
)

# Qt6ライブラリとリンク
target_link_libraries(TVTestAndroid PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Widgets
)

# FFmpegライブラリとリンク（FFmpegが見つかった場合）
if(FFMPEG_FOUND)
    target_include_directories(TVTestAndroid PRIVATE
        ${AVCODEC_INCLUDE_DIR}
        ${AVFORMAT_INCLUDE_DIR}
        ${AVUTIL_INCLUDE_DIR}
        ${SWSCALE_INCLUDE_DIR}
    )
    
    target_link_libraries(TVTestAndroid PRIVATE
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${SWSCALE_LIBRARY}
    )
    
    message(STATUS "FFmpeg libraries linked successfully")
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
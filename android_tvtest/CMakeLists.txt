cmake_minimum_required(VERSION 3.16)

project(TVTestAndroid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt modules
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia MultimediaWidgets Network Sql)
qt_standard_project_setup()

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/core/CoreEngine.cpp
    src/network/NetworkManager.cpp
    src/database/EPGDatabase.cpp
    src/utils/Logger.cpp
)

set(HEADERS
    src/mainwindow.h
    src/core/CoreEngine.h
    src/network/NetworkManager.h
    src/database/EPGDatabase.h
    src/utils/Logger.h
)

# UI Files (disabled for initial build)
# set(UI_FILES
#     ui/mainwindow.ui
#     ui/channellist.ui
#     ui/epgdialog.ui
#     ui/settings.ui
# )

# Qt executable
qt_add_executable(TVTestAndroid ${SOURCES} ${HEADERS})

# Qt Resources (disabled for initial build)
# TODO: Add resources after basic build works
# qt_add_resources(TVTestAndroid "resources" 
#     PREFIX "/"
#     FILES 
#         resources/icons/play.png
#         resources/icons/stop.png
#         resources/icons/settings.png
# )

# Link Qt modules
target_link_libraries(TVTestAndroid PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Network
    Qt6::Sql
)

# Include LibISDB (when ported)
# target_include_directories(TVTestAndroid PRIVATE "../src/LibISDB/LibISDB")

# Android specific
if(ANDROID)
    set_target_properties(TVTestAndroid PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()
cmake_minimum_required(VERSION 3.7)

project(NetworksLabs LANGUAGES CXX)

include(cmake/config.cmake)
include(cmake/functions.cmake)

set(Qt5_DIR ${QT5_LOCATION}/lib/cmake/Qt5)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES Release Debug)

find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

add_subdirectory(Lab1/TCPClient)
add_subdirectory(Lab1/TCPServer)
add_subdirectory(Lab1/UDPClient)
add_subdirectory(Lab1/UDPServer)
add_subdirectory(Lab2/TCPClientFiles)
add_subdirectory(Lab2/TCPServerFiles)
add_subdirectory(Lab2/UDPClientFiles)
add_subdirectory(Lab2/UDPServerFiles)
add_subdirectory(Lab3/Base64)
add_subdirectory(Lab4/SMTPSender)

foreach(QT5_LIB ${QT5_LIBS})
	set(QT5_DLLS_RELEASE ${QT5_DLLS_RELEASE} ${QT5_LIB}.dll)
	set(QT5_DLLS_DEBUG ${QT5_DLLS_DEBUG} ${QT5_LIB}d.dll)
endforeach()

COPY_FILES(${QT5_LOCATION}/bin "${QT5_DLLS_RELEASE}" ${PROJECT_DIR}/bin/Release)
COPY_FILES(${QT5_LOCATION}/bin "${QT5_DLLS_DEBUG}" ${PROJECT_DIR}/bin/Debug)

COPY_DIRS_FILES(${QT5_LOCATION}/plugins "${QT5_PLUGINS}" "*[^d].dll" ${PROJECT_DIR}/bin/Release/plugins)
COPY_DIRS_FILES(${QT5_LOCATION}/plugins "${QT5_PLUGINS}" "*d.dll" ${PROJECT_DIR}/bin/Debug/plugins)
COPY_DIRS_FILES(${QT5_LOCATION} translations "*.qm" ${PROJECT_DIR}/bin/Release/translations)
COPY_DIRS_FILES(${QT5_LOCATION} translations "*.qm" ${PROJECT_DIR}/bin/Debug/translations)

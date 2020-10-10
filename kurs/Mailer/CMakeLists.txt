cmake_minimum_required(VERSION 3.7)

set(PROJECT_NAME KursMailer)

project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

find_package(Qt5 COMPONENTS Core Widgets Network WebEngineWidgets REQUIRED)

include(../../cmake/functions.cmake)
include(../../cmake/config.cmake)

set(CLASSES_WITH_UI mailer mailsender portdialog)
set(CLASSES_WITHOUT_UI )
set(CPP_ONLY main)
set(HEADER_ONLY )

set(EXTENTIONS_WITH_UI h cpp ui)
set(EXTENTIONS_WITHOUT_UI h cpp)
set(EXTENTIONS_CPP_ONLY cpp)
set(EXTENTIONS_HEADER_ONLY h)

ADD_EXTENTION(FILES "${CLASSES_WITH_UI}" "${EXTENTIONS_WITH_UI}")
ADD_EXTENTION(FILES "${CLASSES_WITHOUT_UI}" "${EXTENTIONS_WITHOUT_UI}")
ADD_EXTENTION(FILES "${CPP_ONLY}" "${EXTENTIONS_CPP_ONLY}")
ADD_EXTENTION(FILES "${HEADER_ONLY}" "${EXTENTIONS_HEADER_ONLY}")
set(FILES "${FILES}" resource.qrc)

INIT_GUI_EXECUTABLE(${PROJECT_NAME} "${FILES}")
target_include_directories(${PROJECT_NAME} PRIVATE ${MIMETIC_LOCATION})

if(WIN32)
	set(QT5_PLUGINS Qt5::Widgets Qt5::Core Qt5::Network Qt5::WebEngineWidgets ${MIMETIC_LOCATION}/win32/libmimetic_mt.lib)
	set(QT5_PLUGINSD Qt5::Widgets Qt5::Core Qt5::Network Qt5::WebEngineWidgets ${MIMETIC_LOCATION}/win32/libmimetic_mtd.lib)
elseif(UNIX)
	set(QT5_PLUGINS Qt5::Widgets Qt5::Core Qt5::Network Qt5::WebEngineWidgets ${MIMETIC_LOCATION}/bin/libmimetic.a)
	set(QT5_PLUGINSD Qt5::Widgets Qt5::Core Qt5::Network Qt5::WebEngineWidgets ${MIMETIC_LOCATION}/bin/libmimetic.a)
endif()

target_link_libraries(${PROJECT_NAME} debug "${QT5_PLUGINSD}" optimized "${QT5_PLUGINS}")

set_target_properties(${PROJECT_NAME}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${PROJECT_DIR}/bin
)

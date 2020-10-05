function(COPY_FILES SOURCE_DIR FILES DEST_DIR)
	foreach(FILE ${FILES})
		if (EXISTS ${SOURCE_DIR}/${FILE})
			message(STATUS "Copying ${FILE} to ${DEST_DIR}")
			file(COPY ${SOURCE_DIR}/${FILE}
				DESTINATION ${DEST_DIR})
		endif()
	endforeach()
endfunction()

function(COPY_DIRS_FILES SOURCE_DIR FILES_DIRS TEMPLATE DEST_DIR)
	foreach(FILE_DIR ${FILES_DIRS})
		file(GLOB FILES_PATHS
			${SOURCE_DIR}/${FILE_DIR}/${TEMPLATE})
		message(STATUS "Copying ${FILES_PATHS} to ${DEST_DIR}/${FILE_DIR}")
		file(COPY ${FILES_PATHS}
			DESTINATION ${DEST_DIR}/${FILE_DIR})
	endforeach()
endfunction()

function(ADD_EXTENTION FILES_LIST NAMES_LIST EXTENTIONS)
	foreach(NAME ${NAMES_LIST})
		foreach(EXTENTION ${EXTENTIONS})
			set(FILES_RES ${FILES_RES} ${NAME}.${EXTENTION})
		endforeach()
	endforeach()
	
	set(${FILES_LIST} "${${FILES_LIST}}" "${FILES_RES}" PARENT_SCOPE)
endfunction()

function(INIT_GUI_EXECUTABLE EXECUTABLE FILES )
	if(WIN32)
		set(GUI_OPTION WIN32)
	elseif(APPLE)
		set(GUI_OPTION MACOSX_BUNDLE)
	elseif(UNIX)
		set(GUI_OPTION "")
	endif()

	add_executable(${EXECUTABLE} ${GUI_OPTION} "${FILES}")
endfunction()

function(SET_LIBS EXECUTABLE OUTPUT_DIR QT5_PLUGINS)
	target_link_libraries(${EXECUTABLE} PRIVATE "${QT5_PLUGINS}")
	set_target_properties(${EXECUTABLE}
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
	)
endfunction()

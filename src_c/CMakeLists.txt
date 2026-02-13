cmake_minimum_required(VERSION 4.2)
project("my-test" C)
set(PROJECT_DESCRIPTION "放自己写的各种东西的玩意")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(lib/)

set(LIB_REQUIRE_M
	ALSA
	sin
	input
	neuro
	social)


file(GLOB APP_SOURCES "./*.c")
foreach(SOURCE ${APP_SOURCES})
	get_filename_component(EXE_NAME ${SOURCE} NAME_WE)
	set(EXE_LIBS ctools)

	if(${EXE_NAME} IN_LIST LIB_REQUIRE_M)
		list(APPEND EXE_LIBS m)
	endif()
	if(${EXE_NAME} STREQUAL "ALSA")
		list(APPEND EXE_LIBS asound)
	elseif(${EXE_NAME} STREQUAL "lrc")
		find_package(SDL2)
		find_package(SDL2_mixer)
		if(NOT ${SDL2_FOUND})
			message("[WARN] lrc.c 将无法编译（缺乏SDL2库）")
			continue()
		endif()
		if(NOT ${SDL2_mixer_FOUND})
			message("[WARN] lrc.c 将无法编译（缺乏SDL2_mixer库）")
			continue()
		endif()
		list(APPEND EXE_LIBS ${SDL2_LIBRARIES})
		list(APPEND EXE_LIBS SDL2_mixer)
	elseif(${EXE_NAME} STREQUAL "history_note")
		list(APPEND EXE_LIBS cmenu ncurses)
	endif()

	#message("增加目标文件: ${EXE_NAME} (${EXE_LIBS})")
	add_executable(${EXE_NAME} ${SOURCE})
	target_link_libraries(${EXE_NAME} ${EXE_LIBS})
	if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		target_compile_options(${EXE_NAME} PRIVATE -Wall)
	endif()
endforeach()

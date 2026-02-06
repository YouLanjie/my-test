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
	add_executable(${EXE_NAME} ${SOURCE})

	# 公共链接库
	target_link_libraries(${EXE_NAME} ctools)

	# 如果有特殊需求，可以单独处理
	if(${EXE_NAME} IN_LIST LIB_REQUIRE_M)
		target_link_libraries(${EXE_NAME} m)
	endif()
	if(${EXE_NAME} STREQUAL "ALSA")
		target_link_libraries(${EXE_NAME} asound)
	endif()
	if(${EXE_NAME} STREQUAL "lrc")
		target_link_libraries(${EXE_NAME} SDL2 SDL2_mixer)
	endif()
	if(${EXE_NAME} STREQUAL "history_note")
		target_link_libraries(${EXE_NAME} cmenu ncurses)
	endif()
endforeach()

cmake_minimum_required(VERSION 3.28)

project(RTControl)

set(CMAKE_CXX_STANDARD 17)

set(RTCONTROL_HEADERS 
	${CMAKE_CURRENT_SOURCE_DIR}/src/App.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Controller.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/LiveStream.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/PacketQueue.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Statistics.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/dispatchqueue.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/DummyOpusStream.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/h264Stream.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/helpers.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/stream.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Capturer/CaptureApp.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Capturer/Capturer.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Capturer/pch.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Capturer/util.h	
)

set(RTCONTROL_SOURCES 
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/App.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Controller.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/LiveStream.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/PacketQueue.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Statistics.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/dispatchqueue.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/DummyOpusStream.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/h264Stream.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/helpers.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Streamer/stream.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Capturer/CaptureApp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Capturer/Capturer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/Capturer/pch.cpp
)

add_executable(RTControl ${RTCONTROL_SOURCES} ${RTCONTROL_HEADERS})

# Turn off building examples and tests projects for libdatachannel
set(NO_EXAMPLES ON)
set(NO_TESTS ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd)

target_link_libraries(RTControl PRIVATE datachannel)

set(DATACHANNEL_DLL_PATH_DEBUG "${PROJECT_SOURCE_DIR}/build/3rd/datachannel/Debug/datachannel.dll")
set(DATACHANNEL_DLL_PATH_RELEASE "${PROJECT_SOURCE_DIR}/build/3rd/datachannel/Release/datachannel.dll")

if (DEFINED ENV{OPENSSL_ROOT_DIR})
	set(LIBCRYPTO_DLL_PATH "$ENV{OPENSSL_ROOT_DIR}/bin/libcrypto-1_1-x64.dll")
	set(LIBSSL_DLL_PATH "$ENV{OPENSSL_ROOT_DIR}/bin/libssl-1_1-x64.dll")
endif()

add_custom_command(TARGET RTControl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
	"$<$<CONFIG:Debug>:${DATACHANNEL_DLL_PATH_DEBUG}>"
    	"$<$<CONFIG:Release>:${DATACHANNEL_DLL_PATH_RELEASE}>"
	"${LIBCRYPTO_DLL_PATH}"
	"${LIBSSL_DLL_PATH}"
	$<TARGET_FILE_DIR:RTControl>)
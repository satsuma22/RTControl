cmake_minimum_required(VERSION 3.28)

project(RTControl)

set(CMAKE_CXX_STANDARD 17)

if (NOT DEFINED ENV{NVIDIAENCODE_ROOT_DIR})
	message(FATAL_ERROR "The environment variable NVIDIAENCODE_ROOT_DIR is not set!")
else()
	set(NVENCODE_PATH "$ENV{NVIDIAENCODE_ROOT_DIR}")
	file(TO_CMAKE_PATH "${NVENCODE_PATH}" NVENCODE_PATH)
endif()

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
	"${NVENCODE_PATH}/Samples/NvCodec/NvEncoder/NvEncoder.h"
	"${NVENCODE_PATH}/Samples/NvCodec/NvEncoder/NvEncoderD3D11.h"
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
	"${NVENCODE_PATH}/Samples/NvCodec/NvEncoder/NvEncoder.cpp"
	"${NVENCODE_PATH}/Samples/NvCodec/NvEncoder/NvEncoderD3D11.cpp"
)

add_executable(RTControl ${RTCONTROL_SOURCES} ${RTCONTROL_HEADERS})

target_include_directories(RTControl PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/3rd/datachannel/deps/json/include
	"${NVENCODE_PATH}/Samples/NvCodec"
	"${NVENCODE_PATH}/Samples/Utils"
	"${NVENCODE_PATH}/Interface"
)

target_compile_definitions(RTControl PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

# Turn off building examples and tests projects for libdatachannel
set(NO_EXAMPLES ON)
set(NO_TESTS ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd)

target_link_libraries(RTControl PRIVATE 
	datachannel
	"${NVENCODE_PATH}/Lib/x64/nvencodeapi.lib"	
)

set(DATACHANNEL_DLL_PATH_DEBUG "${PROJECT_BINARY_DIR}/3rd/datachannel/Debug/datachannel.dll")
set(DATACHANNEL_DLL_PATH_RELEASE "${PROJECT_BINARY_DIR}/3rd/datachannel/Release/datachannel.dll")

if (DEFINED ENV{OPENSSL_ROOT_DIR})
	file(GLOB LIBCRYPTO_DLL_PATH "$ENV{OPENSSL_ROOT_DIR}/bin/libcrypto-*-x64.dll")
	file(GLOB LIBSSL_DLL_PATH "$ENV{OPENSSL_ROOT_DIR}/bin/libssl-*-x64.dll")
endif()

add_custom_command(TARGET RTControl POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
	"$<$<CONFIG:Debug>:${DATACHANNEL_DLL_PATH_DEBUG}>"
    "$<$<CONFIG:Release>:${DATACHANNEL_DLL_PATH_RELEASE}>"
	"${LIBCRYPTO_DLL_PATH}"
	"${LIBSSL_DLL_PATH}"
	$<TARGET_FILE_DIR:RTControl>)
cmake_minimum_required(VERSION 3.28)

project(RTControl)

set(CMAKE_CXX_STANDARD 17)

set(RTCONTROL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

set(RTCONTROL_HEADERS "")

add_executable(RTControl RTCONTROL_HEADERS RTCONTROL_SOURCES)

# Turn off building examples and tests projects for libdatachannel
set(NO_EXAMPLES ON)
set(NO_TESTS ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd)

target_link_libraries(RTControl PRIVATE datachannel)

set(DATACHANNEL_DLL_PATH_DEBUG "${PROJECT_SOURCE_DIR}/build/3rd/libdatachannel/Debug/datachannel.dll")
set(DATACHANNEL_DLL_PATH_RELEASE "${PROJECT_SOURCE_DIR}/build/3rd/libdatachannel/Release/datachannel.dll")

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
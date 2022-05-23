cmake_minimum_required(VERSION 3.4.1)

project(TinyEngine)

set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")

if(ANDROID)
    add_definitions(-DPLATFORM_ANDROID)
    add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR)
elseif(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
    add_compile_definitions(_UNICODE UNICODE)
else()
    # todo
endif()


file(GLOB_RECURSE LAUNCH_ANDROID_FILES Source/Launch/Android/*.cpp)
file(GLOB_RECURSE LAUNCH_WINDOWS_FILES Source/Launch/Windows/*.cpp)

if(ANDROID)
    set(LAUNCH_SOURCE_FILES ${LAUNCH_ANDROID_FILES})
elseif(WIN32)
    set(LAUNCH_SOURCE_FILES ${LAUNCH_WINDOWS_FILES})
endif()


list(APPEND LAUNCH_SOURCE_FILES Source/Launch/Launch.cpp)


add_subdirectory(Source/Core)


if(WIN32)
        find_package(Vulkan)

        # Try extracting VulkanSDK path from ${Vulkan_INCLUDE_DIRS}
        if (NOT ${Vulkan_INCLUDE_DIRS} STREQUAL "")
                set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS})
                STRING(REGEX REPLACE "/Include" "" VULKAN_PATH ${VULKAN_PATH})
        endif()

        add_executable(${PROJECT_NAME}
                ${LAUNCH_SOURCE_FILES}
        )

        target_include_directories(${PROJECT_NAME} PRIVATE
                ${VULKAN_PATH}/Include
        )

        target_link_directories(${PROJECT_NAME} PRIVATE
                ${VULKAN_PATH}/Bin
                ${VULKAN_PATH}/Lib
        )

        target_link_libraries(
                ${PROJECT_NAME}
                vulkan-1
                Core
        )
else()
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

        set(VULKAN_SRC_DIR ${ANDROID_NDK}/sources/third_party/vulkan/src)

        add_library(native_app_glue STATIC
                ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
        )

        add_library(${PROJECT_NAME} SHARED
                ${VULKAN_SRC_DIR}/common/vulkan_wrapper.cpp
                ${LAUNCH_SOURCE_FILES}
        )

        target_include_directories(${PROJECT_NAME} PRIVATE
                ${VULKAN_SRC_DIR}/common
                ${ANDROID_NDK}/sources/android/native_app_glue
                ${VULKAN_SRC_DIR}/include
        )

        target_link_libraries(
                ${PROJECT_NAME}
                android
                native_app_glue
                Core
        )
endif()
file(GLOB_RECURSE CORE_ANDROID_FILES Android/*.cpp Android/*.h)
file(GLOB_RECURSE CORE_WINDOWS_FILES Windows/*.cpp Windows/*.h)
file(GLOB_RECURSE CORE_HAL_FILES HAL/*.cpp HAL/*.h)
file(GLOB_RECURSE CORE_GENERIC_FILES GenericPlatform/*.cpp GenericPlatform/*.h)

if(ANDROID)
    set(CORE_SOURCE_FILES ${CORE_ANDROID_FILES})
elseif(WIN32)
    set(CORE_SOURCE_FILES ${CORE_WINDOWS_FILES})
endif()

list(APPEND CORE_SOURCE_FILES ${CORE_HAL_FILES})
list(APPEND CORE_SOURCE_FILES ${CORE_GENERIC_FILES})
message(STATUS "Core Source files: ${SOURCE_FILES}")

add_library(Core ${CORE_SOURCE_FILES})

message(STATUS "!!!!! PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")

target_include_directories(Core PUBLIC
        ${PROJECT_SOURCE_DIR}/Source/Core
        ${ANDROID_NDK}/sources/android/native_app_glue
)

if(ANDROID)
    # set(CMAKE_C_FLAGS "-Wno-error=format-security")

    find_library(log-lib log)
        target_link_libraries(Core
                android
                ${log-lib}
                native_app_glue)
endif()
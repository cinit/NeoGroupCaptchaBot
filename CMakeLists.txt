cmake_minimum_required(VERSION 3.10)

project(NeoGroupCaptchaBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(CLANG_CXX_EXTRA_OPT "-Werror=unknown-warning-option -Werror=format-invalid-specifier -Werror=call-to-pure-virtual-from-ctor-dtor")
    SET(CLANG_C_EXTRA_OPT "-Werror=format-invalid-specifier")
else ()
    SET(CLANG_CXX_EXTRA_OPT "")
    SET(CLANG_C_EXTRA_OPT "")
endif ()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_CXX_EXTRA_OPT} -fPIC -Werror=delete-non-virtual-dtor -Werror=return-type -Werror=non-virtual-dtor -Wno-invalid-offsetof")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_C_EXTRA_OPT} -fPIC -Werror=return-type")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-allow-shlib-undefined,--no-undefined")

add_subdirectory(libs/MMKV/Core core)
add_subdirectory(libs/td)

add_executable(NeoGroupCaptchaBot
        src/core/startup.cpp

        src/utils/ProcessUtils.cpp src/utils/TextUtils.cpp src/utils/SharedBuffer.cpp src/utils/FileMemMap.cpp
        src/utils/auto_close_fd.cpp src/utils/io_utils.cpp src/utils/Uuid.cpp src/utils/shared_memory.cpp
        src/utils/file_utils.cpp src/utils/CachedThreadPool.cpp src/utils/SyncUtils.cpp

        src/utils/log/Log.cpp
        src/utils/config/ConfigManager.cpp src/core/manager/SessionManager.cpp src/core/manager/ClientSession.cpp)

include_directories(libs/rapidjson/include)
include_directories(libs/MMKV/Core)

include_directories(src)

set_target_properties(NeoGroupCaptchaBot PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )

target_link_libraries(NeoGroupCaptchaBot c core memprof tdcore tdnet tdutils tdclient)

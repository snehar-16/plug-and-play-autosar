cmake_minimum_required(VERSION 3.16)
project(autosar_diag_stack C CXX)

set(AUTOSAR_PROFILE "CLASSIC" CACHE STRING "CLASSIC or ADAPTIVE")
set(AUTOSAR_TARGET  "SIM"     CACHE STRING "SIM or STM32")
set(AUTOSAR_OS      "BAREMETAL" CACHE STRING "BAREMETAL or FREERTOS")

message(STATUS "Profile : ${AUTOSAR_PROFILE}")
message(STATUS "Target  : ${AUTOSAR_TARGET}")
message(STATUS "OS      : ${AUTOSAR_OS}")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# C flags — suppress openAUTOSAR legacy warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter \
    -Wno-implicit-function-declaration \
    -Wno-duplicate-decl-specifier \
    -Wno-missing-braces")

# C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

if(AUTOSAR_PROFILE STREQUAL "CLASSIC")
    add_compile_definitions(AUTOSAR_CLASSIC)
else()
    add_compile_definitions(AUTOSAR_ADAPTIVE)
endif()

if(AUTOSAR_OS STREQUAL "FREERTOS")
    add_compile_definitions(AUTOSAR_OS_FREERTOS)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/abstraction/osal
    ${CMAKE_SOURCE_DIR}/abstraction/tpal
    ${CMAKE_SOURCE_DIR}/abstraction/memal
    ${CMAKE_SOURCE_DIR}/hal
    ${CMAKE_SOURCE_DIR}/dem/src
    ${CMAKE_SOURCE_DIR}/dem/inc
    ${CMAKE_SOURCE_DIR}/dem/config
)

set(OSAL_SOURCES
    abstraction/osal/OsAl_Baremetal.cpp
)
if(AUTOSAR_PROFILE STREQUAL "ADAPTIVE")
    list(APPEND OSAL_SOURCES abstraction/osal/OsAl_POSIX.cpp)
endif()

set(TPAL_SOURCES
    abstraction/tpal/TpAl.cpp
    abstraction/tpal/TpAl_UDP.cpp
)

set(MEMAL_SOURCES
    abstraction/memal/MemAl.cpp
    abstraction/memal/MemAl_Static.cpp
    abstraction/memal/MemAl_File.cpp
)

set(DEM_SOURCES
    dem/src/Dem.c
    dem/src/Dem_Debounce.c
    dem/src/Dem_NvM.c
    dem/src/Dem_Extension.c
    dem/src/Det.c
    dem/config/Dem_LCfg.c
)

set(HAL_SOURCES
    hal/Hal_Sim.cpp
)

add_executable(autosar_diag
    main.cpp
    ${OSAL_SOURCES}
    ${TPAL_SOURCES}
    ${MEMAL_SOURCES}
    ${DEM_SOURCES}
    ${HAL_SOURCES}
)

target_link_libraries(autosar_diag pthread)

cmake_minimum_required(VERSION 2.6)

set(
    PROJECT
    BackTest
)

# determine architecture
if(NOT ${CUSTOM_ARCH} MATCHES "x32" AND NOT ${CUSTOM_ARCH} MATCHES "x64")
    set(
        CUSTOM_ARCH
        x32
    )
endif()

# Ouput - path
# All binaries are located in the bin - folder
set(
    EXECUTABLE_OUTPUT_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/../bin
)
set(
    LIBRARY_OUTPUT_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/../bin
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../snapshot)
    # build from snapshot
    set(
        SNAPSHOT_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/../../snapshot
    )
else()
    #build from package
    set(
        SNAPSHOT_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
    )
endif()

# if(NOT DEFINED HPTOOLS_BUILD)
#     add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../hptools" "${CMAKE_CURRENT_SOURCE_DIR}/../hptools")
# endif()


include_directories(
    # project directories
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    # Dependencies
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)
set(
    HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/source/stdafx.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/Trade.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/Straregy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/DataRepository.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/CostFunction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/CommonSources.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/PriceData/Offer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/PriceData/Period.h
    ${CMAKE_CURRENT_SOURCE_DIR}/source/PriceData/PriceDataInterfaces.h
)

set(
    SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/Trade.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/Strategy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/DataRepository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/CostFunction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/CommonSources.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/PriceData/Offer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/PriceData/Period.cpp
)

add_executable(
    ${PROJECT}
    ${SOURCES}
)

target_link_libraries(
    ${PROJECT}
    quotesmgr2
    pricehistorymgr
    hptools
    ForexConnect
)

# add_dependencies(
#     ${PROJECT}
# )

set(
    SHAREDSUFFIX
    so
)
if(${CMAKE_SYSTEM} MATCHES "Darwin")
    set(
        SHAREDSUFFIX
        dylib
    )
endif()

# Post - build event to create symlinks for shared objects
# add_custom_command(
#     TARGET
#         ${PROJECT}
#     POST_BUILD
#         COMMAND find ${SNAPSHOT_DIR}/lib/ -name lib*.${SHAREDSUFFIX} -exec ln -sf {} . \;
#     WORKING_DIRECTORY
#         ${EXECUTABLE_OUTPUT_PATH}
#     VERBATIM	
# )

if(${CMAKE_SYSTEM} MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS "--std=c++14 ${CMAKE_CXX_FLAGS} -g")
    set_target_properties(
        ${PROJECT}
        PROPERTIES
            LINK_FLAGS
                -Wl,-rpath,.
    )
elseif(${CMAKE_SYSTEM} MATCHES "Darwin")
    SET(CMAKE_CXX_FLAGS "--stdlib=libc++ --std=c++14 ${CMAKE_CXX_FLAGS} -g")
    set_target_properties(
        ${PROJECT}
        PROPERTIES
            BUILD_WITH_INSTALL_RPATH
                1
            LINK_FLAGS
                -Wl,-rpath,@loader_path/
    )

    add_custom_command(
    TARGET
        ${PROJECT}
    POST_BUILD
        COMMAND find ${CMAKE_CURRENT_SOURCE_DIR}/../lib/ -name lib*.${SHAREDSUFFIX} -exec ln -sf {} . \;
    WORKING_DIRECTORY
        ${EXECUTABLE_OUTPUT_PATH}
    VERBATIM
)
endif()

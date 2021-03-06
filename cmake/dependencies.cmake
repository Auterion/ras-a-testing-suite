include(ExternalProject)

if(MAVSDK_INSTALL_DIR)
    message(STATUS "Using MAVSDK from: ${MAVSDK_INSTALL_DIR}")
    include_directories(${MAVSDK_INSTALL_DIR}/include)
    link_directories(${MAVSDK_INSTALL_DIR}/lib)
else()
    # clone and build MAVSDK via ExternalProject
    set(MAVSDK_TAG 613debe3e943181829c75050b8c3ed3cd3cd3821)
    message(STATUS "Building MAVSDK tag ${MAVSDK_TAG}")
    ExternalProject_Add(third_party_mavsdk
        SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/third_party/MAVSDK"
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/build_mavsdk/install
            -DBUILD_TESTS=OFF
        GIT_REPOSITORY https://github.com/Auterion/MAVSDK.git
        GIT_TAG ${MAVSDK_TAG}
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build_mavsdk
        INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
        )
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/build_mavsdk/install/include)
    # Hack needed for plugin_base.h include.
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/build_mavsdk/install/include/mavsdk)
    # Whichever path works :)
    link_directories(${CMAKE_CURRENT_BINARY_DIR}/build_mavsdk/install/lib)
    link_directories(${CMAKE_CURRENT_BINARY_DIR}/build_mavsdk/install/lib64)
    list(APPEND dependencies third_party_mavsdk)
endif()


# add YAML (build from source)
ExternalProject_Add(third_party_yaml
    SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/yaml-cpp
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/build_yaml/install
        -DYAML_CPP_BUILD_TESTS=OFF
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.6.3
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build_yaml
    INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
    )
include_directories(${CMAKE_CURRENT_BINARY_DIR}/build_yaml/install/include)
# Whichever path works :)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/build_yaml/install/lib)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/build_yaml/install/lib64)
list(APPEND dependencies third_party_yaml)

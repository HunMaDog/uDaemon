cmake_minimum_required(VERSION 3.12)

add_executable(udaemon_test
    main.cpp

    udaemon/udaemon.cpp
)

target_include_directories(udaemon_test PRIVATE
                           $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

target_compile_features(udaemon_test PRIVATE cxx_std_20)

target_compile_options(udaemon_test
                       PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/std:c++20>
                       $<$<CXX_COMPILER_ID:MSVC>:/W4>
                       $<$<CXX_COMPILER_ID:MSVC>:/WX>
                       $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall>
                       $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Werror>
                       $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-pedantic>)

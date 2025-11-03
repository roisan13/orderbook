cmake_minimum_required(VERSION 3.20)
project(Orderbook CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(orderbook
    src/Order.cpp
    src/Trade.cpp
    src/OrderModify.cpp
    src/OrderbookLevelInfos.cpp
    src/Orderbook.cpp
)

target_include_directories(orderbook PUBLIC include)

add_executable(orderbook_app main.cpp)
target_link_libraries(orderbook_app PRIVATE orderbook)


option(BUILD_TESTS "Build tests" ON)

if (BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )

    FetchContent_MakeAvailable(googletest)

    add_executable(orderbook_test
        tests/orderbook_test.cpp
    )

    target_link_libraries(orderbook_test
        PRIVATE
        orderbook
        GTest::gtest_main
    
    )

    include(GoogleTest)
    gtest_discover_tests(orderbook_test)
endif()
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
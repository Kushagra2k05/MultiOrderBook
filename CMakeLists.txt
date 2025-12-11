cmake_minimum_required(VERSION 4.0)
project(untitled3)

set(CMAKE_CXX_STANDARD 20)

add_executable(untitled3 main.cpp
        Order.h
        Order.cpp
        OrderModify.h
        Trade.h
        OrderbookLevelInfos.h
        Orderbook.h
        Orderbook.cpp
        Constants.h
        Types.h
        printUtils.h
)

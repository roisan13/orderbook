#pragma once
#include <cstdint>
#include <numeric>


using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderID = std::uint64_t;

// a small mix of concepts, 
enum class OrderType {
    Market,
    PostOnly,
    GoodTillCancel,
    StopOrder,
    FillAndKill, // IOC
    FillOrKill   // All or nothing
};

enum class Side {
    Buy,
    Sell
};

// Market sell orders use MIN_PRICE (0) to cross with all bids
constexpr Price MIN_PRICE = 0;
constexpr Price MAX_PRICE = std::numeric_limits<Price>::max();

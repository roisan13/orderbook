#pragma once
#include "Types.h"
#include <vector>

struct LevelInfo {
    Price price_;
    Quantity quantity_;
};
using LevelInfos = std::vector<LevelInfo>;

/**
 * View of the orderbook showing total quantities at each price level
 * Used for market data analysis
 */
class OrderbookLevelInfos {
public:
    OrderbookLevelInfos(const LevelInfos& bids, const LevelInfos& asks);

    const LevelInfos& GetBids() const { return bids_; }
    const LevelInfos& GetAsks() const { return asks_; }


private:
    LevelInfos bids_;
    LevelInfos asks_;
};

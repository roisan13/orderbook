#pragma once
#include "Types.h"
#include <vector>

/**
 * Information about one side of the trade
 */
struct TradeInfo {
    OrderID orderID;
    Price price;
    Quantity quantity;
};

/**
 * Representa a trade between a bid and an ask
 * Both sides trade at the same price (maker's price)
 */

class Trade {
public:
    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade);
    
    const TradeInfo& GetBidTrade() const { return bidTrade_; }
    const TradeInfo& GetAskTrade() const { return askTrade_; }

private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
};

using Trades = std::vector<Trade>;
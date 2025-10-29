#include "Orderbook.h"
#include <algorithm>
#include <numeric>

Trades Orderbook::AddOrder(OrderPointer order) {
    // Validation
    if (!order)
        throw std::invalid_argument("Order cannot be null");

    if (order->GetRemainingQuantity() == 0)
        throw std::invalid_argument("Order quantity must be greater than zero");
    
    if (order->GetPrice() < 0)
        throw std::invalid_argument("Order price must be positive");

    if (orders_.contains(order->GetOrderID()))
        return {};

    // Early exits for special order types
    if (order->GetOrderType() == OrderType::FillAndKill && 
        !CanMatch(order->GetSide(), order->GetPrice()))
        return {};

    if (order->GetOrderType() == OrderType::FillOrKill && 
        !CanFullyMatch(order->GetSide(), order->GetPrice(), order->GetRemainingQuantity()))
        return {};

    if (order->GetOrderType() == OrderType::PostOnly && 
        CanMatch(order->GetSide(), order->GetPrice()))
        return {};

    if (order->IsStopOrder()) {
        pendingStopOrders_.push_back(order);
        return {};
    }

    // Match the order
    auto trades = MatchAggressiveOrder(order);

    // Check for triggered stops
    if (!trades.empty()) {
        auto tradePrice = trades.back().GetAskTrade().price;
        CheckAndTriggerStopOrders(tradePrice);
    }

    // Handle FillAndKill
    if (order->GetOrderType() == OrderType::FillAndKill)
        return trades;

    // Add to book if not fully filled (GTC or PostOnly)
    if (!order->IsFilled() &&
        (order->GetOrderType() == OrderType::GoodTillCancel || 
         order->GetOrderType() == OrderType::PostOnly)) {
        
        OrderPointers::iterator iterator;
        if (order->GetSide() == Side::Buy) {
            auto& orders = bids_[order->GetPrice()];
            iterator = orders.insert(orders.end(), order);
        } else {
            auto& orders = asks_[order->GetPrice()];
            iterator = orders.insert(orders.end(), order);
        }
        
        orders_.insert({order->GetOrderID(), OrderEntry{order, iterator}});
    }

    return trades;
}

void Orderbook::CancelOrder(OrderID orderID) {
    // Check active orders first
    if (!orders_.contains(orderID)) {
        // Check pending stop orders
        auto it = std::find_if(pendingStopOrders_.begin(), pendingStopOrders_.end(),
            [orderID](const OrderPointer& order) { 
                return order->GetOrderID() == orderID; 
            });
        
        if (it != pendingStopOrders_.end()) {
            pendingStopOrders_.erase(it);
        }
        return;
    }
    
    const auto [order, iterator] = orders_.at(orderID);
    orders_.erase(orderID);

    auto price = order->GetPrice();
    
    if (order->GetSide() == Side::Sell) {
        auto& orders = asks_.at(price);
        orders.erase(iterator);
        if (orders.empty())
            asks_.erase(price);
    } else {
        auto& orders = bids_.at(price);
        orders.erase(iterator);
        if (orders.empty())
            bids_.erase(price);
    }
}

Trades Orderbook::ModifyOrder(OrderModify order) {
    if (!orders_.contains(order.GetOrderID()))
        return {};

    auto orderType = orders_.at(order.GetOrderID()).order->GetOrderType();
    CancelOrder(order.GetOrderID());
    return AddOrder(order.ToOrderPointer(orderType));
}

std::size_t Orderbook::Size() const {
    return orders_.size();
}

OrderbookLevelInfos Orderbook::GetOrderInfos() const {
    LevelInfos bidInfos, askInfos;
    bidInfos.reserve(bids_.size());
    askInfos.reserve(asks_.size());

    auto CreateLevelInfos = [](Price price, const OrderPointers& orders) {
        return LevelInfo{price, std::accumulate(orders.begin(), orders.end(), Quantity{0},
            [](Quantity runningSum, const OrderPointer& order) {
                return runningSum + order->GetRemainingQuantity();
            })};
    };

    for (const auto& [price, orders] : bids_) {
        bidInfos.push_back(CreateLevelInfos(price, orders));
    }

    for (const auto& [price, orders] : asks_) {
        askInfos.push_back(CreateLevelInfos(price, orders));
    }

    return {bidInfos, askInfos};
}

// Private helper methods
bool Orderbook::CanMatch(Side side, Price price) const {
    if (side == Side::Buy) {
        if (asks_.empty())
            return false;
        
        const auto& [bestAsk, _] = *asks_.begin();
        return price >= bestAsk;
    } else {
        if (bids_.empty())
            return false;
        
        const auto& [bestBid, _] = *bids_.begin();
        return price <= bestBid;
    }
}

bool Orderbook::CanFullyMatch(Side side, Price price, Quantity quantity) const {
    if (!CanMatch(side, price))
        return false;

    Quantity availableQuantity = 0;

    if (side == Side::Buy) {
        for (const auto& [levelPrice, asks] : asks_) {
            bool canCross = price >= levelPrice;
            if (!canCross)
                break;

            for (const auto& ask : asks) {
                availableQuantity += ask->GetRemainingQuantity();
                if (availableQuantity >= quantity)
                    return true;
            }
        }
    } else {
        for (const auto& [levelPrice, bids] : bids_) {
            bool canCross = price <= levelPrice;
            if (!canCross)
                break;

            for (const auto& bid : bids) {
                availableQuantity += bid->GetRemainingQuantity();
                if (availableQuantity >= quantity)
                    return true;
            }
        }
    }
    
    return false;  // FIX: Added missing return
}

void Orderbook::CheckAndTriggerStopOrders(Price tradePrice) {
    std::vector<OrderPointer> triggeredOrders;

    std::erase_if(pendingStopOrders_, [&](OrderPointer order) {
        auto stopPrice = order->GetStopPrice().value();
        bool hasTriggered = (order->GetSide() == Side::Buy)
            ? tradePrice >= stopPrice
            : tradePrice <= stopPrice;
        
        if (hasTriggered)
            triggeredOrders.push_back(order);

        return hasTriggered;
    });

    for (auto& triggered : triggeredOrders) {
        MatchAggressiveOrder(triggered);
    }
}

void Orderbook::MatchAtPriceLevel(OrderPointer& aggressive, OrderPointers& restingOrders, 
                                  Trades& trades) {
    while (!restingOrders.empty() && !aggressive->IsFilled()) {
        OrderPointer& restingOrder = restingOrders.front();

        Quantity quantity = std::min(restingOrder->GetRemainingQuantity(), 
                                     aggressive->GetRemainingQuantity());
        
        // Trade at maker's price (resting order)
        Price tradePrice = restingOrder->GetPrice();

        aggressive->Fill(quantity);
        restingOrder->Fill(quantity);

        // Create trade with correct bid/ask order
        if (aggressive->GetSide() == Side::Buy) {
            trades.emplace_back(
                TradeInfo{aggressive->GetOrderID(), tradePrice, quantity},
                TradeInfo{restingOrder->GetOrderID(), tradePrice, quantity}
            );
        } else {
            trades.emplace_back(
                TradeInfo{restingOrder->GetOrderID(), tradePrice, quantity},
                TradeInfo{aggressive->GetOrderID(), tradePrice, quantity}
            );
        }

        if (restingOrder->IsFilled()) {
            orders_.erase(restingOrder->GetOrderID());
            restingOrders.pop_front();
        }
    }
}

Trades Orderbook::MatchAggressiveOrder(OrderPointer& order) {
    Trades trades;

    if (order->GetSide() == Side::Buy) {
        while (!asks_.empty() && !order->IsFilled()) {
            auto askPrice = asks_.begin()->first;

            if (order->GetPrice() < askPrice)
                break;

            auto& askOrders = asks_.begin()->second;
            
            MatchAtPriceLevel(order, askOrders, trades);

            if (askOrders.empty())
                asks_.erase(askPrice);
        }
    } else {
        while (!bids_.empty() && !order->IsFilled()) {
            auto bidPrice = bids_.begin()->first;

            if (order->GetPrice() > bidPrice)
                break;

            auto& bidOrders = bids_.begin()->second;
        
            MatchAtPriceLevel(order, bidOrders, trades);

            if (bidOrders.empty())
                bids_.erase(bidPrice);
        }
    }

    return trades;
}
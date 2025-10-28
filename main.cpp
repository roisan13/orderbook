#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <list>
#include <unordered_map>
#include <set>
#include <deque>
#include <queue>
#include <memory>
#include <list>
#include <format>
#include <cstdint>
#include <numeric>
#include <optional>

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

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderID = std::uint64_t;

struct LevelInfo {
    Price price_;
    Quantity quantity_;
};
using LevelInfos = std::vector<LevelInfo>;

class OrderbookLevelInfos {
public:
    OrderbookLevelInfos(const LevelInfos& bids, const LevelInfos& asks) : bids_{ bids }, asks_ { asks } { }

    const LevelInfos& GetBids() const { return bids_; }
    const LevelInfos& GetAsks() const { return asks_; }


private:
    LevelInfos bids_;
    LevelInfos asks_;
};

class Order{
public:
    Order(OrderType orderType, OrderID orderID, Side side, Price price, Quantity quantity) :
        orderType_ { orderType }, orderID_ { orderID }, side_ { side }, price_ { price },
        stopPrice_ {},  initialQuantity_ { quantity }, remainingQuantity_ { quantity }
        { }

    Order(OrderType orderType, OrderID orderID, Side side, Price price, std::optional<Price> stopPrice, Quantity quantity) :
        orderType_ { orderType }, orderID_ { orderID }, side_ { side }, price_ { price },
        stopPrice_ { stopPrice},  initialQuantity_ { quantity }, remainingQuantity_ { quantity }
        { }

    
    OrderID GetOrderID() const { return orderID_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    std::optional<Price> GetStopPrice() const { return stopPrice_; }
    OrderType GetOrderType() const { return orderType_; }
    Quantity GetInitialQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }

    void SetPrice(Price price) {
        if (price <= 0)
            throw std::logic_error(std::format("Order ({}) must have a strictly positive price", GetOrderID()));
        
        price_ = price;
    }


    bool IsFilled() const { return GetRemainingQuantity() == 0; }
    bool isStopOrder() const { return stopPrice_.has_value(); }

    void Fill(Quantity quantity) {
        if (quantity > GetRemainingQuantity())
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderID()));

        remainingQuantity_ -= quantity;
    }
    
private:
    OrderType orderType_;
    OrderID orderID_;
    Side side_;
    Price price_;
    std::optional<Price> stopPrice_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;


class OrderModify {
public:
    OrderModify(OrderID orderID, Side side, Price price, Quantity quantity) :
        orderID_ { orderID }, side_ { side }, price_ { price }, quantity_ { quantity }
        { }

    OrderID GetOrderID() const { return orderID_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    Quantity GetQuantity() const { return quantity_; }

    OrderPointer ToOrderPointer(OrderType type) const {
        return std::make_shared<Order>(type, GetOrderID(), GetSide(), GetPrice(), GetQuantity());
    }
    
private:
    OrderID orderID_;
    Side side_;
    Price price_;
    Quantity quantity_;

};

struct TradeInfo {
    OrderID orderID;
    Price price;
    Quantity quantity;
};

class Trade {
public:
    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade)
        : bidTrade_ { bidTrade }, askTrade_ { askTrade } { };
    
    const TradeInfo& GetBidTrade() const { return bidTrade_; }
    const TradeInfo& GetAskTrade() const { return askTrade_; }

private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
};

using Trades = std::vector<Trade>;

class Orderbook {
public:
    Trades AddOrder(OrderPointer order) {

        if (!order)
            throw std::invalid_argument("Order cannot be null");

        if (order->GetRemainingQuantity() == 0)
            throw std::invalid_argument("Order quantity must be greater than zero");
        
        if (order->GetPrice() <= 0 )
            throw std::invalid_argument("Order price must be strictly positive");

        if (orders_.contains(order->GetOrderID()))
            return { };

        if (order->GetOrderType() == OrderType::FillAndKill && !CanMatch(order->GetSide(), order->GetPrice()))
            return { };

        if (order->GetOrderType() == OrderType::FillOrKill && !CanFullyMatch(order->GetSide(), order->GetPrice(), order->GetRemainingQuantity()))
            return { };

        if (order->GetOrderType() == OrderType::Market) {
            if (order->GetSide() == Side::Buy)  {
                order->SetPrice(std::numeric_limits<Price>::max());
            }
            else {
                order->SetPrice(1); // min viable price
            }
        }

        if (order->GetOrderType() == OrderType::PostOnly && CanMatch(order->GetSide(), order->GetPrice()))
            return { };

        if (order->GetOrderType() == OrderType::StopOrder) {
            pendingStopOrders_.push_back(order);
            return { };
        }
            

        auto trades = MatchAggressiveOrder(order);


        if (!trades.empty()){
            auto tradePrice = trades.back().GetAskTrade().price; // both ask and bid trade is same (maker's price)
            CheckAndTriggerStopOrders(tradePrice);
        }


        if (order->GetOrderType() == OrderType::FillAndKill)
            return trades;

        if (!order->IsFilled()) {
            OrderPointers::iterator iterator;
            if (order->GetSide() == Side::Buy) {
                auto& orders = bids_[order->GetPrice()];
                iterator = orders.insert(orders.end(), order);
            }
            else { // Side::Sell
                auto& orders = asks_[order->GetPrice()];
                iterator = orders.insert(orders.end(), order);
            }

            orders_.insert({ order->GetOrderID(), OrderEntry{ order, iterator}});
        }

        return trades;
    }

    void CancelOrder(OrderID orderID) {
        if (!orders_.contains(orderID))
            return;
        
        const auto [order, iterator] = orders_.at(orderID);
        orders_.erase(orderID);

        if (order->GetSide() == Side::Sell) {
            auto price = order->GetPrice();
            auto& orders = asks_.at(price);
            orders.erase(iterator);
            if (orders.empty())
                asks_.erase(price);
        }
        else {
            auto price = order->GetPrice();
            auto& orders = bids_.at(price);
            orders.erase(iterator);
            if(orders.empty())
                bids_.erase(price);
        }
    }

    Trades ModifyOrder(OrderModify order){ 
        if (!orders_.contains(order.GetOrderID()))
            return { };

        auto orderType = orders_.at(order.GetOrderID()).order->GetOrderType();
        CancelOrder(order.GetOrderID());
        return AddOrder(order.ToOrderPointer(orderType));
        
    }

    std::size_t Size() const { return orders_.size(); }

    OrderbookLevelInfos GetOrderInfos() const {
        LevelInfos bidInfos, askInfos;
        bidInfos.reserve(bids_.size());
        askInfos.reserve(asks_.size());

        auto CreateLevelInfos = [](Price price, const OrderPointers& orders) {
            return LevelInfo { price, std::accumulate(orders.begin(), orders.end(), Quantity{0},
                [](Quantity runningSum, const OrderPointer& order) {
                    return runningSum + order->GetRemainingQuantity();
                })};};

        for (const auto& [price, orders] : bids_) {
            bidInfos.push_back(CreateLevelInfos(price, orders));
        }

        for (const auto& [price, orders] : asks_) {
            askInfos.push_back(CreateLevelInfos(price, orders));
        }

        return {bidInfos, askInfos};


    }
private:
    struct OrderEntry {
        OrderPointer order { nullptr };
        OrderPointers::iterator location;
    };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderID, OrderEntry> orders_;
    std::vector<OrderPointer> pendingStopOrders_;

    bool CanMatch(Side side, Price price) const {
        if (side == Side::Buy) {
            if (asks_.empty())
                return false;
            
            const auto& [bestAsk, _] = *asks_.begin();

            return price >= bestAsk;
        }
        else { // Side::Sell
            if (bids_.empty())
                return false;
            
            const auto& [bestBid, _] = *bids_.begin();

            return price <= bestBid;
        }
    }

    bool CanFullyMatch(Side side,Price price, Quantity quantity) const {

        if (!CanMatch(side, price))
            return false;

        auto orderInfos = GetOrderInfos();
        auto levelInfos = (side == Side::Sell) ? orderInfos.GetBids() : orderInfos.GetAsks();  
        
        Quantity availableQuantity = 0;

        for (const auto& [levelPrice, levelQty] : levelInfos) {

            bool canCross = (side == Side::Sell) ? price <= levelPrice : price >= levelPrice;

            if (!canCross)
                break;
            
            availableQuantity += levelQty;
            if (availableQuantity >= quantity)
                return true;
        }

        return false;
    }

    void CheckAndTriggerStopOrders(Price tradePrice) {
        std::vector<OrderPointer> triggedOrders;

        std::erase_if(pendingStopOrders_, [&](OrderPointer order) {
            auto stopPrice = order->GetStopPrice().value();
            bool hasTriggered = (order->GetSide() == Side::Buy) ? tradePrice >= stopPrice : tradePrice <= stopPrice;
            if (hasTriggered)
                triggedOrders.push_back(order);

            return hasTriggered;
        });

        for (auto& triggered : triggedOrders) {
            auto newOrder = std::make_shared<Order>(
                    OrderType::FillAndKill,
                    triggered->GetOrderID(),
                    triggered->GetSide(),
                    triggered->GetStopPrice().value(),
                    triggered->GetInitialQuantity()
                );

            AddOrder(newOrder);
        }

    }

    void MatchAtPriceLevel(OrderPointer& aggressive, OrderPointers& restingOrders, Trades& trades) {
        while (!restingOrders.empty() && !aggressive->IsFilled()) {
                    OrderPointer& restingOrder = restingOrders.front();

                    Quantity quantity = std::min(restingOrder->GetRemainingQuantity(), aggressive->GetRemainingQuantity());
                    
                    // MAKER's price - the resting order dictates the price
                    Price tradePrice = restingOrder->GetPrice();

                    aggressive->Fill(quantity);
                    restingOrder->Fill(quantity);

                    // small duplicate segment
                    if (aggressive->GetSide() == Side::Buy){
                        trades.emplace_back(
                            TradeInfo{aggressive->GetOrderID(), tradePrice, quantity},
                            TradeInfo{restingOrder->GetOrderID(), tradePrice, quantity}
                        );
                    }
                    else {
                        trades.emplace_back(
                            TradeInfo{restingOrder->GetOrderID(), tradePrice, quantity},
                            TradeInfo{aggressive->GetOrderID(), tradePrice, quantity}
                        );
                    }

                    if (restingOrder->IsFilled()){
                        orders_.erase(restingOrder->GetOrderID());
                        restingOrders.pop_front();
                    }
                }

    }
    
    Trades MatchAggressiveOrder(OrderPointer& order) {
        Trades trades;

        /*
         * This code holds a lot of duplicate logic, but as I went through 2-3 refactors
         * I came to the conclusion that this is most readable and also most easy to debug
         * other options included using std::ref or templates
         */

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
        }
        else { // Side::Sell
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
};

int main() {
    Orderbook orderbook;
    const OrderID orderID = 1;
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderID, Side::Buy, 100, 10));
    std::cout << orderbook.Size() << "\n"; // 1
    orderbook.CancelOrder(orderID);
    std::cout << orderbook.Size() << "\n"; // 0
    return 0;

}
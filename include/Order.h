#pragma once
#include "Types.h"
#include <optional>
#include <memory>
#include <list>


class Order{
public:

    Order(OrderType orderType, OrderID orderID, Side side, Price price, Quantity quantity, std::optional<Price> stopPrice = std::nullopt);

    // Getters
    OrderID GetOrderID() const { return orderID_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    std::optional<Price> GetStopPrice() const { return stopPrice_; }
    OrderType GetOrderType() const { return orderType_; }
    Quantity GetInitialQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return GetInitialQuantity() - GetRemainingQuantity(); }


    bool IsFilled() const;
    bool IsStopOrder() const;

    void Fill(Quantity quantity);
    
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

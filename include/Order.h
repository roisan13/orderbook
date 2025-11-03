#pragma once
#include "Types.h"
#include <optional>
#include <memory>
#include <list>


class Order{
public:

    Order(OrderType orderType, OrderID orderID, Side side, Price price, Quantity quantity, std::optional<Price> stopPrice = std::nullopt);

    // Getters
    [[nodisacrd]] OrderID GetOrderID() const noexcept { return orderID_; }
    [[nodiscard]] Side GetSide() const noexcept { return side_; }
    [[nodiscard]] Price GetPrice() const noexcept { return price_; }
    [[nodiscard]] std::optional<Price> GetStopPrice() const noexcept { return stopPrice_; }
    [[nodiscard]] OrderType GetOrderType() const noexcept { return orderType_; }
    [[nodiscard]] Quantity GetInitialQuantity() const noexcept { return initialQuantity_; }
    [[nodiscard]] Quantity GetRemainingQuantity() const noexcept { return remainingQuantity_; }
    [[nodiscard]] Quantity GetFilledQuantity() const noexcept { return GetInitialQuantity() - GetRemainingQuantity(); }


    [[nodiscard]] bool IsFilled() const noexcept;
    [[nodiscard]] bool IsStopOrder() const noexcept;

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

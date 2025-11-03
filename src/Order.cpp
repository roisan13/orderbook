#include "Order.h"
#include <format>
#include <stdexcept>
#include <limits>


Order::Order(OrderType orderType, OrderID orderID, Side side, Price price, Quantity quantity, std::optional<Price> stopPrice) :
    orderType_ { orderType },
    orderID_ { orderID },
    side_ { side },
    price_ { (orderType == OrderType::Market) ? ((side == Side::Buy) ? MAX_PRICE : MIN_PRICE) : price},
    stopPrice_ { stopPrice},
    initialQuantity_ { quantity },
    remainingQuantity_ { quantity }
    { }

bool Order::IsFilled() const noexcept { 
    return GetRemainingQuantity() == 0;
}

bool Order::IsStopOrder() const noexcept {
    return stopPrice_.has_value();
}

void Order::Fill(Quantity quantity) {
    if (quantity > GetRemainingQuantity())
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderID()));

        remainingQuantity_ -= quantity;
}
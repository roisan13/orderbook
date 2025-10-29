#include "OrderModify.h"

OrderModify::OrderModify(OrderID orderID, Side side, Price price, Quantity quantity)
    : orderID_{orderID}, side_{side}, price_{price}, quantity_{quantity} {}

OrderPointer OrderModify::ToOrderPointer(OrderType type) const {
    return std::make_shared<Order>(type, GetOrderID(), GetSide(), GetPrice(), GetQuantity());
}
#pragma once
#include "Order.h"

/**
 * Represents a request to modify an existing order.
 * Implemented as cancel-and-replace.
 */
class OrderModify {
public:
    OrderModify(OrderID orderID, Side side, Price price, Quantity quantity);
    
    OrderID GetOrderID() const { return orderID_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const { return price_; }
    Quantity GetQuantity() const { return quantity_; }
    
    OrderPointer ToOrderPointer(OrderType type) const;
    
private:
    OrderID orderID_;
    Side side_;
    Price price_;
    Quantity quantity_;
};
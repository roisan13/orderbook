#include <iostream>
#include "Orderbook.h"

int main() {
    Orderbook orderbook;
    const OrderID orderID = 1;

    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCancel, orderID, Side::Buy, 100, 10));
    
    std::cout << orderbook.Size() << "\n"; // 1
    
    orderbook.CancelOrder(orderID);
    
    std::cout << orderbook.Size() << "\n"; // 0
    return 0;

}
#pragma once
#include "Order.h"
#include "Trade.h"
#include "OrderModify.h"
#include "OrderbookLevelInfos.h"
#include <map>
#include <unordered_map>
#include <vector>

/**
 * Orderbook implementation.
 * 
 * Matching Algorithm: FIFO with price-time priority
 * Trade Pricing: Maker's price (resting order's price)
 * 
 * Supported Order Types:
 * - Market: Execute at any available price
 * - GoodTillCancel: Rest in book until filled or cancelled
 * - FillAndKill (IOC): Fill immediately, cancel remainder
 * - FillOrKill: All-or-nothing execution
 * - PostOnly: Only add liquidity (maker-only)
 * - StopOrder: Trigger based on trade price
 */
class Orderbook {
public:
    /**
     * Adds an order to the orderbook and attempts to match it.
     * 
     * @param order The order to add
     * @return Vector of trades resulting from this order
     * @throws std::invalid_argument if order validation fails
     */
    Trades AddOrder(OrderPointer order);
    
    /**
     * Cancels an order by ID.
     * Handles both active and pending stop orders.
     * 
     * @param orderID The ID of the order to cancel
     */
    void CancelOrder(OrderID orderID);
    
    /**
     * Modifies an existing order (implemented as cancel-and-replace).
     * 
     * @param order The modification details
     * @return Vector of trades if the new order matches
     */
    Trades ModifyOrder(OrderModify order);
    
    /**
     * Returns the number of active orders in the book.
     * Note: Does not include pending stop orders.
     */
    [[nodiscard]] std::size_t Size() const noexcept;

    /**
     * Returns the number of pending stop orders
     */
    [[nodiscard]] std::size_t PendingStopCount() const noexcept;
    
    /**
     * Returns aggregated order information by price level.
     */
    OrderbookLevelInfos GetOrderInfos() const;

private:
    struct OrderEntry {
        OrderPointer order{nullptr};
        OrderPointers::iterator location;
    };
    
    // Price-sorted books
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    
    // Fast lookup by order ID
    std::unordered_map<OrderID, OrderEntry> orders_;
    
    // Stop orders waiting for trigger
    std::vector<OrderPointer> pendingStopOrders_;
    
    // Helper methods
    bool CanMatch(Side side, Price price) const;
    bool CanFullyMatch(Side side, Price price, Quantity quantity) const;
    
    Trades CheckAndTriggerStopOrders(Price tradePrice);
    void MatchAtPriceLevel(OrderPointer& aggressive, OrderPointers& restingOrders, 
                          Trades& trades);
    Trades MatchAggressiveOrder(OrderPointer& order);
};
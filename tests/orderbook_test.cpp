#include <gtest/gtest.h>
#include "Orderbook.h"
#include <memory>

// ===============================
//          Basic tests
// ===============================

TEST(OrderbookTest, AddOrderIncreasesSize) {
    Orderbook book;

    auto order = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 100, 10);

    book.AddOrder(order);

    EXPECT_EQ(book.Size(), 1);
}

TEST(OrderbookTest, CancelOrderDecreasesSize) {
    Orderbook book;

    auto order = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 100, 10);

    book.AddOrder(order);
    EXPECT_EQ(book.Size(), 1);

    book.CancelOrder(1);
    EXPECT_EQ(book.Size(), 0);
}

TEST(OrderbookTest, CancelNonExistentOrderDoesNothing) {
    Orderbook book;
    book.CancelOrder(999); // shouldn't crash

    EXPECT_EQ(book.Size(), 0);
}

// ===============================
//          Matching tests
// ===============================

TEST(OrderbookTest, BuyMatchesAsk) {
    Orderbook book;

    // Add sell order
    auto ask = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 10);
    book.AddOrder(ask);

    // Add matching buy order
    auto bid = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 100, 10);
    auto trades = book.AddOrder(bid);

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].GetBidTrade().orderID, 2);
    EXPECT_EQ(trades[0].GetAskTrade().orderID, 1);
    EXPECT_EQ(trades[0].GetBidTrade().price, 100);
    EXPECT_EQ(trades[0].GetBidTrade().quantity, 10);

    EXPECT_EQ(book.Size(), 0);
}

TEST(OrderbookTest, PartialFill) {
    Orderbook book;

    auto ask = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 50);
    book.AddOrder(ask);

    auto bid = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 100, 30);
    auto trades = book.AddOrder(bid);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].GetBidTrade().quantity, 30);

    EXPECT_EQ(book.Size(), 1);
}

TEST(OrderbookTest, NoCrossNoMatch) {
    Orderbook book;

    auto ask = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 105, 10);
    book.AddOrder(ask);

    auto bid = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 95, 10);
    auto trades = book.AddOrder(bid);

    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(book.Size(), 2);
}

// ===============================
//          FIFO tests
// ===============================

TEST(OrderbookTest, FIFOMatching) {
    Orderbook book;

    auto ask1 = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 10);
    auto ask2 = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Sell, 100, 20);
    auto ask3 = std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 100, 30);

    book.AddOrder(ask1);
    book.AddOrder(ask2);
    book.AddOrder(ask3);

    // fill first two completely
    auto bid = std::make_shared<Order>(OrderType::GoodTillCancel, 4, Side::Buy, 100, 25);
    auto trades = book.AddOrder(bid);

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].GetAskTrade().orderID, 1);
    EXPECT_EQ(trades[0].GetAskTrade().quantity, 10);

    EXPECT_EQ(trades[1].GetAskTrade().orderID, 2);
    EXPECT_EQ(trades[1].GetAskTrade().quantity, 15);

    // ask2 (5 remaining) and ask3 should be in the book
    EXPECT_EQ(book.Size(), 2);
}

// ===============================
//    Price-Time Priority Tests
// ===============================

TEST(OrderbookTest, PricePriorityOverTime) {
    Orderbook book;

    auto ask1 = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 105, 10);
    book.AddOrder(ask1);

    auto ask2 = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Sell, 100, 10);
    book.AddOrder(ask2);

    auto bid = std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Buy, 105, 5);
    auto trades = book.AddOrder(bid);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].GetAskTrade().orderID, 2);
    EXPECT_EQ(trades[0].GetAskTrade().price, 100);
}

// ===============================
//       Order-Type Tests
// ===============================

TEST(OrderbookTest, MarketOrderExecutesImmediately) {
    Orderbook book;

    auto ask = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 10);
    book.AddOrder(ask);

    auto marketBuy = std::make_shared<Order>(OrderType::Market, 2, Side::Buy, 0, 10);
    auto trades = book.AddOrder(marketBuy);

    ASSERT_EQ(trades.size(), 1);
    ASSERT_EQ(trades[0].GetBidTrade().quantity, 10);
    EXPECT_EQ(book.Size(), 0);
}

TEST(OrderbookTest, FillAndKillPartialFill) {
    Orderbook book;

    auto ask = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 5);
    book.AddOrder(ask);

    auto fak = std::make_shared<Order>(OrderType::FillAndKill, 2, Side::Buy, 100, 10);
    auto trades = book.AddOrder(fak);

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].GetBidTrade().quantity, 5);

    EXPECT_EQ(book.Size(), 0); // FAK order shouldn't rest in book
}

TEST(OrderbookTest, FillOrKillSuccess) {
    Orderbook book;

    auto ask1 = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 30);
    auto ask2 = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Sell, 101, 50);

    book.AddOrder(ask1);
    book.AddOrder(ask2);

    auto fok = std::make_shared<Order>(OrderType::FillOrKill, 3, Side::Buy, 101, 70);
    auto trades= book.AddOrder(fok);

    EXPECT_EQ(trades.size(), 2);
}

TEST(OrderbookTest, FillOrKillFailure) {
    Orderbook book;

    auto ask1 = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 30);
    auto ask2 = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Sell, 101, 50);

    book.AddOrder(ask1);
    book.AddOrder(ask2);

    auto fok = std::make_shared<Order>(OrderType::FillOrKill, 3, Side::Buy, 101, 90);
    auto trades= book.AddOrder(fok);

    EXPECT_EQ(trades.size(), 0);

    EXPECT_EQ(book.Size(), 2);
}

TEST(OrderbookTest, PostOnlyDoesNotCross) {
    Orderbook book;

    auto ask = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 10);
    book.AddOrder(ask);

    auto postOnly = std::make_shared<Order>(OrderType::PostOnly, 2, Side::Buy, 100, 10);
    auto trades = book.AddOrder(postOnly);

    EXPECT_EQ(trades.size(), 0); // no trade
    EXPECT_EQ(book.Size(), 1); // original ask should remain
}

TEST(OrderbookTest, PostOnlyAddsToBook) {
    Orderbook book;

    auto ask = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 10);
    book.AddOrder(ask);

    auto postOnly = std::make_shared<Order>(OrderType::PostOnly, 2, Side::Buy, 95, 10);
    auto trades = book.AddOrder(postOnly);

    EXPECT_EQ(trades.size(), 0);
    EXPECT_EQ(book.Size(), 2);
}

// ===============================
//   Multi-Level Matching Tests
// ===============================

TEST(OrderbookTest, WalkTheBook) {
    Orderbook book;

    auto ask1 = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 10);
    auto ask2 = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Sell, 101, 20);
    auto ask3 = std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 102, 30);

    book.AddOrder(ask1);
    book.AddOrder(ask2);
    book.AddOrder(ask3);

    auto bid = std::make_shared<Order>(OrderType::GoodTillCancel, 4, Side::Buy, 105, 50);
    auto trades = book.AddOrder(bid);

    ASSERT_EQ(trades.size(), 3);

    EXPECT_EQ(trades[0].GetAskTrade().price, 100);
    EXPECT_EQ(trades[0].GetAskTrade().quantity, 10);

    EXPECT_EQ(trades[1].GetAskTrade().price, 101);
    EXPECT_EQ(trades[1].GetAskTrade().quantity, 20);

    EXPECT_EQ(trades[2].GetAskTrade().price, 102);
    EXPECT_EQ(trades[2].GetAskTrade().quantity, 20); // ask3 only partially fileld

    EXPECT_EQ(book.Size(), 1);
}

// ===============================
//    Exception Handling Tests
// ===============================

TEST(OrderbookTest, NullOrderThrows) {
    Orderbook book;
    EXPECT_THROW(
        book.AddOrder(nullptr),
        std::invalid_argument
    );
}

TEST(OrderbookTest, ZeroQuantityThrows) {
    Orderbook book;
    auto order = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 100, 0);

    EXPECT_THROW(
        book.AddOrder(order),
        std::invalid_argument
    );
}

TEST(OrderbookTest, NegativePriceThrows) {
    Orderbook book;

    auto order = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, -10, 1);

    EXPECT_THROW(
        book.AddOrder(order),
        std::invalid_argument
    );
}

// ===============================
//        Maker Price Tests
// ===============================



TEST(OrderbookTest, TradeAtMakerPrice) {
    Orderbook book;

    auto ask = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Sell, 100, 10);
    book.AddOrder(ask);

    auto bid = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 105, 10);
    auto trades = book.AddOrder(bid);

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].GetBidTrade().price, 100);
    EXPECT_EQ(trades[0].GetAskTrade().price, 100);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}



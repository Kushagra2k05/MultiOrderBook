#include <iostream>
#include "Orderbook.h"
#include "PrintUtils.h"

int main() {
    Orderbook book;

    auto o1 = std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 100, 10);
    auto o2 = std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Sell, 100, 5);

    PrintOrder(*o1);
    PrintOrder(*o2);

    auto trades1 = book.AddOrder(o1);
    PrintTrades(trades1);

    auto trades2 = book.AddOrder(o2);
    PrintTrades(trades2);

    PrintOrderbookDepth(book);

    return 0;
}


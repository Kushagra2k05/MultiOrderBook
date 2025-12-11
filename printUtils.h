#pragma once
#include <iostream>
#include <iomanip>

#include "Order.h"
#include "Trade.h"
#include "Orderbook.h"
#include "OrderbookLevelInfos.h"

inline void PrintOrder(const Order& o) {
    std::cout << "ORDER CREATED:\n";
    std::cout << "  ID:        " << o.GetOrderId() << "\n";

    std::cout << "  Type:      ";
    switch (o.GetOrderType()) {
        case OrderType::GoodTillCancel: std::cout << "GoodTillCancel"; break;
        case OrderType::GoodForDay:     std::cout << "GoodForDay"; break;
        case OrderType::FillAndKill:    std::cout << "FillAndKill"; break;
        case OrderType::FillOrKill:     std::cout << "FillOrKill"; break;
        case OrderType::Market:         std::cout << "Market"; break;
    }

    std::cout << "\n  Side:      " << (o.GetSide() == Side::Buy ? "BUY" : "SELL");
    std::cout << "\n  Price:     " << o.GetPrice();
    std::cout << "\n  Quantity:  " << o.GetInitialQuantity();
    std::cout << "\n\n";
}

inline void PrintTrade(const Trade& t) {
    const auto& buy  = t.GetBidTrade();
    const auto& sell = t.GetAskTrade();

    std::cout << "TRADE EXECUTED:\n"
              << "  BuyOrder:  ID=" << buy.orderId_
              << "  Price="       << buy.price_
              << "  Qty="         << buy.quantity_ << "\n"
              << "  SellOrder: ID=" << sell.orderId_
              << "  Price="        << sell.price_
              << "  Qty="          << sell.quantity_ << "\n\n";
}



inline void PrintOrderbookDepth(const Orderbook& book) {
    auto depth = book.GetOrderInfos();

    std::cout << "========= ORDERBOOK DEPTH =========\n";

    std::cout << "--- BIDS (BUY) ---\n";
    for (const auto& lvl : depth.GetBids()) {
        std::cout << "  Price: " << std::setw(8) << lvl.price
                  << " | Qty: "  << lvl.quantity << "\n";
    }

    std::cout << "--- ASKS (SELL) ---\n";
    for (const auto& lvl : depth.GetAsks()) {
        std::cout << "  Price: " << std::setw(8) << lvl.price
                  << " | Qty: "  << lvl.quantity << "\n";
    }

    std::cout << "===================================\n\n";
}

inline void PrintTrades(const Traders& trades) {
    if (trades.empty()) {
        std::cout << "No trades executed.\n\n";
        return;
    }
    for (const auto& t : trades)
        PrintTrade(t);
}

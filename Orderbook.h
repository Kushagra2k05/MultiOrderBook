#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <map>
#include <list>
#include <unordered_map>
#include <numeric>
#include <memory>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <vector>        // <- added

#include "Order.h"
#include "OrderModify.h"
#include "Trade.h"
#include "OrderbookLevelInfos.h"

// Aliases used by this header
using OrderPointer  = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;
using OrderIds      = std::vector<OrderId>;

class Orderbook {
public:
    Orderbook();
    ~Orderbook() noexcept;

    // disable copy/move because the book owns a running thread
    Orderbook(const Orderbook&) = delete;
    Orderbook& operator=(const Orderbook&) = delete;
    Orderbook(Orderbook&&) = delete;
    Orderbook& operator=(Orderbook&&) = delete;

    // Basic inspection
    size_t Size() const;

    // Matching helpers
    bool CanMatch(Side side, Price price) const;

    // Matching operations
    Traders MatchOrders(); // public thread-safe wrapper (locks internally)
    Traders AddOrder(OrderPointer order);
    void CancelOrder(OrderId id);
    Traders MatchOrder(const OrderModify& req);
    bool CanFullyFill(Side side, Price price, Quantity quantity) const;

    // Market depth / L2/L3 infos
    OrderbookLevelInfos GetOrderInfos() const;

private:
    // Internal representation of an order entry
    struct OrderEntry {
        OrderPointer order_{nullptr};
        OrderPointers::iterator location_;
    };

    // Orderbook containers (price -> list of orders)
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    // ---------- Concurrency & background pruning ----------
    mutable std::mutex bookMutex_; // protects bids_, asks_, orders_

    // Background pruning thread: started in ctor, joined in dtor.
    void PruneGoodForDayOrders();

    // Internal cancel helpers (assume caller holds bookMutex_)
    void CancelOrderInternal(OrderId orderId);
    void CancelOrders(const OrderIds& orderIds);

    std::atomic<bool> shutdown_{false};
    std::condition_variable shutdownConditionVariable_;
    std::mutex shutdownMutex_; // used only with condition_variable wait
    std::thread ordersPruneThread_;

    // ---------- Internal helpers (assume bookMutex_ is locked) ----------
    Traders MatchOrdersUnlocked();
};

#endif // ORDERBOOK_H

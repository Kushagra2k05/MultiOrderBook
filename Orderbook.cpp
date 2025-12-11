
#include "Orderbook.h"

#include <algorithm> 
#include <numeric>   
#include <iostream>  
#include <ctime>     



Orderbook::Orderbook()
    : ordersPruneThread_{ [this] { PruneGoodForDayOrders(); } }
{

}

Orderbook::~Orderbook()
{
    // Signal shutdown and wake the pruning thread
    shutdown_.store(true, std::memory_order_release);
    shutdownConditionVariable_.notify_one();

    if (ordersPruneThread_.joinable())
        ordersPruneThread_.join();
}

// ---------------- Inspection ----------------

size_t Orderbook::Size() const
{
    std::scoped_lock lock(bookMutex_);
    return orders_.size();
}

// ---------------- Matching condition ----------------

bool Orderbook::CanMatch(Side side, Price price) const
{
    std::scoped_lock lock(bookMutex_);

    if (side == Side::Buy) {
        if (asks_.empty()) return false;
        return price >= asks_.begin()->first;
    } else {
        if (bids_.empty()) return false;
        return bids_.begin()->first >= price;
    }
}

// ---------------- CanFullyFill (used by FOK) ----------------
bool Orderbook::CanFullyFill(Side side, Price price, Quantity quantity) const
{
    std::scoped_lock lock(bookMutex_);

    if (!CanMatch(side, price))
        return false;

    std::optional<Price> threshold;
    if (side == Side::Buy) {
        if (asks_.empty()) return false;
        threshold = asks_.begin()->first;
    } else {
        if (bids_.empty()) return false;
        threshold = bids_.begin()->first;
    }


    if (side == Side::Buy) {
        // examine asks (ascending)
        for (const auto& [levelPrice, list] : asks_) {
            if (levelPrice > price) break;

            Quantity levelQuantity = std::accumulate(
                list.begin(), list.end(), Quantity(0),
                [](Quantity acc, const OrderPointer& o) { return acc + o->GetRemainingQuantity(); });

            if (quantity <= levelQuantity) return true;
            quantity -= levelQuantity;
        }
    } else {
        // side == Sell: examine bids (descending)
        for (const auto& [levelPrice, list] : bids_) {
            if (levelPrice < price) break;

            Quantity levelQuantity = std::accumulate(
                list.begin(), list.end(), Quantity(0),
                [](Quantity acc, const OrderPointer& o) { return acc + o->GetRemainingQuantity(); });

            if (quantity <= levelQuantity) return true;
            quantity -= levelQuantity;
        }
    }

    return false;
}

Traders Orderbook::MatchOrdersUnlocked()
{
    Traders trades;

    while (!bids_.empty() && !asks_.empty())
    {
        auto bidIt = bids_.begin();
        auto askIt = asks_.begin();

        if (bidIt->first < askIt->first)
            break;

        auto& bidList = bidIt->second;
        auto& askList = askIt->second;

        while (!bidList.empty() && !askList.empty())
        {
            auto bid = bidList.front();
            auto ask = askList.front();

            Quantity qty = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

            bid->Fill(qty);
            ask->Fill(qty);

            trades.emplace_back(
                TradeInfo{ bid->GetOrderId(), bid->GetPrice(), qty },
                TradeInfo{ ask->GetOrderId(), ask->GetPrice(), qty }
            );


            if (bid->IsFilled()) {
                orders_.erase(bid->GetOrderId());
                bidList.pop_front();
            }
            if (ask->IsFilled()) {
                orders_.erase(ask->GetOrderId());
                askList.pop_front();
            }
        }

        if (bidList.empty()) bids_.erase(bidIt);
        if (askList.empty()) asks_.erase(askIt);
    }

    return trades;
}

// Public wrapper for matching: acquires lock then calls unlocked helper
Traders Orderbook::MatchOrders()
{
    std::scoped_lock lock(bookMutex_);
    return MatchOrdersUnlocked();
}

// ---------------- Add Order ----------------

Traders Orderbook::AddOrder(OrderPointer order)
{
    // Validate FOK (Fill-Or-Kill) before touching the book.
    if (order->GetOrderType() == OrderType::FillOrKill)
    {
        if (!CanFullyFill(order->GetSide(), order->GetPrice(), order->GetInitialQuantity()))
            return {};
    }

    std::scoped_lock lock(bookMutex_);

    if (orders_.count(order->GetOrderId()))
        return {};

    // Fill-and-kill must be immediately matchable (example semantics)
    if (order->GetOrderType() == OrderType::FillAndKill &&
        ! ( (order->GetSide() == Side::Buy && !asks_.empty() && order->GetPrice() >= asks_.begin()->first)
         || (order->GetSide() == Side::Sell && !bids_.empty() && bids_.begin()->first >= order->GetPrice()) )
    ) {
        return {};
    }

    OrderPointers::iterator it;

    if (order->GetSide() == Side::Buy)
    {
        auto& list = bids_[order->GetPrice()];
        list.push_back(order);
        it = std::prev(list.end());
    }
    else
    {
        auto& list = asks_[order->GetPrice()];
        list.push_back(order);
        it = std::prev(list.end());
    }

    orders_.insert({ order->GetOrderId(), { order, it } });

    
    return MatchOrdersUnlocked();
}


void Orderbook::CancelOrder(OrderId id)
{
    std::scoped_lock lock(bookMutex_);

    // If not present, noop
    if (!orders_.contains(id)) return;

    CancelOrderInternal(id);
}



void Orderbook::CancelOrderInternal(OrderId orderId)
{
    
    if (!orders_.contains(orderId))
        return;

    const auto entry = orders_.at(orderId);
    OrderPointer order = entry.order_;
    auto iter = entry.location_;
    orders_.erase(orderId);

    Price p = order->GetPrice();

    if (order->GetSide() == Side::Sell)
    {
        auto it = asks_.find(p);
        if (it != asks_.end()) {
            auto& lvl = it->second;
            lvl.erase(iter);
            if (lvl.empty()) asks_.erase(it);
        }
    }
    else
    {
        auto it = bids_.find(p);
        if (it != bids_.end()) {
            auto& lvl = it->second;
            lvl.erase(iter);
            if (lvl.empty()) bids_.erase(it);
        }
    }


}

void Orderbook::CancelOrders(const OrderIds& orderIds)
{
    std::scoped_lock lock(bookMutex_);

    for (const auto& id : orderIds)
        CancelOrderInternal(id);
}



Traders Orderbook::MatchOrder(const OrderModify& req)
{
    std::scoped_lock lock(bookMutex_);

    if (!orders_.contains(req.GetOrderId()))
        return {};

    auto [oldOrder, oldIter] = orders_[req.GetOrderId()];


    {
        Price p = oldOrder->GetPrice();
        if (oldOrder->GetSide() == Side::Sell) {
            auto it = asks_.find(p);
            if (it != asks_.end()) {
                it->second.erase(oldIter);
                if (it->second.empty()) asks_.erase(it);
            }
        } else {
            auto it = bids_.find(p);
            if (it != bids_.end()) {
                it->second.erase(oldIter);
                if (it->second.empty()) bids_.erase(it);
            }
        }
        orders_.erase(req.GetOrderId());
    }

    
    OrderPointer updated = std::make_shared<Order>(
        oldOrder->GetOrderType(),
        req.GetOrderId(),
        req.GetSide(),
        req.GetPrice(),
        req.GetQuantity()
    );

    OrderPointers::iterator it;
    if (updated->GetSide() == Side::Buy) {
        auto& list = bids_[updated->GetPrice()];
        list.push_back(updated);
        it = std::prev(list.end());
    } else {
        auto& list = asks_[updated->GetPrice()];
        list.push_back(updated);
        it = std::prev(list.end());
    }

    orders_.insert({ updated->GetOrderId(), { updated, it } });

    // Run matching
    return MatchOrdersUnlocked();
}


OrderbookLevelInfos Orderbook::GetOrderInfos() const
{
    std::scoped_lock lock(bookMutex_);

    levelInfos bidInfos, askInfos;

    bidInfos.reserve(bids_.size());
    askInfos.reserve(asks_.size());

    auto CreateLevel = [&](Price price, const OrderPointers& list)
    {
        Quantity total = std::accumulate(
            list.begin(),
            list.end(),
            Quantity(0),
            [](Quantity acc, const OrderPointer& o) {
                return acc + o->GetRemainingQuantity();
            }
        );
        return levelInfo{ price, total };
    };

    for (const auto& [price, orders] : bids_)
        bidInfos.push_back(CreateLevel(price, orders));

    for (const auto& [price, orders] : asks_)
        askInfos.push_back(CreateLevel(price, orders));

    return OrderbookLevelInfos(bidInfos, askInfos);
}


void Orderbook::PruneGoodForDayOrders()
{
    using namespace std::chrono;

    const auto marketClose = hours(16);  

    while (true)
    {
        const auto now = system_clock::now();
        std::time_t now_c = system_clock::to_time_t(now);
        std::tm now_parts{};
#if defined(_WIN32)
        localtime_s(&now_parts, &now_c);
#else
        localtime_r(&now_c, &now_parts);
#endif

        
        if (now_parts.tm_hour >= marketClose.count())
            now_parts.tm_mday += 1;

        
        now_parts.tm_hour = marketClose.count();
        now_parts.tm_min = 0;
        now_parts.tm_sec = 0;

        
        auto nextPruneTime = system_clock::from_time_t(std::mktime(&now_parts));

        
        auto waitDuration = nextPruneTime - now + milliseconds(100);

        {
            // Use shutdown mutex for condition_variable waiting
            std::unique_lock<std::mutex> waitLock(shutdownMutex_);

            
            if (shutdown_.load(std::memory_order_acquire) ||
                shutdownConditionVariable_.wait_for(waitLock, waitDuration) == std::cv_status::no_timeout)
            {
                return;
            }
        }

    
        OrderIds toCancel;
        {
            std::scoped_lock bookLock(bookMutex_);

            for (const auto& [id, entry] : orders_)
            {
                const OrderPointer& o = entry.order_;
                if (o->IsGoodForDay()) {
                    toCancel.push_back(id);
                }
            }
        }

        if (!toCancel.empty())
            CancelOrders(toCancel);
    }
}

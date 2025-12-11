#ifndef ORDERBOOKLEVELINFOS_H
#define ORDERBOOKLEVELINFOS_H

#include "Order.h"
#include <vector>

struct levelInfo {
    Price price;
    Quantity quantity;
};
using levelInfos = std::vector<levelInfo>;

class OrderbookLevelInfos {
public:
    OrderbookLevelInfos(const levelInfos& bids, const levelInfos& asks)
        : bids_(bids), asks_(asks) {}

    const levelInfos& GetBids() const { return bids_; }
    const levelInfos& GetAsks() const { return asks_; }

private:
    levelInfos bids_;
    levelInfos asks_;
};

#endif

#ifndef TRADE_H
#define TRADE_H
#include <vector>


#include "Types.h"

struct TradeInfo {
    OrderId orderId_;
    Price price_;
    Quantity quantity_;
};

class Trade {
public:
    Trade(const TradeInfo& b, const TradeInfo& a)
        : bidTrade(b), askTrade(a) {}

    // Public getters
    const TradeInfo& GetBidTrade() const { return bidTrade; }
    const TradeInfo& GetAskTrade() const { return askTrade; }

private:
    TradeInfo bidTrade;
    TradeInfo askTrade;
};

using Traders = std::vector<Trade>;

#endif

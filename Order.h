#pragma once

#include "Types.h"
#include "Constants.h"

#include <stdexcept>
#include <cstdint>
#include <memory>

enum class OrderType {
    GoodTillCancel,
    GoodForDay,
    FillAndKill,
    FillOrKill,
    Market
};

enum class Side { Buy, Sell };

class Order {
public:
    // Full constructor
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity);

    // Convenience constructor for market order (no price)
    Order(OrderId orderId, Side side, Quantity quantity)
        : Order(OrderType::Market, orderId, side, Constants::InvalidPrice, quantity) {}

    // Getters
    OrderId GetOrderId() const;
    Side GetSide() const;
    Price GetPrice() const;
    OrderType GetOrderType() const;
    Quantity GetRemainingQuantity() const;
    Quantity GetInitialQuantity() const;
    bool IsFilled() const;

    // Mutators
    void Fill(Quantity qty);

    // Helpers
    bool IsFillOrKill() const { return orderType_ == OrderType::FillOrKill; }
    bool IsGoodForDay() const { return orderType_ == OrderType::GoodForDay; }

private:
    OrderType orderType_;
    OrderId  orderId_;
    Side     side_;
    Price    price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
};

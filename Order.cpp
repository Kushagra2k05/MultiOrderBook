#include "Order.h"


Order::Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
    : orderType_(orderType),
      orderId_(orderId),
      side_(side),
      price_(price),
      initialQuantity_(quantity),
      remainingQuantity_(quantity)
{}


OrderId Order::GetOrderId() const { return orderId_; }
Side    Order::GetSide() const { return side_; }
Price   Order::GetPrice() const { return price_; }
OrderType Order::GetOrderType() const { return orderType_; }
Quantity Order::GetRemainingQuantity() const { return remainingQuantity_; }
Quantity Order::GetInitialQuantity() const { return initialQuantity_; }
bool    Order::IsFilled() const { return remainingQuantity_ == 0; }


void Order::Fill(Quantity qty)
{
    if (qty > remainingQuantity_)
        throw std::logic_error("Fill exceeds remaining quantity!");
    remainingQuantity_ -= qty;
}

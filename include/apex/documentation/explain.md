***Order.hpp***

- In a stock exchange, think about Nasdaq. They get thousands of people every millisecond sending messages like "I want to buy 250 shares of Apple at $100", "I want to sell 100 shares of Tesla at $5k"

That is the order. The request sent to the exchange. 

- every order must have an ID, timestamp, price, quantity, side(buy/sell)
- filled quantity: Suppose you want to buy 500 shares at $100 but the exchange only has one seller selling 100 shares. So it will match for the 100 but the order is still remaining. We want the quantity and filled quantity to know the original order instead of just subtracting the 100 from 500. 

order: q: 500, filled_q: 100

- orders can have different types:

1. Limit order: Ex - Buy 100 shares at $100 means never pay more than that, if no one is selling at that price, wait 
2. Market order: You want shares right away, don't care about prices. The exchange will start buying from the cheapest sellers
3. IOC: "buy 100 shares at $100 NOW" if only 40 shares exist, buy them and cancel the rest
4. Fork or fill: "buy 100 shares at $100" - if all 100 exist, buy otherwise buy nothing

- status: new/partiallyFilled/Filled/Cancelled/Rejected


***event.hpp***

Anytime something happens in the engine, it emits an event. For ex: 

- OrderAdded,OrderCancelled,OrderModified, OrderPartiallyFilled,OrderFilled,TradeExecuted
- every event will have its own type, timestamp, order_id, quantity and side
- events also have trade_ids because its actually the exchange that executes trades from orders
- for example one order could execute three trades if theres a remaining amount and it gets matched again


***Price_level***

- all the orders that share the same price will be grouped together. 
- it should also enforce things like: who get filled first? one order might be partially filled, remove an order in 0(1), know total volume at this price
- in c++, we have hpp and cpp where .hpp is the declaration and .cpp is the implementation

***price_level.hpp***

- this is the declaration file for price level
- the orderbook can add an order, cancel, see the price, quantity etc which is why they are public
- however, having public access to the data itself would allow changing the data itself like level.price = 100; which is not good. So we can see the data and call functions but allowed to modify the data itself
- the list of orders and the ordermap should be private itself





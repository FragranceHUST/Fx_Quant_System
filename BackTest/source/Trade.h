#pragma once

class Trade
{
    enum DirectionType {
        Bid,
        Sell
    };
private:
    UUID uuid;
    std::string instrument;
    
    double entryPrice;
    double exitPrice;

    double stoplossPrice;
    double profitTargetPrice;

    double commissionrate;
    double commission;

    double profit;

    int quantity;

    DATE entryTime;
    DATE exitTime;

    DirectionType direction;

public:
    Trade(/* args */);
    ~Trade();
};

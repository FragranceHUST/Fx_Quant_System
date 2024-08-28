#pragma once
#include "uuid/uuid.h"

class Trade
{
public:
    enum class DirectionType
    {
        Ask,
        Bid
    };
private:
    static uint64_t start_uuid;
    // 唯一交易ID
    typedef uint64_t UUID;
    UUID uuid;
    // 交易品种
    std::string m_instrument;
    
    // 入场价
    double m_entryPrice;
    // 出场价
    double m_exitPrice;

    // 止损价
    double m_stoplossPrice;
    // 止盈价
    double m_profitTargetPrice;

    // 佣金比例
    double m_commissionrate;
    // 佣金
    double m_commission;
    // 收益
    double m_profit;
    // 仓位/量
    int m_quantity;
    // 入场时间
    DATE m_entryTime;
    // 出场时间
    DATE m_exitTime;
    // 买卖方向
    DirectionType m_direction;

    // 是否实盘交易
    bool m_real;
    bool m_closed;

public:
    Trade(const std::string instrument,
        DATE entryTime, 
        const double entry, 
        const double profit_target, 
        const double stopless, 
        const int quant, 
        DirectionType direction, 
        bool real);
    ~Trade();

    double getEntryPrice();
    double getExitPrice();
    double getStoplossPrice();
    double getProfitTargetPrice();
    double getHoldingTime();
    double getProfit();
    DirectionType getDirection();
    bool isClosed();
    void close(bool targetclose, DATE closedtime);
};

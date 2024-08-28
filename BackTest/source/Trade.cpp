#include "stdafx.h"
#include "Trade.h"

uint64_t Trade::start_uuid = 0;

Trade::Trade(const std::string instrument,
    DATE entryTime, 
    const double entry, 
    const double profit_target, 
    const double stopless, 
    const int quant, 
    DirectionType direction, 
    bool real) : uuid(start_uuid++), m_instrument(instrument), m_entryTime(entryTime), m_entryPrice(entry), 
    m_profitTargetPrice(profit_target), m_stoplossPrice(stopless), m_quantity(quant), m_direction(direction), m_real(real), m_closed(false)
{

}

Trade::~Trade()
{
    if (m_real && !m_closed)
    {
        // todo
    }
}

void Trade::close(bool targetclose, DATE closedtime)
{
    if (m_closed)
    {
        return;
    }

    if (m_real)
    {
        // 处理实盘逻辑
        m_exitTime = hptools::date::DateNow();
    } else {
        m_exitTime = closedtime;
        m_exitPrice = targetclose ? m_profitTargetPrice : m_stoplossPrice;
        m_profit = m_exitPrice - m_entryPrice;
    }
    
    m_closed = true;
}

double Trade::getEntryPrice()
{
    return m_entryPrice;
}

double Trade::getExitPrice()
{
    return m_exitPrice;
}

double Trade::getStoplossPrice()
{
    return m_stoplossPrice;
}

double Trade::getProfitTargetPrice()
{
    return m_profitTargetPrice;
}

double Trade::getHoldingTime()
{
    return m_exitTime - m_entryTime;
}

double Trade::getProfit()
{
    return m_exitPrice - m_entryPrice;
}

Trade::DirectionType Trade::getDirection()
{
    return m_direction;
}

bool Trade::isClosed()
{
    return m_closed;
}
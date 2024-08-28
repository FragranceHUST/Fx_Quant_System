#include "stdafx.h"

#include "CostFunction.h"
#include "Trade.h"
#include "PriceData/Period.h"
#include "DataRepository.h"
#include "Strategy.h"
#include "ThreadSafeAddRefImpl.h"

Strategy::Strategy(const std::string instrument, const double p1, const double p2, const double stoploss) : 
mInstrument(instrument), param1(p1), param2(p2), default_stoploss(stoploss), cf(std::make_unique<CostFunction>())
{

}

Strategy::~Strategy()
{
    
}

double calcRange(std::queue<O2G2Ptr<Period>>& q)
{
    double maxval = q.front()->getAsk()->getHigh(), minval = q.front()->getAsk()->getLow();
    for (int i = 0; i < 12; i++)
    {
        const auto& x = q.front();
        maxval = x->getAsk()->getHigh() > maxval ? x->getAsk()->getHigh() : maxval;
        minval = x->getAsk()->getLow() < minval ? x->getAsk()->getLow() : minval;
        q.pop();
        q.push(x);
    }
    q.pop();    // 计算完成后去掉第一个数据
    return maxval - minval;
}

bool calcEntryExitCondition(const O2G2Ptr<Period> bin1, const O2G2Ptr<Period> bin2, double limit, Trade::DirectionType direction)
{
    if (direction == Trade::DirectionType::Ask)
    {
        return bin1->getAsk()->getLow() <= limit && bin2->getAsk()->getClose() > limit;
    }
    else
    {
        return bin1->getBid()->getHigh() >= limit && bin2->getBid()->getClose() < limit;
    }
}

void Strategy::runSimulation()
{
    DataRepository* datarepo = DataRepository::getInstance();
    // datarepo->setInstrument(mInstrument);
    // datarepo->loadFile();

    // R -- range of last 48h, R1 -- range of last 72h
    double R = 0, R1 = 0;
    double entry_limit = 0;
    std::queue<O2G2Ptr<Period>> q;

    int daily_index = 2, hour_index = 0, min_index = 0;
    const auto& binsD1 = datarepo->binsDaily;
    const auto& binsH4 = datarepo->binsH4;
    const auto& binsM1 = datarepo->binsM1;
    // 先用前48小时的数据计算R值，然后不断迭代
    while (q.size() < 12)
    {
        q.push(binsH4[hour_index++]);
    }

    // 从第48小时的数据开始进行回测
    // for (const auto bin_day : datarepo->binsDaily)
    for (; daily_index < binsD1.size(); daily_index++)
    {
        DATE day_time = binsD1[daily_index]->getTime();

        for (; hour_index < binsH4.size(); hour_index++)
        {
            DATE hour_time = binsH4[hour_index]->getTime();
            if (hour_time - day_time >= 1.0)
            {
                // 当天的每个4小时bin已经遍历完
                break;
            }

            // 计算过去48小时的range值
            R = calcRange(q);
            q.push(binsH4[hour_index]);

            // 计算挂单价格和目标止盈
            double long_limit = binsH4[hour_index]->getAsk()->getOpen() - R * param1;
            double profit_target = R * param2;

            for (; min_index < binsM1.size(); min_index++)
            {
                // 回测当天的每一分钟
                double min_time = binsM1[min_index]->getTime();
                if (min_time < hour_time)
                {
                    continue;
                }
                else if (min_time - hour_time >= 1.0 / 6)
                {
                    break;
                }

                
                if (calcEntryExitCondition(binsM1[min_index], binsM1[min_index - 1], long_limit, Trade::DirectionType::Ask))
                {
                    DATE entryTime = min_time; // hptools::date::DateNow();
                    mTrades.emplace_back(std::make_unique<Trade>(
                        mInstrument, entryTime, long_limit, long_limit + profit_target, long_limit - 0.001 * default_stoploss, 1, Trade::DirectionType::Ask, false
                    ));
                }
                for (const auto& trade : mTrades)
                {   
                    if (trade->isClosed())
                    {
                        continue;
                    }

                    Trade::DirectionType closedirection = trade->getDirection() == Trade::DirectionType::Ask ? Trade::DirectionType::Bid : Trade::DirectionType::Ask;
                    bool targetclose = calcEntryExitCondition(binsM1[min_index], binsM1[min_index - 1], trade->getProfitTargetPrice(), closedirection);
                    bool stoplessclose = calcEntryExitCondition(binsM1[min_index], binsM1[min_index - 1], trade->getStoplossPrice(), closedirection);
                    if (targetclose || stoplessclose)
                    {
                        trade->close(targetclose, min_time); // Realtime中需要在close里用forexAPI关掉订单
                    }
                    // mClosedTrades.emplace_back(trade);
                    // mOpenedTrades.erase()
                }
            }
        }
    }

    // 计算所有的constFunction
    calcCostFunction(binsM1.back()->getBid()->getClose());
}

void Strategy::runRealtime()
{

}

void Strategy::calcCostFunction(double curPrice)
{
    cf->calc_winrate(mTrades);
    cf->calc_absoluterror(mTrades);
    cf->calc_avgtime(mTrades);
    cf->calc_maxconloss(mTrades);
    cf->calc_maxconprofit(mTrades);
    cf->calc_maxdrawdown(mTrades);
    cf->calc_meansqaurerror(mTrades);
    cf->calc_realizedPnL(mTrades);
    cf->calc_sharpe(mTrades);
    cf->calc_tradecount(mTrades);
    cf->calc_unreadlizedPnL(mTrades, curPrice);
}

void Strategy::outputCostFunctoin(const std::string& outputfile)
{
    
}
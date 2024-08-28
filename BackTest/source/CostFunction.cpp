#include "stdafx.h"
#include "Trade.h"
#include "CostFunction.h"

CostFunction::CostFunction()
{
    winrate = 0;
    sharpe_ratio = 0;
    max_profitloss_ratio = 0;
    max_drawdown = 0;
    max_consecutive_loss = 0;
    max_consecutive_profit = 0;
    trade_cnt = 0;
    closed_trade_cnt = 0;
    avg_holding_time = 0;
    mean_square_error = std::numeric_limits<int>::max();
    absolute_error = std::numeric_limits<int>::max();
    total_commision = std::numeric_limits<int>::max();
    total_slippage = std::numeric_limits<int>::max();
    realized_PnL = 0;
    unrealized_PnL = 0;
}

CostFunction::~CostFunction()
{
}

void CostFunction::calc_winrate(Trades& mTrades)
{
    int profit_cnt = 0;
    for (const auto& trade : mTrades)
    {
        if (trade->isClosed())
        {
            closed_trade_cnt++;
            profit_cnt += trade->getExitPrice() - trade->getEntryPrice() > 0 ? 1 : 0;
        }
    }

    winrate = profit_cnt * 1.0 / closed_trade_cnt;
}

void CostFunction::calc_sharpe(Trades& mTrades, double rf_rate)
{
    double totalReturns = 0.0;
    double totalSquaredReturns = 0.0;
    for (const auto& trade : mTrades)
    {
        if (trade->isClosed())
        {
            double returnRate = trade->getProfit() / trade->getEntryPrice();
            totalReturns += returnRate;
            totalSquaredReturns += returnRate * returnRate;
        }
    }

    double avgReturn = totalReturns / closed_trade_cnt;
    double variance = (totalSquaredReturns / closed_trade_cnt) - (avgReturn * avgReturn);
    double standardDeviation = std::sqrt(variance);

    sharpe_ratio = (avgReturn - rf_rate) / standardDeviation;
}

void CostFunction::calc_maxPLratio(Trades& mTrades)
{
    double maxProfit = 0.0, maxLoss = 0.0;
    for (const auto& trade : mTrades)
    {
        if (trade->isClosed())
        {
            maxProfit = std::max(maxProfit, trade->getProfit());
            maxLoss = std::min(maxLoss, trade->getProfit());
        }
    }

    if (maxLoss == 0)
    {
        max_profitloss_ratio = std::numeric_limits<double>::infinity();
        return;
    }

    max_profitloss_ratio = std::abs(maxProfit / maxLoss);
}

void CostFunction::calc_maxdrawdown(Trades& mTrades)
{
    double maxdrowdown = 0.0;
    double cumulativeProfit = 0.0;
    double maxProfitSoFar = 0.0;
    for (const auto& trade : mTrades)
    {
        if (trade->isClosed())
        {
            cumulativeProfit += trade->getProfit();
            maxProfitSoFar = std::max(maxProfitSoFar, cumulativeProfit);
            double drowdown = maxProfitSoFar - cumulativeProfit;
            maxdrowdown = std::max(maxdrowdown, drowdown);
        }
    }

    max_drawdown = maxdrowdown;
}

void CostFunction::calc_maxconprofit(Trades& mTrades)
{
    int cnt = 0;
    for (const auto& trade : mTrades)
    {
        if (trade->getExitPrice() - trade->getEntryPrice() > 0)
        {
            max_consecutive_profit = std::max(max_consecutive_profit, ++cnt);
        }
        else
        {
            cnt = 0;
        }
    }
}

void CostFunction::calc_maxconloss(Trades& mTrades)
{
    int cnt = 0;
    for (const auto& trade : mTrades)
    {
        if (trade->getExitPrice() - trade->getEntryPrice() < 0)
        {
            max_consecutive_loss = std::max(max_consecutive_loss, ++cnt);
        }
        else
        {
            cnt = 0;
        }
    }
}

void CostFunction::calc_meansqaurerror(Trades& mTrades)
{
    
}

void CostFunction::calc_absoluterror(Trades& mTrades)
{
    
}

void CostFunction::calc_tradecount(Trades& mTrades)
{
    trade_cnt = mTrades.size();
}

void CostFunction::calc_avgtime(Trades& mTrades)
{
    double total_time = 0;
    for (const auto& trade: mTrades)
    {
        if (trade->isClosed())
        {
            total_time += trade->getHoldingTime();
        }
    }
    avg_holding_time = total_time / closed_trade_cnt;
}

void CostFunction::calc_realizedPnL(Trades& mTrades)
{
    for (const auto& trade : mTrades)
    {
        if (trade->isClosed())
        {
            realized_PnL += trade->getExitPrice() - trade->getEntryPrice();
        }
    }
}

void CostFunction::calc_unreadlizedPnL(Trades& mTrades, double curPrice)
{
    for (const auto& trade : mTrades)
    {
        if (!trade->isClosed())
        {
            unrealized_PnL += curPrice - trade->getEntryPrice();
        }
    }
}

double CostFunction::get_winrate()
{
    return winrate;
}

double CostFunction::get_sharpe()
{
    return sharpe_ratio;
}

double CostFunction::get_maxPLratio()
{
    return max_profitloss_ratio;
}

double CostFunction::get_maxdrawdown()
{
    return max_drawdown;
}

double CostFunction::get_meansquarerror()
{
    return mean_square_error;
}

double CostFunction::get_absoluterror()
{
    return absolute_error;
}

double CostFunction::get_avgtime()
{
    return avg_holding_time;
}

double CostFunction::get_realizedPnL()
{
    return realized_PnL;
}

double CostFunction::get_unrealizedPnL()
{
    return unrealized_PnL;
}

int CostFunction::get_maxconprofit()
{
    return max_consecutive_profit;
}

int CostFunction::get_maxconloss()
{
    return max_consecutive_loss;
}

int CostFunction::get_tradecount()
{
    return trade_cnt;
}
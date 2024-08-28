#pragma once

class CostFunction;

class Strategy
{
private:
    int type;   // 策略类型(CTA/套利/...)
    std::string mInstrument;    // 策略品种

    double param1;
    double param2;
    double param3;
    double default_stoploss;
    Trade::DirectionType direction;

    double total_quantity;  // 总资金量（标准手数）
    double pre_quantity;    // 每单开仓手数
    double rf_rate;         // 无风险利率

    typedef std::vector<std::unique_ptr<Trade>> Trades;
    Trades mTrades;
    // Trades mClosedTrades;

    std::unique_ptr<CostFunction> cf;
private:
    Strategy() = delete;    // 禁用默认构造函数
    Strategy(const Strategy& other) = delete;   // 禁用复制构造函数

private:
    // double calcRange(std::queue<O2G2Ptr<Period>>& q);   // 计算48h的range
    // bool calcEntryExitCondition(const O2G2Ptr<Period> bin1, const O2G2Ptr<Period> bin2, double limit, DirectionType direction); // 计算当前是否满足入场/出场条件
public:
    Strategy(const std::string instrument, const double p1, const double p2, const double stopless = 60);
    ~Strategy();

    void runSimulation();       // 回测
    void runRealtime();         // 实时运行
    void calcCostFunction(double curPrice);    
    void outputCostFunctoin(const std::string& outputfile);
};
#pragma once

class Strategy
{
private:
    /* data */
public:
    Strategy(/* args */);
    ~Strategy();

    void runSimulation();
    void runRealtime();

    // typedef std::queue<Trade>
    CostFunction cf;
    void calcCostFunction();
};

Strategy::Strategy(/* args */)
{
}

Strategy::~Strategy()
{
}

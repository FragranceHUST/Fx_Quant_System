#include "stdafx.h"

#include "DataRepository.h"
#include "Trade.h"
#include "Strategy.h"
#include "CostFunction.h"
#include "helpers.h"
#include "PriceData/Period.h"

int main(int argc, char* argv[])
{
    // multithread
    std::string instrument("USD/JPY");
    const std::string backtest_output = helpers::getBackTestFilename(instrument, hptools::date::DateNow());

    // initialize datarepository
    DataRepository* datarepo = DataRepository::getInstance();
    datarepo->setInstrument(instrument);
    datarepo->loadFile();

    // todo: 1.thread pool 2.GUI
    std::shared_ptr<Strategy> s = std::make_shared<Strategy>(instrument, 0.12, 0.32);
    s->runSimulation();
    s->outputCostFunctoin(backtest_output);
    return 0;
}
/*

*/
#include "stdafx.h"

#include "LoginParams.h"
#include "SampleParams.h"
#include "SessionStatusListener.h"
#include "CommunicatorStatusListener.h"
#include "ResponseListener.h"
#include "PriceUpdateController.h"
#include "PeriodCollectionUpdateObserver.h"
#include "CommonSources.h"
#include "LocalFormat.h"
#include "helpers.h"

#include "PriceData/PriceDataInterfaces.h"
#include "PriceData/PeriodCollection.h"

/** Forward declaration. */
void printHelp(std::string &);
bool checkObligatoryParams(LoginParams *, SampleParams *);
void printSampleParams(std::string &, LoginParams *, SampleParams *);

int getInstrumentPrecision(pricehistorymgr::IPriceHistoryCommunicator *communicator, const char *instrument);
void writePrices(pricehistorymgr::IPriceHistoryCommunicator *communicator, pricehistorymgr::IPriceHistoryCommunicatorResponse *response,
    const char *instrument, const char *outputFile, bool utcMode);
bool getHistoryPrices(pricehistorymgr::IPriceHistoryCommunicator *communicator, const char *instrument,
    DATE from, DATE to, int quotesCount, ResponseListener *responseListener, bool utcMode);
void processHistoricalPrices(pricehistorymgr::IPriceHistoryCommunicator *communicator, pricehistorymgr::IPriceHistoryCommunicatorResponse *response,
                             PeriodCollection *periods, const char * instrument);
bool getLivePrices(pricehistorymgr::IPriceHistoryCommunicator *communicator, const char *instrument,
                   const char *timeframe, DATE from, DATE fo, int quotesCount, IO2GSession *session, ResponseListener *responseListener);

// bool getHistoryPrices_D1(pricehistorymgr::IPriceHistoryCommunicator *communicator, const char *instrument,
//     DATE from, DATE to, int quotesCount, ResponseListener *responseListener, bool utcMode);
// bool getHistoryPrices_H4(pricehistorymgr::IPriceHistoryCommunicator *communicator, const char *instrument,
//     DATE from, DATE to, int quotesCount, ResponseListener *responseListener, bool utcMode);


int main(int argc, char* argv[]) {
    std::string procName = "GetLivePrices";
    if (argc == 1)
    {
        printHelp(procName);
        return -1;
    }

    bool bWasError = false;

    LoginParams loginParams(argc, argv);
    SampleParams sampleParams(argc, argv);

    printSampleParams(procName, &loginParams, &sampleParams);
    if (!checkObligatoryParams(&loginParams, &sampleParams))
    {
        return -1;
    }

    // create the ForexConnect trading session
    O2G2Ptr<IO2GSession> session = CO2GTransport::createSession();
    O2G2Ptr<SessionStatusListener> statusListener = new SessionStatusListener(session, true,
                                                                              loginParams.getSessionID(),
                                                                              loginParams.getPin());
    // subscribe IO2GSessionStatus interface implementation for the status events
    session->subscribeSessionStatus(statusListener);
    statusListener->reset();

    // create an instance of IPriceHistoryCommunicator
    pricehistorymgr::IError *error = NULL;
    O2G2Ptr<pricehistorymgr::IPriceHistoryCommunicator> communicator(
        pricehistorymgr::PriceHistoryCommunicatorFactory::createCommunicator(session, "History", &error));
    O2G2Ptr<pricehistorymgr::IError> autoError(error);

    if (!communicator)
    {
        std::cout << error->getMessage() << std::endl;
        return -1;
    }

    // log in to ForexConnect
    session->login(loginParams.getLogin(), loginParams.getPassword(),
        loginParams.getURL(), loginParams.getConnection());
    if (statusListener->waitEvents() && statusListener->isConnected())
    {
        /*todo:
        1.获取单个品种的历史价格 m1, H4, D1
        2.写入对应的csv文件中
        3.获取lastBarTime，把数据更新到三个文件中
        4.实时更新文件
        */
        O2G2Ptr<CommunicatorStatusListener> communicatorStatusListener(new CommunicatorStatusListener());
        communicator->addStatusListener(communicatorStatusListener);
        // wait until the communicator signals that it is ready
        if (communicator->isReady() ||
            communicatorStatusListener->waitEvents() && communicatorStatusListener->isReady())
        {
            // attach the instance of the class that implements the IPriceHistoryCommunicatorListener
            // interface to the communicator
            O2G2Ptr<ResponseListener> responseListener(new ResponseListener());
            communicator->addListener(responseListener);

            if (!getLivePrices(communicator, sampleParams.getInstrument(),
                sampleParams.getTimeframe(), sampleParams.getDateFrom(), sampleParams.getDateTo(),
                sampleParams.getQuotesCount(), session, responseListener))
            {
                bWasError = true;
            }

            // std::cout << "Done!" << std::endl;

            communicator->removeListener(responseListener);
        }

        communicator->removeStatusListener(communicatorStatusListener);
        statusListener->reset();

        session->logout();
        statusListener->waitEvents();
    }
    else
    {
        bWasError = true;
    }

    session->unsubscribeSessionStatus(statusListener);
    if (bWasError)
    {
        return -1;
    }
    
    return 0;
}


/** Request historical prices for the specified timeframe of the specified period,
    then show live prices

    @param communicator
        The price history communicator.
    @param instrument
        The instrument.
    @param timeframe
        The timeframe.
    @param from
        From-date.
    @param to
        To-date
    @param quotesCount
        The quotes count.
    @param session
        The session.
    @param responseListener
        The response listener.
 */
bool getLivePrices(pricehistorymgr::IPriceHistoryCommunicator *communicator, const char *instrument,
                   const char *timeframe, DATE from, DATE to, int quotesCount,
                   IO2GSession *session, ResponseListener *responseListener)
{
    if (!communicator->isReady())
    {
        std::cout << "History communicator is not ready." << std::endl;
        return false;
    }

    // create timeframe entity
    O2G2Ptr<pricehistorymgr::ITimeframeFactory> timeframeFactory = communicator->getTimeframeFactory();
    pricehistorymgr::IError *error = NULL;
    O2G2Ptr<IO2GTimeframe> timeframeObj = timeframeFactory->create(timeframe, &error);
    O2G2Ptr<pricehistorymgr::IError> autoError(error);
    if (!timeframeObj)
    {
        std::cout << "Timeframe '" << timeframe << "' is incorrect! " << std::endl;
        return false;
    }

    // check timeframe for ticks
    if (Tick == timeframeObj->getUnit())
    {
        std::cout << "Application works only for bars. Don't use tick as timeframe." << std::endl;
        return false;
    }

    // load Offers table and start ticks listening
    PriceUpdateController priceUpdateController(session, instrument);
    if (!priceUpdateController.wait())
        return false;

    // create period collection and update listener
    bool alive = true;
    int precision = getInstrumentPrecision(communicator, instrument);
    O2G2Ptr<PeriodCollection> periods(new PeriodCollection(instrument, timeframe, precision, alive, &priceUpdateController));
    PeriodCollectionUpdateObserver livePriceViewer(periods);

    // Get D1 and H4 history prices
    getHistoryPrices(communicator, instrument, from, to, quotesCount, responseListener, false);

    // create and send a history request
    O2G2Ptr<pricehistorymgr::IPriceHistoryCommunicatorRequest> request(
        communicator->createRequest(instrument, timeframeObj, from, to, quotesCount, &error));
    if (!request)
    {
        std::cout << error->getMessage() << std::endl;
        return false;
    }
    
    responseListener->setRequest(request);
    if (!communicator->sendRequest(request, &error))
    {
        std::cout << error->getMessage() << std::endl;
        return false;
    }

    // wait results
    responseListener->wait();

    O2G2Ptr<pricehistorymgr::IPriceHistoryCommunicatorResponse> response(responseListener->getResponse());
    if (response)
    {
        std::string outputFile = helpers::getOutputFilename(instrument, timeframe);
        writePrices(communicator, response, instrument, outputFile.c_str(), false);
    }
    O2G2Ptr<IO2GMarketDataSnapshotResponseReader> reader = communicator->createResponseReader(response, &error);
    if (error)
    {
        std::cout << "Can not create Response Reader: " << error->getMessage() << std::endl;
        return false;
    }

    processHistoricalPrices(communicator, response, periods, instrument);

    // // finally notify the collection that all bars are added, so it can
    // // add all ticks collected while the request was being executed
    // // and start update the data by forthcoming ticks
    // periods->finish(reader->getLastBarTime(), reader->getLastBarVolume());

    // continue update the data until cancelled by a user
    std::cout << std::endl << "Done" << std::endl;
    std::cin.get();

    return true;
}

bool getHistoryPrices(pricehistorymgr::IPriceHistoryCommunicator *communicator, const char *instrument,
    DATE from, DATE to, int quotesCount, ResponseListener *responseListener, bool utcMode)
{
    const char* timeframes[] = {"D1", "H4"};
    // todo: get starttime from outputfile if it exist
    
    // create timeframe entity
    for (int i = 0; i < 2; i++) {
        O2G2Ptr<pricehistorymgr::ITimeframeFactory> timeframeFactory = communicator->getTimeframeFactory();
        pricehistorymgr::IError *error = NULL;
        O2G2Ptr<IO2GTimeframe> timeframeObj = timeframeFactory->create(timeframes[i], &error);
        O2G2Ptr<pricehistorymgr::IError> autoError(error);

        // create and send a history request
        O2G2Ptr<pricehistorymgr::IPriceHistoryCommunicatorRequest> request =
            communicator->createRequest(instrument, timeframeObj, from, to, quotesCount, &error);
        if (!request)
        {
            std::cout << error->getMessage() << std::endl;
            return false;
        }

        responseListener->setRequest(request);
        if (!communicator->sendRequest(request, &error))
        {
            std::cout << error->getMessage() << std::endl;
            return false;
        }

        // wait results
        responseListener->wait();

        // print results if any
        O2G2Ptr<pricehistorymgr::IPriceHistoryCommunicatorResponse> response = responseListener->getResponse();
        if (response) {
            std::string outputFile = helpers::getOutputFilename(instrument, timeframes[i]);
            writePrices(communicator, response, instrument, outputFile.c_str(), utcMode);
        }
    }
    
    return true;
}

/** Gets precision of a specified instrument.

    @param communicator
        The price history communicator.
    @param instrument
        The instrument.
    @return
        The precision.
 */
int getInstrumentPrecision(pricehistorymgr::IPriceHistoryCommunicator *communicator, const char *instrument)
{
    int precision = 6;

    O2G2Ptr<quotesmgr::IQuotesManager> quotesManager = communicator->getQuotesManager();
    quotesmgr::IError *error = NULL;
    O2G2Ptr<quotesmgr::IInstruments> instruments = quotesManager->getInstruments(&error);
    O2G2Ptr<quotesmgr::IError> autoError(error);
    if (instruments)
    {
        O2G2Ptr<quotesmgr::IInstrument> instr = instruments->find(instrument);
        if (instr)
            precision = instr->getPrecision();
    }

    return precision;
}

/** Print history data from response.

    @param communicator
        The price history communicator.
    @param response
        The response. Cannot be null.
 */
void processHistoricalPrices(pricehistorymgr::IPriceHistoryCommunicator *communicator, pricehistorymgr::IPriceHistoryCommunicatorResponse *response, PeriodCollection *periods, const char *instrument)
{
    // use IO2GMarketDataSnapshotResponseReader to extract price data from the response object 
    pricehistorymgr::IError *error = NULL;
    O2G2Ptr<IO2GMarketDataSnapshotResponseReader> reader = communicator->createResponseReader(response, &error);
    O2G2Ptr<pricehistorymgr::IError> autoError(error);
    if (reader)
    {
        char time[20];
        for (int i = 0; i < reader->size(); ++i)
        {
            DATE dt = reader->getDate(i);
            formatDate(dt, time);

            periods->add(reader->getDate(i), reader->getBidOpen(i), reader->getBidHigh(i), reader->getBidLow(i), reader->getBidClose(i),
                        reader->getAskOpen(i), reader->getAskHigh(i), reader->getAskLow(i), reader->getAskClose(i), reader->getVolume(i));
        }
    }
}

/** Writes history data from response.

    @param communicator
        The price history communicator.
    @param response
        The response. Cannot be null.
    @param instrument
        The instrument.
    @param outputFile
        The output file name.
 */
void writePrices(pricehistorymgr::IPriceHistoryCommunicator *communicator, pricehistorymgr::IPriceHistoryCommunicatorResponse *response,
                 const char *instrument, const char *outputFile, bool utcMode)
{
    auto start = std::chrono::high_resolution_clock::now();
    std::ifstream file(outputFile);
    bool titleflag = file.peek() == std::ifstream::traits_type::eof();
    file.close();

    std::fstream fs;
    fs.open(outputFile, std::fstream::out | std::fstream::app);
    if (!fs.is_open())
    {
        std::cout << "Could not open the output file." << std::endl;
        return;
    }

    LocalFormat localFormat;
    const char *separator = localFormat.getCsvSeparator();
    int precision = getInstrumentPrecision(communicator, instrument);

    // use IO2GMarketDataSnapshotResponseReader to extract price data from the response object 
    pricehistorymgr::IError *error = NULL;
    O2G2Ptr<IO2GMarketDataSnapshotResponseReader> reader = communicator->createResponseReader(response, &error);
    O2G2Ptr<pricehistorymgr::IError> autoError(error);
    if (reader)
    {
        if (reader->isBar() && titleflag)
        {
            fs << "DateTime" << separator
               << "BidOpen"  << separator << "BidHigh"  << separator
               << "BidLow"   << separator << "BidClose" << separator
               << "AskOpen"  << separator << "AskHigh"  << separator
               << "AskLow"   << separator << "AskClose" << separator
               << "Volume"   << separator << std::endl;
        }
        else if (!reader->isBar() && titleflag)
        {
            fs << "DateTime" << separator
               << "Bid"      << separator << "Ask" << separator << std::endl;
        }

        for (int i = 0; i < reader->size(); ++i)
        {
            DATE dt = reader->getDate(i);

            if (!utcMode)
                dt = hptools::date::DateConvertTz(dt, hptools::date::UTC, hptools::date::EST);
            
            std::string time = localFormat.formatDate(dt);            
            if (reader->isBar())
            {
                fs << time << separator
                   << localFormat.formatDouble(reader->getBidOpen(i), precision)  << separator
                   << localFormat.formatDouble(reader->getBidHigh(i), precision)  << separator
                   << localFormat.formatDouble(reader->getBidLow(i), precision)   << separator
                   << localFormat.formatDouble(reader->getBidClose(i), precision) << separator
                   << localFormat.formatDouble(reader->getAskOpen(i), precision)  << separator
                   << localFormat.formatDouble(reader->getAskHigh(i), precision)  << separator
                   << localFormat.formatDouble(reader->getAskLow(i), precision)   << separator
                   << localFormat.formatDouble(reader->getAskClose(i), precision) << separator
                   << localFormat.formatDouble(reader->getVolume(i), 0)   << separator
                   << std::endl;
            }
            else
            {
                fs << time << separator
                   << localFormat.formatDouble(reader->getBid(i), precision) << separator
                   << localFormat.formatDouble(reader->getAsk(i), precision) << separator
                   << std::endl;
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Time taken by writePrices: "
              << duration.count() << " microseconds" << std::endl;
    fs.close();
}

/** Print sample parameters data.

    @param sProcName
        The sample process name.
    @param loginParams
        The LoginParams instance pointer.
    @param sampleParams
        The LoginParams instance pointer.
 */
void printSampleParams(std::string &sProcName, LoginParams *loginParams, SampleParams *sampleParams)
{
    std::cout << "Running " << sProcName << " with arguments:" << std::endl;

    // login (common) information
    if (loginParams)
    {
        std::cout << loginParams->getLogin() << " * "
                  << loginParams->getURL() << " "
                  << loginParams->getConnection() << " "
                  << loginParams->getSessionID() << " "
                  << loginParams->getPin() << std::endl;
    }

    // sample specific information
    if (sampleParams)
    {
        std::cout << "Instrument='" << sampleParams->getInstrument() << "', "
                  << "Timeframe='" << sampleParams->getTimeframe() << "', ";
        if (isNaN(sampleParams->getDateFrom()))
        {
            std::cout << "DateFrom='', ";
        }
        else
        {
            char sDateFrom[20];
            formatDate(sampleParams->getDateFrom(), sDateFrom);
            std::cout << "DateFrom='" << sDateFrom << "', ";
        }
        
        std::cout << "QuotesCount='" << sampleParams->getQuotesCount() << "'";

        std::cout << std::endl;
    }
}

/** Print expected sample-login parameters and their description. 

    @param sProcName
        The sample process name.
 */
void printHelp(std::string &sProcName)
{
    std::cout << sProcName << " sample parameters:" << std::endl << std::endl;
            
    std::cout << "/login | --login | /l | -l" << std::endl;
    std::cout << "Your user name." << std::endl << std::endl;
                
    std::cout << "/password | --password | /p | -p" << std::endl;
    std::cout << "Your password." << std::endl << std::endl;
                
    std::cout << "/url | --url | /u | -u" << std::endl;
    std::cout << "The server URL. For example, http://www.fxcorporate.com/Hosts.jsp." << std::endl << std::endl;
                
    std::cout << "/connection | --connection | /c | -c" << std::endl;
    std::cout << "The connection name. For example, \"Demo\" or \"Real\"." << std::endl << std::endl;
                
    std::cout << "/sessionid | --sessionid " << std::endl;
    std::cout << "The database name. Required only for users who have accounts in more than one database. "
        "Optional parameter." << std::endl << std::endl;
                
    std::cout << "/pin | --pin " << std::endl;
    std::cout << "Your pin code. Required only for users who have a pin. "
        "Optional parameter." << std::endl << std::endl;
                
    std::cout << "/instrument | --instrument | /i | -i" << std::endl;
    std::cout << "An instrument which you want to use in sample. "
        "For example, \"EUR/USD\"." << std::endl << std::endl;

    std::cout << "/timeframe | --timeframe " << std::endl;
    std::cout << "Time period which forms a single candle. "
        "For example, m1 - for 1 minute, H1 - for 1 hour." << std::endl << std::endl;

    std::cout << "/datefrom | --datefrom " << std::endl;
    std::cout << "Date/time from which you want to receive historical prices. "
        "If you leave this argument as it is, it will mean from last trading day. "
        "Format is \"m.d.Y H:M:S\". Optional parameter." << std::endl << std::endl;

    std::cout << "/count | --count " << std::endl;
    std::cout << "Count of historical prices you want to receive. If you "
        << "leave this argument as it is, it will mean -1 (use some default "
        << "value or ignore if datefrom is specified)"
        << std::endl;
    
    // std::cout << "/output | --output " << std::endl;
    // std::cout << "The output file name." << std::endl;
}

/** Check parameters for correct values.

    @param loginParams
        The LoginParams instance pointer.
    @param sampleParams
        The SampleParams instance pointer.
    @return
        true if parameters are correct.
 */
bool checkObligatoryParams(LoginParams *loginParams, SampleParams *sampleParams)
{
    // check login parameters
    if (strlen(loginParams->getLogin()) == 0)
    {
        std::cout << LoginParams::Strings::loginNotSpecified << std::endl;
        return false;
    }
    if (strlen(loginParams->getPassword()) == 0)
    {
        std::cout << LoginParams::Strings::passwordNotSpecified << std::endl;
        return false;
    }
    if (strlen(loginParams->getURL()) == 0)
    {
        std::cout << LoginParams::Strings::urlNotSpecified << std::endl;
        return false;
    }
    if (strlen(loginParams->getConnection()) == 0)
    {
        std::cout << LoginParams::Strings::connectionNotSpecified << std::endl;
        return false;
    }

    // check other parameters
    if (strlen(sampleParams->getInstrument()) == 0)
    {
        std::cout << SampleParams::Strings::instrumentNotSpecified << std::endl;
        return false;
    }
    if (strlen(sampleParams->getTimeframe()) == 0)
    {
        std::cout << SampleParams::Strings::timeframeNotSpecified << std::endl;
        return false;
    }
    // if (strlen(sampleParams->getOutputFile()) == 0)
    // {
    //     std::cout << SampleParams::Strings::outputFileNotSpecified << std::endl;
    //     return false;
    // }

    bool bIsDateFromNotSpecified = false;
    bool bIsDateToNotSpecified = false;
    DATE dtFrom = sampleParams->getDateFrom();
    DATE dtTo = sampleParams->getDateTo();
    char sDateFrom[20];
    char sDateTo[20];

    time_t tNow = time(NULL); // get time now
    struct tm *tmNow = gmtime(&tNow);
    DATE dtNow = 0;
    CO2GDateUtils::CTimeToOleTime(tmNow, &dtNow);

    if (isNaN(dtFrom))
    {
        bIsDateFromNotSpecified = true;
        dtFrom = 0;
        sampleParams->setDateFrom(dtFrom);
    }
    else
    {
        if (dtFrom - dtNow > 0.0001)
        {
            formatDate(dtFrom, sDateFrom);
            std::cout << "Sorry, 'DateFrom' value " << sDateFrom << " should be in the past" << std::endl;
            return false;
        }
    }

    if (isNaN(dtTo))
    {
        bIsDateToNotSpecified = true;
        dtTo = 0;
        sampleParams->setDateTo(dtTo);
    }
    else
    {
        if (!bIsDateFromNotSpecified && dtFrom - dtTo > 0.001)
        {
            formatDate(dtTo, sDateTo);
            formatDate(dtFrom, sDateFrom);
            std::cout << "Sorry, 'DateTo' value " << sDateTo << " should be later than 'DateFrom' value "
                << sDateFrom << std::endl;
            return false;
        }
    }

    return true;
}

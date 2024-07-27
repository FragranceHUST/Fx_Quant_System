#pragma once

class helpers
{
private:
    /* data */
public:
    helpers(/* args */);
    ~helpers();
    static std::string getOutputFilename(const char *instrument, const char *timeframe);
};
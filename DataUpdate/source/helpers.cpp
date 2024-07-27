#include "stdafx.h"
#include "helpers.h"

/** Gets precision of a specified instrument.

    @param instrument
        The instrument.
    @param timeframe
        The timeframe.
    @return
        The outputFile name.
 */
std::string helpers::getOutputFilename(const char *instrument, const char *timeframe)
{
    std::string outputFile = std::string(instrument) + "_" + std::string(timeframe) + ".csv";
    int pos = outputFile.find('/');
    outputFile.replace(pos, 1, "_");
    return outputFile;
}
#pragma once

#include <sstream>
#include <string>
#include <vector>

class helpers
{
private:
    /* data */
public:
    helpers(/* args */);
    ~helpers();
    static std::string getOutputFilename(const char *instrument, const char *timeframe)
    {
        std::string outputFile = std::string(instrument) + "_" + std::string(timeframe) + ".csv";
        int pos = outputFile.find('/');
        if (pos != std::string::npos)
        {
            outputFile.replace(pos, 1, "_");
        }
        return outputFile;
    }

    static void split(const std::string& line, char delimiter, std::vector<std::string>& vecofData)
    {
        std::stringstream ss(line);
        std::string token;

        while (std::getline(ss, token, delimiter)) {
            vecofData.push_back(token);
        }
    }
};
#include "stdafx.h"

#include "DataRepository.h"
#include "PriceData/Period.h"
#include "helpers.h"

#include <sys/inotify.h>

void DataRepository::loadFile()
{
    const std::string filename_D1 = helpers::getOutputFilename(mInstrument.c_str(), "D1");
    const std::string filename_H4 = helpers::getOutputFilename(mInstrument.c_str(), "H4");
    const std::string filename_M1 = helpers::getOutputFilename(mInstrument.c_str(), "m1");
    std::vector<std::string> vec_Filename = {filename_D1, filename_H4, filename_M1};
    // 将数据读取到vector<IPeriod>
    for (auto& s : vec_Filename)
    {
        std::ifstream ifs(s);
        if (!ifs.is_open())
        {
            // 需要定义异常，并且添加日志输出
            std::cerr << "Failed to open file: " << s << std::endl;
            return;
        }

        // 读取表头并丢弃
        std::string firstLine;
        std::getline(ifs, firstLine);

        std::string line;
        Periods& mPeriod = s.find("D1") != std::string::npos ? binsDaily : (s.find("H4") != std::string::npos ? binsH4 : binsM1);
        while (std::getline(ifs, line))
        {
            std::vector<std::string> tokens;
            helpers::split(line, ';',  tokens);
            DATE time;
            tm t;
            hptools::date::DateStringToCTime(tokens[0].c_str(), "%m.%d.%Y %H:%M:%S", &t);
            hptools::date::CTimeToOleTime(&t, &time);
            O2G2Ptr<Period> period(new Period(
                time,
                std::stod(tokens[1]), std::stod(tokens[2]), std::stod(tokens[3]), std::stod(tokens[4]),
                std::stod(tokens[5]), std::stod(tokens[6]), std::stod(tokens[7]), std::stod(tokens[8]),
                std::stoi(tokens[9])
            ));
            mPeriod.push_back(period);
        }

        ifs.close();
    }
}

// 开始监听文件的变化
void DataRepository::startWatchingFile(const std::string& filename) {
    if (watch_thread_) {
        std::cerr << "Already watching a file." << std::endl;
        return;
    }

    int fd = inotify_init();
    if (fd < 0) {
        std::cerr << "Failed to initialize inotify" << std::endl;
        return;
    }

    wd_ = inotify_add_watch(fd, filename.c_str(), IN_MODIFY);
    if (wd_ < 0) {
        std::cerr << "Failed to add watch for file: " << filename << std::endl;
        return;
    }

    watch_thread_ = std::make_shared<std::thread>([this, filename, fd]() mutable {
        char buffer[1024];
        while (!should_stop_watching_.load(std::memory_order_relaxed)) {
            ssize_t len = read(fd, buffer, sizeof(buffer));
            if (len == -1) {
                std::cerr << "Error reading from inotify" << std::endl;
                break;
            }

            for (ssize_t i = 0; i < len; i++) {
                struct inotify_event *event = (struct inotify_event *)&buffer[i];
                if (event->len) {
                    std::cout << "File event detected: " << event->name << std::endl;
                    loadFile(); // 当文件发生变化时重新加载
                    i += event->len - 1;
                }
            }
        }

        inotify_rm_watch(fd, wd_);
        close(fd);
    });
}

// 停止监听文件的变化
void DataRepository::stopWatchingFile() {
    if (watch_thread_) {
        should_stop_watching_.store(true, std::memory_order_release);
        if (watch_thread_->joinable()) {
            watch_thread_->join();
        }
        watch_thread_.reset();
    }
}

// 设置当前监听的品种名称
void DataRepository::setInstrument(const std::string& Instrument)
{
        mInstrument = Instrument;
}
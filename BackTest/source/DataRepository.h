#pragma once

class Period;

class DataRepository {
public:
    static DataRepository* getInstance() {
        if (s_instance == nullptr)
        {
            s_instance = new DataRepository();
        }
        return s_instance;
    }
    void setInstrument(const std::string& Instrument);
    void loadFile();
    void startWatchingFile(const std::string& filename);
    void stopWatchingFile();

    /** Storage of the periods(bars). */
    typedef std::vector<O2G2Ptr<Period> > Periods;
    Periods binsDaily;
    Periods binsH4;
    Periods binsM1;

private:
    DataRepository() = default; // 私有构造函数
    ~DataRepository() = default; // 私有析构函数

    std::map<std::string, std::string> data;
    std::mutex mutex_;
    std::shared_ptr<std::thread> watch_thread_;
    std::atomic<bool> should_stop_watching_{false};
    int wd_; // inotify watch descriptor
    std::string mInstrument;

    static DataRepository* s_instance;
};
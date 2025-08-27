#ifndef _LOGGING_
#define _LOGGING_

#include <sstream>
#include <string>
#include <memory>
#include <time.h>

#include "noncopyable.hpp"
#include "Timestamp.hpp"

#define LOGGING_INFO Logging(INFO, __FILE__, __LINE__, TimeStamp::now()).getss()
#define LOGGING_WARNING Logging(WARNING, __FILE__, __LINE__, TimeStamp::now()).getss()
#define LOGGING_ERROR Logging(ERROR, __FILE__, __LINE__, TimeStamp::now()).getss()
#define LOGGING_FATAL Logging(FATAL, __FILE__, __LINE__, TimeStamp::now()).getss()

enum LoggingLevel{
    INFO,
    WARNING,
    ERROR,
    FATAL
};

inline std::string levelToString(enum LoggingLevel level){
    switch (level)
    {
#define X(x) case x: return #x
    X(INFO);
    X(WARNING);
    X(ERROR);
    X(FATAL);
#undef X
    default:
        return "";
    }
}

class Logging : noncopyable{
public:
    Logging(enum LoggingLevel level, const std::string& file, int line, TimeStamp t);
    std::stringstream& getss(){return *m_sstream;}
    ~Logging();
private:
    LoggingLevel m_level;
    std::unique_ptr<std::stringstream> m_sstream;
};

#endif
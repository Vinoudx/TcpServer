#include "Logging.hpp"

Logging::Logging(enum LoggingLevel level, const std::string& file, int line, TimeStamp t):m_level(level),m_sstream(std::make_unique<std::stringstream>()){
    (*m_sstream)<< levelToString(m_level)<< ' ' << "In File: " << file << ", Line: " << line << " " << t.toString() << ' ';
}


Logging::~Logging(){
    (*m_sstream) << " \n";
    std::string log = m_sstream->str();
    write(STDOUT_FILENO, log.c_str(), log.size());
}


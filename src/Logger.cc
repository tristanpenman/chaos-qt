#include <iostream>
#include <mutex>
#include <utility>

#include "Logger.h"

using namespace std;

atomic<ostream*> Logger::os_ = nullptr;
atomic<Logger::Level> Logger::minLevel_ = Level::Info;
mutex Logger::mutex_;

namespace {

const char* levelLabel(Logger::Level level)
{
    switch (level) {
    case Logger::Level::Info:
        return "I";
    case Logger::Level::Warning:
        return "W";
    case Logger::Level::Error:
        return "E";
    case Logger::Level::Verbose:
        return "V";
    default:
        return "U";
    }
}

}  // namespace

Logger::Logger(string name)
    : name_(std::move(name))
{
}

void Logger::configure()
{
    os_ = &cout;
}

void Logger::configure(ostream& os)
{
    os_ = &os;
}

void Logger::configure(Level minLevel)
{
    minLevel_ = minLevel;
}

void Logger::configure(ostream& os, Level minLevel)
{
    os_ = &os;
    minLevel_ = minLevel;
}

//
// Logger::Writer implementation
//

Logger::Writer::Writer(Logger& logger, Level level)
    : logger_(logger)
{
    enabled_ = os_.load() && level >= minLevel_.load();
    if (!enabled_) {
        return;
    }

    ss_ << "[" << levelLabel(level) << "]";
    if (!logger_.name_.empty()) {
        ss_ << "[" << logger_.name_ << "]";
    }
    ss_ << " ";
}

Logger::Writer::~Writer()
{
    if (!enabled_) {
        return;
    }

    ostream* os = os_.load();
    if (!os) {
        return;
    }

    lock_guard lock(mutex_);
    *os << ss_.str() << '\n';
}

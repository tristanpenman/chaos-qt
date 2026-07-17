#include "Logger.h"

#include <iostream>
#include <mutex>
#include <utility>



std::atomic<std::ostream*> Logger::os_ = nullptr;
std::atomic<Logger::Level> Logger::minLevel_ = Level::kInfo;
std::mutex Logger::mutex_;

namespace {

const char* levelLabel(Logger::Level level)
{
    switch (level) {
    case Logger::Level::kInfo:
        return "I";
    case Logger::Level::kWarning:
        return "W";
    case Logger::Level::kError:
        return "E";
    case Logger::Level::kVerbose:
        return "V";
    default:
        return "U";
    }
}

}  // namespace

Logger::Logger(std::string name)
    : name_(std::move(name))
{
}

void Logger::configure()
{
    os_ = &std::cout;
}

void Logger::configure(std::ostream& os)
{
    os_ = &os;
}

void Logger::configure(Level minLevel)
{
    minLevel_ = minLevel;
}

void Logger::configure(std::ostream& os, Level minLevel)
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

    std::ostream* os = os_.load();
    if (!os) {
        return;
    }

    std::lock_guard lock(mutex_);
    *os << ss_.str() << '\n';
}

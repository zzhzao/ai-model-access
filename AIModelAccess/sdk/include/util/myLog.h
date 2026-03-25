#pragma once 
#include <spdlog/spdlog.h>  
#include <spdlog/logger.h>
#include <memory>
#include <mutex>



namespace util_log{
    class Logger{
        public:
            static void initLogger(const std::string& loggerName, const std::string& loggerFile, spdlog::level::level_enum logLevel = spdlog::level::info);
            static std::shared_ptr<spdlog::logger> getLogger();
        
        private:
            Logger();
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;
        
        private:
            static std::shared_ptr<spdlog::logger> _logger;
            static std::mutex _mutex;
    };
}
// 采用了fmt库 {}中的内容被后面的数据替代
// 例如：DBG("{} {}", "hello", "world") 会输出 [文件名 + 行号]hello world
#define TRACE(format, ...) util_log::Logger::getLogger()->trace(std::string("[{:>10s}:{:<4d}]")+format,__FILE__,__LINE__,##__VA_ARGS__)
#define DBG(format, ...) util_log::Logger::getLogger()->debug(std::string("[{:>10s}:{:<4d}]")+format,__FILE__,__LINE__,##__VA_ARGS__)
#define INFO(format, ...) util_log::Logger::getLogger()->info(std::string("[{:>10s}:{:<4d}]")+format,__FILE__,__LINE__,##__VA_ARGS__)
#define WARN(format, ...) util_log::Logger::getLogger()->warn(std::string("[{:>10s}:{:<4d}]")+format,__FILE__,__LINE__,##__VA_ARGS__)
#define ERR(format, ...) util_log::Logger::getLogger()->error(std::string("[{:>10s}:{:<4d}]")+format,__FILE__,__LINE__,##__VA_ARGS__)
#define CRIT(format, ...) util_log::Logger::getLogger()->critical(std::string("[{:>10s}:{:<4d}]")+format,__FILE__,__LINE__,##__VA_ARGS__)


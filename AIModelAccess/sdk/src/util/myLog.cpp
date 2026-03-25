#include "../../include/util/myLog.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>

namespace util_log{
    std::shared_ptr<spdlog::logger> Logger::_logger = nullptr;
    std::mutex Logger::_mutex;

    Logger::Logger()
    {

    }
    void Logger::initLogger(const std::string& loggerName, const std::string& loggerFile, spdlog::level::level_enum logLevel)
    {
        if(_logger == nullptr){
          std::lock_guard<std::mutex> lock(_mutex);
          if(_logger == nullptr){
            // 设置全局自动刷新级别，当日志级别 >= logLevel , 日志 会被立即刷新到文件。
            spdlog::flush_on(logLevel);
            // 启用异步日志 ， 线程池大小为 32768， 队列大小为 1。
            spdlog::init_thread_pool(32768, 1);
            if("stdout" == loggerFile){
                // 创建一个输出到控制台的日志器
                _logger = spdlog::stdout_color_mt(loggerName);
            }
            else
            {
                _logger = spdlog::basic_logger_mt<spdlog::async_logger>(loggerName, loggerFile);
            }
          
            // %n 日志名称
            // %-7l 日志级别，左对齐，宽度为 7
            // %v 日志消息
            _logger->set_pattern("[%H:%M:%S][%n][%-7l]%v");
            _logger->set_level(logLevel);
          }
        }
    }

    std::shared_ptr<spdlog::logger> Logger::getLogger()
    {
        return _logger;
    }
}
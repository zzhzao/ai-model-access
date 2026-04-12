#pragma once    
#include<unordered_map>
#include"common.h"
#include<memory>
#include<mutex>
#include"DataManager.h"
#include<atomic>


namespace ai_chat_sdk{

    class SessionManager{
    public:
        SessionManager(const std::string& dbName = "chatDB.db");
        // 创建会话
        std::string createSession(const std::string& modelName);    
        // 获取会话
        std::shared_ptr<Session> getSession(const std::string& sessionId);
        // 往会话添加消息
        bool addMessage(const std::string& sessionId, const Message& message);
        // 获取某个会话的历史消息
        std::vector<Message> getHistoryMessages(const std::string& sessionId) const;
        // 获取所有的会话列表
        std::vector<std::string> getSessionLists() const;
        // 删除会话
        bool deleteSession(const std::string& sessionId);
        // 更新会话时间戳
        void updateSessionTimestamp(const std::string& sessionId);
        // 清空所有会话
        void clearAllSessions();
        // 获取会话总数
        size_t getSessionCount() const;
    private:
        std::string generateSessionId();
        std::string generateMessageId(size_t messageCounter);

    private:
      // 管理所有的会话信息  key : session id  value : session info
        std::unordered_map<std::string, std::shared_ptr<Session>> _sessions;
        mutable std::mutex _mutex;
        std::atomic<int64_t> _sessionIdCounter = {0}; // 记录所有会话总数
        DataManager _dataManager;
    };

}

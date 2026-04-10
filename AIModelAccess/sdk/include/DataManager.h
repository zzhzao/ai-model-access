#pragma once
#include<sqlite3.h>
#include<string>
#include<mutex>
#include"common.h"



namespace ai_chat_sdk{
class DataManager{
    public:
        DataManager(const std::string& dbName);
        ~DataManager();   
        // Session操作
        // 插入新会话
        bool insertSession(const Session& session);
        // 获取会话
        std::shared_ptr<Session> getSession(const std::string& sessionId) const;
        // 更新会话时间戳
        bool updateSessionTimestamp(const std::string& sessionId);
        // 删除指定会话  删除会话时，也需要删除该会话中的所有消息
        bool deleteSession(const std::string& sessionId);
        // 获取所有会话ID
        std::vector<std::string> getSessionIds() const;
        // 获取所有会话
        std::vector<std::shared_ptr<Session>> getAllSessions() const;
        // 获取会话总数
        size_t getSessionCount() const;

        // Message操作
        // 插入新消息 需要更新会话时间戳
        bool insertMessage(const std::string& sessionId, const Message& message);
        // 获取所有消息
        std::vector<Message> getSessionMessages(const std::string& sessionId) const;
        // 删除指定会话的所有消息
        bool deleteMessages(const std::string& sessionId);
    private: 
    // 初始化数据库 -- 创建数据库表
    void initDatabase();
    // 执行SQL语句
    bool executeSQL(const std::string& sql);
    
    private:
      sqlite3* _db = nullptr;
      std::string _dbName;
      mutable std::mutex _mutex;



}; // end class DataManager
} // end namespace ai_chat_sdk


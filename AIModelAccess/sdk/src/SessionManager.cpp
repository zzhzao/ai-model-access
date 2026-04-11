#include"../include/SessionManager.h"
#include"../include/util/mylog.h"

namespace ai_chat_sdk{
    // 生成会话ID 会话ID格式：： session_时间戳_会话计数
    std::string SessionManager::generateSessionId(){
        // 会话计数自增
        _sessionIdCounter.fetch_add(1);
        std::time_t time = std::time(nullptr);
        //生成会话ID
        std::ostringstream os;
        os << "session_" << time << "_" << std::setw(8) << std::setfill('0') << _sessionIdCounter;
        return os.str();
    }
    // 生成消息ID   msg_时间戳_会话消息计数
    std::string SessionManager::generateMessageId(size_t messageCounter){
        messageCounter++;
        std::time_t time = std::time(nullptr);

        std::ostringstream os;
        os << "msg_" << time << "_" << std::setw(8) << std::setfill('0') << messageCounter;
        return os.str();
    }

    // 创建会话
    std::string SessionManager::createSession(const std::string& modelName){
        std::lock_guard<std::mutex> lock(_mutex);
        // 生成会话ID
        std::string sessionId = generateSessionId();
        // 创建会话
        std::shared_ptr<Session> session = std::make_shared<Session> (modelName);
        session._sessionId = sessionId;
        // 加入到会话列表
        _sessions[sessionId] = session;
        return sessionId;
    }
    // 获取会话
    std::shared_ptr<Session> SessionManager::getSession(const std::string& sessionId){
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end()){
            return nullptr;
        }
        return it->second;
    }
    // 往会话添加消息
    bool SessionManager::addMessage(const std::string& sessionId, const Message& message){
        std::lock_guard<std::mutex> lock(_mutex);
        // 获取会话
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end()){
            return false;
        }
        Message msg(message._role, message._content);
        msg._messageId = generateMessageId(it->second->_messages.size());
    
        it->second->_messages.push_back(msg);
        it->second->_updateTime = std::time(nullptr);
        INFO("add message to session %s", sessionId.c_str());
        return true;
    }
    // 获取某个会话的历史消息
    std::vector<Message> SessionManager::getHistoryMessages(const std::string& sessionId) const{
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end()){
            return {};
        }
        return it->second->_messages;
    }
    // 获取所有的会话列表  返回的是所有会话的会话ID
    std::vector<std::string> SessionManager::getSessionLists() const{
        std::lock_guard<std::mutex> lock(_mutex);
        // 构建一个临时对话列表，将其内部的会话按时间戳降序排序
        std::vector<std::pair<std::time_t,std::shared_ptr<Session>>> temp;
        temp.reserve(_sessions.size());

        for(auto it = _sessions.begin(); it != _sessions.end(); it++){
            temp.push_back({it->second->_updateTime, it->second});
        }
        // 按时间戳降序排序
        std::sort(temp.begin(), temp.end(), [](const std::pair<std::time_t,std::shared_ptr<Session>>& a, const std::pair<std::time_t,std::shared_ptr<Session>>& b){
            return a.first > b.first;

        });

        std::vector<std::string> sessions;
        sessions.reserve(_sessions.size());
        for(auto it = temp.begin(); it != temp.end(); it++){
            sessions.push_back(it->second->_sessionId);
        }
        return sessions;
    }
    // 删除会话
    bool SessionManager::deleteSession(const std::string& sessionId){
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end()){
            return false;
        }
        _sessions.erase(it);
        return true;
    }
    // 更新会话时间戳
    void SessionManager::updateSessionTimestamp(const std::string& sessionId){
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end()){
            return;
        }
        it->second->_updateTime = std::time(nullptr);
        INFO("update session %s timestamp", sessionId.c_str());
    }
    // 清空所有会话
    void SessionManager::clearAllSessions(){
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.clear();
    }
    // 获取会话总数
    size_t SessionManager::getSessionCount() const{
        std::lock_guard<std::mutex> lock(_mutex);
        return _sessionIdCounter;
    }

}
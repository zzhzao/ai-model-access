#include"../include/SessionManager.h"
#include"../include/util/myLog.h"
#include <iomanip>  
namespace ai_chat_sdk{
    SessionManager::SessionManager(const std::string& dbName)
     : _dataManager(dbName)
    {
        auto sessions = _dataManager.getAllSessions();
        for(auto& session : sessions){
            _sessions[session->_sessionId] = session;
        }
    }
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
        _mutex.lock();
        // 生成会话ID
        std::string sessionId = generateSessionId();
        // 创建会话
        std::shared_ptr<Session> session = std::make_shared<Session> (modelName);
        session->_sessionId = sessionId;
        session->_updateTime = std::time(nullptr);
        session->_createTime = session->_updateTime;
        // 加入到会话列表
        _sessions[sessionId] = session;
        _mutex.unlock();
        // 插入会话到数据库
        _dataManager.insertSession(*session);
        return sessionId;
    }
    // 获取会话
    std::shared_ptr<Session> SessionManager::getSession(const std::string& sessionId){
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end()){
            // 获取当前会话的历史消息
            _mutex.unlock();
            it->second->_messages = _dataManager.getSessionMessages(sessionId);
            return it->second;
        }
        _mutex.unlock();

        auto session = _dataManager.getSession(sessionId);
        if(session){
            _mutex.lock();
            auto it = _sessions.find(session->_sessionId);
            if(it == _sessions.end()){
                _sessions[session->_sessionId] = session;
            }
            _mutex.unlock();
            session->_messages = _dataManager.getSessionMessages(sessionId);
            return session;
        }
        WARN("getSession %s failed", sessionId.c_str());
        return nullptr;
    }
    // 往会话添加消息
    bool SessionManager::addMessage(const std::string& sessionId, const Message& message){
        _mutex.lock();
        // 获取会话
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end()){
            _mutex.unlock();
            return false;
        }

        Message msg(message._role, message._content);
        msg._messageId = generateMessageId(it->second->_messages.size());
    
        it->second->_messages.push_back(msg);
        it->second->_updateTime = std::time(nullptr);
        INFO("add message to session %s", sessionId.c_str());
        _mutex.unlock();
        // 插入消息到数据库
        _dataManager.insertMessage(sessionId, msg);
        return true;
    }
    // 获取某个会话的历史消息
    std::vector<Message> SessionManager::getHistoryMessages(const std::string& sessionId) const{
        //先从内存中获取会话 获取不到，去数据库中获取
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end()){
            return it->second->_messages;
        }
        _mutex.unlock();
        // 从数据库中获取会话
        return _dataManager.getSessionMessages(sessionId);
    }
    // 获取所有的会话列表  返回的是所有会话的会话ID
    std::vector<std::string> SessionManager::getSessionLists() const{
        auto sessions = _dataManager.getAllSessions();


        std::lock_guard<std::mutex> lock(_mutex);
        // 构建一个临时对话列表，将其内部的会话按时间戳降序排序
        std::vector<std::pair<std::time_t,std::shared_ptr<Session>>> temp;
        temp.reserve(_sessions.size());

        for(auto it = _sessions.begin(); it != _sessions.end(); it++){
            temp.push_back({it->second->_updateTime, it->second});
        }
        for(auto& session : sessions){
            if(_sessions.find(session->_sessionId) == _sessions.end()){
                temp.push_back({session->_updateTime, session});
            }
               }
        // 按时间戳降序排序
        std::sort(temp.begin(), temp.end(), [](const std::pair<std::time_t,std::shared_ptr<Session>>& a, const std::pair<std::time_t,std::shared_ptr<Session>>& b){
            return a.first > b.first;

        });

        std::vector<std::string> sessionIDs;
        sessionIDs.reserve(_sessions.size());
        for(auto it = temp.begin(); it != temp.end(); it++){
            sessionIDs.push_back(it->second->_sessionId);
        }
        return sessionIDs;
    }
    // 删除会话
    bool SessionManager::deleteSession(const std::string& sessionId){
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end()){
            _mutex.unlock();
            return false;
        }
        _sessions.erase(it);
        _mutex.unlock();
        // 删除数据库中的会话
        _dataManager.deleteSession(sessionId);
        return true;
    }
    // 更新会话时间戳
    void SessionManager::updateSessionTimestamp(const std::string& sessionId){
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end()){
            it->second->_updateTime = std::time(nullptr);
        }
        _mutex.unlock();
        // 更新数据库时间戳
        _dataManager.updateSessionTimestamp(sessionId);
        INFO("update session %s timestamp", sessionId.c_str());
    }
    // 清空所有会话
    void SessionManager::clearAllSessions(){
        _mutex.lock();
        _sessions.clear();
        _mutex.unlock();
        // 清空数据库中的会话
        _dataManager.clearAllSessions();
    }
    // 获取会话总数
    size_t SessionManager::getSessionCount() const{
        std::lock_guard<std::mutex> lock(_mutex);
        return _sessions.size();
    }

}
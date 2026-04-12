#include"../include/DataManager.h"
#include"../include/util/myLog.h"

namespace ai_chat_sdk
{
DataManager::DataManager(const std::string& dbName)
    : _dbName(dbName)
{
    //创建并打开数据库
    int rc = sqlite3_open(_dbName.c_str(), &_db);
    if(rc != SQLITE_OK){
        ERR("打开数据库失败，{}",sqlite3_errmsg(_db));
    }
    INFO("数据库打开成功:{}",_dbName);

    if(!initDatabase()){
        sqlite3_close(_db);
        _db = nullptr;
        ERR("初始化数据库表失败");
    }
}
DataManager::~DataManager(){
    if(_db){
        sqlite3_close(_db);
        _db = nullptr;
    }
}
bool DataManager::initDatabase(){
    // 创建会话表
    std::string createSessionTable = R"(CREATE TABLE IF NOT EXISTS sessions (
        session_id TEXT PRIMARY KEY, 
        model_name TEXT NOT NULL, 
        create_time INTEGER NOT NULL,
        update_time INTEGER NOT NULL);)";
    if(!executeSQL(createSessionTable)){
        ERR("创建会话表失败");
        return false;
    }
    // 创建消息表
    std::string createMessageTable = R"(CREATE TABLE IF NOT EXISTS messages (
        message_id TEXT PRIMARY KEY, 
        session_id TEXT NOT NULL, 
        role TEXT NOT NULL, 
        content TEXT NOT NULL, 
        timestamp INTEGER NOT NULL,
        FOREIGN KEY (session_id) REFERENCES sessions (session_id) ON DELETE CASCADE
        );)";
    if(!executeSQL(createMessageTable)){
        ERR("创建消息表失败");  
        return false;
    }
    return true;
}
bool DataManager::executeSQL(const std::string& sql){
    if(!_db){
        ERR("数据库未打开");
        return false;
    }
    char* errMsg = nullptr;
    int rc = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errMsg);
    if(rc != SQLITE_OK){
        ERR("执行SQL语句失败，{}",errMsg);
        sqlite3_free(errMsg);
        return false;
    }
    else{
        INFO("执行SQL语句成功，{}",sql);
        sqlite3_free(errMsg);   
    }
    return true;
}
// 插入会话
bool DataManager::insertSession(const Session& session){
    std::lock_guard<std::mutex> lock(_mutex);
    std::string insertSql = R"(INSERT INTO sessions (session_id, model_name, create_time, update_time) VALUES (?, ?, ?, ?);)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, insertSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("insertSession准备SQL语句失败");
        return false;
    }
    sqlite3_bind_text(stmt, 1, session._sessionId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, session._modelName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(session._createTime));
    sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(session._updateTime));

    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        ERR("insertSession执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    INFO("insertSession插入会话成功，{}",session._sessionId);
    return true;
}
// 获取指定session的会话信息
std::shared_ptr<Session> DataManager::getSession(const std::string& sessionId) const{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string selectSql = R"(SELECT model_name,create_time,update_time FROM sessions WHERE session_id = ?;)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, selectSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("getSession准备SQL语句失败");
        return nullptr;
    }
    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW){
        ERR("getSession执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return nullptr;
    }
    std::string modelName = (const char*)sqlite3_column_text(stmt, 0);
    int createTime = sqlite3_column_int64(stmt, 1);
    int updateTime = sqlite3_column_int64(stmt, 2);

    auto session = std::make_shared<Session>(modelName, createTime, updateTime);
    session->_sessionId = sessionId;
    sqlite3_finalize(stmt);

    session->_messages = getSessionMessages(sessionId);

    return session;
}
// 更新指定会话的时间戳
bool DataManager::updateSessionTimestamp(const std::string& sessionId){
    std::lock_guard<std::mutex> lock(_mutex);
    std::string updateSql = R"(UPDATE sessions SET update_time = ? WHERE session_id = ?;)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, updateSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("updateSessionTimestamp准备SQL语句失败");
        return false;
    }
    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(std::time(nullptr)));
    sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        ERR("updateSessionTimestamp执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    INFO("updateSessionTimestamp更新会话时间戳成功，{}",sessionId);
    return true;
}
// 删除指定会话
bool DataManager::deleteSession(const std::string& sessionId){
    std::lock_guard<std::mutex> lock(_mutex);
    std::string deleteSql = R"(DELETE FROM sessions WHERE session_id = ?;)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, deleteSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("deleteSession准备SQL语句失败");
        return false;
    }
    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        ERR("deleteSession执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    INFO("deleteSession删除会话成功，{}",sessionId);
    return true;
}
// 获取所有会话ID
std::vector<std::string> DataManager::getSessionIds() const{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<std::string> sessionIds;
    std::string selectSql = R"(SELECT session_id FROM sessions ORDER BY update_time DESC;)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, selectSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("getSessionIds准备SQL语句失败");
        return sessionIds;
    }
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW){
        ERR("getSessionIds执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return sessionIds;
    }
    while(sqlite3_step(stmt) == SQLITE_ROW){
        sessionIds.push_back((const char*)sqlite3_column_text(stmt, 0));
        rc = sqlite3_step(stmt);
   
    }
    sqlite3_finalize(stmt);
    return sessionIds;
}

// 获取所有会话
std::vector<std::shared_ptr<Session>> DataManager::getAllSessions() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<std::shared_ptr<Session>> sessions;
    std::string selectSql = R"(SELECT session_id, model_name, create_time, update_time FROM sessions ORDER BY update_time DESC;)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, selectSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("getAllSessions准备SQL语句失败");
        return sessions;
    }
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW){
        ERR("getAllSessions执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return sessions;
    }
    while(sqlite3_step(stmt) == SQLITE_ROW){
        std::string sessionId = (const char*)sqlite3_column_text(stmt, 0);
        std::string modelName = (const char*)sqlite3_column_text(stmt, 1);
        int createTime = sqlite3_column_int64(stmt, 2);
        int updateTime = sqlite3_column_int64(stmt, 3);
        auto session = std::make_shared<Session>(modelName, createTime, updateTime);
        session->_sessionId = sessionId;
        sessions.push_back(session);
        // 历史消息暂时不获取
    }
    sqlite3_finalize(stmt);
    return sessions;
}
//获取会话总数   
size_t DataManager::getSessionCount() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string selectSql = R"(SELECT COUNT(*) FROM sessions;)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, selectSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("getSessionCount准备SQL语句失败");
        return 0;
    }
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_ROW){
        ERR("getSessionCount执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return 0;
    }
    size_t count = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}
// 插入新消息 需要更新会话时间戳
bool DataManager::insertMessage(const std::string& sessionId, const Message& message){
    std::lock_guard<std::mutex> lock(_mutex);
    std::string insertSql = R"(INSERT INTO messages (message_id, session_id, role, content, timestamp) VALUES (?, ?, ?, ?, ?);)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, insertSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("insertMessage准备SQL语句失败");
        return false;
    }
    sqlite3_bind_text(stmt, 1, message._messageId.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, message._role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, message._content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, static_cast<int64_t>(message._timestamp));
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        ERR("insertMessage执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }

    std::string updateSessionSql = R"(UPDATE sessions SET update_time = ? WHERE session_id = ?;)";
    // 准备SQL语句
    sqlite3_stmt* stmt2 = nullptr;
    if(sqlite3_prepare_v2(_db, updateSessionSql.c_str(), -1, &stmt2, nullptr) != SQLITE_OK){
        ERR("insertMessage更新会话时间戳准备SQL语句失败");
        return false;
    }
    sqlite3_bind_int64(stmt2, 1, static_cast<int64_t>(message._timestamp));
    sqlite3_bind_text(stmt2, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt2);
    if(rc != SQLITE_DONE){
        ERR("insertMessage更新会话时间戳执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt2);
        return false;
    }
    sqlite3_finalize(stmt2);

    sqlite3_finalize(stmt);
    INFO("insertMessage插入消息成功，{}",message._messageId);
    return true;
}

// 获取指定会话的所有消息

std::vector<Message> DataManager::getSessionMessages(const std::string& sessionId) const{
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<Message> messages;
    std::string selectSql = R"(SELECT message_id, role, content, timestamp FROM messages WHERE session_id = ?;)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, selectSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("getSessionMessages准备SQL语句失败");
        return messages;
    }
    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);

    while(rc = sqlite3_step(stmt) == SQLITE_ROW){
        std::string messageId = (const char*)sqlite3_column_text(stmt, 0);
        std::string role = (const char*)sqlite3_column_text(stmt, 1);
        std::string content = (const char*)sqlite3_column_text(stmt, 2);
        int timestamp = sqlite3_column_int64(stmt, 3);
        messages.emplace_back(messageId, role, content, timestamp);
    }
    if(rc != SQLITE_DONE){
        ERR("getSessionMessages执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return messages;
    }
    sqlite3_finalize(stmt);
    return messages;    
}
// 删除指定会话的所有消息
bool DataManager::deleteMessages(const std::string& sessionId){
    std::lock_guard<std::mutex> lock(_mutex);
    std::string deleteSql = R"(DELETE FROM messages WHERE session_id = ?;)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, deleteSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("deleteMessages准备SQL语句失败");
        return false;
    }
    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        ERR("deleteMessages执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    INFO("deleteMessages删除会话所有消息成功，{}",sessionId);
    return true;
}
// 清空所有会话
void DataManager::clearAllSessions(){
    std::lock_guard<std::mutex> lock(_mutex);
    std::string deleteSql = R"(DELETE FROM sessions;)";
    // 准备SQL语句
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(_db, deleteSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK){
        ERR("clearAllSessions准备SQL语句失败");
        return;
    }
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        ERR("clearAllSessions执行SQL语句失败，{}",sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return;
    }
    sqlite3_finalize(stmt);
    INFO("clearAllSessions清空所有会话成功");
}


}// end namespace ai_chat_sdk

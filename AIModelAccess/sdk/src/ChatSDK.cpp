#include"../include/ChatSDK.h"
#include"../include/DeepSeekProvider.h"
#include"../include/ChatGPTProvider.h"
#include"../include/OllamaLLMProvider.h"
#include"../include/GeminiProvider.h"
#include"../include/util/myLog.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
namespace ai_chat_sdk{

//初始化支持的模型
bool ChatSDK::initModels(const std::vector<std::shared_ptr<Config>>& configs){
    registerALLProviders(configs);
    initProviders(configs);
    _initialized = true;
    return true;
}


// 注册所有支持的模型
void ChatSDK::registerALLProviders(const std::vector<std::shared_ptr<Config>>& configs){

    if(!_llManager.isModelAvailable("deepseek-chat"))
    {
        auto deepseekProvider = std::make_unique<DeepSeekProvider>();

        _llManager.registerProvider("deepseek-chat",std::move(deepseekProvider));
        INFO("register deepseek-chat provider");
    }
    if(!_llManager.isModelAvailable("gpt-4o-mini")){
        auto gptProvider = std::make_unique<ChatGPTProvider>();
        _llManager.registerProvider("gpt-4o-mini",std::move(gptProvider));
        INFO("register gpt-4o-mini provider");
    }
    if(!_llManager.isModelAvailable("gemini-2.0-flash")){
        auto geminiProvider = std::make_unique<ChatGPTProvider>();
        _llManager.registerProvider("gemini-2.0-flash",std::move(geminiProvider));
        INFO("register gemini-2.0-flash provider");
    }
    std::unordered_set<std::string> modelNames;
    for(const auto& config : configs){
        auto ollamaConfig = dynamic_pointer_cast<OllamaConfig>(config);
        if(ollamaConfig){
            auto modelName = ollamaConfig->_modelName;
            if(modelNames.find(modelName) == modelNames.end()){
                modelNames.insert(modelName);
                if(!_llManager.isModelAvailable(modelName)){
                    auto ollamaProvider = std::make_unique<OllamaLLMProvider>();
                    _llManager.registerProvider(modelName,std::move(ollamaProvider));
                    INFO("register " + modelName + " provider");
                }
            }
        }
    }

}
// 初始化所有支持的模型
bool ChatSDK::initProviders(const std::vector<std::shared_ptr<Config>>& configs){
    for(const auto& config : configs){
        
        if(auto apiConfig = dynamic_pointer_cast<APIConfig>(config)){
            if(apiConfig->_modelName == "deepseek-chat" || apiConfig->_modelName == "gpt-4o-mini" || apiConfig->_modelName == "gemini-2.0-flash"){
                initAPIProvider(apiConfig->_modelName, apiConfig);
            }
            else {
                ERR("model name " + apiConfig->_modelName + " not supported");
            }
        }
        else if(auto ollamaConfig = dynamic_pointer_cast<OllamaConfig>(config)){
            initOllamaProvider(ollamaConfig->_modelName, ollamaConfig);
        }
        else {
            ERR("config type not supported");
        }
    }
}
// 初始化模型提供者 -- API模型提供者

bool ChatSDK::initAPIProvider(const std::string& modelName,const std::shared_ptr<APIConfig>& apiConfig){
    // 参数检测
    if(modelName.empty()){
        ERR("model name is empty");
        return false;
    }
    if(!apiConfig || apiConfig->_apiKey.empty()){
        ERR("api key is empty");
        return false;
    }
    if(!_llManager.isModelAvailable(modelName)){
        ERR("model " + modelName + " not available");
        return false;
    }

    std::map<std::string, std::string> modelParams = {
        {"api_key", apiConfig->_apiKey}
    };
    // 初始化模型
    if(!_llManager.initModel(modelName, modelParams)){
        ERR("init model " + modelName + " failed");
        return false;
    }
    // 更新模型信息
    _modelConfigs[modelName] = apiConfig;
    return true;
}
// 初始化模型提供者 -- Ollama模型提供者
bool ChatSDK::initOllamaProvider(const std::string& modelName,const std::shared_ptr<OllamaConfig>& ollamaConfig){
    // 参数检测
    if(modelName.empty()){
        ERR("model name is empty");
        return false;
    }
    if(!ollamaConfig || ollamaConfig->_endpoint.empty()){
        ERR("endpoint is empty");
        return false;
    }
    if(!_llManager.isModelAvailable(modelName)){
        ERR("model " + modelName + " not available");
        return false;
    }
    std::map<std::string, std::string> modelParams = {
        {"model_name", ollamaConfig->_modelName},
        {"model_desc", ollamaConfig->_modelDesc},
        {"endpoint", ollamaConfig->_endpoint}
    };
    // 初始化模型
    if(!_llManager.initModel(modelName, modelParams)){
        ERR("init model " + modelName + " failed");
        return false;
    }
    // 更新模型信息
    _modelConfigs[modelName] = ollamaConfig;
    return true;
}
// 创建会话
std::string ChatSDK::createSession(const std::string& modelName){
// 检测SDK是否初始化
    if(!_initialized){
        ERR("SDK not initialized");
        return "";
    }
    // 通过sessionManager创建会话
    auto sessionId = _sessionManager.createSession(modelName);
    if(sessionId.empty()){
        ERR("create session failed");
        return "";
    }
    INFO("create session " + sessionId + " successed");
    return sessionId;
}
// 获取指定会话
std::shared_ptr<Session> ChatSDK::getSession(const std::string& sessionId){
        // 检测SDK是否初始化
    if(!_initialized){
        ERR("SDK not initialized");
        return nullptr;
    }
    auto session = _sessionManager.getSession(sessionId);
    if(!session){
        ERR("session " + sessionId + " not found");
        return nullptr;
    }
    return session;
}
std::vector<std::string> ChatSDK::getSessionLists() const{
    // 检测SDK是否初始化
    if(!_initialized){
        ERR("SDK not initialized");
        return {};
    }
    return _sessionManager.getSessionLists();
}
//删除会话
bool ChatSDK::deleteSession(const std::string& sessionId){
    // 检测SDK是否初始化
    if(!_initialized){
        ERR("SDK not initialized");
        return false;
    }
    // 删除会话
    if(!_sessionManager.deleteSession(sessionId)){
        ERR("delete session " + sessionId + " failed");
        return false;
    }
    INFO("delete session " + sessionId + " successed");
    return true;
}

// 获取可用模型列表
std::vector<ModelInfo> ChatSDK::getAvailableModels()const
{
    return _llManager.getAvailableModels();
}
// 给模型发送消息
std::string ChatSDK::sendMessage(const std::string& sessionId, const std::string& message)
{
    // 检测SDK是否初始化
    if(!_initialized){
        ERR("SDK not initialized");
        return "";
    }
    // 检测会话是否存在
    auto session = _sessionManager.getSession(sessionId);
    if(!session){
        ERR("session " + sessionId + " not found");
        return "";
    }
    // 构造请求参数
    Message userMessage("user", message);
    _sessionManager.addMessage(sessionId, userMessage);
    auto historyMessages = _sessionManager.getHistoryMessages(sessionId);

    auto it = _modelConfigs.find(session->_modelName);
    if(it == _modelConfigs.end()){
        ERR("model " + session->_modelName + " not found");
        return "";
    }
    std::map<std::string, std::string> requestParam = {
        {"temperature", std::to_string(it->second->_temperature)},
        {"max_tokens", std::to_string(it->second->_maxTokens)},
    };
    // 发送消息
    auto response = _llManager.sendMessage(session->_modelName, historyMessages, requestParam);
    if(response.empty()){
        ERR("sendMessage failed");
        return "";
    }
    // 添加助手消息并更新会话时间
    Message assistantMessage("assistant", response);
    _sessionManager.addMessage(sessionId, assistantMessage);
    _sessionManager.updateSessionTimestamp(sessionId);

    INFO("assistant sendMessage successed");
    return response;
}

// 给模型发送消息 - 增量返回
std::string ChatSDK::sendMessageStream(const std::string& sessionId, const std::string& message, std::function<void(const std::string&, bool)> callback){
    // 检测SDK是否初始化成功
    if(!_initialized){
        ERR("ChatSDK::sendMessageStream: SDK is not initialized");
        return "";
    }

    // 获取sessionId对应的session对象
    auto session = _sessionManager.getSession(sessionId);
    if(!session){
        ERR("ChatSDK::sendMessageStream: session {} not found", sessionId);
        return "";
    }

    // 构造历史消息
    Message userMessage("user", message);
    _sessionManager.addMessage(sessionId, userMessage);
    auto historyMessages = _sessionManager.getHistoryMessages(sessionId);

    // 构建请求参数
    auto it = _modelConfigs.find(session->_modelName);
    if(it == _modelConfigs.end()){
        ERR("ChatSDK::sendMessageStream: model {} not found", session->_modelName);
        return "";
    }
    std::map<std::string, std::string> requestParam;
    requestParam["temperature"] = std::to_string(it->second->_temperature);
    requestParam["max_tokens"] = std::to_string(it->second->_maxTokens);

    // 调用LLMManager发送消息
    auto response = _llManager.sendMessageStream(session->_modelName, historyMessages, requestParam, callback);
    if(response.empty()){
        ERR("ChatSDK::sendMessageStream: send message to model {} failed", session->_modelName);
        return "";
    }

    // 添加助手消息并更新会话时间
    Message assistantMessage("assistant", response);
    _sessionManager.addMessage(sessionId, assistantMessage);
    _sessionManager.updateSessionTimestamp(sessionId);
    INFO("ChatSDK::sendMessageStream: send message to model {} successed", session->_modelName);
    return response;
}



}
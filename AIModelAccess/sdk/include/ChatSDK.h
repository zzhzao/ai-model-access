#pragma once
#include"common.h"
#include<string>
#include<vector>
#include<map>
#include<unordered_map>
#include<functional>
#include<memory>
#include"LLMManager.h"
#include <atomic>
#include"SessionManager.h"

namespace ai_chat_sdk{

    class ChatSDK{
    public:
        // 初始化模型
        bool initModels(const std::vector<std::shared_ptr<Config>>& configs);
        // 创建会话
        std::string createSession(const std::string& modelName);
        // 获取指定会话
        std::shared_ptr<Session> getSession(const std::string& sessionId);
        // 获取所有的会话列表
        std::vector<std::string> getSessionLists() const;
        // 删除指定会话
        bool deleteSession(const std::string& sessionId);
        // 获取可用模型信息
        std::vector<ModelInfo> getAvailableModels() const;
        // 发送消息  --全量返回
        std::string sendMessage(const std::string& sessionId, const std::string& message);
        // 发送消息  --流式返回
        std::string sendMessageStream(const std::string& sessionId, const std::string& message,
            std::function<void(const std::string&,bool)> callback);
    
    private:
        // 注册所支持的模型
        void registerALLProviders(const std::vector<std::shared_ptr<Config>>& configs);
        // 初始化所支持的模型
        bool initProviders(const std::vector<std::shared_ptr<Config>>& configs);
        // 初始化模型提供者 -- API模型提供者
        bool initAPIProvider(const std::string& modelName,const std::shared_ptr<APIConfig>& apiConfig);
        // 初始化模型提供者 -- Ollama模型提供者
        bool initOllamaProvider(const std::string& modelName,const std::shared_ptr<OllamaConfig>& ollamaConfig);

    private:
        bool _initialized = false;  // 是否初始化
        std::unordered_map<std::string, std::shared_ptr<Config>> _modelConfigs;  // 模型配置
        LLMManager _llManager;     // 和模型交互
        SessionManager _sessionManager;    // 与会话进行交互

    };


} // end namespace ai_chat_sdk

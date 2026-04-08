#include"../include/LLMManager.h"
#include"../include/util/myLog.h"
#include"../include/common.h"
namespace ai_chat_sdk{

    // 注册LLM提供者
    bool LLMManager::registerProvider(const std::string& modelName, std::unique_ptr<LLMProvider> provider)
    {
        //参数检测
        if(provider == nullptr){ 
            ERR("LLMManager registerProvider provider is null");
            return false;
        }
        _providers[modelName] = std::move(provider);
        //添加模型信息
        _modelInfos[modelName] = ModelInfo(modelName);
        INFO("LLMManager registerProvider model {} success",modelName);
        return true;
    }
    // 初始化指定模型
    bool LLMManager::initModel(const std::string& modelName, const std::map<std::string, std::string>& modelConfig)
    {
        //检测模型是否注册
        auto it = _providers.find(modelName);
        if(it == _providers.end()){
            ERR("LLMManager initModel model not found");
            return false;
        }
        bool isSuccess = it->second->initModel(modelConfig);
        if(!isSuccess){
            ERR("LLMManager initModel model {} failed",modelName);
        }
        else
        {
            INFO("LLMManager initModel model {} success",modelName);
            _modelInfos[modelName]._modelDesc = it->second->getModelDesc();
            _modelInfos[modelName]._isAvailable = true;
        }
        return isSuccess;
    }
    // 获取可用模型列表
    std::vector<ModelInfo> LLMManager::getAvailableModels() const
    {
        std::vector<ModelInfo> models;
        for(const auto& it : _modelInfos){
            if(it.second._isAvailable){
                models.push_back(it.second);
            }
        }
        return models;
    }
    // 检测模型是否可用
    bool LLMManager::isModelAvailable(const std::string& modelName) const
    {
        auto it = _modelInfos.find(modelName);
        if(it == _modelInfos.end()){
            return false;
        }
        return it->second._isAvailable;
    }


    // 发送消息 -- 全量返回 
    std::string LLMManager::sendMessage(const std::string& modelName, const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam)
    {
        // 检测模型是否注册
        auto it = _providers.find(modelName);
        if(it == _providers.end()){
            ERR("LLMManager sendMessage model not found");
            return "";
        }
        // 检测模型是否可用
        if(!isModelAvailable(modelName)){
            ERR("LLMManager sendMessage model not available");
            return "";
        }
        return it->second->sendMessage(messages, requestParam);
    }
    // 发送消息 -- 流式返回
    std::string LLMManager::sendMessageStream(const std::string& modelName, const std::vector<Message>& messages,
        const std::map<std::string,std::string>& requestParam,
        std::function<void(const std::string&,bool)> callback)
    {
        // 检测模型是否注册
        auto it = _providers.find(modelName);
        if(it == _providers.end()){
            ERR("LLMManager sendMessageStream model not found");
            return "";
        }
        // 检测模型是否可用
        if(!isModelAvailable(modelName)){
            ERR("LLMManager sendMessageStream model not available");
            return "";
        }   
        return it->second->sendMessageStream(messages, requestParam, callback);
    }


}
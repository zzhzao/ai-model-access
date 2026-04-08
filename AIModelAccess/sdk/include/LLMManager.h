#pragma once
#include <map>
#include <memory>
#include <vector>
#include"common.h"
#include "LLMProvider.h"

namespace ai_chat_sdk{

class LLMManager{
public:
    // 注册LLM提供者
    bool registerProvider(const std::string& modelName, std::unique_ptr<LLMProvider> provider);
    // 初始化指定模型
    bool initModel(const std::string& modelName, const std::map<std::string, std::string>& modelConfig);
    // 获取可用模型列表
    std::vector<ModelInfo> getAvailableModels() const;
    // 检测模型是否可用
    bool isModelAvailable(const std::string& modelName) const;
    // 发送消息 -- 全量返回
    std::string sendMessage(const std::string& modelName, const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam);
    // 发送消息 -- 流式返回
    std::string sendMessageStream(const std::string& modelName, const std::vector<Message>& messages,
        const std::map<std::string,std::string>& requestParam,
        std::function<void(const std::string&,bool)> callback);
private:
    //key : model name
    std::map<std::string, std::unique_ptr<LLMProvider>> _providers; // 保存以支持的模型
    std::map<std::string, ModelInfo> _modelInfos; // 保存模型信息

};
}

#include"../include/ChatGPTProvider.h"
#include"../include/util/myLog.h"
#include <cstddef>
#include <jsoncpp/json/reader.h>
#include<jsoncpp/json/json.h>
#include<httplib.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <sstream>
#include <string>

namespace ai_chat_sdk
{
    bool ChatGPTProvider::initModel(const std::map<std::string, std::string>& modelconfig)
    {
        // 初始化api key
        auto it = modelconfig.find("_apiKey");
        if(it == modelconfig.end())
        {
            ERR("ChatGPTProvider initModel: _apiKey not found");
            return false;
        }
        else{
            _apiKey = it->second;

        }
        it = modelconfig.find("endpoint");
        if(it == modelconfig.end())
        {
            ERR("ChatGPTProvider initModel: endpoint not found");
            return false;
        }
        else{
            _endpoint = it->second;
        }
        INFO("ChatGPTProvider initModel: endpoint = {}", _endpoint.c_str());   
        _isAvailable = true;
        return true;
    }

        // 检测模型是否可用
    bool ChatGPTProvider::isAvailable() const
    {
        return _isAvailable;
    }
    // 获取模型名称
    std::string ChatGPTProvider::getModelName() const
    {
        return "gpt-4o-mini";
    }
    // 获取模型描述信息
    std::string ChatGPTProvider::getModelDesc() const
    {
        return "GPT-4o Mini 是一个基于 GPT-4o 模型的对话助手，它提供了一个简单、方便的 API 接口，用于构建自己的应用。";
    }
    std::string ChatGPTProvider::sendMessage(const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam)
    {
        //检测模型是否可用
        if(!isAvailable())
        {
            ERR("ChatGPTProvider sendMessage: model not available");
            return "";
        }
        // 构造请求参数
        double temperature = 0.7;
        if(requestParam.find("temperature") != requestParam.end())
        {
            temperature = std::stod(requestParam.at("temperature"));
        }
        int max_tokens = 2048;
        if(requestParam.find("max_output_tokens") != requestParam.end())
        {
            max_tokens = std::stoi(requestParam.at("max_output_tokens"));
        }

        // 构建消息列表
        Json::Value messagesJson(Json::arrayValue);
        for(const auto& msg : messages)
        {
            Json::Value messageJson(Json::objectValue);
            messageJson["role"] = msg._role;
            messageJson["content"] = msg._content;
            messagesJson.append(messageJson);
        }
        // 构建请求体
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["input"] = messagesJson;
        requestBody["temperature"] = temperature;
        requestBody["max_out_tokens"] = max_tokens;

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "  ";
        std::string requestBodyJsonstr = Json::writeString(writer,requestBody);

        // 创建http客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30,0);
        client.set_read_timeout(60,0);
        client.set_proxy("127.0.0.1",7890);

        httplib::Headers headers = {
                {"Authorization", "Bearer " + _apiKey}
           // {"Content-Type", "application/json"},
        };

        auto response = client.Post("/v1/response",  headers,requestBodyJsonstr, "application/json");
        if(!response)
        {
            ERR("ChatGPTProvider sendMessage: request failed, error = {}", to_string(response.error()));
            return "";
        }
        // 查看相应是否成功
        if(response->status != 200)
        {
            ERR("ChatGPTProvider sendMessage: request failed, status = {}", response->status);
            return "";
        }
        INFO("ChatGPTProvider sendMessage: request success, status = {}", response->status);
        // 解析响应体
        Json::CharReaderBuilder reader;
        std::string errorJson;
        Json::Value responseJson;
        std::istringstream responseStream(response->body);
        if(!Json::parseFromStream(reader, responseStream, &responseJson,&errorJson))
        {
            ERR("ChatGPTProvider sendMessage: parse response body failed");
            return "";
        }
        
        if(responseJson.isMember("output")&& responseJson["output"].isArray() && responseJson["output"].size() > 0)
        {
            auto output = responseJson["output"][0];
            if(output.isMember("content") && output["content"].isArray() && output["content"].size() > 0 && output["content"][0].isMember("text"))
            {
                return output["content"][0]["text"].asString();
            }
        }

    }
        // 发送消息 -- 流式返回
    std::string ChatGPTProvider::sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback)
            {
                //检测模型是否可用
                if(!isAvailable())
                {
                    ERR("ChatGPTProvider sendMessageStream: model not available");
                    return "";
                }

                //构造请求参数
                double temperature = 0.7;
                if(requestParam.find("temperature") != requestParam.end())
                {
                    temperature = std::stod(requestParam.at("temperature"));
                }
                int max_tokens = 2048;
                if(requestParam.find("max_output_tokens") != requestParam.end())
                {
                    max_tokens = std::stoi(requestParam.at("max_output_tokens"));
                }

                // 构建消息列表
                Json::Value messagesJson(Json::arrayValue);
                for(const auto& msg : messages)
                {
                    Json::Value messageJson(Json::objectValue);
                    messageJson["role"] = msg._role;
                    messageJson["content"] = msg._content;
                    messagesJson.append(messageJson);
                }

                // 构建请求体
                Json::Value requestBody;
                requestBody["model"] = getModelName();
                requestBody["input"] = messagesJson;
                requestBody["temperature"] = temperature;
                requestBody["max_out_tokens"] = max_tokens;
                requestBody["stream"] = true;

                //序列化
                Json::StreamWriterBuilder writer;
                writer["indentation"] = "  ";
                std::string requestBodyJsonstr = Json::writeString(writer,requestBody);
                
                // 创建http客户端
                httplib::Client client(_endpoint.c_str());
                client.set_connection_timeout(60,0);
                client.set_read_timeout(300,0);
                client.set_proxy("127.0.0.1",7890);

                httplib::Headers headers = {
                {"Authorization", "Bearer " + _apiKey},
                {"Content-Type", "application/json"},
                {"Accept", "text/event-stream"},
                };
                
                //流式处理的相关变量
                std::string buffer;
                bool gotError = false;
                std::string errorMsg;
                int statusCode = 0;
                bool streamFinish = false;
                std::string fullResponse;  //累计完整的数据

                //创建请求对象
                httplib::Request request;
                request.path = "/v1/response";
                request.method = "POST";
                request.body = requestBodyJsonstr;
                request.headers = headers;

                request.response_handler = [&](const httplib::Response& response){
                    if(response.status != 200)
                    {
                        gotError = true;
                        ERR("ChatGPTProvider sendMessageStream: request failed, status = {}", response.status);
                        return false;
                    }
                    return true;
                };

                // 数据接受处理器
                request.content_receiver = [&](const char* data , size_t dataLength,size_t offset,size_t totalLength){
                    if(gotError)
                    {
                        return false;
                    }
                    buffer.append(data,dataLength);
                    INFO("buffer: {}",buffer);
                    // 检测是否收到了完整的事件读数据
                    size_t pos = 0;
                    while((pos = buffer.find("\n\n",pos)) != std::string::npos)
                    {
                        std::string event = buffer.substr(pos,2);
                        buffer.erase(0,pos+2);

                        //解析事件类型和具体的数据位置
                        std::istringstream  eventStream(event);
                        std::string eventType;
                        std::string eventData;
                        std::string line;
                        while(std::getline(eventStream,line))
                        {
                            if(line.empty())
                            {
                                continue;
                            }
                            if(line.compare(0,6,"event:") == 0)
                            {
                                eventType = line.substr(7);   // 我们在这里要把空格一并删去
                            }
                            else if(line.compare(0,5,"data:") == 0)
                            {
                                eventData = line.substr(6);
                            }
                        }

                        Json::Value chunk;
                        Json::CharReaderBuilder reader;
                        std::string errs;
                        std::istringstream eventDataStream(eventData);
                        if(!Json::parseFromStream(reader,eventDataStream,&chunk,&errs))
                        {
                            ERR("ChatGPTProvider sendMessageStream: parse modelData failed, errors = {}",errs);
                            continue;
                        }

                        // 按照事件类型进行数据分析
                        if(eventType == "response.output_text.delta")
                        {
                            if(chunk.isMember("delta") && chunk["delta"].isString())
                            {
                                std::string delta = chunk["delta"].asString();
                                callback(delta,false);                               
                            }
                        }
                        else if(eventType == "response.output_item.done")
                        {
                            //表示一个output_item传输结束。需要将数据拼接
                            if(chunk.isMember("item") && chunk["item"].isObject())
                            {
                                Json::Value item = chunk["item"];
                                if(item.isMember("content") && item["content"].isArray() && item["content"].empty() && item["content"][0].isMember("text") \
                                && item["content"][0]["text"].isString())
                                {
                                    fullResponse += item["content"][0]["text"].asString();
                                }
                            }

                        }
                        else if(eventType == "response.completed")
                        {
                            streamFinish = true;
                            callback("",true);
                            return true;
                        }
                    }

                    return true;
                };
                auto result = client.send(request);
            if(!result)
            {
            // 请求发送失败，出现网络问题，比如DNS解析失败，请求超时
                ERR("Network error: {}", to_string(result.error()));
                return "";
            }
            //确保流式操作正常结束
            if(!streamFinish)
            {
                WARN("DeepSeekProvider sendMessageStream: stream not finish");
                callback("",true);
            }
            return fullResponse;
        }  //callback: 对模型返回的增量数据如何处理，第一个参数为增量数据，第二个参数为是否为最后一个增量数据。
} // end ai_chat_sdk
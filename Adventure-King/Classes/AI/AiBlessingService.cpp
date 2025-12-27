#include "AI/AiBlessingService.h"
#include "Configs/GameConfig.h"
#include "cocos2d.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

USING_NS_CC;

AiBlessingService *AiBlessingService::_instance = nullptr;

namespace
{
// OpenAI 兼容：choices[0].message.content 输出的 JSON 结构
// {
//   "npcText": "一句 NPC 台词",
//   "buff": [
//      {"key":"strength","value": 6},
//      {"key":"maxHp","value": 80}
//   ]
// }

struct BlessingRange
{
    const char *key = "";
    AttributeType type = AttributeType::STRENGTH;
    float minValue = 0.0f;
    float maxValue = 0.0f;
};

static std::vector<BlessingRange> buildRanges()
{
    std::vector<BlessingRange> ranges;

    ranges.push_back({"strength", AttributeType::STRENGTH, GameConfig::AI::Blessing::STRENGTH_MIN, GameConfig::AI::Blessing::STRENGTH_MAX});
    ranges.push_back({"defense", AttributeType::DEFENSE, GameConfig::AI::Blessing::DEFENSE_MIN, GameConfig::AI::Blessing::DEFENSE_MAX});
    ranges.push_back({"criticalRate", AttributeType::CRITICAL_RATE, GameConfig::AI::Blessing::CRIT_RATE_MIN, GameConfig::AI::Blessing::CRIT_RATE_MAX});
    ranges.push_back({"moveSpeed", AttributeType::MOVE_SPEED, GameConfig::AI::Blessing::MOVE_SPEED_MIN, GameConfig::AI::Blessing::MOVE_SPEED_MAX});
    ranges.push_back({"maxHp", AttributeType::MAX_HP, GameConfig::AI::Blessing::MAX_HP_MIN, GameConfig::AI::Blessing::MAX_HP_MAX});
    ranges.push_back({"maxMp", AttributeType::MAX_MP, GameConfig::AI::Blessing::MAX_MP_MIN, GameConfig::AI::Blessing::MAX_MP_MAX});

    // 攻速可选：默认范围为 0 表示禁用该项，避免在未确认含义前写入导致数值异常
    if (GameConfig::AI::Blessing::ATTACK_INTERVAL_MAX > 0.0f &&
        GameConfig::AI::Blessing::ATTACK_INTERVAL_MAX >= GameConfig::AI::Blessing::ATTACK_INTERVAL_MIN)
    {
        ranges.push_back({"attackInterval", AttributeType::ATTACKINTERVAL,
                          GameConfig::AI::Blessing::ATTACK_INTERVAL_MIN, GameConfig::AI::Blessing::ATTACK_INTERVAL_MAX});
    }

    return ranges;
}

static std::string toLowerCopy(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static std::string stripCodeFence(const std::string &s)
{
    // 去掉 ```json ... ``` 这种包裹（兼容 AI 误输出）
    std::string out = s;
    auto trim = [](std::string &t) {
        size_t start = 0;
        while (start < t.size() && std::isspace(static_cast<unsigned char>(t[start])))
        {
            ++start;
        }
        size_t end = t.size();
        while (end > start && std::isspace(static_cast<unsigned char>(t[end - 1])))
        {
            --end;
        }
        t = t.substr(start, end - start);
    };

    trim(out);
    if (out.rfind("```", 0) != 0)
    {
        return out;
    }

    // 找到第一行结束
    size_t firstNewline = out.find('\n');
    if (firstNewline == std::string::npos)
    {
        return out;
    }

    // 去掉开头 ```xxx
    out = out.substr(firstNewline + 1);
    // 去掉末尾 ```
    size_t fencePos = out.rfind("```");
    if (fencePos != std::string::npos)
    {
        out = out.substr(0, fencePos);
    }
    trim(out);
    return out;
}

static std::string shortResp(const std::string &s, size_t maxLen = 300)
{
    if (s.size() <= maxLen)
    {
        return s;
    }
    return s.substr(0, maxLen) + "...";
}
}

AiBlessingService *AiBlessingService::getInstance()
{
    if (!_instance)
    {
        _instance = new AiBlessingService();
    }
    return _instance;
}

void AiBlessingService::destroyInstance()
{
    CC_SAFE_DELETE(_instance);
}

void AiBlessingService::setRuntimeConfig(const Config &cfg)
{
    _runtimeConfig = cfg;
    _runtimeConfig.baseUrl = trimTrailingSlash(cfg.baseUrl);
    _hasRuntimeConfig = true;
}

void AiBlessingService::clearRuntimeConfig()
{
    _runtimeConfig = Config{};
    _hasRuntimeConfig = false;
}

AiBlessingService::Config AiBlessingService::getRuntimeConfig() const
{
    return _hasRuntimeConfig ? _runtimeConfig : Config{};
}

bool AiBlessingService::isConfigured(std::string *outHint) const
{
    Config cfg = getRuntimeConfig();
    const bool ok = !cfg.baseUrl.empty() && !cfg.apiKey.empty();
    if (!ok && outHint)
    {
        std::ostringstream oss;
        oss << "未配置 AI 服务。\n\n";
        oss << "请在赐福界面填写：\n";
        oss << "- baseUrl（例如 http://127.0.0.1:8000 或 http://127.0.0.1:8000/v1）\n";
        oss << "- apiKey（OpenAI 格式 Bearer Token）\n";
        *outHint = oss.str();
    }
    return ok;
}

void AiBlessingService::requestBlessing(const std::string &userPrompt, const BlessingCallback &cb)
{
    std::string hint;
    if (!isConfigured(&hint))
    {
        cb(false, "", Attributes{}, hint);
        return;
    }

    Config cfg = getRuntimeConfig();
    if (cfg.model.empty())
    {
        cfg.model = GameConfig::AI::Blessing::DEFAULT_MODEL;
    }

    const std::vector<BlessingRange> ranges = buildRanges();

    // 构造 system prompt：强制输出 JSON，且严格在范围内挑选
    std::ostringstream sys;
    sys << "你是游戏里的“赐福NPC”。玩家来请求赐福。\\n";
    sys << "你必须在给定属性范围内选择赐福属性，返回严格 JSON（不要输出多余文字、不要 Markdown）。\\n";
    sys << "JSON 格式必须为：{\\\"npcText\\\":string,\\\"buff\\\":[{\\\"key\\\":string,\\\"value\\\":number}...]}\\n";
    sys << "要求：\\n";
    sys << "- buff 数组长度必须等于 " << GameConfig::AI::Blessing::PICK_COUNT << "\\n";
    sys << "- key 必须来自候选列表（不要发明新 key）\\n";
    sys << "- value 必须落在对应的 [min,max] 范围内\\n";
    sys << "- npcText 用一句中文台词（不超过 30 字）\\n";
    sys << "候选列表：\\n";
    for (size_t i = 0; i < ranges.size(); ++i)
    {
        sys << "- " << ranges[i].key << " : [" << ranges[i].minValue << "," << ranges[i].maxValue << "]\\n";
    }

    const std::string user = userPrompt.empty() ? "请赐予我新的赐福（会覆盖旧赐福）。" : userPrompt;

    rapidjson::Document doc;
    doc.SetObject();
    auto &allocator = doc.GetAllocator();

    doc.AddMember("model", rapidjson::Value(cfg.model.c_str(), allocator).Move(), allocator);
    doc.AddMember("temperature", 0.6, allocator);

    rapidjson::Value messages(rapidjson::kArrayType);
    {
        rapidjson::Value m(rapidjson::kObjectType);
        m.AddMember("role", "system", allocator);
        m.AddMember("content", rapidjson::Value(sys.str().c_str(), allocator).Move(), allocator);
        messages.PushBack(m, allocator);
    }
    {
        rapidjson::Value m(rapidjson::kObjectType);
        m.AddMember("role", "user", allocator);
        m.AddMember("content", rapidjson::Value(user.c_str(), allocator).Move(), allocator);
        messages.PushBack(m, allocator);
    }
    doc.AddMember("messages", messages, allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    const std::string url = buildChatCompletionsUrl(cfg.baseUrl);
    sendJsonRequest(url, cfg.apiKey, buffer.GetString(),
                    [this, cb](bool ok, long httpCode, const std::string &respBody, const std::string &err) {
                        if (!ok)
                        {
                            std::ostringstream oss;
                            oss << "AI 请求失败";
                            if (httpCode > 0)
                            {
                                oss << "（HTTP " << httpCode << "）";
                            }
                            if (!err.empty())
                            {
                                oss << "：" << err;
                            }
                            cb(false, "", Attributes{}, oss.str());
                            return;
                        }

                        std::string npcText;
                        Attributes bonus;
                        std::string parseErr;
                        if (!parseBlessingFromResponse(respBody, npcText, bonus, parseErr))
                        {
                            std::ostringstream oss;
                            oss << "AI 响应解析失败：" << parseErr;
                            oss << "\n原始响应片段：" << shortResp(respBody);
                            cb(false, "", Attributes{}, oss.str());
                            return;
                        }

                        cb(true, npcText, bonus, "");
                    });
}

std::string AiBlessingService::trimTrailingSlash(const std::string &s)
{
    std::string out = s;
    while (!out.empty() && (out.back() == '/' || out.back() == '\\'))
    {
        out.pop_back();
    }
    return out;
}

std::string AiBlessingService::buildChatCompletionsUrl(const std::string &baseUrl)
{
    std::string trimmed = trimTrailingSlash(baseUrl);
    std::string lower = toLowerCopy(trimmed);
    if (lower.size() >= 3 && lower.rfind("/v1") == lower.size() - 3)
    {
        return trimmed + "/chat/completions";
    }
    return trimmed + "/v1/chat/completions";
}

std::string AiBlessingService::safeExtractJsonObject(const std::string &text)
{
    // 尝试从混杂输出中提取最外层 { ... }
    const size_t first = text.find('{');
    const size_t last = text.rfind('}');
    if (first == std::string::npos || last == std::string::npos || last <= first)
    {
        return "";
    }
    return text.substr(first, last - first + 1);
}

void AiBlessingService::sendJsonRequest(const std::string &url,
                                       const std::string &apiKey,
                                       const std::string &body,
                                       const std::function<void(bool ok, long httpCode, const std::string &respBody, const std::string &err)> &cb)
{
    using namespace cocos2d::network;

    auto req = new (std::nothrow) HttpRequest();
    if (!req)
    {
        cb(false, 0, "", "创建 HttpRequest 失败");
        return;
    }
    req->setUrl(url.c_str());
    req->setRequestType(HttpRequest::Type::POST);

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back("Accept: application/json");
    if (!apiKey.empty())
    {
        headers.push_back(std::string("Authorization: Bearer ") + apiKey);
    }
    req->setHeaders(headers);

    req->setRequestData(body.c_str(), body.size());

    req->setResponseCallback([cb](HttpClient * /*client*/, HttpResponse *response) {
        if (!response)
        {
            cb(false, 0, "", "response 为空");
            return;
        }

        const long httpCode = response->getResponseCode();
        if (!response->isSucceed())
        {
            std::string err = response->getErrorBuffer();
            cb(false, httpCode, "", err.empty() ? "网络请求失败" : err);
            return;
        }

        std::string respBody;
        std::vector<char> *data = response->getResponseData();
        if (data && !data->empty())
        {
            respBody.assign(data->begin(), data->end());
        }
        cb(true, httpCode, respBody, "");
    });

    HttpClient::getInstance()->send(req);
    req->release();
}

bool AiBlessingService::parseBlessingFromResponse(const std::string &respBody,
                                                 std::string &outNpcText,
                                                 Attributes &outBonus,
                                                 std::string &outErr) const
{
    outNpcText.clear();
    outBonus.clear();
    outErr.clear();

    rapidjson::Document root;
    root.Parse(respBody.c_str());
    if (root.HasParseError() || !root.IsObject())
    {
        outErr = "响应不是合法 JSON";
        return false;
    }

    if (root.HasMember("error"))
    {
        outErr = "服务端返回 error";
        if (root["error"].IsObject() && root["error"].HasMember("message") && root["error"]["message"].IsString())
        {
            outErr = root["error"]["message"].GetString();
        }
        return false;
    }

    if (!root.HasMember("choices") || !root["choices"].IsArray() || root["choices"].Empty())
    {
        outErr = "缺少 choices";
        return false;
    }

    const auto &choice0 = root["choices"][0];
    std::string content;
    if (choice0.IsObject())
    {
        if (choice0.HasMember("message") && choice0["message"].IsObject())
        {
            const auto &msg = choice0["message"];
            if (msg.HasMember("content") && msg["content"].IsString())
            {
                content = msg["content"].GetString();
            }
        }
        // 兼容某些实现：choices[0].text
        if (content.empty() && choice0.HasMember("text") && choice0["text"].IsString())
        {
            content = choice0["text"].GetString();
        }
    }

    if (content.empty())
    {
        outErr = "choices[0] 未包含 content";
        return false;
    }

    content = stripCodeFence(content);
    std::string jsonText = content;

    rapidjson::Document obj;
    obj.Parse(jsonText.c_str());
    if (obj.HasParseError())
    {
        // 尝试从混杂文本中截取 JSON
        jsonText = safeExtractJsonObject(content);
        if (jsonText.empty())
        {
            outErr = "content 不是 JSON，且无法提取 { }";
            return false;
        }
        obj.Parse(jsonText.c_str());
    }
    if (obj.HasParseError() || !obj.IsObject())
    {
        outErr = "无法解析赐福 JSON";
        return false;
    }

    if (obj.HasMember("npcText") && obj["npcText"].IsString())
    {
        outNpcText = obj["npcText"].GetString();
    }
    if (outNpcText.empty())
    {
        outNpcText = "（NPC 沉默地看着你）";
    }

    if (!obj.HasMember("buff") || !obj["buff"].IsArray())
    {
        outErr = "缺少 buff 数组";
        return false;
    }

    const std::vector<BlessingRange> ranges = buildRanges();
    auto findRange = [&](const std::string &key) -> const BlessingRange * {
        for (size_t i = 0; i < ranges.size(); ++i)
        {
            if (key == ranges[i].key)
            {
                return &ranges[i];
            }
        }
        return nullptr;
    };

    for (auto it = obj["buff"].Begin(); it != obj["buff"].End(); ++it)
    {
        if (!it->IsObject())
        {
            continue;
        }
        if (!it->HasMember("key") || !(*it)["key"].IsString())
        {
            continue;
        }
        if (!it->HasMember("value") || !(*it)["value"].IsNumber())
        {
            continue;
        }

        const std::string key = (*it)["key"].GetString();
        const float value = static_cast<float>((*it)["value"].GetDouble());

        const BlessingRange *range = findRange(key);
        if (!range)
        {
            continue;
        }

        const float clamped = std::max(range->minValue, std::min(value, range->maxValue));
        outBonus.add(range->type, clamped);
    }

    if (outBonus.values.empty())
    {
        outErr = "buff 为空或无合法 key";
        return false;
    }

    // 保底：若 AI 没按数量返回，我们仍按“可用值”接受，但 UI 会提示这次的条目数
    return true;
}

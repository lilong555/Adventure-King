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
        oss << "- baseUrl（赐福后端地址，例如 http://127.0.0.1:5181）\n";
        oss << "- apiKey（OpenAI 兼容 Bearer Token，用于后端调用 LLM）\n";
        *outHint = oss.str();
    }
    return ok;
}

void AiBlessingService::requestBlessing(const std::string &userPrompt, const BlessingCallback &cb)
{
    // 兼容接口：直接赐福（无对话），仍通过后端 answer 接口走工具调用约束
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

    const std::string npcQuestions = "（无对话）";
    const std::string playerAnswer = userPrompt.empty() ? "请赐予我赐福（覆盖旧赐福）。" : userPrompt;

    rapidjson::Document doc;
    doc.SetObject();
    auto &allocator = doc.GetAllocator();
    doc.AddMember("model", rapidjson::Value(cfg.model.c_str(), allocator).Move(), allocator);
    doc.AddMember("npc_questions", rapidjson::Value(npcQuestions.c_str(), allocator).Move(), allocator);
    doc.AddMember("player_answer", rapidjson::Value(playerAnswer.c_str(), allocator).Move(), allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    const std::string url = buildBlessingAnswerUrl(cfg.baseUrl);
    sendJsonRequest(url, cfg.apiKey, buffer.GetString(),
                    [this, cb, url](bool ok, long httpCode, const std::string &respBody, const std::string &err) {
                        if (!ok)
                        {
                            std::ostringstream oss;
                            oss << "赐福请求失败";
                            if (httpCode > 0)
                            {
                                oss << "（HTTP " << httpCode << "）";
                            }
                            if (!err.empty())
                            {
                                oss << "：" << err;
                            }
                            oss << "\n请求URL：" << url;
                            if (!respBody.empty())
                            {
                                oss << "\n响应片段：" << shortResp(respBody);
                            }
                            cb(false, "", Attributes{}, oss.str());
                            return;
                        }

                        std::string npcText;
                        Attributes bonus;
                        std::string parseErr;
                        if (!parseBlessingFromServerResponse(respBody, npcText, bonus, parseErr))
                        {
                            std::ostringstream oss;
                            oss << "赐福响应解析失败：" << parseErr;
                            oss << "\n原始响应片段：" << shortResp(respBody);
                            cb(false, "", Attributes{}, oss.str());
                            return;
                        }

                        cb(true, npcText, bonus, "");
                    });
}

void AiBlessingService::requestChallengeQuestions(const std::string &userPrompt, const ChallengeCallback &cb)
{
    std::string hint;
    if (!isConfigured(&hint))
    {
        cb(false, "", hint);
        return;
    }

    Config cfg = getRuntimeConfig();
    if (cfg.model.empty())
    {
        cfg.model = GameConfig::AI::Blessing::DEFAULT_MODEL;
    }

    const std::string user = userPrompt.empty() ? "我来请求赐福，请考验我的决心。" : userPrompt;

    rapidjson::Document doc;
    doc.SetObject();
    auto &allocator = doc.GetAllocator();
    doc.AddMember("model", rapidjson::Value(cfg.model.c_str(), allocator).Move(), allocator);
    doc.AddMember("user_prompt", rapidjson::Value(user.c_str(), allocator).Move(), allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    const std::string url = buildBlessingQuestionUrl(cfg.baseUrl);
    sendJsonRequest(url, cfg.apiKey, buffer.GetString(),
                    [this, cb, url](bool ok, long httpCode, const std::string &respBody, const std::string &err) {
                        if (!ok)
                        {
                            std::ostringstream oss;
                            oss << "生成问题失败";
                            if (httpCode > 0)
                            {
                                oss << "（HTTP " << httpCode << "）";
                            }
                            if (!err.empty())
                            {
                                oss << "：" << err;
                            }
                            oss << "\n请求URL：" << url;
                            if (!respBody.empty())
                            {
                                oss << "\n响应片段：" << shortResp(respBody);
                            }
                            cb(false, "", oss.str());
                            return;
                        }

                        std::string questions;
                        std::string parseErr;
                        if (!parseQuestionsFromServerResponse(respBody, questions, parseErr))
                        {
                            std::ostringstream oss;
                            oss << "问题解析失败：" << parseErr;
                            oss << "\n原始响应片段：" << shortResp(respBody);
                            cb(false, "", oss.str());
                            return;
                        }

                        cb(true, questions, "");
                    });
}

void AiBlessingService::requestBlessingFromDialogue(const std::string &npcQuestions,
                                                   const std::string &playerAnswer,
                                                   const BlessingCallback &cb)
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

    rapidjson::Document doc;
    doc.SetObject();
    auto &allocator = doc.GetAllocator();
    doc.AddMember("model", rapidjson::Value(cfg.model.c_str(), allocator).Move(), allocator);
    doc.AddMember("npc_questions", rapidjson::Value(npcQuestions.c_str(), allocator).Move(), allocator);
    doc.AddMember("player_answer", rapidjson::Value(playerAnswer.c_str(), allocator).Move(), allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    const std::string url = buildBlessingAnswerUrl(cfg.baseUrl);
    sendJsonRequest(url, cfg.apiKey, buffer.GetString(),
                    [this, cb, url](bool ok, long httpCode, const std::string &respBody, const std::string &err) {
                        if (!ok)
                        {
                            std::ostringstream oss;
                            oss << "赐福请求失败";
                            if (httpCode > 0)
                            {
                                oss << "（HTTP " << httpCode << "）";
                            }
                            if (!err.empty())
                            {
                                oss << "：" << err;
                            }
                            oss << "\n请求URL：" << url;
                            if (!respBody.empty())
                            {
                                oss << "\n响应片段：" << shortResp(respBody);
                            }
                            cb(false, "", Attributes{}, oss.str());
                            return;
                        }

                        std::string npcText;
                        Attributes bonus;
                        std::string parseErr;
                        if (!parseBlessingFromServerResponse(respBody, npcText, bonus, parseErr))
                        {
                            std::ostringstream oss;
                            oss << "赐福响应解析失败：" << parseErr;
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

std::string AiBlessingService::buildBlessingQuestionUrl(const std::string &baseUrl)
{
    // baseUrl 为赐福后端根地址（不带 /api 前缀也可）
    const std::string trimmed = trimTrailingSlash(baseUrl);
    return trimmed + "/api/blessing/question";
}

std::string AiBlessingService::buildBlessingAnswerUrl(const std::string &baseUrl)
{
    const std::string trimmed = trimTrailingSlash(baseUrl);
    return trimmed + "/api/blessing/answer";
}

void AiBlessingService::sendJsonRequest(const std::string &url,
                                       const std::string &apiKey,
                                       const std::string &body,
                                       const std::function<void(bool ok, long httpCode, const std::string &respBody, const std::string &err)> &cb)
{
    using namespace cocos2d::network;

    // 安全提示：若使用 HTTP（非 HTTPS）发送 Bearer Token，会明文传输，存在被窃听风险。
    // 本项目展示阶段允许本地/自建服务使用 HTTP，但会在日志中提示风险。
    if (!apiKey.empty())
    {
        const std::string lower = toLowerCopy(url);
        const bool isHttps = lower.rfind("https://", 0) == 0;
        const bool isHttp = lower.rfind("http://", 0) == 0;
        const bool isLocal = lower.rfind("http://127.0.0.1", 0) == 0 || lower.rfind("http://localhost", 0) == 0;
        if (isHttp && !isHttps)
        {
            if (isLocal)
            {
                CCLOG("AiBlessingService - 提示：当前使用 HTTP 本地服务，apiKey 将明文传输，仅建议本地开发环境使用。");
            }
            else
            {
                CCLOG("AiBlessingService - 警告：当前使用非 HTTPS 服务，apiKey 将明文传输，存在泄露风险：%s", url.c_str());
            }
        }
    }

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

        // 即使失败，也尽量把响应体带回（很多服务会在 4xx/5xx 中返回 JSON 错误信息）
        std::string respBody;
        std::vector<char> *data = response->getResponseData();
        if (data && !data->empty())
        {
            respBody.assign(data->begin(), data->end());
        }

        if (!response->isSucceed())
        {
            std::string err = response->getErrorBuffer();
            cb(false, httpCode, respBody, err.empty() ? "网络请求失败" : err);
            return;
        }
        cb(true, httpCode, respBody, "");
    });

    HttpClient::getInstance()->send(req);
    req->release();
}

bool AiBlessingService::parseBlessingFromServerResponse(const std::string &respBody,
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

    // 统一后端格式：{ok:bool, data?:object, error?:string}
    if (root.HasMember("ok") && root["ok"].IsBool() && !root["ok"].GetBool())
    {
        outErr = "服务端返回失败";
        if (root.HasMember("error") && root["error"].IsString())
        {
            outErr = root["error"].GetString();
        }
        return false;
    }

    if (!root.HasMember("data") || !root["data"].IsObject())
    {
        outErr = "缺少 data";
        return false;
    }

    const auto &data = root["data"];

    if (data.HasMember("npcText") && data["npcText"].IsString())
    {
        outNpcText = data["npcText"].GetString();
    }
    if (outNpcText.empty())
    {
        outNpcText = "（NPC 沉默地看着你）";
    }

    if (!data.HasMember("buff") || !data["buff"].IsArray())
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

    for (auto it = data["buff"].Begin(); it != data["buff"].End(); ++it)
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

        // 客户端二次校验：越界直接忽略（避免服务端异常/被篡改）
        if (value < range->minValue || value > range->maxValue)
        {
            continue;
        }
        outBonus.add(range->type, value);
    }

    if (outBonus.values.empty())
    {
        outErr = "buff 为空或无合法 key";
        return false;
    }

    return true;
}

bool AiBlessingService::parseQuestionsFromServerResponse(const std::string &respBody,
                                                         std::string &outQuestions,
                                                         std::string &outErr) const
{
    outQuestions.clear();
    outErr.clear();

    rapidjson::Document root;
    root.Parse(respBody.c_str());
    if (root.HasParseError() || !root.IsObject())
    {
        outErr = "响应不是合法 JSON";
        return false;
    }

    if (root.HasMember("ok") && root["ok"].IsBool() && !root["ok"].GetBool())
    {
        outErr = "服务端返回失败";
        if (root.HasMember("error") && root["error"].IsString())
        {
            outErr = root["error"].GetString();
        }
        return false;
    }

    if (!root.HasMember("data") || !root["data"].IsObject())
    {
        outErr = "缺少 data";
        return false;
    }

    const auto &data = root["data"];
    if (!data.HasMember("npcQuestions") || !data["npcQuestions"].IsString())
    {
        outErr = "缺少 npcQuestions";
        return false;
    }

    outQuestions = data["npcQuestions"].GetString();
    outQuestions = stripCodeFence(outQuestions);
    if (outQuestions.empty())
    {
        outErr = "npcQuestions 为空";
        return false;
    }
    return true;
}

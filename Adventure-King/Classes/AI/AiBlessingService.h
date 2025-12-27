#pragma once

#include "Character/Base/CharacterData.h"
#include "network/HttpClient.h"
#include <functional>
#include <string>

/**
 * @brief AI 赐福服务（LangChain 后端）
 *
 * 设计目标（展示阶段）：
 * - 游戏内由玩家手动填写：赐福后端 baseUrl + OpenAI 兼容 apiKey（不写死在代码里）
 * - 客户端调用后端接口：
 *   - POST {baseUrl}/api/blessing/question 生成考验问题
 *   - POST {baseUrl}/api/blessing/answer   根据回答返回赐福（工具调用约束）
 * - 后端再通过 LangChain 调用 OpenAI 兼容接口（base url 由后端环境变量配置）
 * - 客户端仍会二次校验范围，确保数值安全；赐福为覆盖式 buff
 */
class AiBlessingService final
{
public:
    struct Config
    {
        std::string baseUrl; // 赐福后端地址，例如 http://127.0.0.1:5181
        std::string apiKey;  // OpenAI 兼容 Bearer token（展示阶段由玩家填写；用于后端调用 LLM）
        std::string model;   // 例如 gemini-3-flash-preview（取决于你使用的网关/模型提供方）
    };

    using BlessingCallback = std::function<void(bool ok,
                                                const std::string &npcText,
                                                const Attributes &bonus,
                                                const std::string &err)>;

    using ChallengeCallback = std::function<void(bool ok,
                                                 const std::string &npcQuestions,
                                                 const std::string &err)>;

    static AiBlessingService *getInstance();
    static void destroyInstance();

    /// @brief 设置运行时配置（进程内；展示阶段不做持久化）
    void setRuntimeConfig(const Config &cfg);
    /// @brief 清空运行时配置
    void clearRuntimeConfig();
    /// @brief 获取当前运行时配置（未配置则为空）
    Config getRuntimeConfig() const;

    /// @brief 是否已配置（baseUrl + apiKey）
    bool isConfigured(std::string *outHint = nullptr) const;

    /// @brief 发送赐福请求（userPrompt 可为空，空则使用默认文案）
    void requestBlessing(const std::string &userPrompt, const BlessingCallback &cb);

    /// @brief 赐福对话阶段1：让 AI 先提出“考验冒险决心”的问题
    void requestChallengeQuestions(const std::string &userPrompt, const ChallengeCallback &cb);

    /// @brief 赐福对话阶段2：玩家回答后，AI 给出赐福（严格 JSON）
    void requestBlessingFromDialogue(const std::string &npcQuestions,
                                    const std::string &playerAnswer,
                                    const BlessingCallback &cb);

private:
    AiBlessingService() = default;
    ~AiBlessingService() = default;
    AiBlessingService(const AiBlessingService &) = delete;
    AiBlessingService &operator=(const AiBlessingService &) = delete;

    static std::string trimTrailingSlash(const std::string &s);
    static std::string buildBlessingQuestionUrl(const std::string &baseUrl);
    static std::string buildBlessingAnswerUrl(const std::string &baseUrl);

    void sendJsonRequest(const std::string &url,
                         const std::string &apiKey,
                         const std::string &body,
                         const std::function<void(bool ok, long httpCode, const std::string &respBody, const std::string &err)> &cb);

    bool parseBlessingFromServerResponse(const std::string &respBody,
                                         std::string &outNpcText,
                                         Attributes &outBonus,
                                         std::string &outErr) const;

    bool parseQuestionsFromServerResponse(const std::string &respBody,
                                          std::string &outQuestions,
                                          std::string &outErr) const;

    Config _runtimeConfig;
    bool _hasRuntimeConfig = false;

    static AiBlessingService *_instance;
};

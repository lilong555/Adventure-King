#pragma once

#include "Character/Base/CharacterData.h"
#include "network/HttpClient.h"
#include <functional>
#include <string>

/**
 * @brief AI 赐福服务（OpenAI 兼容 ChatCompletions）
 *
 * 设计目标（展示阶段）：
 * - 游戏内由玩家手动填写 baseUrl + apiKey（不写死在代码里）
 * - 走 OpenAI 格式 API：POST {baseUrl}/v1/chat/completions
 * - 由 AI 在“写死的属性范围”内选择赐福属性，并返回结构化 JSON
 * - 客户端只负责解析与应用 Buff（覆盖旧赐福）
 */
class AiBlessingService final
{
public:
    struct Config
    {
        std::string baseUrl; // 例如 http://127.0.0.1:8000 或 http://127.0.0.1:8000/v1
        std::string apiKey;  // Bearer token（展示阶段由玩家填写）
        std::string model;   // 例如 gpt-4o-mini（可选）
    };

    using BlessingCallback = std::function<void(bool ok,
                                                const std::string &npcText,
                                                const Attributes &bonus,
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

private:
    AiBlessingService() = default;
    ~AiBlessingService() = default;
    AiBlessingService(const AiBlessingService &) = delete;
    AiBlessingService &operator=(const AiBlessingService &) = delete;

    static std::string trimTrailingSlash(const std::string &s);
    static std::string buildChatCompletionsUrl(const std::string &baseUrl);
    static std::string safeExtractJsonObject(const std::string &text);

    void sendJsonRequest(const std::string &url,
                         const std::string &apiKey,
                         const std::string &body,
                         const std::function<void(bool ok, long httpCode, const std::string &respBody, const std::string &err)> &cb);

    bool parseBlessingFromResponse(const std::string &respBody,
                                  std::string &outNpcText,
                                  Attributes &outBonus,
                                  std::string &outErr) const;

    Config _runtimeConfig;
    bool _hasRuntimeConfig = false;

    static AiBlessingService *_instance;
};


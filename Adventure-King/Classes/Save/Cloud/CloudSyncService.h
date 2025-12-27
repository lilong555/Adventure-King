#pragma once

#include "Save/SaveManager.h"
#include "cocos2d.h"
#include "network/HttpClient.h"
#include <functional>
#include <string>

/**
 * 云端同步服务（HTTP）
 *
 * 目标：
 * - “云存”：把本地【全部存档 + 设置】打包上传到云端账号
 * - “云同步”：从云端拉取存档包，与本地按时间戳合并（取最新），再回传合并结果
 *
 * 配置方式（不在代码里写死任何服务器地址/IP）：
 * - AK_CLOUD_SYNC_URL  ：例如 http://127.0.0.1:5174
 * - AK_CLOUD_SYNC_USER ：用户名
 * - AK_CLOUD_SYNC_PASS ：密码
 */
class CloudSyncService final
{
public:
    using ResultCallback = std::function<void(bool ok, const std::string &message)>;

    static CloudSyncService *getInstance();
    static void destroyInstance();

    // ==========================================================
    // 账号状态（会话级：不落盘）
    // ==========================================================

    /// @brief 设置游客模式：启用后禁用云端功能（不读取环境变量/不使用运行时账号）
    void setGuestMode(bool enabled);
    /// @brief 当前是否为游客模式
    bool isGuestMode() const;
    /// @brief 设置运行时账号配置（用于主菜单“登录/注册”弹窗）
    void setRuntimeAccount(const std::string &baseUrl,
                           const std::string &username,
                           const std::string &password);
    /// @brief 清空运行时账号配置（回退到环境变量配置）
    void clearRuntimeAccount();
    /// @brief 当前账号名（游客/未配置则为空）
    std::string getActiveUsername() const;

    // 当前是否已配置云同步（URL/账号/密码）
    bool isConfigured(std::string *outHint = nullptr) const;

    /// @brief 登录（成功后会写入运行时账号配置并缓存 token）
    void login(const std::string &baseUrl,
               const std::string &username,
               const std::string &password,
               const ResultCallback &cb);
    /// @brief 注册并自动登录（成功后会写入运行时账号配置并缓存 token）
    void registerAndLogin(const std::string &baseUrl,
                          const std::string &username,
                          const std::string &password,
                          const ResultCallback &cb);

    // 仅上传（覆盖云端该用户的存档包）
    void uploadAllSaves(const ResultCallback &cb);

    // 一键同步：云端拉取 -> 与本地合并（取最新）-> 上传合并后的本地结果
    void syncAll(const ResultCallback &cb);

private:
    CloudSyncService() = default;
    ~CloudSyncService() = default;
    CloudSyncService(const CloudSyncService &) = delete;
    CloudSyncService &operator=(const CloudSyncService &) = delete;

    struct Config
    {
        std::string baseUrl;
        std::string user;
        std::string pass;
    };

    static int64_t nowMs();
    static std::string trimTrailingSlash(const std::string &s);

    Config loadConfig(std::string *outErr) const;

    void ensureLogin(const Config &cfg, const std::function<void(bool ok, const std::string &token, const std::string &err)> &cb);
    void sendJsonRequest(const std::string &method,
                         const std::string &url,
                         const std::string &body,
                         const std::vector<std::string> &headers,
                         const std::function<void(bool ok, long httpCode, const std::string &respBody, const std::string &err)> &cb);
    void sendAuthedJsonRequestWithRetry(const Config &cfg,
                                        const std::string &method,
                                        const std::string &path,
                                        const std::string &body,
                                        const std::function<void(bool ok, long httpCode, const std::string &respBody, const std::string &err)> &cb,
                                        bool hasRetriedAuth = false);

    bool buildLocalPackageJson(std::string &outJson, std::string &outErr) const;
    bool applyRemotePackageMergeToLocal(const std::string &packageJson, std::string &outErr, bool &outLocalChanged);

    // 简单的 token 缓存（进程内）
    std::string _token;
    int64_t _tokenExpireAtMs = 0;

    // 会话级账号状态：不落盘
    bool _guestMode = false;
    bool _hasRuntimeAccount = false;
    Config _runtimeAccount;

    static CloudSyncService *_instance;
};

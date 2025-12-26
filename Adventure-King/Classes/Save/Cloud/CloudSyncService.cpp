#include "Save/Cloud/CloudSyncService.h"
#include "Save/JsonSerializer.h"
#include "cocos2d.h"
#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <sstream>

USING_NS_CC;

namespace
{
static const char *const ENV_SYNC_URL = "AK_CLOUD_SYNC_URL";
static const char *const ENV_SYNC_USER = "AK_CLOUD_SYNC_USER";
static const char *const ENV_SYNC_PASS = "AK_CLOUD_SYNC_PASS";

static std::string getEnvOrEmpty(const char *key)
{
    const char *val = std::getenv(key);
    if (!val || !val[0])
    {
        return "";
    }
    return std::string(val);
}

static std::string buildUrl(const std::string &baseUrl, const std::string &path)
{
    if (baseUrl.empty())
    {
        return path;
    }
    if (path.empty())
    {
        return baseUrl;
    }

    std::string base = baseUrl;
    if (base.back() == '/')
    {
        base.pop_back();
    }

    if (path.front() == '/')
    {
        return base + path;
    }
    return base + "/" + path;
}

static std::string jsonStringify(const rapidjson::Value &val)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    val.Accept(writer);
    return std::string(buffer.GetString(), buffer.GetSize());
}

static bool parseJsonObject(const std::string &json, rapidjson::Document &outDoc, std::string &outErr)
{
    outDoc.Parse(json.c_str());
    if (outDoc.HasParseError() || !outDoc.IsObject())
    {
        outErr = "JSON 解析失败";
        return false;
    }
    return true;
}

static int64_t safeGetSaveTimestampFromSlotJsonValue(const rapidjson::Value &slotObj)
{
    if (!slotObj.IsObject())
    {
        return 0;
    }
    if (!slotObj.HasMember("meta") || !slotObj["meta"].IsObject())
    {
        return 0;
    }
    const auto &meta = slotObj["meta"];
    if (meta.HasMember("saveTimestamp") && meta["saveTimestamp"].IsInt64())
    {
        return meta["saveTimestamp"].GetInt64();
    }
    if (meta.HasMember("saveTimestamp") && meta["saveTimestamp"].IsNumber())
    {
        return static_cast<int64_t>(meta["saveTimestamp"].GetDouble());
    }
    return 0;
}
}

CloudSyncService *CloudSyncService::_instance = nullptr;

CloudSyncService *CloudSyncService::getInstance()
{
    if (!_instance)
    {
        _instance = new CloudSyncService();
    }
    return _instance;
}

void CloudSyncService::destroyInstance()
{
    CC_SAFE_DELETE(_instance);
}

int64_t CloudSyncService::nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string CloudSyncService::trimTrailingSlash(const std::string &s)
{
    std::string out = s;
    while (!out.empty() && (out.back() == '/' || out.back() == '\\'))
    {
        out.pop_back();
    }
    return out;
}

CloudSyncService::Config CloudSyncService::loadConfig(std::string *outErr) const
{
    Config cfg;
    cfg.baseUrl = trimTrailingSlash(getEnvOrEmpty(ENV_SYNC_URL));
    cfg.user = getEnvOrEmpty(ENV_SYNC_USER);
    cfg.pass = getEnvOrEmpty(ENV_SYNC_PASS);

    if (outErr)
    {
        outErr->clear();
        if (cfg.baseUrl.empty())
        {
            *outErr = "未配置 AK_CLOUD_SYNC_URL";
        }
        else if (cfg.user.empty())
        {
            *outErr = "未配置 AK_CLOUD_SYNC_USER";
        }
        else if (cfg.pass.empty())
        {
            *outErr = "未配置 AK_CLOUD_SYNC_PASS";
        }
    }

    return cfg;
}

bool CloudSyncService::isConfigured(std::string *outHint) const
{
    std::string err;
    const Config cfg = loadConfig(&err);
    const bool ok = !cfg.baseUrl.empty() && !cfg.user.empty() && !cfg.pass.empty();
    if (!ok && outHint)
    {
        std::ostringstream oss;
        oss << "未配置云端同步。\n\n";
        oss << "请设置环境变量：\n";
        oss << "- AK_CLOUD_SYNC_URL（例如 http://127.0.0.1:5173）\n";
        oss << "- AK_CLOUD_SYNC_USER（用户名）\n";
        oss << "- AK_CLOUD_SYNC_PASS（密码）\n\n";
        oss << "当前缺失： " << err;
        *outHint = oss.str();
    }
    return ok;
}

void CloudSyncService::sendJsonRequest(const std::string &method,
                                       const std::string &url,
                                       const std::string &body,
                                       const std::vector<std::string> &headers,
                                       const std::function<void(bool ok, long httpCode, const std::string &respBody, const std::string &err)> &cb)
{
    using namespace cocos2d::network;

    auto req = new (std::nothrow) HttpRequest();
    if (!req)
    {
        cb(false, 0, "", "创建 HttpRequest 失败");
        return;
    }

    if (method == "GET")
    {
        req->setRequestType(HttpRequest::Type::GET);
    }
    else if (method == "POST")
    {
        req->setRequestType(HttpRequest::Type::POST);
    }
    else
    {
        req->release();
        cb(false, 0, "", "不支持的 HTTP 方法: " + method);
        return;
    }

    req->setUrl(url.c_str());

    std::vector<std::string> finalHeaders = headers;
    finalHeaders.emplace_back("Content-Type: application/json; charset=utf-8");
    req->setHeaders(finalHeaders);

    if (!body.empty())
    {
        req->setRequestData(body.c_str(), body.size());
    }

    req->setResponseCallback([cb](HttpClient *, HttpResponse *response) {
        if (!response)
        {
            cb(false, 0, "", "HttpResponse 为空");
            return;
        }

        const long code = response->getResponseCode();
        std::vector<char> *data = response->getResponseData();
        std::string respBody;
        if (data && !data->empty())
        {
            respBody.assign(data->begin(), data->end());
        }

        if (!response->isSucceed())
        {
            std::string err = response->getErrorBuffer();
            if (err.empty())
            {
                err = "HTTP 请求失败";
            }
            cb(false, code, respBody, err);
            return;
        }

        cb(true, code, respBody, "");
    });

    HttpClient::getInstance()->send(req);
    req->release();
}

void CloudSyncService::ensureLogin(const Config &cfg,
                                  const std::function<void(bool ok, const std::string &token, const std::string &err)> &cb)
{
    const int64_t now = nowMs();
    if (!_token.empty() && now + 5000 < _tokenExpireAtMs)
    {
        cb(true, _token, "");
        return;
    }

    // 组装登录 JSON
    rapidjson::Document doc;
    doc.SetObject();
    auto &alloc = doc.GetAllocator();
    doc.AddMember("username", rapidjson::Value(cfg.user.c_str(), alloc).Move(), alloc);
    doc.AddMember("password", rapidjson::Value(cfg.pass.c_str(), alloc).Move(), alloc);

    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    doc.Accept(writer);
    const std::string body(buf.GetString(), buf.GetSize());

    const std::string url = buildUrl(cfg.baseUrl, "/api/login");
    sendJsonRequest("POST", url, body, {}, [this, cb](bool ok, long httpCode, const std::string &respBody, const std::string &err) {
        if (!ok)
        {
            std::string serverMsg = err;
            rapidjson::Document errDoc;
            std::string parseErr;
            if (parseJsonObject(respBody, errDoc, parseErr) &&
                errDoc.HasMember("message") && errDoc["message"].IsString())
            {
                serverMsg = errDoc["message"].GetString();
            }

            std::ostringstream oss;
            oss << "登录失败(" << httpCode << "): " << serverMsg;
            cb(false, "", oss.str());
            return;
        }

        rapidjson::Document resp;
        std::string parseErr;
        if (!parseJsonObject(respBody, resp, parseErr))
        {
            cb(false, "", "登录返回解析失败");
            return;
        }

        if (!resp.HasMember("token") || !resp["token"].IsString())
        {
            cb(false, "", "登录返回缺少 token");
            return;
        }

        const std::string token = resp["token"].GetString();
        int expiresSec = 3600;
        if (resp.HasMember("expiresInSeconds") && resp["expiresInSeconds"].IsInt())
        {
            expiresSec = resp["expiresInSeconds"].GetInt();
        }

        _token = token;
        _tokenExpireAtMs = nowMs() + static_cast<int64_t>(expiresSec) * 1000;
        cb(true, _token, "");
    });
}

bool CloudSyncService::buildLocalPackageJson(std::string &outJson, std::string &outErr) const
{
    auto saveManager = SaveManager::getInstance();

    rapidjson::Document doc;
    doc.SetObject();
    auto &alloc = doc.GetAllocator();

    doc.AddMember("schemaVersion", 1, alloc);
    doc.AddMember("uploadedAt", nowMs(), alloc);
    doc.AddMember("client", "Adventure-King", alloc);

    // 设置
    {
        std::string settingsJson;
        if (saveManager->exportSettingsToJsonString(settingsJson) && !settingsJson.empty())
        {
            rapidjson::Document settingsDoc;
            std::string parseErr;
            if (parseJsonObject(settingsJson, settingsDoc, parseErr))
            {
                rapidjson::Value settingsVal;
                settingsVal.CopyFrom(settingsDoc, alloc);
                doc.AddMember("settings", settingsVal, alloc);
            }
            else
            {
                // 兜底：不影响云同步主流程
                doc.AddMember("settings", rapidjson::Value(rapidjson::kNullType), alloc);
            }
        }
        else
        {
            doc.AddMember("settings", rapidjson::Value(rapidjson::kNullType), alloc);
        }
    }

    // 存档槽
    rapidjson::Value savesObj(rapidjson::kObjectType);
    for (int slot = 0; slot < SaveManager::MAX_SAVE_SLOTS; ++slot)
    {
        std::string slotJson;
        if (!saveManager->exportSaveSlotToJsonString(slot, slotJson) || slotJson.empty())
        {
            continue;
        }

        rapidjson::Document slotDoc;
        std::string parseErr;
        if (!parseJsonObject(slotJson, slotDoc, parseErr))
        {
            continue;
        }

        rapidjson::Value slotVal;
        slotVal.CopyFrom(slotDoc, alloc);

        std::string key = std::to_string(slot);
        savesObj.AddMember(rapidjson::Value(key.c_str(), alloc).Move(), slotVal, alloc);
    }
    doc.AddMember("saves", savesObj, alloc);

    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buf);
    doc.Accept(writer);
    outJson.assign(buf.GetString(), buf.GetSize());

    if (outJson.empty())
    {
        outErr = "打包 JSON 为空";
        return false;
    }
    return true;
}

bool CloudSyncService::applyRemotePackageMergeToLocal(const std::string &packageJson, std::string &outErr, bool &outLocalChanged)
{
    outLocalChanged = false;
    outErr.clear();

    rapidjson::Document pkg;
    std::string parseErr;
    if (!parseJsonObject(packageJson, pkg, parseErr))
    {
        outErr = "云端包解析失败";
        return false;
    }

    if (!pkg.HasMember("saves") || !pkg["saves"].IsObject())
    {
        outErr = "云端包缺少 saves";
        return false;
    }

    auto saveManager = SaveManager::getInstance();
    const auto localInfos = saveManager->getAllSaveSlotInfos();
    std::vector<int64_t> localTs(SaveManager::MAX_SAVE_SLOTS, 0);
    for (const auto &info : localInfos)
    {
        if (info.slotIndex >= 0 && info.slotIndex < SaveManager::MAX_SAVE_SLOTS)
        {
            localTs[info.slotIndex] = info.saveTimestamp;
        }
    }

    const auto &savesObj = pkg["saves"];
    for (auto it = savesObj.MemberBegin(); it != savesObj.MemberEnd(); ++it)
    {
        if (!it->name.IsString())
        {
            continue;
        }

        int slotIndex = -1;
        try
        {
            slotIndex = std::stoi(it->name.GetString());
        }
        catch (...)
        {
            continue;
        }
        if (slotIndex < 0 || slotIndex >= SaveManager::MAX_SAVE_SLOTS)
        {
            continue;
        }

        const rapidjson::Value &slotVal = it->value;
        if (!slotVal.IsObject())
        {
            continue;
        }

        const int64_t remoteTs = safeGetSaveTimestampFromSlotJsonValue(slotVal);
        const int64_t localTimestamp = localTs[slotIndex];

        // 合并策略：按时间戳取最新
        if (remoteTs <= 0)
        {
            continue;
        }
        if (localTimestamp >= remoteTs && localTimestamp > 0)
        {
            continue;
        }

        const std::string slotJson = jsonStringify(slotVal);
        if (!saveManager->importSaveSlotFromJsonString(slotIndex, slotJson, true))
        {
            outErr = "导入云端槽位失败: " + std::to_string(slotIndex);
            return false;
        }

        localTs[slotIndex] = remoteTs;
        outLocalChanged = true;
    }

    // 设置：默认以云端为准（若存在）
    if (pkg.HasMember("settings") && pkg["settings"].IsObject())
    {
        const std::string settingsJson = jsonStringify(pkg["settings"]);
        if (!settingsJson.empty())
        {
            // 不强制失败：设置问题不应阻断云同步
            if (saveManager->importSettingsFromJsonString(settingsJson))
            {
                outLocalChanged = true;
            }
        }
    }

    return true;
}

void CloudSyncService::uploadAllSaves(const ResultCallback &cb)
{
    std::string cfgErr;
    const Config cfg = loadConfig(&cfgErr);
    if (!cfgErr.empty())
    {
        cb(false, cfgErr);
        return;
    }

    ensureLogin(cfg, [this, cfg, cb](bool ok, const std::string &token, const std::string &err) {
        if (!ok)
        {
            cb(false, err);
            return;
        }

        std::string pkgJson;
        std::string pkgErr;
        if (!buildLocalPackageJson(pkgJson, pkgErr))
        {
            cb(false, pkgErr);
            return;
        }

        std::vector<std::string> headers;
        headers.emplace_back("Authorization: Bearer " + token);

        const std::string url = buildUrl(cfg.baseUrl, "/api/sync/push");
        sendJsonRequest("POST", url, pkgJson, headers, [cb](bool ok2, long code, const std::string &, const std::string &err2) {
            if (!ok2)
            {
                std::ostringstream oss;
                oss << "云存失败(" << code << "): " << err2;
                cb(false, oss.str());
                return;
            }
            cb(true, "云存成功");
        });
    });
}

void CloudSyncService::syncAll(const ResultCallback &cb)
{
    std::string cfgErr;
    const Config cfg = loadConfig(&cfgErr);
    if (!cfgErr.empty())
    {
        cb(false, cfgErr);
        return;
    }

    ensureLogin(cfg, [this, cfg, cb](bool ok, const std::string &token, const std::string &err) {
        if (!ok)
        {
            cb(false, err);
            return;
        }

        std::vector<std::string> headers;
        headers.emplace_back("Authorization: Bearer " + token);

        const std::string pullUrl = buildUrl(cfg.baseUrl, "/api/sync/pull");
        sendJsonRequest("GET", pullUrl, "", headers, [this, cfg, token, cb](bool ok2, long code, const std::string &respBody, const std::string &err2) {
            // 允许云端为空（404）：此时直接上传本地作为初始化
            bool localChanged = false;
            if (ok2)
            {
                std::string mergeErr;
                if (!applyRemotePackageMergeToLocal(respBody, mergeErr, localChanged))
                {
                    cb(false, mergeErr);
                    return;
                }
            }
            else if (code != 404)
            {
                std::ostringstream oss;
                oss << "云同步拉取失败(" << code << "): " << err2;
                cb(false, oss.str());
                return;
            }

            // 回传合并后的本地结果（保证“同步”两端一致）
            std::string pkgJson;
            std::string pkgErr;
            if (!buildLocalPackageJson(pkgJson, pkgErr))
            {
                cb(false, pkgErr);
                return;
            }

            std::vector<std::string> headers2;
            headers2.emplace_back("Authorization: Bearer " + token);
            const std::string pushUrl = buildUrl(cfg.baseUrl, "/api/sync/push");
            sendJsonRequest("POST", pushUrl, pkgJson, headers2, [cb, localChanged](bool ok3, long code3, const std::string &, const std::string &err3) {
                if (!ok3)
                {
                    std::ostringstream oss;
                    oss << "云同步回传失败(" << code3 << "): " << err3;
                    cb(false, oss.str());
                    return;
                }

                cb(true, localChanged ? "云同步成功（已合并云端更新）" : "云同步成功");
            });
        });
    });
}

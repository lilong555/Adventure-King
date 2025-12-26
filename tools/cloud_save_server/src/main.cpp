#include "third_party/cpp-httplib/httplib.h"

#include "json/document.h"
#include "json/stringbuffer.h"
#include "json/writer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace
{
static int64_t nowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

static std::string toHex(const std::vector<uint8_t> &bytes)
{
    std::ostringstream oss;
    for (uint8_t b : bytes)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

static std::string randomHex(size_t numBytes)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 255);

    std::vector<uint8_t> bytes;
    bytes.reserve(numBytes);
    for (size_t i = 0; i < numBytes; ++i)
    {
        bytes.push_back(static_cast<uint8_t>(dis(gen)));
    }
    return toHex(bytes);
}

static bool readTextFile(const fs::path &path, std::string &out)
{
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs.is_open())
    {
        return false;
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}

static bool writeTextFileAtomic(const fs::path &path, const std::string &content, std::string &outErr)
{
    outErr.clear();
    const fs::path tmp = path.string() + ".tmp";

    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    if (ec)
    {
        outErr = "创建目录失败: " + ec.message();
        return false;
    }

    {
        std::ofstream ofs(tmp, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!ofs.is_open())
        {
            outErr = "打开临时文件失败";
            return false;
        }
        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
        ofs.close();
    }

    // Windows 上 rename 目标存在会失败，这里先删除
    fs::remove(path, ec);
    ec.clear();
    fs::rename(tmp, path, ec);
    if (ec)
    {
        outErr = "重命名失败: " + ec.message();
        return false;
    }
    return true;
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

// ======================== SHA-256（最小实现，用于密码哈希） ========================
// 说明：这里只用于“演示环境”的密码存储（salt + sha256），避免明文落盘。
// 若用于生产，请替换为成熟密码哈希算法（bcrypt/argon2）并加入更完整的安全策略。

class Sha256
{
public:
    static std::string hexDigest(const std::string &input)
    {
        Sha256 ctx;
        ctx.update(reinterpret_cast<const uint8_t *>(input.data()), input.size());
        std::array<uint8_t, 32> digest = ctx.finalize();
        std::vector<uint8_t> v(digest.begin(), digest.end());
        return toHex(v);
    }

private:
    static constexpr uint32_t k[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    static uint32_t bsig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    static uint32_t bsig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    static uint32_t ssig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static uint32_t ssig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    void update(const uint8_t *data, size_t len)
    {
        _bitLen += static_cast<uint64_t>(len) * 8;
        for (size_t i = 0; i < len; ++i)
        {
            _buffer[_bufferLen++] = data[i];
            if (_bufferLen == 64)
            {
                transform(_buffer.data());
                _bufferLen = 0;
            }
        }
    }

    std::array<uint8_t, 32> finalize()
    {
        // padding
        _buffer[_bufferLen++] = 0x80;
        if (_bufferLen > 56)
        {
            while (_bufferLen < 64)
            {
                _buffer[_bufferLen++] = 0;
            }
            transform(_buffer.data());
            _bufferLen = 0;
        }
        while (_bufferLen < 56)
        {
            _buffer[_bufferLen++] = 0;
        }

        // append length (big endian)
        for (int i = 7; i >= 0; --i)
        {
            _buffer[_bufferLen++] = static_cast<uint8_t>((_bitLen >> (i * 8)) & 0xff);
        }
        transform(_buffer.data());

        std::array<uint8_t, 32> out{};
        for (int i = 0; i < 8; ++i)
        {
            out[i * 4 + 0] = static_cast<uint8_t>((_state[i] >> 24) & 0xff);
            out[i * 4 + 1] = static_cast<uint8_t>((_state[i] >> 16) & 0xff);
            out[i * 4 + 2] = static_cast<uint8_t>((_state[i] >> 8) & 0xff);
            out[i * 4 + 3] = static_cast<uint8_t>((_state[i] >> 0) & 0xff);
        }
        return out;
    }

    void transform(const uint8_t block[64])
    {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
        {
            w[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
                   (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(block[i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i)
        {
            w[i] = ssig1(w[i - 2]) + w[i - 7] + ssig0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = _state[0];
        uint32_t b = _state[1];
        uint32_t c = _state[2];
        uint32_t d = _state[3];
        uint32_t e = _state[4];
        uint32_t f = _state[5];
        uint32_t g = _state[6];
        uint32_t h = _state[7];

        for (int i = 0; i < 64; ++i)
        {
            uint32_t t1 = h + bsig1(e) + ch(e, f, g) + k[i] + w[i];
            uint32_t t2 = bsig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        _state[0] += a;
        _state[1] += b;
        _state[2] += c;
        _state[3] += d;
        _state[4] += e;
        _state[5] += f;
        _state[6] += g;
        _state[7] += h;
    }

    uint64_t _bitLen = 0;
    std::array<uint8_t, 64> _buffer{};
    size_t _bufferLen = 0;

    std::array<uint32_t, 8> _state = {
        0x6a09e667u,
        0xbb67ae85u,
        0x3c6ef372u,
        0xa54ff53au,
        0x510e527fu,
        0x9b05688cu,
        0x1f83d9abu,
        0x5be0cd19u};
};

// ======================== 用户/会话/存档存储 ========================

struct UserRecord
{
    std::string username;
    std::string salt;
    std::string passwordHash;
    int64_t createdAtMs = 0;
};

struct UserListItem
{
    std::string username;
    int64_t createdAtMs = 0;
};

static bool isValidUsername(const std::string &s)
{
    if (s.size() < 3 || s.size() > 32)
    {
        return false;
    }
    for (char c : s)
    {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_');
        if (!ok)
        {
            return false;
        }
    }
    return true;
}

static std::string hashPassword(const std::string &salt, const std::string &password)
{
    // 说明：演示用 sha256(salt + ":" + password)
    return Sha256::hexDigest(salt + ":" + password);
}

class UserStore
{
public:
    explicit UserStore(fs::path root)
        : _root(std::move(root)), _dbPath(_root / "users.json")
    {
        load();
    }

    bool registerUser(const std::string &username, const std::string &password, std::string &outErr)
    {
        outErr.clear();

        std::lock_guard<std::mutex> lock(_mu);

        if (!isValidUsername(username))
        {
            outErr = "用户名不合法（仅支持字母/数字/下划线，长度 3-32）";
            return false;
        }
        if (password.size() < 6)
        {
            outErr = "密码过短（至少 6 位）";
            return false;
        }
        if (_users.find(username) != _users.end())
        {
            outErr = "用户名已存在";
            return false;
        }

        UserRecord rec;
        rec.username = username;
        rec.salt = randomHex(16);
        rec.passwordHash = hashPassword(rec.salt, password);
        rec.createdAtMs = nowMs();
        _users[username] = rec;

        return save(outErr);
    }

    bool verifyPassword(const std::string &username, const std::string &password) const
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto it = _users.find(username);
        if (it == _users.end())
        {
            return false;
        }
        const UserRecord &rec = it->second;
        return hashPassword(rec.salt, password) == rec.passwordHash;
    }

    std::vector<UserListItem> listUsers() const
    {
        std::lock_guard<std::mutex> lock(_mu);
        std::vector<UserListItem> out;
        out.reserve(_users.size());
        for (const auto &kv : _users)
        {
            UserListItem item;
            item.username = kv.second.username;
            item.createdAtMs = kv.second.createdAtMs;
            out.push_back(std::move(item));
        }
        std::sort(out.begin(), out.end(), [](const UserListItem &a, const UserListItem &b) {
            return a.createdAtMs > b.createdAtMs;
        });
        return out;
    }

    bool deleteUser(const std::string &username, std::string &outErr)
    {
        outErr.clear();
        std::lock_guard<std::mutex> lock(_mu);

        auto it = _users.find(username);
        if (it == _users.end())
        {
            outErr = "用户不存在";
            return false;
        }

        _users.erase(it);
        return save(outErr);
    }

private:
    bool load()
    {
        std::string content;
        if (!readTextFile(_dbPath, content))
        {
            return true; // 文件不存在：视为无用户
        }

        rapidjson::Document doc;
        std::string err;
        if (!parseJsonObject(content, doc, err))
        {
            std::cerr << "[UserStore] users.json 解析失败，忽略并重建\n";
            return true;
        }

        if (!doc.HasMember("users") || !doc["users"].IsArray())
        {
            return true;
        }

        _users.clear();
        for (auto &v : doc["users"].GetArray())
        {
            if (!v.IsObject())
            {
                continue;
            }
            if (!v.HasMember("username") || !v["username"].IsString())
            {
                continue;
            }
            if (!v.HasMember("salt") || !v["salt"].IsString())
            {
                continue;
            }
            if (!v.HasMember("passwordHash") || !v["passwordHash"].IsString())
            {
                continue;
            }

            UserRecord rec;
            rec.username = v["username"].GetString();
            rec.salt = v["salt"].GetString();
            rec.passwordHash = v["passwordHash"].GetString();
            if (v.HasMember("createdAtMs") && v["createdAtMs"].IsInt64())
            {
                rec.createdAtMs = v["createdAtMs"].GetInt64();
            }
            _users[rec.username] = rec;
        }
        return true;
    }

    bool save(std::string &outErr) const
    {
        rapidjson::Document doc;
        doc.SetObject();
        auto &alloc = doc.GetAllocator();

        doc.AddMember("schemaVersion", 1, alloc);
        rapidjson::Value arr(rapidjson::kArrayType);
        for (const auto &kv : _users)
        {
            const UserRecord &rec = kv.second;
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("username", rapidjson::Value(rec.username.c_str(), alloc).Move(), alloc);
            obj.AddMember("salt", rapidjson::Value(rec.salt.c_str(), alloc).Move(), alloc);
            obj.AddMember("passwordHash", rapidjson::Value(rec.passwordHash.c_str(), alloc).Move(), alloc);
            obj.AddMember("createdAtMs", rec.createdAtMs, alloc);
            arr.PushBack(obj, alloc);
        }
        doc.AddMember("users", arr, alloc);

        const std::string json = jsonStringify(doc);
        return writeTextFileAtomic(_dbPath, json, outErr);
    }

    fs::path _root;
    fs::path _dbPath;
    mutable std::mutex _mu;
    std::unordered_map<std::string, UserRecord> _users;

    friend class CloudServer;
};

struct Session
{
    std::string username;
    int64_t expireAtMs = 0;
};

class SessionStore
{
public:
    std::string createToken(const std::string &username, int expiresSec)
    {
        std::lock_guard<std::mutex> lock(_mu);
        const std::string token = randomHex(32);
        Session s;
        s.username = username;
        s.expireAtMs = nowMs() + static_cast<int64_t>(expiresSec) * 1000;
        _sessions[token] = s;
        return token;
    }

    bool verifyToken(const std::string &token, std::string &outUsername)
    {
        std::lock_guard<std::mutex> lock(_mu);
        auto it = _sessions.find(token);
        if (it == _sessions.end())
        {
            return false;
        }
        const int64_t now = nowMs();
        if (now >= it->second.expireAtMs)
        {
            _sessions.erase(it);
            return false;
        }
        outUsername = it->second.username;
        return true;
    }

    void revokeUser(const std::string &username)
    {
        std::lock_guard<std::mutex> lock(_mu);
        for (auto it = _sessions.begin(); it != _sessions.end();)
        {
            if (it->second.username == username)
            {
                it = _sessions.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    std::mutex _mu;
    std::unordered_map<std::string, Session> _sessions;
};

class SaveStore
{
public:
    explicit SaveStore(fs::path root)
        : _root(std::move(root))
    {
    }

    bool savePackageForUser(const std::string &username, const std::string &packageJson, std::string &outErr)
    {
        outErr.clear();
        std::lock_guard<std::mutex> lock(_mu);

        // 校验 JSON 可解析
        rapidjson::Document pkg;
        std::string parseErr;
        if (!parseJsonObject(packageJson, pkg, parseErr))
        {
            outErr = "存档包 JSON 解析失败";
            return false;
        }

        const fs::path userDir = _root / "users" / username;
        const fs::path historyDir = userDir / "history";
        const fs::path savesDir = userDir / "saves";

        std::error_code ec;
        fs::create_directories(historyDir, ec);
        fs::create_directories(savesDir, ec);
        if (ec)
        {
            outErr = "创建用户目录失败: " + ec.message();
            return false;
        }

        const fs::path packagePath = userDir / "package.json";
        if (fs::exists(packagePath))
        {
            const fs::path backupPath = historyDir / ("package_" + std::to_string(nowMs()) + ".json");
            fs::copy_file(packagePath, backupPath, fs::copy_options::overwrite_existing, ec);
            ec.clear();
        }

        // 保存完整包
        if (!writeTextFileAtomic(packagePath, packageJson, outErr))
        {
            return false;
        }

        // 解析拆分：settings + saves（便于管理/排查）
        if (pkg.HasMember("settings") && pkg["settings"].IsObject())
        {
            const fs::path settingsPath = userDir / "settings.json";
            const std::string settingsJson = jsonStringify(pkg["settings"]);
            std::string tmpErr;
            writeTextFileAtomic(settingsPath, settingsJson, tmpErr);
        }

        if (pkg.HasMember("saves") && pkg["saves"].IsObject())
        {
            const auto &saves = pkg["saves"];
            for (auto it = saves.MemberBegin(); it != saves.MemberEnd(); ++it)
            {
                if (!it->name.IsString() || !it->value.IsObject())
                {
                    continue;
                }
                const std::string slotKey = it->name.GetString();
                const fs::path slotPath = savesDir / ("save_" + slotKey + ".json");
                const std::string slotJson = jsonStringify(it->value);
                std::string tmpErr;
                writeTextFileAtomic(slotPath, slotJson, tmpErr);
            }
        }

        return true;
    }

    bool loadPackageForUser(const std::string &username, std::string &outJson, std::string &outErr) const
    {
        outErr.clear();
        std::lock_guard<std::mutex> lock(_mu);
        const fs::path packagePath = _root / "users" / username / "package.json";
        if (!readTextFile(packagePath, outJson))
        {
            outErr = "云端暂无存档包";
            return false;
        }
        return true;
    }

    struct HistoryItem
    {
        int64_t timestampMs = 0;
        std::string filename;
        uintmax_t sizeBytes = 0;
    };

    std::vector<HistoryItem> listUserHistory(const std::string &username) const
    {
        std::lock_guard<std::mutex> lock(_mu);
        std::vector<HistoryItem> out;

        if (!isValidUsername(username))
        {
            return out;
        }

        const fs::path historyDir = _root / "users" / username / "history";
        std::error_code ec;
        if (!fs::exists(historyDir, ec) || ec)
        {
            return out;
        }

        for (const auto &entry : fs::directory_iterator(historyDir, ec))
        {
            if (ec)
            {
                break;
            }
            if (!entry.is_regular_file())
            {
                continue;
            }

            const std::string name = entry.path().filename().string();
            const std::string prefix = "package_";
            const std::string suffix = ".json";
            if (name.size() <= prefix.size() + suffix.size())
            {
                continue;
            }
            if (name.rfind(prefix, 0) != 0)
            {
                continue;
            }
            if (name.substr(name.size() - suffix.size()) != suffix)
            {
                continue;
            }

            const std::string tsStr = name.substr(prefix.size(), name.size() - prefix.size() - suffix.size());
            int64_t ts = 0;
            try
            {
                ts = std::stoll(tsStr);
            }
            catch (...)
            {
                continue;
            }

            HistoryItem item;
            item.timestampMs = ts;
            item.filename = name;
            item.sizeBytes = entry.file_size(ec);
            if (ec)
            {
                item.sizeBytes = 0;
                ec.clear();
            }
            out.push_back(std::move(item));
        }

        std::sort(out.begin(), out.end(), [](const HistoryItem &a, const HistoryItem &b) {
            return a.timestampMs > b.timestampMs;
        });
        return out;
    }

    bool rollbackUserToHistory(const std::string &username, int64_t timestampMs, std::string &outErr)
    {
        outErr.clear();
        if (!isValidUsername(username))
        {
            outErr = "用户名不合法";
            return false;
        }
        if (timestampMs <= 0)
        {
            outErr = "时间戳不合法";
            return false;
        }

        const fs::path historyPath = _root / "users" / username / "history" / ("package_" + std::to_string(timestampMs) + ".json");
        std::string json;
        if (!readTextFile(historyPath, json))
        {
            outErr = "未找到历史文件";
            return false;
        }

        // 复用存档保存逻辑：会自动备份当前 package.json 到 history
        return savePackageForUser(username, json, outErr);
    }

    bool deleteUserData(const std::string &username, std::string &outErr)
    {
        outErr.clear();
        if (!isValidUsername(username))
        {
            outErr = "用户名不合法";
            return false;
        }

        std::lock_guard<std::mutex> lock(_mu);
        const fs::path userDir = _root / "users" / username;
        std::error_code ec;
        fs::remove_all(userDir, ec);
        if (ec)
        {
            outErr = "删除用户目录失败: " + ec.message();
            return false;
        }
        return true;
    }

private:
    fs::path _root;
    mutable std::mutex _mu;
};

static void respondJson(httplib::Response &res, int statusCode, const std::string &json)
{
    res.status = statusCode;
    // 用于快速识别“是否命中正确服务”，排查端口冲突/反向代理等问题
    res.set_header("X-AK-Server", "ak_cloud_save_server");
    res.set_header("Content-Type", "application/json; charset=utf-8");
    res.set_content(json, "application/json; charset=utf-8");
}

static std::string makeErrorJson(const std::string &message)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto &alloc = doc.GetAllocator();
    doc.AddMember("ok", false, alloc);
    doc.AddMember("message", rapidjson::Value(message.c_str(), alloc).Move(), alloc);
    return jsonStringify(doc);
}

static std::string makeOkJson(const std::string &message)
{
    rapidjson::Document doc;
    doc.SetObject();
    auto &alloc = doc.GetAllocator();
    doc.AddMember("ok", true, alloc);
    doc.AddMember("message", rapidjson::Value(message.c_str(), alloc).Move(), alloc);
    return jsonStringify(doc);
}

static bool extractBearerToken(const httplib::Request &req, std::string &outToken)
{
    outToken.clear();
    const auto auth = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    if (auth.size() <= prefix.size())
    {
        return false;
    }
    if (auth.substr(0, prefix.size()) != prefix)
    {
        return false;
    }
    outToken = auth.substr(prefix.size());
    return !outToken.empty();
}

static int64_t extractUploadedAtFromPackageJson(const std::string &json)
{
    rapidjson::Document doc;
    std::string err;
    if (!parseJsonObject(json, doc, err))
    {
        return 0;
    }
    if (!doc.HasMember("uploadedAt"))
    {
        return 0;
    }
    const auto &v = doc["uploadedAt"];
    if (v.IsInt64())
    {
        return v.GetInt64();
    }
    if (v.IsNumber())
    {
        return static_cast<int64_t>(v.GetDouble());
    }
    return 0;
}

static void respondHtml(httplib::Response &res, int statusCode, const std::string &html)
{
    res.status = statusCode;
    // 用于快速识别“是否命中正确服务”，排查端口冲突/反向代理等问题
    res.set_header("X-AK-Server", "ak_cloud_save_server");
    res.set_header("Content-Type", "text/html; charset=utf-8");
    res.set_content(html, "text/html; charset=utf-8");
}

struct Args
{
    fs::path root = fs::path("cloud_data");
    // 默认监听 0.0.0.0：
    // - WSL 场景下便于 Windows 通过 localhost/WSL IP 访问
    // - 避免部分环境下绑定 127.0.0.1 失败而“服务瞬间退出”，导致访问命中其它程序（表现为 404/401）
    std::string host = "0.0.0.0";
    int port = 5173;
    std::string adminToken;
};

static bool parseArgs(int argc, char **argv, Args &out)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--root" && i + 1 < argc)
        {
            out.root = fs::path(argv[++i]);
            continue;
        }
        if (a == "--host" && i + 1 < argc)
        {
            out.host = argv[++i];
            continue;
        }
        if (a == "--port" && i + 1 < argc)
        {
            out.port = std::stoi(argv[++i]);
            continue;
        }
        if (a == "--admin-token" && i + 1 < argc)
        {
            out.adminToken = argv[++i];
            continue;
        }
        if (a == "-h" || a == "--help")
        {
            return false;
        }
    }
    return true;
}
}

int main(int argc, char **argv)
{
    Args args;
    if (!parseArgs(argc, argv, args))
    {
        std::cout << "用法：ak_cloud_save_server --root <dir> --host <ip> --port <port>\n";
        std::cout << "示例：ak_cloud_save_server --root ./cloud_data --host 0.0.0.0 --port 5173\n";
        std::cout << "管理：可选 --admin-token <token>；或设置环境变量 AK_CLOUD_ADMIN_TOKEN\n";
        return 1;
    }

    std::error_code ec;
    fs::create_directories(args.root, ec);
    if (ec)
    {
        std::cerr << "创建 root 目录失败: " << ec.message() << "\n";
        return 1;
    }

    UserStore userStore(args.root);
    SessionStore sessionStore;
    SaveStore saveStore(args.root);

    // 管理员 token：优先命令行，其次环境变量；若仍为空则自动生成一份随机 token
    bool adminTokenGenerated = false;
    if (args.adminToken.empty())
    {
        const char *envToken = std::getenv("AK_CLOUD_ADMIN_TOKEN");
        if (envToken && envToken[0])
        {
            args.adminToken = envToken;
        }
    }
    if (args.adminToken.empty())
    {
        args.adminToken = randomHex(16);
        adminTokenGenerated = true;
    }

    httplib::Server svr;

    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
        const std::string body = R"({"ok":true,"message":"Adventure-King Cloud Save Server"})";
        respondJson(res, 200, body);
    });

    // ========================= 管理页面（可视化） =========================
    const std::string adminPageHtml = R"HTML(<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Adventure-King 云存管理</title>
  <style>
    :root { --bg:#0f1115; --panel:#171a21; --border:#2a2f3a; --text:#e7e7e7; --muted:#a8b0c0; --danger:#ff5b5b; --ok:#35d07f; --btn:#2b3342; }
    body { margin:0; font-family: ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Arial, "Noto Sans SC", "PingFang SC", "Microsoft YaHei", sans-serif; background:var(--bg); color:var(--text); }
    header { padding:14px 16px; border-bottom:1px solid var(--border); display:flex; align-items:center; gap:12px; }
    header h1 { font-size:16px; margin:0; font-weight:600; color:var(--text); }
    .row { display:flex; gap:10px; align-items:center; flex-wrap:wrap; }
    input { background:#0b0d12; border:1px solid var(--border); color:var(--text); padding:8px 10px; border-radius:8px; min-width:240px; }
    button { background:var(--btn); border:1px solid var(--border); color:var(--text); padding:8px 10px; border-radius:8px; cursor:pointer; }
    button:hover { filter:brightness(1.1); }
    button.danger { background:rgba(255,91,91,0.12); border-color:rgba(255,91,91,0.35); color:#ffb5b5; }
    main { display:grid; grid-template-columns: 300px 1fr; gap:12px; padding:12px; }
    .card { background:var(--panel); border:1px solid var(--border); border-radius:12px; overflow:hidden; }
    .card h2 { margin:0; padding:12px 12px; font-size:14px; border-bottom:1px solid var(--border); color:var(--muted); }
    .list { max-height: calc(100vh - 140px); overflow:auto; }
    .item { padding:10px 12px; border-bottom:1px solid rgba(42,47,58,0.6); cursor:pointer; }
    .item:hover { background: rgba(255,255,255,0.03); }
    .item.active { background: rgba(53,208,127,0.08); }
    .item .name { font-weight:600; }
    .item .meta { font-size:12px; color:var(--muted); margin-top:4px; }
    .content { padding:12px; }
    .kv { display:grid; grid-template-columns: 120px 1fr; gap:8px 10px; margin-bottom:10px; }
    .kv div { font-size:13px; }
    .kv .k { color:var(--muted); }
    .toolbar { display:flex; gap:8px; flex-wrap:wrap; margin:10px 0 16px; }
    .msg { margin-left:auto; font-size:12px; color:var(--muted); }
    pre { background:#0b0d12; border:1px solid var(--border); padding:10px; border-radius:10px; overflow:auto; max-height: 360px; }
    .history { display:flex; flex-direction:column; gap:8px; }
    .hist-item { display:flex; align-items:center; justify-content:space-between; gap:10px; padding:10px; background:#0b0d12; border:1px solid var(--border); border-radius:10px; }
    .hist-item .left { display:flex; flex-direction:column; }
    .hist-item .ts { font-weight:600; }
    .hist-item .sub { font-size:12px; color:var(--muted); }
    .pill { display:inline-block; padding:2px 8px; border-radius:999px; font-size:12px; border:1px solid var(--border); color:var(--muted); }
    .ok { color:var(--ok); }
    .dangerText { color:var(--danger); }
  </style>
</head>
<body>
  <header>
    <h1>Adventure-King 云存管理</h1>
    <div class="row">
      <input id="adminToken" placeholder="管理员 Token（请求头 X-AK-Admin-Token）" />
      <button id="saveToken">保存</button>
      <button id="refresh">刷新</button>
      <span class="msg" id="msg"></span>
    </div>
  </header>
  <main>
    <div class="card">
      <h2>用户列表</h2>
      <div class="list" id="userList"></div>
    </div>
    <div class="card">
      <h2>用户详情</h2>
      <div class="content">
        <div class="kv">
          <div class="k">用户名</div><div id="detailUser">-</div>
          <div class="k">创建时间</div><div id="detailCreated">-</div>
          <div class="k">最近上传</div><div id="detailUploaded">-</div>
        </div>
        <div class="toolbar">
          <button id="btnLoadPackage">查看当前存档包</button>
          <button class="danger" id="btnDeleteUser">删除用户</button>
          <span class="pill" id="detailHint">请选择左侧用户</span>
        </div>
        <h3 style="margin:0 0 8px;color:var(--muted);font-size:13px;">历史版本（可回滚）</h3>
        <div class="history" id="history"></div>
        <h3 style="margin:16px 0 8px;color:var(--muted);font-size:13px;">当前存档包（只读预览）</h3>
        <pre id="packagePreview">{}</pre>
      </div>
    </div>
  </main>

<script>
  const $ = (id) => document.getElementById(id);
  const msg = (text, ok) => { $('msg').textContent = text || ''; $('msg').className = ok ? 'msg ok' : 'msg'; };
  const fmtTime = (ms) => {
    if (!ms || ms <= 0) return '-';
    const d = new Date(ms);
    return d.toLocaleString();
  };

  let selectedUser = '';

  function getToken() { return $('adminToken').value.trim(); }
  function headers() {
    const h = { 'Content-Type': 'application/json' };
    const t = getToken();
    if (t) h['X-AK-Admin-Token'] = t;
    return h;
  }

  async function api(path, opts) {
    const res = await fetch(path, { ...opts, headers: { ...(opts && opts.headers ? opts.headers : {}), ...headers() } });
    const text = await res.text();
    let json = null;
    try { json = JSON.parse(text); } catch (_) {}
    if (!res.ok) {
      const m = (json && json.message) ? json.message : (text || ('HTTP ' + res.status));
      throw new Error(m);
    }
    return json !== null ? json : text;
  }

  function renderUsers(users) {
    const list = $('userList');
    list.innerHTML = '';
    users.forEach(u => {
      const div = document.createElement('div');
      div.className = 'item' + (u.username === selectedUser ? ' active' : '');
      div.onclick = () => selectUser(u.username, u);
      div.innerHTML = `
        <div class="name">${u.username}</div>
        <div class="meta">创建：${fmtTime(u.createdAtMs)} · 最近上传：${fmtTime(u.lastUploadAtMs)}</div>
      `;
      list.appendChild(div);
    });
    if (!users.length) list.innerHTML = '<div class="item"><div class="meta">暂无用户</div></div>';
  }

  function setDetail(user, createdAtMs, lastUploadAtMs) {
    $('detailUser').textContent = user || '-';
    $('detailCreated').textContent = fmtTime(createdAtMs);
    $('detailUploaded').textContent = fmtTime(lastUploadAtMs);
    $('detailHint').textContent = user ? '已选择用户，可执行删除/回滚' : '请选择左侧用户';
  }

  async function refreshUsers() {
    msg('刷新中...', true);
    const data = await api('/api/admin/users', { method: 'GET' });
    renderUsers(data.users || []);
    msg('已刷新', true);
  }

  async function selectUser(username, info) {
    selectedUser = username;
    renderUsers((await api('/api/admin/users', { method: 'GET' })).users || []);
    $('packagePreview').textContent = '{}';
    $('history').innerHTML = '';
    setDetail(username, info.createdAtMs, info.lastUploadAtMs);
    await refreshHistory();
  }

  async function refreshHistory() {
    if (!selectedUser) return;
    const data = await api(`/api/admin/users/${selectedUser}/history`, { method: 'GET' });
    const items = data.history || [];
    const box = $('history');
    box.innerHTML = '';
    if (!items.length) {
      box.innerHTML = '<div class="hist-item"><div class="left"><div class="ts">暂无历史版本</div><div class="sub">用户还未产生回滚点</div></div></div>';
      return;
    }
    items.forEach(h => {
      const row = document.createElement('div');
      row.className = 'hist-item';
      row.innerHTML = `
        <div class="left">
          <div class="ts">${fmtTime(h.timestampMs)}</div>
          <div class="sub">${h.filename} · ${Math.round((h.sizeBytes||0)/1024)} KB</div>
        </div>
        <div class="right">
          <button class="danger" data-ts="${h.timestampMs}">回滚到此版本</button>
        </div>
      `;
      row.querySelector('button').onclick = async () => {
        if (!confirm('确认回滚？回滚会覆盖该用户当前云端 package.json，并自动备份当前版本到 history。')) return;
        try {
          msg('回滚中...', true);
          await api(`/api/admin/users/${selectedUser}/rollback`, { method: 'POST', body: JSON.stringify({ timestampMs: h.timestampMs }) });
          msg('回滚成功', true);
          await refreshUsers();
          await refreshHistory();
        } catch (e) { msg('回滚失败：' + e.message, false); }
      };
      box.appendChild(row);
    });
  }

  async function loadPackage() {
    if (!selectedUser) return;
    msg('加载存档包...', true);
    const data = await api(`/api/admin/users/${selectedUser}/package`, { method: 'GET' });
    $('packagePreview').textContent = JSON.stringify(data.package || {}, null, 2);
    msg('已加载', true);
  }

  async function deleteUser() {
    if (!selectedUser) return;
    if (!confirm('确认删除该用户？此操作会删除用户账号与其所有云端存档（不可恢复）。')) return;
    msg('删除中...', true);
    await api(`/api/admin/users/${selectedUser}/delete`, { method: 'POST', body: '{}' });
    selectedUser = '';
    setDetail('', 0, 0);
    $('packagePreview').textContent = '{}';
    $('history').innerHTML = '';
    await refreshUsers();
    msg('已删除', true);
  }

  $('saveToken').onclick = () => {
    localStorage.setItem('ak_admin_token', getToken());
    msg('已保存 Token（仅本机浏览器）', true);
  };
  $('refresh').onclick = async () => {
    try { await refreshUsers(); } catch (e) { msg('刷新失败：' + e.message, false); }
  };
  $('btnLoadPackage').onclick = async () => { try { await loadPackage(); } catch (e) { msg('加载失败：' + e.message, false); } };
  $('btnDeleteUser').onclick = async () => { try { await deleteUser(); } catch (e) { msg('删除失败：' + e.message, false); } };

  // init
  const saved = localStorage.getItem('ak_admin_token') || '';
  $('adminToken').value = saved;
  refreshUsers().catch(e => msg('请先填写正确 Token：' + e.message, false));
</script>
</body>
</html>)HTML";

    auto requireAdmin = [&args](const httplib::Request &req, httplib::Response &res) -> bool {
        const std::string token = req.get_header_value("X-AK-Admin-Token");
        if (token.empty() || token != args.adminToken)
        {
            respondJson(res, 403, makeErrorJson("管理员鉴权失败（请在请求头 X-AK-Admin-Token 中提供正确 token）"));
            return false;
        }
        return true;
    };

    svr.Get("/admin", [&adminPageHtml](const httplib::Request &, httplib::Response &res) {
        respondHtml(res, 200, adminPageHtml);
    });

    // 管理：用户列表
    svr.Get("/api/admin/users", [&userStore, &saveStore, &requireAdmin](const httplib::Request &req, httplib::Response &res) {
        if (!requireAdmin(req, res))
        {
            return;
        }

        rapidjson::Document doc;
        doc.SetObject();
        auto &alloc = doc.GetAllocator();
        doc.AddMember("ok", true, alloc);
        doc.AddMember("serverTimeMs", nowMs(), alloc);

        rapidjson::Value arr(rapidjson::kArrayType);
        for (const auto &u : userStore.listUsers())
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("username", rapidjson::Value(u.username.c_str(), alloc).Move(), alloc);
            obj.AddMember("createdAtMs", u.createdAtMs, alloc);

            std::string pkg;
            std::string err;
            int64_t uploadedAt = 0;
            if (saveStore.loadPackageForUser(u.username, pkg, err))
            {
                uploadedAt = extractUploadedAtFromPackageJson(pkg);
            }
            obj.AddMember("lastUploadAtMs", uploadedAt, alloc);
            arr.PushBack(obj, alloc);
        }
        doc.AddMember("users", arr, alloc);
        respondJson(res, 200, jsonStringify(doc));
    });

    // 管理：查看用户当前存档包
    svr.Get(R"(/api/admin/users/([A-Za-z0-9_]+)/package)", [&saveStore, &requireAdmin](const httplib::Request &req, httplib::Response &res) {
        if (!requireAdmin(req, res))
        {
            return;
        }

        const std::string username = req.matches[1];
        std::string pkg;
        std::string err;
        if (!saveStore.loadPackageForUser(username, pkg, err))
        {
            respondJson(res, 404, makeErrorJson("云端暂无存档包"));
            return;
        }

        rapidjson::Document pkgDoc;
        std::string parseErr;
        if (!parseJsonObject(pkg, pkgDoc, parseErr))
        {
            respondJson(res, 500, makeErrorJson("存档包 JSON 解析失败"));
            return;
        }

        rapidjson::Document resp;
        resp.SetObject();
        auto &alloc = resp.GetAllocator();
        resp.AddMember("ok", true, alloc);
        resp.AddMember("username", rapidjson::Value(username.c_str(), alloc).Move(), alloc);
        rapidjson::Value pkgVal;
        pkgVal.CopyFrom(pkgDoc, alloc);
        resp.AddMember("package", pkgVal, alloc);
        respondJson(res, 200, jsonStringify(resp));
    });

    // 管理：历史列表
    svr.Get(R"(/api/admin/users/([A-Za-z0-9_]+)/history)", [&saveStore, &requireAdmin](const httplib::Request &req, httplib::Response &res) {
        if (!requireAdmin(req, res))
        {
            return;
        }

        const std::string username = req.matches[1];
        rapidjson::Document doc;
        doc.SetObject();
        auto &alloc = doc.GetAllocator();
        doc.AddMember("ok", true, alloc);
        doc.AddMember("username", rapidjson::Value(username.c_str(), alloc).Move(), alloc);

        rapidjson::Value arr(rapidjson::kArrayType);
        for (const auto &h : saveStore.listUserHistory(username))
        {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("timestampMs", h.timestampMs, alloc);
            obj.AddMember("filename", rapidjson::Value(h.filename.c_str(), alloc).Move(), alloc);
            obj.AddMember("sizeBytes", static_cast<uint64_t>(h.sizeBytes), alloc);
            arr.PushBack(obj, alloc);
        }
        doc.AddMember("history", arr, alloc);
        respondJson(res, 200, jsonStringify(doc));
    });

    // 管理：回滚
    svr.Post(R"(/api/admin/users/([A-Za-z0-9_]+)/rollback)", [&saveStore, &requireAdmin](const httplib::Request &req, httplib::Response &res) {
        if (!requireAdmin(req, res))
        {
            return;
        }

        const std::string username = req.matches[1];
        rapidjson::Document doc;
        std::string parseErr;
        if (!parseJsonObject(req.body, doc, parseErr))
        {
            respondJson(res, 400, makeErrorJson("请求体 JSON 解析失败"));
            return;
        }

        int64_t ts = 0;
        if (doc.HasMember("timestampMs") && doc["timestampMs"].IsInt64())
        {
            ts = doc["timestampMs"].GetInt64();
        }
        else if (doc.HasMember("timestampMs") && doc["timestampMs"].IsNumber())
        {
            ts = static_cast<int64_t>(doc["timestampMs"].GetDouble());
        }

        std::string err;
        if (!saveStore.rollbackUserToHistory(username, ts, err))
        {
            respondJson(res, 400, makeErrorJson(err.empty() ? "回滚失败" : err));
            return;
        }
        respondJson(res, 200, makeOkJson("回滚成功"));
    });

    // 管理：删除用户（账号 + 云端数据）
    svr.Post(R"(/api/admin/users/([A-Za-z0-9_]+)/delete)", [&userStore, &sessionStore, &saveStore, &requireAdmin](const httplib::Request &req, httplib::Response &res) {
        if (!requireAdmin(req, res))
        {
            return;
        }

        const std::string username = req.matches[1];

        std::string err;
        if (!userStore.deleteUser(username, err))
        {
            respondJson(res, 400, makeErrorJson(err.empty() ? "删除用户失败" : err));
            return;
        }

        sessionStore.revokeUser(username);

        std::string err2;
        saveStore.deleteUserData(username, err2);
        respondJson(res, 200, makeOkJson("删除成功"));
    });

    // 注册
    svr.Post("/api/register", [&userStore](const httplib::Request &req, httplib::Response &res) {
        rapidjson::Document doc;
        std::string err;
        if (!parseJsonObject(req.body, doc, err))
        {
            respondJson(res, 400, makeErrorJson("请求体 JSON 解析失败"));
            return;
        }

        const std::string username = (doc.HasMember("username") && doc["username"].IsString()) ? doc["username"].GetString() : "";
        const std::string password = (doc.HasMember("password") && doc["password"].IsString()) ? doc["password"].GetString() : "";

        std::string regErr;
        if (!userStore.registerUser(username, password, regErr))
        {
            respondJson(res, 400, makeErrorJson(regErr));
            return;
        }

        respondJson(res, 200, makeOkJson("注册成功"));
    });

    // 登录
    svr.Post("/api/login", [&userStore, &sessionStore](const httplib::Request &req, httplib::Response &res) {
        rapidjson::Document doc;
        std::string err;
        if (!parseJsonObject(req.body, doc, err))
        {
            respondJson(res, 400, makeErrorJson("请求体 JSON 解析失败"));
            return;
        }

        const std::string username = (doc.HasMember("username") && doc["username"].IsString()) ? doc["username"].GetString() : "";
        const std::string password = (doc.HasMember("password") && doc["password"].IsString()) ? doc["password"].GetString() : "";

        if (username.empty() || password.empty())
        {
            respondJson(res, 400, makeErrorJson("缺少 username/password"));
            return;
        }

        if (!userStore.verifyPassword(username, password))
        {
            respondJson(res, 401, makeErrorJson("用户名或密码错误"));
            return;
        }

        const int expiresSec = 3600;
        const std::string token = sessionStore.createToken(username, expiresSec);

        rapidjson::Document resp;
        resp.SetObject();
        auto &alloc = resp.GetAllocator();
        resp.AddMember("token", rapidjson::Value(token.c_str(), alloc).Move(), alloc);
        resp.AddMember("expiresInSeconds", expiresSec, alloc);
        respondJson(res, 200, jsonStringify(resp));
    });

    auto requireAuth = [&sessionStore](const httplib::Request &req, httplib::Response &res, std::string &outUsername) -> bool {
        std::string token;
        if (!extractBearerToken(req, token))
        {
            respondJson(res, 401, makeErrorJson("未登录（缺少 Authorization: Bearer <token>）"));
            return false;
        }

        if (!sessionStore.verifyToken(token, outUsername))
        {
            respondJson(res, 401, makeErrorJson("登录已失效，请重新登录"));
            return false;
        }
        return true;
    };

    // 上传存档包
    svr.Post("/api/sync/push", [&saveStore, &requireAuth](const httplib::Request &req, httplib::Response &res) {
        std::string username;
        if (!requireAuth(req, res, username))
        {
            return;
        }

        std::string saveErr;
        if (!saveStore.savePackageForUser(username, req.body, saveErr))
        {
            respondJson(res, 400, makeErrorJson(saveErr));
            return;
        }

        respondJson(res, 200, makeOkJson("上传成功"));
    });

    // 拉取存档包
    svr.Get("/api/sync/pull", [&saveStore, &requireAuth](const httplib::Request &req, httplib::Response &res) {
        std::string username;
        if (!requireAuth(req, res, username))
        {
            return;
        }

        std::string pkg;
        std::string err;
        if (!saveStore.loadPackageForUser(username, pkg, err))
        {
            respondJson(res, 404, makeErrorJson("云端暂无存档包"));
            return;
        }

        res.status = 200;
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_content(pkg, "application/json; charset=utf-8");
    });

    std::cout << "Adventure-King Cloud Save Server\n";
    std::cout << "Root: " << args.root.string() << "\n";
    std::cout << "Listen: http://" << args.host << ":" << args.port << "\n";
    std::cout << "Admin UI: http://" << args.host << ":" << args.port << "/admin\n";
    std::cout << "Admin token (X-AK-Admin-Token): " << args.adminToken << (adminTokenGenerated ? "  [auto-generated]" : "") << "\n";

    if (!svr.listen(args.host, args.port))
    {
        std::cerr << "[ERROR] 监听失败：host=" << args.host << " port=" << args.port << "\n";
        std::cerr << "        可能原因：端口被占用 / host 不可用 / 权限不足。\n";
        std::cerr << "        建议：\n";
        std::cerr << "        1) 改用 --host 0.0.0.0（WSL 推荐）\n";
        std::cerr << "        2) 换一个端口（例如 --port 5174）\n";
        return 2;
    }
    return 0;
}

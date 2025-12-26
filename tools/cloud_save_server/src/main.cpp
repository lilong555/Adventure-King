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
        auto it = _users.find(username);
        if (it == _users.end())
        {
            return false;
        }
        const UserRecord &rec = it->second;
        return hashPassword(rec.salt, password) == rec.passwordHash;
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
        const std::string token = randomHex(32);
        Session s;
        s.username = username;
        s.expireAtMs = nowMs() + static_cast<int64_t>(expiresSec) * 1000;
        _sessions[token] = s;
        return token;
    }

    bool verifyToken(const std::string &token, std::string &outUsername)
    {
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

private:
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
        const fs::path packagePath = _root / "users" / username / "package.json";
        if (!readTextFile(packagePath, outJson))
        {
            outErr = "云端暂无存档包";
            return false;
        }
        return true;
    }

private:
    fs::path _root;
};

static void respondJson(httplib::Response &res, int statusCode, const std::string &json)
{
    res.status = statusCode;
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

struct Args
{
    fs::path root = fs::path("cloud_data");
    std::string host = "127.0.0.1";
    int port = 5173;
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
        std::cout << "示例：ak_cloud_save_server --root ./cloud_data --host 127.0.0.1 --port 5173\n";
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

    httplib::Server svr;

    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
        const std::string body = R"({"ok":true,"message":"Adventure-King Cloud Save Server"})";
        respondJson(res, 200, body);
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

    svr.listen(args.host, args.port);
    return 0;
}

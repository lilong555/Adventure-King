#pragma once

#include "SaveData.h"
#include <string>

/**
 * JSON 序列化工具类
 * 使用 Cocos2d-x 内置的 rapidjson 库进行序列化和反序列化
 */
class JsonSerializer
{
public:
    /**
     * 序列化存档槽位数据为 JSON 字符串
     * @param data 存档槽位数据
     * @return JSON 字符串，失败返回空字符串
     */
    static std::string serialize(const SaveSlotData &data);

    /**
     * 序列化设置数据为 JSON 字符串
     * @param data 设置数据
     * @return JSON 字符串，失败返回空字符串
     */
    static std::string serialize(const SettingsSaveData &data);

    /**
     * 从 JSON 字符串反序列化存档槽位数据
     * @param json JSON 字符串
     * @param outData 输出的存档槽位数据
     * @return 成功返回 true，失败返回 false
     */
    static bool deserialize(const std::string &json, SaveSlotData &outData);

    /**
     * 从 JSON 字符串反序列化设置数据
     * @param json JSON 字符串
     * @param outData 输出的设置数据
     * @return 成功返回 true，失败返回 false
     */
    static bool deserialize(const std::string &json, SettingsSaveData &outData);

private:
    // 禁止实例化
    JsonSerializer() = delete;
};

#pragma once
#ifndef __MUSIC_MANAGER_H__
#define __MUSIC_MANAGER_H__

#include "cocos2d.h"
#include "audio/include/AudioEngine.h"

class MusicManager
{
public:
    // 获取单例
    static MusicManager* getInstance();

    // 是否启用背景音乐
    bool isEnabled() const;

    // 启用/关闭背景音乐
    void setEnabled(bool enabled);
    // 播放背景音乐，自动停止之前的音乐
    void playBGM(const std::string& filePath, bool loop = true, float volume = 0.5f);

    // 暂停背景音乐
    void pauseBGM();

    // 继续播放背景音乐
    void resumeBGM();

    // 停止背景音乐
    void stopBGM();

    // 设置音量
    void setVolume(float volume);

    // 获取音量
    float getVolume() const;

private:
    // 私有构造，外部不可实例化
    MusicManager();
    // 私有析构，由单例生命周期管理
    ~MusicManager();

    static MusicManager* _instance;
    bool _enabled;
    int _bgmId;        // AudioEngine 返回的音乐 ID
    float _volume;     // 音量
};

#endif // __MUSIC_MANAGER_H__

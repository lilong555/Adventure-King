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
    MusicManager();
    ~MusicManager();

    static MusicManager* _instance;

    int _bgmId;        // AudioEngine 返回的音乐 ID
    float _volume;     // 音量
};

#endif // __MUSIC_MANAGER_H__

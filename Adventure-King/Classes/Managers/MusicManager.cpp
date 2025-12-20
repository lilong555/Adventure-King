#include "MusicManager.h"


using namespace cocos2d;
using namespace cocos2d::experimental;

MusicManager* MusicManager::_instance = nullptr;

// 获取全局单例（首次调用时创建）
MusicManager* MusicManager::getInstance()
{
    if (!_instance)
        _instance = new MusicManager();
    return _instance;
}
// 构造：初始化音量与状态
MusicManager::MusicManager()
    : _bgmId(-1)
    , _volume(0.5f)
    , _enabled(true)  // 默认开启音乐
{
}
// 析构：确保背景音乐停止
MusicManager::~MusicManager()
{
    stopBGM();
}
// 当前音乐是否启用
bool MusicManager::isEnabled() const
{
    return _enabled;
}
// 切换音乐开关（暂停/恢复）
void MusicManager::setEnabled(bool enabled)
{
    if (_enabled == enabled)
        return; // 状态一样，不用做任何事

    _enabled = enabled;

    if (!_enabled)
    {
        // 关闭音乐：只暂停，不 stop
        if (_bgmId != -1)
            AudioEngine::pause(_bgmId);
        return;
    }

    // 开启音乐：恢复播放
    if (_bgmId != -1)
        AudioEngine::resume(_bgmId);
}

// 播放背景音乐（若已有则先停止）
void MusicManager::playBGM(const std::string& filePath, bool loop, float volume)
{
    if (!_enabled)
        return;

    // 停止之前的音乐
    if (_bgmId != -1)
        AudioEngine::stop(_bgmId);

    _volume = volume;
    _bgmId = AudioEngine::play2d(filePath, loop, _volume);
}

// 暂停背景音乐
void MusicManager::pauseBGM()
{
    if (_bgmId != -1)
        AudioEngine::pause(_bgmId);
}

// 恢复背景音乐
void MusicManager::resumeBGM()
{
    if (_bgmId != -1)
        AudioEngine::resume(_bgmId);
}

// 停止并清空当前背景音乐
void MusicManager::stopBGM()
{
    if (_bgmId != -1)
    {
        AudioEngine::stop(_bgmId);
        _bgmId = -1;
    }
}

// 设置播放音量
void MusicManager::setVolume(float volume)
{
    _volume = volume;
    if (_bgmId != -1)
        AudioEngine::setVolume(_bgmId, _volume);
}

// 获取当前音量
float MusicManager::getVolume() const
{
    return _volume;
}

#include "MusicManager.h"


using namespace cocos2d;
using namespace cocos2d::experimental;

MusicManager* MusicManager::_instance = nullptr;

MusicManager* MusicManager::getInstance()
{
    if (!_instance)
        _instance = new MusicManager();
    return _instance;
}
MusicManager::MusicManager()
    : _bgmId(-1)
    , _volume(0.5f)
    , _enabled(true)  // 默认开启音乐
{
}
MusicManager::~MusicManager()
{
    stopBGM();
}
bool MusicManager::isEnabled() const
{
    return _enabled;
}
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

void MusicManager::pauseBGM()
{
    if (_bgmId != -1)
        AudioEngine::pause(_bgmId);
}

void MusicManager::resumeBGM()
{
    if (_bgmId != -1)
        AudioEngine::resume(_bgmId);
}

void MusicManager::stopBGM()
{
    if (_bgmId != -1)
    {
        AudioEngine::stop(_bgmId);
        _bgmId = -1;
    }
}

void MusicManager::setVolume(float volume)
{
    _volume = volume;
    if (_bgmId != -1)
        AudioEngine::setVolume(_bgmId, _volume);
}

float MusicManager::getVolume() const
{
    return _volume;
}

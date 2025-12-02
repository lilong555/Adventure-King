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
    : _bgmId(-1), _volume(0.5f)
{
}

MusicManager::~MusicManager()
{
    stopBGM();
}

void MusicManager::playBGM(const std::string& filePath, bool loop, float volume)
{
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

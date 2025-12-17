/**
 * @file GameUIController.h
 * @brief GameScene 的 UI 编排：初始化 GameUI、暂停菜单、交互提示刷新
 */

#pragma once

#include "cocos2d.h"
#include <functional>
#include <string>

class PlayerCharacter;
class GameUI;
struct SaveSlotData;

class GameUIController
{
public:
    bool init(cocos2d::Scene *scene,
              PlayerCharacter *player,
              const std::string &levelName,
              const std::function<void()> &onReturnToMap,
              const std::function<void(bool paused)> &onPauseChanged,
              const std::function<bool()> &isPlayerAtGate,
              const std::function<void(const SaveSlotData &)> &onLoadSuccess);

    void update();

    void togglePauseMenu();
    bool isPaused() const { return _paused; }

    GameUI *getGameUI() const { return _gameUI; }

private:
    cocos2d::Scene *_scene = nullptr;
    PlayerCharacter *_player = nullptr;
    GameUI *_gameUI = nullptr;

    bool _paused = false;
    bool _wasAtGate = false;

    std::function<void()> _onReturnToMap;
    std::function<void(bool)> _onPauseChanged;
    std::function<bool()> _isPlayerAtGate;
    std::function<void(const SaveSlotData &)> _onLoadSuccess;
};


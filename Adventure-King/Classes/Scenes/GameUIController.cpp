/**
 * @file GameUIController.cpp
 * @brief GameScene UI 编排实现
 */

#include "Scenes/GameUIController.h"
#include "Character/Player/PlayerCharacter.h"
#include "GameUI.h"
#include "Scenes/HelloWorldScene.h"
#include "Scenes/Layers/SaveMenuLayer.h"
#include "Save/SaveData.h"
#include "UI/PauseMenu.h"

USING_NS_CC;

namespace
{
    const char *const GATE_INTERACTION_HINT = "Press W to enter gate";
    constexpr int UI_Z_ORDER = 100;
    constexpr float UI_UPDATE_INTERVAL_SECONDS = 0.05f;
}

bool GameUIController::init(Scene *scene,
                            PlayerCharacter *player,
                            const std::string &levelName,
                            const std::function<void()> &onReturnToMap,
                            const std::function<void(bool paused)> &onPauseChanged,
                            const std::function<bool()> &isPlayerAtGate,
                            const std::function<void(const SaveSlotData &)> &onLoadSuccess)
{
    if (!scene)
    {
        CCLOG("GameUIController::init - scene is null");
        return false;
    }

    _scene = scene;
    _player = player;
    _onReturnToMap = onReturnToMap;
    _onPauseChanged = onPauseChanged;
    _isPlayerAtGate = isPlayerAtGate;
    _onLoadSuccess = onLoadSuccess;

    _gameUI = GameUI::create();
    if (!_gameUI)
    {
        CCLOG("Warning: Failed to create GameUI");
        return false;
    }

    _gameUI->setMapButtonCallback([this]()
                                  {
                                      if (_onReturnToMap)
                                      {
                                          _onReturnToMap();
                                      } });
    _gameUI->setLevelName(levelName);

    if (_player)
    {
        _gameUI->bindPlayer(_player);
    }

    auto pauseMenu = _gameUI->getPauseMenu();
    if (pauseMenu)
    {
        pauseMenu->setResumeCallback([this]()
                                     {
                                         _paused = false;
                                         if (_onPauseChanged)
                                         {
                                             _onPauseChanged(false);
                                         }
                                         CCLOG("Game resumed from pause menu"); });

        pauseMenu->setMainMenuCallback([]()
                                       {
                                           auto director = Director::getInstance();
                                           director->popToRootScene();

                                           auto mainMenuScene = HelloWorld::createScene();
                                           if (mainMenuScene)
                                           {
                                               auto transition = TransitionFade::create(0.5f, mainMenuScene, Color3B::BLACK);
                                               director->replaceScene(transition);
                                           }
                                           else
                                           {
                                               CCLOG("Error: Failed to create main menu scene!");
                                           } });

        pauseMenu->setSaveCallback([this, levelName]()
                                  {
                                      if (!_gameUI)
                                          return;

                                      _gameUI->hidePauseMenu();
                                      _paused = false;
                                      if (_onPauseChanged)
                                      {
                                          _onPauseChanged(false);
                                      }

                                      auto saveMenu = SaveMenuLayer::create(
                                          SaveMenuLayer::Mode::SAVE,
                                          _player,
                                          levelName,
                                          _player ? _player->getPosition() : Vec2::ZERO);
                                      if (saveMenu && _scene)
                                      {
                                          _scene->addChild(saveMenu, UI_Z_ORDER + 1);
                                      }
                                      CCLOG("GameScene - 打开保存游戏菜单"); });

        pauseMenu->setLoadCallback([this]()
                                  {
                                      if (!_gameUI)
                                          return;

                                      _gameUI->hidePauseMenu();
                                      _paused = false;
                                      if (_onPauseChanged)
                                      {
                                          _onPauseChanged(false);
                                      }

                                      auto saveMenu = SaveMenuLayer::create(SaveMenuLayer::Mode::LOAD);
                                      if (saveMenu)
                                      {
                                          saveMenu->setLoadSuccessCallback([this](const SaveSlotData &saveData)
                                                                           {
                                                                               if (_onLoadSuccess)
                                                                               {
                                                                                   _onLoadSuccess(saveData);
                                                                               } });
                                          if (_scene)
                                          {
                                              _scene->addChild(saveMenu, UI_Z_ORDER + 1);
                                          }
                                      }
                                      CCLOG("GameScene - 打开加载游戏菜单"); });
    }

    _scene->addChild(_gameUI, UI_Z_ORDER);
    CCLOG("GameUI initialized for level: %s", levelName.c_str());

    _updateAccumulator = UI_UPDATE_INTERVAL_SECONDS;
    _gameUI->updateDisplay();
    return true;
}

void GameUIController::update(float dt)
{
    if (!_gameUI)
        return;

    bool atGate = false;
    if (_isPlayerAtGate)
    {
        atGate = _isPlayerAtGate();
    }

    if (atGate && !_wasAtGate)
    {
        _gameUI->showInteractionHint(GATE_INTERACTION_HINT);
    }
    else if (!atGate && _wasAtGate)
    {
        _gameUI->hideInteractionHint();
    }
    _wasAtGate = atGate;

    _updateAccumulator += dt;
    if (_updateAccumulator < UI_UPDATE_INTERVAL_SECONDS)
    {
        return;
    }
    _updateAccumulator = 0.0f;

    _gameUI->updateDisplay();
}

void GameUIController::togglePauseMenu()
{
    if (!_gameUI)
        return;

    if (_gameUI->isPauseMenuShowing())
    {
        _gameUI->hidePauseMenu();
        _paused = false;
        if (_onPauseChanged)
        {
            _onPauseChanged(false);
        }
        CCLOG("Game resumed");
    }
    else
    {
        _gameUI->showPauseMenu();
        _paused = true;
        if (_onPauseChanged)
        {
            _onPauseChanged(true);
        }
        CCLOG("Game paused");
    }
}

/**
 * @file GameUIController.cpp
 * @brief GameScene UI 编排实现
 */

#include "Scenes/GameUIController.h"
#include "Character/Player/PlayerCharacter.h"
#include "Character/components/AttributeComponent.h"
#include "GameUI.h"
#include "Scenes/HelloWorldScene.h"
#include "Scenes/LoadingScene.h"
#include "Scenes/Layers/SaveMenuLayer.h"
#include "Managers/SceneRegistry.h"
#include "Save/SaveData.h"
#include "Save/SaveManager.h"
#include "UI/PauseMenu.h"
#include "UI/InventoryLayer.h"
#include "Configs/GameSceneConfig.h"

USING_NS_CC;

namespace
{
    const char *const GATE_INTERACTION_HINT = GameSceneConfig::UI::GATE_INTERACTION_HINT;
    constexpr int UI_Z_ORDER = GameSceneConfig::UI::Z_ORDER;
    constexpr float UI_UPDATE_INTERVAL_SECONDS = GameSceneConfig::UI::UPDATE_INTERVAL_SECONDS;
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

        pauseMenu->setInventoryCallback([this]()
                                       {
                                           if (!_gameUI)
                                               return;

                                           // 打开背包时保持暂停状态：隐藏暂停菜单，展示背包界面
                                           _gameUI->hidePauseMenu();
                                           _paused = true;
                                           if (_onPauseChanged)
                                           {
                                               _onPauseChanged(true);
                                           }
                                           _gameUI->showInventory();
                                       });
    }

    // 背包关闭后回到暂停菜单（仍保持暂停）
    if (auto inventory = _gameUI->getInventoryLayer())
    {
        inventory->setCloseCallback([this]()
                                    {
                                        if (!_gameUI)
                                            return;
                                        _paused = true;
                                        if (_onPauseChanged)
                                        {
                                            _onPauseChanged(true);
                                        }
                                        _gameUI->showPauseMenu();
                                    });
    }

    // 角色死亡菜单（强制暂停）
    if (auto deathMenu = _gameUI->getDeathMenu())
    {
        // 重新挑战：复活到满血满蓝并缓存运行时数据，然后通过 LoadingScene 重进当前关卡
        deathMenu->setRestartCallback([this, levelName]()
                                      {
                                          if (_player)
                                          {
                                              // 死亡时 HP=0 会被缓存，导致重进关卡立刻死亡；这里先复活到满血满蓝
                                              if (auto attr = _player->getAttributeComponent())
                                              {
                                                  _player->setCurrentHP(attr->getAttributeValue(AttributeType::MAX_HP));
                                                  _player->setCurrentMP(attr->getAttributeValue(AttributeType::MAX_MP));
                                              }
                                          }

                                          if (auto saveManager = SaveManager::getInstance())
                                          {
                                              saveManager->cacheRuntimePlayerData(_player);
                                              saveManager->clearRuntimePlayerPosition();
                                          }

                                          SceneID targetID = SceneID::NONE;
                                          if (auto registry = SceneRegistry::getInstance())
                                          {
                                              targetID = registry->getSceneIDByName(levelName);
                                          }
                                          if (targetID == SceneID::NONE)
                                          {
                                              CCLOG("GameUIController - 重新挑战失败：未找到关卡 [%s] 的 SceneID", levelName.c_str());
                                              return;
                                          }

                                          auto loadingScene = LoadingScene::createScene(targetID);
                                          if (!loadingScene)
                                          {
                                              CCLOG("GameUIController - 重新挑战失败：LoadingScene 创建失败");
                                              return;
                                          }

                                          float duration = GameSceneConfig::Scene::TRANSITION_DURATION;
                                          auto transition = TransitionFade::create(duration, loadingScene, Color3B::BLACK);
                                          Director::getInstance()->replaceScene(transition);
                                      });

        // 返回地图：复活到满血满蓝（避免缓存 0 血），然后走外部回调
        deathMenu->setReturnToMapCallback([this]()
                                          {
                                              if (_player)
                                              {
                                                  if (auto attr = _player->getAttributeComponent())
                                                  {
                                                      _player->setCurrentHP(attr->getAttributeValue(AttributeType::MAX_HP));
                                                      _player->setCurrentMP(attr->getAttributeValue(AttributeType::MAX_MP));
                                                  }
                                              }

                                              if (_onReturnToMap)
                                              {
                                                  _onReturnToMap();
                                              }
                                          });
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
    // UI 更新节流，避免每帧刷新造成开销。
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

    // 角色死亡菜单显示时不允许打开/关闭暂停菜单，避免误恢复游戏
    if (_gameUI->isDeathMenuShowing())
    {
        return;
    }

    // 若背包界面正在显示，Esc 优先关闭背包并回到暂停菜单
    if (_gameUI->isInventoryShowing())
    {
        _gameUI->hideInventory();
        _gameUI->showPauseMenu();
        _paused = true;
        if (_onPauseChanged)
        {
            _onPauseChanged(true);
        }
        return;
    }

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

void GameUIController::showDeathMenu()
{
    if (!_gameUI)
    {
        return;
    }

    // 死亡菜单为强制暂停：先关闭其它 UI，避免重叠
    if (_gameUI->isInventoryShowing())
    {
        _gameUI->hideInventory();
    }
    if (_gameUI->isPauseMenuShowing())
    {
        _gameUI->hidePauseMenu();
    }

    _gameUI->showDeathMenu();
    _paused = true;
    if (_onPauseChanged)
    {
        _onPauseChanged(true);
    }
}

bool GameUIController::isDeathMenuShowing() const
{
    return _gameUI && _gameUI->isDeathMenuShowing();
}

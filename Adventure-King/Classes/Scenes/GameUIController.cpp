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
#include "UI/PlayerDeathMenu.h"
#include "UI/InventoryLayer.h"
#include "UI/BlessingNpcLayer.h"
#include "Configs/GameSceneConfig.h"

USING_NS_CC;

namespace
{
    const char *const GATE_INTERACTION_HINT = GameSceneConfig::UI::GATE_INTERACTION_HINT;
    const char *const BLESSING_NPC_INTERACTION_HINT = "按W进入赐福";
    constexpr int UI_Z_ORDER = GameSceneConfig::UI::Z_ORDER;
    constexpr float UI_UPDATE_INTERVAL_SECONDS = GameSceneConfig::UI::UPDATE_INTERVAL_SECONDS;
}

bool GameUIController::init(Scene *scene,
                            PlayerCharacter *player,
                            const std::string &levelName,
                            const std::function<void()> &onReturnToMap,
                            const std::function<void(bool paused)> &onPauseChanged,
                            const GameUIController::RequestSaveCallback &onRequestSave,
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
    _onRequestSave = onRequestSave;
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
    _gameUI->setInventoryButtonCallback([this]()
                                        {
                                            // HUD 背包按钮：等同于按 B
                                            this->toggleInventory();
                                        });

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

        pauseMenu->setSaveCallback([this]()
                                  {
                                      if (!_gameUI)
                                          return;

                                      // 需求：开始游戏强制选择存档位后，游戏内保存应写入“当前会话绑定的槽位”，不再弹出选槽 UI
                                      std::string hint;
                                      bool ok = false;
                                      if (_onRequestSave)
                                      {
                                          ok = _onRequestSave(hint);
                                      }
                                      else
                                      {
                                          hint = "未配置保存回调";
                                      }

                                      if (hint.empty())
                                      {
                                          hint = ok ? "保存成功" : "保存失败";
                                      }

                                      showToast(hint, ok ? Color3B(200, 255, 200) : Color3B(255, 180, 180));
                                      CCLOG("GameUIController - 手动保存：%s", ok ? "成功" : "失败");
                                  });

        pauseMenu->setLoadCallback([this]()
                                  {
                                      if (!_gameUI)
                                          return;

                                      _gameUI->hidePauseMenu();
                                      // 打开读档菜单时同样保持暂停，避免底层世界继续运行
                                      _paused = true;
                                      if (_onPauseChanged)
                                      {
                                          _onPauseChanged(true);
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
                                          saveMenu->setCloseCallback([this]()
                                                                     {
                                                                         if (!_gameUI)
                                                                         {
                                                                             return;
                                                                         }
                                                                         if (!_scene)
                                                                         {
                                                                             return;
                                                                         }
                                                                         auto runningScene = Director::getInstance()->getRunningScene();
                                                                         if (runningScene != _scene)
                                                                         {
                                                                             return;
                                                                         }
                                                                         if (_gameUI->getParent() != _scene)
                                                                         {
                                                                             return;
                                                                         }
                                                                         _gameUI->showPauseMenu();
                                                                         _paused = true;
                                                                         if (_onPauseChanged)
                                                                         {
                                                                             _onPauseChanged(true);
                                                                         }
                                                                     });
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
                                           _inventoryReturnToPauseOnClose = true;
                                           _paused = true;
                                           if (_onPauseChanged)
                                           {
                                               _onPauseChanged(true);
                                           }
                                           _gameUI->showInventory();
                                       });
    }

    // 背包关闭行为：根据“从哪进入背包”决定回到暂停菜单还是回到游戏
    if (auto inventory = _gameUI->getInventoryLayer())
    {
        inventory->setCloseCallback([this]()
                                    {
                                        if (!_gameUI)
                                            return;
                                        if (_inventoryReturnToPauseOnClose)
                                        {
                                            _paused = true;
                                            if (_onPauseChanged)
                                            {
                                                _onPauseChanged(true);
                                            }
                                            _gameUI->showPauseMenu();
                                        }
                                        else
                                        {
                                            _paused = false;
                                            if (_onPauseChanged)
                                            {
                                                _onPauseChanged(false);
                                            }
                                        }
                                    });
    }

    // 赐福 NPC 关闭后恢复游戏（赐福入口改为地图 NPC 交互，不再挂在暂停菜单里）
    if (auto blessing = _gameUI->getBlessingNpcLayer())
    {
        blessing->setCloseCallback([this]()
                                   {
                                       if (!_gameUI)
                                           return;
                                       _paused = false;
                                       if (_onPauseChanged)
                                       {
                                           _onPauseChanged(false);
                                       }
                                   });
    }

    // 角色死亡菜单（强制暂停）
    if (auto deathMenu = _gameUI->getDeathMenu())
    {
        // 重新挑战：复活到满血满蓝并缓存运行时数据，然后通过 LoadingScene 重进当前关卡
        deathMenu->setRestartCallback([this, levelName, deathMenu]()
                                      {
                                          if (deathMenu)
                                          {
                                              // 先隐藏死亡菜单，避免在转场淡入淡出期间残留显示
                                              deathMenu->hideImmediately();
                                          }

                                          if (_player)
                                          {
                                              // 死亡时 HP=0 会被缓存，导致重进关卡立刻死亡；这里先复活到满血满蓝
                                              // 说明：SaveManager::cacheRuntimePlayerData 会把当前 HP/MP 一并缓存，
                                              // 若不先复活到满血满蓝，重进关卡会沿用 0 血状态并立刻触发死亡菜单。
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
        deathMenu->setReturnToMapCallback([this, deathMenu]()
                                          {
                                              if (deathMenu)
                                              {
                                                  // 先隐藏死亡菜单，避免在转场淡入淡出期间残留显示
                                                  deathMenu->hideImmediately();
                                              }

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

void GameUIController::showToast(const std::string &text, const cocos2d::Color3B &color)
{
    if (!_scene)
    {
        return;
    }

    // 使用固定 tag，确保同一时刻只显示一条提示，避免连点/连存导致叠满屏幕
    constexpr int TOAST_TAG = 88001;
    if (auto old = _scene->getChildByTag(TOAST_TAG))
    {
        old->removeFromParent();
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();

    auto label = Label::createWithTTF(text, GameSceneConfig::Scene::DEFAULT_FONT_PATH, 28);
    if (!label)
    {
        return;
    }

    label->setColor(color);
    label->setOpacity(0);
    label->setTag(TOAST_TAG);
    label->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + visibleSize.height * 0.86f));
    _scene->addChild(label, UI_Z_ORDER + 200);

    // 弹幕效果：淡入 -> 停留 -> 淡出 -> 自销毁
    auto seq = Sequence::create(
        FadeIn::create(0.12f),
        DelayTime::create(1.1f),
        FadeOut::create(0.35f),
        RemoveSelf::create(),
        nullptr);
    label->runAction(seq);
}

void GameUIController::update(float dt)
{
    if (!_gameUI)
        return;

    // 交互提示优先级：NPC（赐福） > Gate（传送门）
    InteractionHintSource newHint = InteractionHintSource::NONE;
    if (_isPlayerAtNpc && _isPlayerAtNpc())
    {
        newHint = InteractionHintSource::BLESSING_NPC;
    }
    else if (_isPlayerAtGate && _isPlayerAtGate())
    {
        newHint = InteractionHintSource::GATE;
    }

    if (newHint != _hintSource)
    {
        if (newHint == InteractionHintSource::NONE)
        {
            _gameUI->hideInteractionHint();
        }
        else if (newHint == InteractionHintSource::BLESSING_NPC)
        {
            _gameUI->showInteractionHint(BLESSING_NPC_INTERACTION_HINT);
        }
        else // GATE
        {
            _gameUI->showInteractionHint(GATE_INTERACTION_HINT);
        }
        _hintSource = newHint;
    }

    _updateAccumulator += dt;
    // UI 更新节流，避免每帧刷新造成开销。
    if (_updateAccumulator < UI_UPDATE_INTERVAL_SECONDS)
    {
        return;
    }
    _updateAccumulator = 0.0f;

    _gameUI->updateDisplay();
}

void GameUIController::togglePauseMenu() // 此时该函数建议理解为“handleEscKey”
{
    if (!_gameUI) return;

    // 1. 角色死亡菜单显示时不允许任何操作
    if (_gameUI->isDeathMenuShowing()) return;

    // 2. 若背包正在显示：Esc 关闭背包并进入暂停菜单（Esc=暂停键）
    if (_gameUI->isInventoryShowing())
    {
        _gameUI->hideInventory();
        _paused = true;
        if (_onPauseChanged) _onPauseChanged(true);
        _gameUI->showPauseMenu();
        CCLOG("Inventory closed, pause menu opened");
        return;
    }

    // 若赐福 NPC 弹窗正在显示，Esc 优先关闭弹窗并恢复游戏
    if (_gameUI->isBlessingNpcShowing())
    {
        _gameUI->hideBlessingNpc();
        _paused = false;
        if (_onPauseChanged)
        {
            _onPauseChanged(false);
        }
        return;
    }
    if (_gameUI->isPauseMenuShowing())
    {
        _gameUI->hidePauseMenu();
        _paused = false;
        if (_onPauseChanged) _onPauseChanged(false);
        CCLOG("Pause menu closed, game resumed");
    }
    // 4. 默认行为：正常状态下按 Esc 呼出暂停菜单
    else
    {
        _paused = true;
        if (_onPauseChanged) _onPauseChanged(true);
        _gameUI->showPauseMenu();
        CCLOG("Pause menu opened, game paused");
    }
}

void GameUIController::toggleInventory()
{
    if (!_gameUI) return;

    // 角色死亡菜单显示时不允许任何操作
    if (_gameUI->isDeathMenuShowing()) return;

    // 赐福弹窗优先级更高，避免 UI 叠层乱序
    if (_gameUI->isBlessingNpcShowing()) return;

    // 背包已开：按 B / 点击按钮关闭（根据来源决定回到暂停菜单还是回到游戏）
    if (_gameUI->isInventoryShowing())
    {
        _gameUI->hideInventory();
        if (_inventoryReturnToPauseOnClose)
        {
            _paused = true;
            if (_onPauseChanged) _onPauseChanged(true);
            _gameUI->showPauseMenu();
        }
        else
        {
            _paused = false;
            if (_onPauseChanged) _onPauseChanged(false);
        }
        return;
    }

    // 暂停菜单显示中：B 直接进入背包（关闭背包后返回暂停菜单）
    if (_gameUI->isPauseMenuShowing())
    {
        _gameUI->hidePauseMenu();
        _inventoryReturnToPauseOnClose = true;
        _paused = true;
        if (_onPauseChanged) _onPauseChanged(true);
        _gameUI->showInventory();
        return;
    }

    // 正常游戏中：B 打开背包并暂停（关闭背包回到游戏）
    _inventoryReturnToPauseOnClose = false;
    _gameUI->showInventory();
    _paused = true;
    if (_onPauseChanged) _onPauseChanged(true);
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

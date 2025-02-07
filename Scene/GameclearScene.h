#pragma once
#include "Secne.h"
#include "../Util/Geometry.h"
#include <memory>//スマートポインタをつかうため
#include <array>
class Player;
/// <summary>
/// ゲームクリアシーン
/// </summary>
class GameclearScene : public Scene
{
public:
    GameclearScene(SceneManager& manager, std::shared_ptr<Player>player);
    virtual ~GameclearScene();

    void Update(const InputState& input);
    void Draw();
private:
    unsigned int m_fadeColor = 0x000000;//フェードの色（黒

    void FadeInUpdat(const InputState& input);
    void FadeOutUpdat(const InputState& input);
    void NormalUpdat(const InputState& input);
    void MojiUpdate(const InputState& input);

    void NormalDraw();
    void MojiDraw();

    void (GameclearScene::* m_updateFunc)(const InputState&);
    void (GameclearScene::* m_drawFunc)();

    //プレイヤー
    std::shared_ptr<Player> m_player;
    int m_playerH = -1;
    int m_idx = 0;
    int m_frame = 0;

    struct Moji
    {
        Position2 pos = {};
        float moveY = 0.0f;
        float add = 0.0f;
    };
    static constexpr int kMojiNum = 10;
    std::array<Moji, kMojiNum> m_moji;
    const wchar_t* const kMoji[GameclearScene::kMojiNum] =
    {
        L"G",
        L"a",
        L"m",
        L"e",
        L" ",
        L"C",
        L"l",
        L"e",
        L"a",
        L"r",
    };
    int m_soundVolume = 0;
};


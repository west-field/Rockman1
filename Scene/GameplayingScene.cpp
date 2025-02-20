#include "GameplayingScene.h"

#include <DxLib.h>
#include <cassert>

#include "SceneManager.h"
#include "TitleScene.h"
#include "PauseScene.h"
#include "GameoverScene.h"
#include "GameclearScene.h"

#include "../game.h"
#include "../Map.h"
#include "../InputState.h"
#include "../ObjectInfo.h"
#include "../Util/Sound.h"
#include "../Util/DrawFunctions.h"
#include "../Util/Graph.h"
#include "../Game/Player.h"
#include "../Game/EnemyFactory.h"
#include "../Game/ShotFactory.h"
#include "../Game/HpBar.h"
#include "../Game/ItemFactory.h"
#include "../Enemy/EnemyBase.h"
#include "../Item/ItemBase.h"

namespace
{
	constexpr float kPlayerMoveSpeed = 5.0f;//プレイヤーの移動速度
	constexpr float kJumpAcc = 11.0f;//ジャンプ力
	constexpr float kShotSpeed = 10.0f;//ショットスピード

	constexpr float kPullPos = 15.0f;//梯子を上る範囲を決める
}

GameplayingScene::GameplayingScene(SceneManager& manager) : Scene(manager), m_updateFunc(&GameplayingScene::FadeInUpdat),
	m_add(), m_fallPlayerSpeed(0.0f), m_isPlayerCenterLR(false),
	m_isScreenMoveUp(false), m_isScreenMoveDown(false), m_isScreenMoveWidth(false), m_correction(), m_playerPos(),
	m_isLadder(false),m_isLadderAlign(false),m_isLadderFirst(false), m_isFall(false),m_isBoss(false),m_crea(0),m_isFirst(false),
	m_bossBgm(-1), m_soundVolume(-1),m_tempScreenH(-1),m_quakeTimer(0),m_quakeX(0.0f)
{
	int se = 0, sh = 0, bit = 0;
	GetScreenState(&se, &sh, &bit);
	m_tempScreenH = MakeScreen(se, sh);
	assert(m_tempScreenH >= 0);

	m_hp[Object_Player] = std::make_shared<HpBar>(Position2{ static_cast<float>(Game::kMapScreenLeftX - 100) ,Game::kScreenHeight / 3 });
	m_hp[Object_EnemyBoss] = std::make_shared<HpBar>(Position2{ static_cast<float>(Game::kMapScreenRightX + 40) ,Game::kScreenHeight / 3 });
	//HPバーのグラフィック
	for (auto& hp : m_hp)
	{
		hp->Init(my::MyLoadGraph(L"Data/hpbar.png"));
	}
	//プレイヤー
	m_player = std::make_shared<Player>(Position2{(Game::kMapScreenLeftX- Game::kDrawSize /2),(Game::kMapScreenBottomY - Game::kDrawSize *5)},m_hp[Object_Player]);//プレイヤーの初期位置
	//敵の弾工場
	m_shotFactory = std::make_shared<ShotFactory>();
	//アイテム工場
	m_itemFactory = std::make_shared<ItemFactory>();
	//敵工場
	m_enemyFactory = std::make_shared<EnemyFactory>(m_player, m_shotFactory, m_itemFactory, m_hp[Object_EnemyBoss]);//プレイヤーとショットと敵を倒した数を渡す
	//マップ
	m_map = std::make_shared<Map>(m_enemyFactory,0);
	m_map->Load(L"Data/map/mapmini.fmf");

	//開始位置
	Position2 pos = { Game::kMapScreenLeftX,((Game::kMapChipNumY * Game::kDrawSize) - Game::kMapScreenBottomY) * -1.0f };
	//Position2 pos = { -7233.0f,-2076.0f };//ボス戦前
	m_map->Movement(pos);
	m_add = pos * -1.0f;
	//背景
	Background::GetInstance().Init();

	//BGM
	m_BgmH = LoadSoundMem(L"Data/Sound/BGM/Disital_Delta.mp3");
	m_bossBgm = LoadSoundMem(L"Data/Sound/BGM/arabiantechno.mp3");
	ChangeVolumeSoundMem(0, m_BgmH);
	PlaySoundMem(m_BgmH, DX_PLAYTYPE_LOOP, true);
}

GameplayingScene::~GameplayingScene()
{
	DeleteSoundMem(m_BgmH);
	DeleteSoundMem(m_bossBgm);
	DeleteGraph(m_tempScreenH);
}

void GameplayingScene::Update(const InputState& input)
{
	//背景を常に移動させる
	Background::GetInstance().Update();
	(this->*m_updateFunc)(input);
}

void GameplayingScene::Draw()
{
	//加工用スクリーンハンドル
	SetDrawScreen(m_tempScreenH);
	Background::GetInstance().Bg();//背景の一部を表示
	m_map->Draw();//マップを表示
	m_itemFactory->Draw(m_correction);//アイテム表示

	m_player->Draw();//プレイヤーを表示
	m_enemyFactory->Draw();//エネミーを表示
	
	m_shotFactory->Draw();//敵ショット表示

	//枠を表示
	DrawBox(Game::kMapScreenLeftX, Game::kMapScreenTopY, Game::kMapScreenLeftX - Game::kDrawSize, Game::kMapScreenBottomY, m_framecolor, true);//左側
	DrawBox(Game::kMapScreenRightX, Game::kMapScreenTopY, Game::kMapScreenRightX + Game::kDrawSize, Game::kMapScreenBottomY, m_framecolor, true);//右側
	DrawBox(Game::kMapScreenLeftX - Game::kDrawSize, Game::kMapScreenTopY - Game::kDrawSize, Game::kMapScreenRightX + Game::kDrawSize, Game::kMapScreenTopY, m_framecolor, true);//上
	DrawBox(Game::kMapScreenLeftX - Game::kDrawSize, Game::kMapScreenBottomY + Game::kDrawSize, Game::kMapScreenRightX + Game::kDrawSize, Game::kMapScreenBottomY, m_framecolor, true);//下

	m_hp[Object_Player]->Draw();//HPバーを表示
	if (m_isBoss)
	{
		if (m_hp[Object_EnemyBoss]->GetHp() > 0)
		{
			m_hp[Object_EnemyBoss]->Draw();
		}
	}

#ifdef _DEBUG
	for (auto& enemy : m_enemyFactory->GetEnemies())
	{
		if (!enemy->IsExist())	continue;

		DrawFormatString(500, 0, 0x00aaff, L"x%3f,y%3f", enemy->GetRect().GetCenter().x, enemy->GetRect().GetCenter().y);
	}
	//梯子に上れる範囲
	DrawBoxAA(
		 m_player->GetRect().GetCenter().x - m_player->GetRect().GetSize().w / 2 + kPullPos
		,m_player->GetRect().GetCenter().y - m_player->GetRect().GetSize().h / 2 + 2.0f
		,m_player->GetRect().GetCenter().x + m_player->GetRect().GetSize().w / 2 - kPullPos
		,m_player->GetRect().GetCenter().y + m_player->GetRect().GetSize().h / 2 + 1.5f
		, 0x0000ff, false);
	int color = 0x000000;
	if (m_isPlayerCenterLR){
		DrawFormatString(150, 40, 0xff00ff, L"lR,true");
	}
	else{
		DrawFormatString(150, 40, 0xff00ff, L"lR,false");
	}
	if (m_isScreenMoveUp){
		DrawFormatString(150, 60, 0xff00ff, L"uD,true");
	}
	else{
		DrawFormatString(150, 60, 0xff00ff, L"uD,false");
	}
	if (m_isLadder){
		DrawFormatString(300, 80, 0xff00ff, L"Ladder,true");
	}
	else {
		DrawFormatString(300, 80, 0xff00ff, L"Ladder,false"); 
	}
	if (m_isLadderFirst){
		DrawFormatString(300, 100, 0xff00ff, L"LadderFirst,true");
	}
	else{
		DrawFormatString(300, 100, 0xff00ff, L"LadderFirst,false");
	}
	int centeridxX = Game::kNumX / 2;
	int centeridxY = Game::kNumY / 2;
	Position2 fieldCenterLeftUp =
	{
		static_cast<float>(centeridxX * Game::kDrawSize),
		static_cast<float>(centeridxY * Game::kDrawSize)
	};
	Position2 fieldCenterRightBottom =
	{
		fieldCenterLeftUp.x + Game::kDrawSize,
		fieldCenterLeftUp.y + Game::kDrawSize
	};
	DrawBoxAA(fieldCenterLeftUp.x, fieldCenterLeftUp.y, fieldCenterRightBottom.x, fieldCenterRightBottom.y, 0xaaffaa, false);//画面の中心位置

	DrawFormatString(0, 80, color, L"player.x%3f,y.%3f", m_player->GetRect().center.x+m_add.x, m_player->GetRect().center.y+m_add.y);//プレイヤーと足す座標
	DrawFormatString(0, 100, color, L"player.x%3f,y.%3f", m_player->GetRect().center.x, m_player->GetRect().center.y);//プレイヤー座標
	DrawFormatString(0, 120, color, L"add.x%3f,y.%3f",m_add.x, m_add.y);//画面がどのくらい移動したか
	DrawFormatString(0, 160, color, L"m_correction.x%3f,y.%3f", m_correction.x, m_correction.y);//画面がどのくらい移動したか

	//表示したいマップ画面
	DrawBox(Game::kMapScreenLeftX, Game::kMapScreenTopY, Game::kMapScreenRightX, Game::kMapScreenBottomY, color, false);
	DrawFormatString(Game::kMapScreenLeftX, Game::kMapScreenTopY, color, L"%d", Game::kMapScreenLeftX);
	DrawFormatString(Game::kMapScreenRightX, Game::kMapScreenTopY, color, L"%d",Game::kMapScreenTopY);
	DrawFormatString(Game::kMapScreenLeftX, Game::kMapScreenBottomY, color, L"%d",Game::kMapScreenBottomY);
	DrawFormatString(Game::kMapScreenRightX, Game::kMapScreenBottomY, color, L"%d",Game::kMapScreenRightX);
	//判定する範囲
	DrawBox(Game::kMapScreenLeftX, Game::kMapScreenTopY, Game::kMapScreenRightX, Game::kMapScreenBottomY - 1, 0xffaaaa, false);

	float posX = m_add.x + m_player->GetRect().GetCenter().x;
	float posY = m_add.y + m_player->GetRect().GetCenter().y;
	//梯子の位置
	Vector2 lader = m_map->GetMapChipPos(posX + m_player->GetRect().GetSize().w / 2 - kPullPos, posY);
	//移動させる量
	Vector2 aling = { (lader.x + m_map->GetPos().x) - (m_player->GetRect().GetCenter().x - m_player->GetRect().GetSize().w / 2 - 1.0f),
						(lader.y + m_map->GetPos().y) - (m_player->GetRect().GetCenter().y) };
	DrawBoxAA(lader.x + m_map->GetPos().x, lader.y+ Game::kDrawSize + m_map->GetPos().y,
		lader.x + Game::kDrawSize + m_map->GetPos().x, lader.y+ Game::kDrawSize + Game::kDrawSize + m_map->GetPos().y, 0xaaffaa, false);//現在いる場所(マップ)
	DrawBoxAA(lader.x + m_map->GetPos().x, lader.y + m_map->GetPos().y,
		lader.x + Game::kDrawSize + m_map->GetPos().x, lader.y + Game::kDrawSize + m_map->GetPos().y, 0xaaffaa, false);//現在いる場所(マップ)の1つ下のマス
	DrawFormatString(500, 100, 0x000000, L"梯子.x%3f,y%3f", lader.x, lader.y);
	DrawFormatString(500, 120, 0x000000, L"移動.x%3f,y%3f", aling.x, aling.y);

	DrawFormatString(500, 500, 0x000000, L"playerPos.%3f", m_playerPos);

	DrawFormatString(500, 540, 0x000000, L"m_soundVolume.%d", m_soundVolume);
#endif
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, m_fadeValue);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, m_fadeColor, true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	//裏画面に戻す
	SetDrawScreen(DX_SCREEN_BACK);

	DrawGraphF(m_quakeX, 0, m_tempScreenH, true);
	if (m_quakeTimer > 0)
	{
		//当たった時光らせる
		SetDrawBlendMode(DX_BLENDMODE_ADD, 50);//加算合成
		DrawGraph(static_cast<int>(m_quakeX), 0, m_tempScreenH, true);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);//もとに戻す
	}
}

//当たり判定を調べて、当たっていなかったら移動させる
void GameplayingScene::Move(std::shared_ptr<Object> object, float MoveX, float MoveY)
{
	float Dummy = 0.0f;
	float hsize, wsize;
	float moveX = 0.0f;
	float moveY = 0.5f;

	// キャラクタの左上、右上、左下、右下部分が当たり判定のあるマップに衝突しているか調べ、衝突していたら補正する
	// 半分のサイズ
	wsize = object->GetRect().GetSize().w * 0.5f;
	hsize = object->GetRect().GetSize().h * 0.5f;

	int ob = 0;

	switch (object->Type())
	{
	case ObjectType::Player:	ob = static_cast<int>(ObjectType::Player); break;
	case ObjectType::Enemy:		ob = static_cast<int>(ObjectType::Enemy); break;
	case ObjectType::Boss:		ob = static_cast<int>(ObjectType::Boss); break;
	default:
		break;
	}

	//プレイヤーとマップの当たり判定
	if (ob == static_cast<int>(ObjectType::Player))
	{
		// 左下のチェック、もしブロックの上辺に着いていたら落下を止める
		if (MapHitCheck(object->GetRect().GetCenter().x - wsize + m_add.x, object->GetRect().GetCenter().y + hsize + m_add.y, Dummy, MoveY) == 3)
		{
			m_fallPlayerSpeed = 0.0f;
		}
		// 右下のチェック、もしブロックの上辺に着いていたら落下を止める
		if (MapHitCheck(object->GetRect().GetCenter().x + wsize + m_add.x, object->GetRect().GetCenter().y + hsize + m_add.y, Dummy, MoveY) == 3)
		{
			m_fallPlayerSpeed = 0.0f;
		}
		// 左上のチェック、もしブロックの下辺に当たっていたら落下させる
		if (MapHitCheck(object->GetRect().GetCenter().x - wsize + m_add.x, object->GetRect().GetCenter().y - hsize + m_add.y, Dummy, MoveY) == 4)
		{
			m_fallPlayerSpeed *= -1.0f;
		}
		// 右上のチェック、もしブロックの下辺に当たっていたら落下させる
		if (MapHitCheck(object->GetRect().GetCenter().x + wsize + m_add.x, object->GetRect().GetCenter().y - hsize + m_add.y, Dummy, MoveY) == 4)
		{
			m_fallPlayerSpeed *= -1.0f;
		}
		object->Movement({ 0.0f,MoveY });//移動

		// 後に左右移動成分だけでチェック
		// 左下のチェック
		MapHitCheck(m_add.x + object->GetRect().GetCenter().x - wsize, object->GetRect().GetCenter().y + hsize + m_add.y, MoveX, Dummy);

		// 右下のチェック
		MapHitCheck(m_add.x + object->GetRect().GetCenter().x + wsize, object->GetRect().GetCenter().y + hsize + m_add.y, MoveX, Dummy);

		// 左上のチェック
		MapHitCheck(m_add.x + object->GetRect().GetCenter().x - wsize, object->GetRect().GetCenter().y - hsize + m_add.y, MoveX, Dummy);

		// 右上のチェック
		MapHitCheck(m_add.x + object->GetRect().GetCenter().x + wsize, object->GetRect().GetCenter().y - hsize + m_add.y, MoveX, Dummy);

		object->Movement({ MoveX,0.0f });//移動

		// 接地判定 キャラクタの左下と右下の下に地面があるか調べる
		if ((m_map->GetMapEventParam(m_add.x + object->GetRect().GetCenter().x - wsize, m_add.y + object->GetRect().GetCenter().y + hsize + 1.0f) == MapEvent_hit) ||
			(m_map->GetMapEventParam(m_add.x + object->GetRect().GetCenter().x + wsize, m_add.y + object->GetRect().GetCenter().y + hsize + 1.0f) == MapEvent_hit))
		{
			//当たり判定のある場所に来たら音を鳴らす
			if (object->IsJump())
			{
				SoundManager::GetInstance().Play(SoundId::BlockMove);
			}

			//足場があったら設置中にする
			m_fallPlayerSpeed = 0.0f;
			m_isFall = false;
			object->SetJump(false);
			m_isLadderFirst = false;
		}
		//梯子があったら足場がある判定にする
		else if ((m_map->GetMapEventParam(m_add.x + object->GetRect().GetCenter().x + object->GetRect().GetSize().w / 2 - kPullPos, m_add.y + object->GetRect().GetCenter().y + object->GetRect().GetSize().h / 2 + 1.0f) == MapEvent_ladder) ||
			(m_map->GetMapEventParam(m_add.x + object->GetRect().GetCenter().x - object->GetRect().GetSize().w / 2 + kPullPos, m_add.y + object->GetRect().GetCenter().y + object->GetRect().GetSize().h / 2 + 1.0f) == MapEvent_ladder))
		{
			m_isFall = false;
			m_fallPlayerSpeed = 0.0f;
			object->SetJump(false);
		}
		else
		{
			//ないときはジャンプ中にする
			object->SetJump(true);
		}
		return;
	}
	else if (ob == static_cast<int>(ObjectType::Enemy))
	{
		//移動量
		moveX = object->GetVec().x;

		object->Movement({ MoveX,MoveY });//画面移動
		//画面の左端に消えたら敵を消す
		if (object->GetRect().GetCenter().x + wsize < Game::kMapScreenLeftX)
		{
			object->EraseExist();
		}
		//画面の下端に消えたら敵を消す
		else if (object->GetRect().GetCenter().y - hsize > Game::kMapScreenBottomY)
		{
			object->EraseExist();
		}
		//画面の上端に消えたら敵を消す
		else if (object->GetRect().GetCenter().y + hsize < Game::kMapScreenTopY)
		{
			object->EraseExist();
		}
		//向いている方向で当たっているか判定する
		if (object->IsLeft())
		{
			object->GetChip(m_map->GetMapEventParam(m_add.x + object->GetRect().GetCenter().x - wsize - moveX, m_add.y + object->GetRect().GetCenter().y + hsize - moveY));
		}
		else
		{
			object->GetChip(m_map->GetMapEventParam(m_add.x + object->GetRect().GetCenter().x + wsize + moveX, m_add.y + object->GetRect().GetCenter().y + hsize - moveY));
		}
		return;
	}
	else if (ob == static_cast<int>(ObjectType::Boss))
	{
		Dummy = 0.0f;
		float PosX, PosY;

		//移動量
		moveX = object->GetVec().x;

		object->Movement({ MoveX,MoveY });//画面移動

		//プレイヤーの中心位置
		PosX = object->GetRect().GetCenter().x + m_add.x;
		PosY = object->GetRect().GetCenter().y + m_add.y;

		moveY = object->GetVec().y;

		// 左下のチェック、もしブロックの上辺に着いていたら落下を止める
		if (MapHitCheck(PosX - wsize, PosY + hsize, Dummy, moveY) == 3)
		{
			m_fallPlayerSpeed = 0.0f;
		}
		// 右下のチェック、もしブロックの上辺に着いていたら落下を止める
		if (MapHitCheck(PosX + wsize, PosY + hsize, Dummy, moveY) == 3)
		{
			m_fallPlayerSpeed = 0.0f;
		}
		// 左上のチェック、もしブロックの下辺に当たっていたら落下させる
		if (MapHitCheck(PosX - wsize, PosY - hsize, Dummy, moveY) == 4)
		{
			m_fallPlayerSpeed *= -1.0f;
		}
		// 右上のチェック、もしブロックの下辺に当たっていたら落下させる
		if (MapHitCheck(PosX + wsize, PosY - hsize, Dummy, moveY) == 4)
		{
			m_fallPlayerSpeed *= -1.0f;
		}
		object->Movement({ 0.0f,moveY });

		//ジャンプしているときだけ横に移動する
		if (object->IsJump())
		{
			// 後に左右移動成分だけでチェック
			// 左下のチェック
			MapHitCheck(PosX - wsize, PosY + hsize, moveX, Dummy);

			// 右下のチェック
			MapHitCheck(PosX + wsize, PosY + hsize, moveX, Dummy);

			// 左上のチェック
			MapHitCheck(PosX - wsize, PosY - hsize, moveX, Dummy);

			// 右上のチェック
			MapHitCheck(PosX + wsize, PosY - hsize, moveX, Dummy);
			//左右移動成分を加算
			object->Movement({ moveX,0.0f });
		}

		// 接地判定 キャラクタの左下と右下の下に地面があるか調べる
		//当たり判定のある場所に来たら音を鳴らして足場がある判定にする
		if ((m_map->GetMapEventParam(PosX - wsize, PosY + hsize + 1.0f) == MapEvent_hit) ||
			(m_map->GetMapEventParam(PosX + wsize, PosY + hsize + 1.0f) == MapEvent_hit))
		{
			//足場があったら設置中にする
			object->SetJump(false);
		}
		else
		{
			//ないときはジャンプ中にする
			object->SetJump(true);
		}
		return;
	}
}

//マップとの当たり判定
int GameplayingScene::MapHitCheck(float X, float Y, float& MoveX, float& MoveY)
{
	float afterX, afterY;
	//移動量を足す
	afterX = X + MoveX;
	afterY = Y + MoveY;
	float blockLeftX = 0.0f, blockTopY = 0.0f, blockRightX = 0.0f, blockBottomY = 0.0f;

	//整数値へ変換
	int noX = static_cast<int>(afterX / Game::kDrawSize);
	int noY = static_cast<int>(afterY / Game::kDrawSize);

	//当たっていたら壁から離す処理を行う、ブロックの左右上下の座標を算出
	blockLeftX = static_cast<float>(noX * Game::kDrawSize);//左　X座標
	blockRightX = static_cast<float>((noX + 1) * Game::kDrawSize);//右　X座標
	blockTopY = static_cast<float>(noY * Game::kDrawSize);//上　Y座標
	blockBottomY = static_cast<float>((noY + 1) * Game::kDrawSize);//下　Y座標

	int mapchip = m_map->GetMapEventParam(afterX, afterY);
	//当たり判定のあるブロックに当たっているか
	if (mapchip  == MapEvent_hit)
	{
		//ブロックの上に当たっていた場合
		if (MoveY > 0.0f)
		{
			//移動量を補正する
			MoveY = blockTopY - Y  - 1.0f;
			//上辺に当たったと返す
			return 3;
		}
		//ブロックの下に当たっていた場合
		if (MoveY < 0.0f)
		{
			MoveY = blockBottomY - Y + 1.0f;
			//下辺に当たったと返す
			return 4;
		}
		//ブロックの左に当たっていた場合
		if (MoveX > 0.0f)
		{
			MoveX = blockLeftX - X - 1.0f;
			//左辺に当たったと返す
			return 1;
		}
		//ブロックの右に当たっていた場合
		if (MoveX < 0.0f)
		{
			MoveX = blockRightX - X + 1.0f;
			//右辺に当たったと返す
			return 2;
		}
		//ここに来たら適当な値を返す
		return 4;
	}
	//どこにも当たらなかったと返す
	return 0;
}

//画面を移動させられるかどうか
void GameplayingScene::ScreenMove()
{
	Position2 fieldCenterLeftUp =//フィールドの中心位置（左上）
	{
		static_cast<float>(Game::kNumX / 2 * Game::kDrawSize),
		static_cast<float>(Game::kNumY / 2 * Game::kDrawSize)
	};
	Position2 fieldCenterRightBottom =//フィールドの中心位置（右下）
	{
		fieldCenterLeftUp.x + Game::kDrawSize,
		fieldCenterLeftUp.y + Game::kDrawSize
	};

	m_isPlayerCenterLR = false;
	m_isScreenMoveUp = false;
	m_isScreenMoveDown = false;
	m_isScreenMoveWidth = false;

	if (m_player->GetRect().GetCenter().x > fieldCenterLeftUp.x && m_player->GetRect().GetCenter().x < fieldCenterRightBottom.x)
	{
		m_isPlayerCenterLR = true;
	}

	//縦移動できる場所についたら移動させる 右側の下と上
	if ((m_map->GetMapEventParam(m_add.x + m_player->GetRect().GetCenter().x + m_player->GetRect().GetSize().w / 2 - kPullPos, m_add.y + m_player->GetRect().GetCenter().y + m_player->GetRect().GetSize().h / 2 + 1.0f) == MapEvent_screenMoveW) &&
		(m_map->GetMapEventParam(m_add.x + m_player->GetRect().GetCenter().x + m_player->GetRect().GetSize().w / 2 - kPullPos, m_add.y + m_player->GetRect().GetCenter().y - m_player->GetRect().GetSize().h / 2 + 2.0f) == MapEvent_screenMoveW))
	{
		m_isScreenMoveWidth = true;
	}
	//プレイヤーの上座標がフィールドの上に当たっていたら移動できる
	if (m_player->GetRect().GetCenter().y < fieldCenterRightBottom.y &&
		(m_map->GetMapEventParam(m_add.x + m_player->GetRect().GetCenter().x - m_player->GetRect().GetSize().w / 2, m_add.y + m_player->GetRect().GetCenter().y + m_player->GetRect().GetSize().h / 2 + 1.0f) == MapEvent_screenMoveU)&&
		(m_map->GetMapEventParam(m_add.x + m_player->GetRect().GetCenter().x + m_player->GetRect().GetSize().w / 2 - kPullPos, m_add.y + m_player->GetRect().GetCenter().y + m_player->GetRect().GetSize().h / 2 + 1.0f) == MapEvent_screenMoveU))
	{
		m_isScreenMoveUp = true;
	}
	//プレイヤーの下座標が縦移動できる場所についたら移動させる
	else if (/*!m_isScreenMoveUp &&*/
		(m_map->GetMapEventParam(m_add.x + m_player->GetRect().GetCenter().x - m_player->GetRect().GetSize().w / 2, m_add.y + m_player->GetRect().GetCenter().y - m_player->GetRect().GetSize().h / 2 - 1.0f) == MapEvent_screenMoveD) &&
		(m_map->GetMapEventParam(m_add.x + m_player->GetRect().GetCenter().x + m_player->GetRect().GetSize().w / 2 - kPullPos, m_add.y + m_player->GetRect().GetCenter().y - m_player->GetRect().GetSize().h / 2 - 1.0f) == MapEvent_screenMoveD))
	{
		m_isScreenMoveDown = true;
	}

	if (!m_isScreenMoveUp && !m_isScreenMoveDown && !m_isScreenMoveWidth)
	{
		m_isFirst = false;
	}

	return;
}

//マップを動かしてキャラクターがマップと当たるかどうか
void GameplayingScene::MoveMap(float MoveX, float MoveY)
{
	float Dummy = 0.0f;
	float wsize = m_player->GetRect().GetSize().w * 0.5f;
	float hsize = m_player->GetRect().GetSize().h * 0.5f;

	MoveX *= -1.0f;
	MoveY *= -1.0f;

	// 左下のチェック、もしブロックの上辺に着いていたら落下を止める
	if (MapHitCheck(m_player->GetRect().GetCenter().x - wsize + m_add.x, m_player->GetRect().GetCenter().y + hsize + m_add.y, Dummy, MoveY) == 3)
	{
		m_fallPlayerSpeed = 0.0f;
	}
	// 右下のチェック、もしブロックの上辺に着いていたら落下を止める
	if (MapHitCheck(m_player->GetRect().GetCenter().x + wsize + m_add.x, m_player->GetRect().GetCenter().y + hsize + m_add.y, Dummy, MoveY) == 3)
	{
		m_fallPlayerSpeed = 0.0f;
	}
	// 左上のチェック、もしブロックの下辺に当たっていたら落下させる
	if (MapHitCheck(m_player->GetRect().GetCenter().x - wsize + m_add.x, m_player->GetRect().GetCenter().y - hsize + m_add.y, Dummy, MoveY) == 4)
	{
		m_fallPlayerSpeed *= -1.0f;
	}
	// 右上のチェック、もしブロックの下辺に当たっていたら落下させる
	if (MapHitCheck(m_player->GetRect().GetCenter().x + wsize + m_add.x, m_player->GetRect().GetCenter().y - hsize + m_add.y, Dummy, MoveY) == 4)
	{
		m_fallPlayerSpeed *= -1.0f;
	}
	
	m_add += {0.0f, MoveY};
	MoveY *= -1.0f;
	m_map->Movement({ 0.0f,MoveY });
	m_correction.y += MoveY;

	// 左下のチェック
	MapHitCheck(m_player->GetRect().GetCenter().x - wsize + m_add.x, m_player->GetRect().GetCenter().y + hsize + m_add.y, MoveX, Dummy);
	// 右下のチェック
	MapHitCheck(m_player->GetRect().GetCenter().x + wsize + m_add.x, m_player->GetRect().GetCenter().y + hsize + m_add.y, MoveX, Dummy);
	// 左上のチェック
	MapHitCheck(m_player->GetRect().GetCenter().x - wsize + m_add.x, m_player->GetRect().GetCenter().y - hsize + m_add.y, MoveX, Dummy);
	// 右上のチェック
	MapHitCheck(m_player->GetRect().GetCenter().x + wsize + m_add.x, m_player->GetRect().GetCenter().y - hsize + m_add.y, MoveX, Dummy);

	m_add += { MoveX, 0.0f };
	MoveX *= -1.0f;
	m_map->Movement({ MoveX,0.0f });
	m_correction.x += MoveX;
	//終了
	return;
}

//梯子移動
void GameplayingScene::Ladder(const InputState& input)
{
	float posX = m_add.x + m_player->GetRect().GetCenter().x;
	float posY = m_add.y + m_player->GetRect().GetCenter().y;
	float PlayerMoveY = 0.0f;

	LadderAlign();
	Vector2 lader = m_map->GetMapChipPos(posX , posY);
	//上キーで梯子を上がれる
	if (input.IsPressed(InputType::up))
	{
		//下に梯子　　右-10と下+1 左-10と下+1
		if ((m_map->GetMapEventParam(lader.x, lader.y) == MapEvent_screenMoveU) ||
			(m_map->GetMapEventParam(lader.x, lader.y) == MapEvent_ladder))
		{
			LadderAlign();//梯子とプレイヤーの位置を合わせる

			m_isLadder = true;//梯子に上っている
			
			PlayerMoveY = 0.0f;//移動量初期化
			PlayerMoveY -= kPlayerMoveSpeed;//上を押しているときは上に移動する
		}

		//現在の位置が梯子ではないとき少し上に移動する
		if (m_isLadder && m_map->GetMapEventParam(lader.x, lader.y) == MapEvent_no)
		{
			m_isLadder = false;
			Vector2 aling = { (lader.x + m_map->GetPos().x) - (m_player->GetRect().GetCenter().x - m_player->GetRect().GetSize().w / 2 - 1.0f),
					(lader.y + m_map->GetPos().y) - (m_player->GetRect().GetCenter().y) };//移動させる量
			Move(m_player,0.0f, aling.y);//移動
		}
	}
	//下キーで梯子を下がれる　
	else if (input.IsPressed(InputType::down))
	{
		//上下どちらかに梯子があるとき
		if ((m_map->GetMapEventParam(lader.x, lader.y) == MapEvent_ladder) ||
			(m_map->GetMapEventParam(lader.x, lader.y) == MapEvent_screenMoveD) ||
			(m_map->GetMapEventParam(lader.x, lader.y + Game::kDrawSize) == MapEvent_ladder)||
			(m_map->GetMapEventParam(lader.x, lader.y + Game::kDrawSize) == MapEvent_screenMoveD))
		{
			m_isLadder = true;
			PlayerMoveY = 0.0f;
			PlayerMoveY += kPlayerMoveSpeed;
		}
		if (m_isLadder && m_map->GetMapEventParam(lader.x, lader.y + Game::kDrawSize) != MapEvent_ladder)
		{
			m_isLadder = false;
			PlayerMoveY = 0.0f;
			PlayerMoveY += kPlayerMoveSpeed;
		}
	}
	else
	{
		m_isFall = true;
	}

	Move(m_player,0.0f, PlayerMoveY);
}

//梯子とプレイヤーの位置を合わせる
void GameplayingScene::LadderAlign()
{
	if (!m_isLadder)	return;

	float posX = m_add.x + m_player->GetRect().GetCenter().x;
	float posY = m_add.y + m_player->GetRect().GetCenter().y;
	//プレイヤーの位置を梯子の位置にする
	//移動する量＝プレイヤーの位置ー梯子の位置
	//梯子の位置
	Vector2 lader = m_map->GetMapChipPos(posX, posY);
	//移動させる量
	Vector2 aling = { (lader.x + m_map->GetPos().x) - (m_player->GetRect().GetCenter().x - m_player->GetRect().GetSize().w / 2 - 1.0f),
						(lader.y + m_map->GetPos().y) - (m_player->GetRect().GetCenter().y) };
	//移動
	Move(m_player,aling.x, 0.0f);
}

//プレイヤー登場シーン(ゲーム開始直後のみ)
void GameplayingScene::PlayerOnScreen(const InputState& input)
{
	float PlayerMoveY = 0.0f, PlayerMoveX = kPlayerMoveSpeed / 2;
	m_player->Update();

	if (!m_isLadder && !m_player->IsJump()&&!m_isFall)
	{
		PlayerMoveY -= 2.0f;
		m_player->Action(ActionType::grah_jump);
		m_player->SetJump(true);
		SoundManager::GetInstance().Play(SoundId::PlayerJump);
	}
	//プレイヤーがジャンプしていたら
	if (m_player->IsJump() && !m_isLadder)
	{
		// 落下処理
		m_fallPlayerSpeed += 0.4f;
		// 落下速度を移動量に加える
		PlayerMoveY = m_fallPlayerSpeed;
	}

	// 移動量に基づいてキャラクタの座標を移動
	Move(m_player,PlayerMoveX, PlayerMoveY);

	if (!m_player->IsJump())
	{
		m_updateFunc = &GameplayingScene::NormalUpdat;
	}
}

//画面のフェードイン
void GameplayingScene::FadeInUpdat(const InputState& input)
{
	m_fadeValue = 255 * m_fadeTimer / kFadeInterval;
	ChangeVolumeSoundMem(SoundManager::GetInstance().GetBGMVolume() - m_fadeValue, m_BgmH);
	if (--m_fadeTimer == 0)
	{
		m_updateFunc = &GameplayingScene::PlayerOnScreen;
		m_fadeValue = 0;
		return;
	}
}

//通常更新
void GameplayingScene::NormalUpdat(const InputState& input)
{
#ifdef _DEBUG
	#if false
		m_updateFunc = &GameplayingScene::FadeOutUpdat;
		m_fadeColor = 0x000000;
		m_crea = 0;
		return;
	#endif
#endif
	m_hp[Object_Player]->UpdatePlayer();
	ScreenMove();//プレイヤーがセンターに居るかどうか

	float PlayerMoveX = 0.0f, PlayerMoveY = 0.0f;//プレイヤーの移動
	m_correction = {0.0f,0.0f};
	//左に移動
	if (input.IsPressed(InputType::left))
	{
		if (input.IsTriggered(InputType::left))
		{
			m_player->SetLeft(true);
		}
		else
		{
			PlayerMoveX -= kPlayerMoveSpeed;
		
			m_player->SetLeft(true);
			m_player->Action(ActionType::grah_walk);
		}
	}
	//右に移動
	else if (input.IsPressed(InputType::right))
	{
		if (input.IsTriggered(InputType::right))
		{
			m_player->SetLeft(false);
		}
		else
		{
			//プレイヤーが画面中央にいる＆マップの表示できる範囲以内の時は動かせる
			if (m_isPlayerCenterLR &&
				m_map->GetMapEventParam(m_add.x + Game::kMapScreenRightX, m_add.y + Game::kMapScreenTopY) != MapEvent_screen &&
				m_map->GetMapEventParam(m_add.x + Game::kMapScreenRightX, m_add.y + Game::kMapScreenBottomY - 1.0f) != MapEvent_screen &&
				m_map->GetMapEventParam(m_add.x + Game::kMapScreenRightX, m_add.y + Game::kMapScreenTopY) != MapEvent_pause &&
				m_map->GetMapEventParam(m_add.x + Game::kMapScreenRightX, m_add.y + Game::kMapScreenBottomY - 1.0f) != MapEvent_pause)
			{
				MoveMap(-kPlayerMoveSpeed, 0.0f);
			}
			else
			{
				PlayerMoveX += kPlayerMoveSpeed;
			}
			m_player->SetLeft(false);
			m_player->Action(ActionType::grah_walk);
		}
	}

	//梯子移動
	Ladder(input);
	
	//プレイヤージャンプ処理
	if (!m_isFall&&!m_isLadder && !m_player->IsJump() && input.IsTriggered(InputType::junp))
	{
		//Button::PushButton(Button::ButtonType_A);

		m_fallPlayerSpeed = -kJumpAcc;
		m_player->Action(ActionType::grah_jump);
		m_player->SetJump(true);
		SoundManager::GetInstance().Play(SoundId::PlayerJump);
	}

	//プレイヤーがジャンプしていたら
	if (m_player->IsJump() && !m_isLadder)
	{
		// 落下処理
		m_fallPlayerSpeed += 0.4f;
		// 落下速度を移動量に加える
		PlayerMoveY = m_fallPlayerSpeed;
	}

	if (m_isScreenMoveUp || m_isScreenMoveDown || m_isScreenMoveWidth)
	{
		m_updateFunc = &GameplayingScene::MoveMapUpdat;
	}

	// 移動量に基づいてキャラクタの座標を移動
	Move(m_player, PlayerMoveX, PlayerMoveY);

	m_map->Update();		//	マップ更新
	m_player->Update();		//	プレイヤー更新
	//エネミー
	float MoveX = m_correction.x, MoveY = m_correction.y;
	//MoveEnemy(MoveX, MoveY);

	for (auto& enemy : m_enemyFactory->GetEnemies())
	{
		if (!enemy->IsExist())	continue;
		Move(enemy,MoveX, MoveY);
	}

	m_enemyFactory->Update();

	m_shotFactory->Update();//ショット更新
	m_itemFactory->Update();//アイテム更新

	//ショット
	if (input.IsTriggered(InputType::shot))//shotを押したら弾を作る
	{
		m_shotFactory->Create(ShotType::RockBuster, m_player->GetRect().GetCenter(), { 0.0f,0.0f }, m_player->IsLeft(), true);
		SoundManager::GetInstance().Play(SoundId::PlayeyShot);
		m_player->Action(ActionType::grah_attack);
	}

	m_correction *= -1.0f;
	m_shotFactory->Movement({ m_correction.x, m_correction.y });

	//自機の弾と、敵機の当たり判定
	for (auto& enemy : m_enemyFactory->GetEnemies())
	{
		if (!enemy->IsExist())	continue;
		if (!enemy->IsCollidable())continue;//当たり判定対象になっていなかったら当たらない
		for (auto& shot : m_shotFactory->GetShot())
		{
			if (!shot->IsExist())	continue;
			if (!shot->IsPlayerShot())	continue;//プレイヤーが撃った弾ではなかったら処理しない

			//敵に弾がヒットした
			if (shot->GetRect().IsHit(enemy->GetRect()))
			{
				shot->EraseExist();
				enemy->Damage(shot->AttackPower());
				break;
			}
		}
	}

	//敵の弾と、プレイヤーの当たり判定
	if (m_player->IsCollidable())
	{
		for (auto& shot : m_shotFactory->GetShot())
		{
			if (!shot->IsExist())	continue;
			if (shot->IsPlayerShot())	continue;
			if (shot->GetRect().IsHit(m_player->GetRect()))
			{
				shot->EraseExist();
				m_player->Damage(shot->AttackPower());
				SoundManager::GetInstance().Play(SoundId::PlayeyHit);
				m_quakeX = 5.0f;
				m_quakeTimer = 40;
				return;
			}
		}
	}
	//プレイヤーと、敵の当たり判定
	if (m_player->IsCollidable())
	{
		for (auto& enemy : m_enemyFactory->GetEnemies())
		{
			if (!enemy->IsExist())	continue;
			if (!enemy->IsCollidable())continue;
			if (enemy->GetRect().IsHit(m_player->GetRect()))
			{
				m_player->Damage(enemy->TouchAttackPower());
				SoundManager::GetInstance().Play(SoundId::PlayeyHit);
				m_quakeX = 5.0f;
				m_quakeTimer = 40;
				break;
			}
		}
	}

	//自機とアイテムの当たり判定
	for (auto& item : m_itemFactory->GetItems())
	{
		if (!item->IsExist()) continue;
		if (item->GetRect().IsHit(m_player->GetRect()))
		{
			SoundManager::GetInstance().Play(SoundId::Recovery);

			switch (item->OnGet())
			{
			case ItemType::Heal:
				m_player->Heal(item->GetHeal());
			default:
				break;
			}
			item->EraseExist();
			continue;
		}
	}

	//ゲームオーバー判定
	//プレイヤーのHPが０になったらゲームオーバーにする
	if (m_hp[Object_Player]->GetHp() <= 0)
	{
		m_updateFunc = &GameplayingScene::FadeOutUpdat;
		m_fadeColor = 0xff0000;
		m_crea = 1;
		return;
	}
	float posX = m_add.x + m_player->GetRect().GetCenter().x;
	float posY = m_add.y + m_player->GetRect().GetCenter().y;
	//プレイヤーの上座標が　death判定の部分に触れたら
	if ((m_map->GetMapEventParam(posX - m_player->GetRect().GetSize().w * 0.5f, posY - m_player->GetRect().GetSize().h / 2 + 10) == MapEvent_death) &&
		(m_map->GetMapEventParam(posX + m_player->GetRect().GetSize().w * 0.5f, posY - m_player->GetRect().GetSize().h / 2 + 10) == MapEvent_death))
	{
		m_updateFunc = &GameplayingScene::FadeOutUpdat;
		m_fadeColor = 0xff0000;
		m_crea = 1;
		return;
	}

	//ポーズ画面
	if (input.IsTriggered(InputType::pause))
	{
		SoundManager::GetInstance().Play(SoundId::MenuOpen);
		int sound = m_BgmH;
		if (m_isBoss)
		{
			sound = m_bossBgm;
		}
		m_manager.PushScene(new PauseScene(m_manager, sound));
		return;
	}

	if (m_isBoss)
	{
		m_hp[Object_EnemyBoss]->UpdateBoss();
		if (m_soundVolume++ >= SoundManager::GetInstance().GetBGMVolume())
		{
			m_soundVolume = SoundManager::GetInstance().GetBGMVolume();
		}
		else
		{
			ChangeVolumeSoundMem(m_soundVolume, m_bossBgm);
		}
		//エネミーのHPが０になったらゲームクリア
		for (auto& enemy : m_enemyFactory->GetEnemies())
		{
			if(!enemy->IsCollidable() && m_quakeTimer == 0)
			{
				m_quakeX = 10.0f;
				m_quakeTimer = 61*2;
			}

			if (!enemy->IsExist())
			{
				m_quakeX = 0.0f;
				m_quakeTimer = 0;

				m_updateFunc = &GameplayingScene::FadeOutUpdat;
				m_fadeColor = 0x000000;
				m_crea = 0;
				return;
			}
		}
	}
	
	if (m_quakeTimer > 0)
	{
		m_quakeX = -m_quakeX;
		m_quakeX *= 0.95f;
		m_quakeTimer--;
	}
	else
	{
		m_quakeX = 0.0f;
		m_quakeTimer = 0;
	}
	
}

//画面移動の更新
void GameplayingScene::MoveMapUpdat(const InputState& input)
{
	//移動する前にすべて消す
	if (!m_isFirst)
	{
		m_isFirst = true;
		//エネミーを削除する
		for (auto& enemy : m_enemyFactory->GetEnemies())
		{
			if (!enemy->IsExist())	continue;
			enemy->EraseExist();
		}
		//ショットを削除する
		for (auto& shot : m_shotFactory->GetShot())
		{
			if (!shot->IsExist())	continue;
			shot->EraseExist();
		}
		//アイテムを削除する
		for (auto& item : m_itemFactory->GetItems())
		{
			if (!item->IsExist()) continue;
			item->EraseExist();
		}
		m_correction = { 0.0f,0.0f };
		if (m_isScreenMoveUp) 
		{
			//プレイヤーの下座標を取得する
			m_playerPos = (m_player->GetRect().GetCenter().y - m_player->GetRect().GetSize().h / 2 );
		}
		else if (m_isScreenMoveDown)
		{
			//プレイヤーの上座標を取得する
			m_playerPos = (m_player->GetRect().GetCenter().y + m_player->GetRect().GetSize().h / 2);
		}
		if (m_isScreenMoveWidth)
		{
			//ボス戦BGMと入れ替える
			m_soundVolume = 0;
			ChangeVolumeSoundMem(m_soundVolume, m_bossBgm);
			PlaySoundMem(m_bossBgm, DX_PLAYTYPE_LOOP, true);
		}
	}

	//今いる場所が画面の下になるように移動させる
	m_map->Update();
	
	float moveY = (Game::kMapNumY * Game::kDrawSize) / 120.0f;
	float moveX = ((Game::kMapNumX * Game::kDrawSize) / 120.0f) * -1.0f;

	//上に移動するとき
	if (m_isScreenMoveUp)
	{
		//プレイヤーの下座標がフィールドの下座標よりも小さいとき
		if (m_playerPos + m_correction.y < Game::kMapScreenBottomY)
		{
			m_map->Movement({ 0.0f,moveY });//mapを移動させる
			//MoveEnemy( 0.0f,moveY);//エネミーを移動させる
			for (auto& enemy : m_enemyFactory->GetEnemies())
			{
				if (!enemy->IsExist())	continue;
				Move(enemy, 0.0f, moveY);
			}
			Move(m_player,0.0f, moveY - 0.5f);//プレイヤーを移動させる
			m_correction.y += moveY;
			moveY *= -1.0f;
			m_add.y += moveY;//どのくらいプレイヤーが移動したか
		}
		else
		{
			m_player->Movement({ 0.0f,0.1f });
			m_updateFunc = &GameplayingScene::NormalUpdat;
			m_isScreenMoveUp = false;
			m_map->EnemyPos();
			return;
		}
	}
	//下に移動するとき
	else if (m_isScreenMoveDown)
	{
		moveY *= -1.0f;
		//プレイヤーの上座標がフィールドの上座標よりも大きいとき
		if (m_playerPos+ m_correction.y > Game::kMapScreenTopY)
		{
			m_map->Movement({ 0.0f,moveY });
			//MoveEnemy(0.0f, moveY);
			for (auto& enemy : m_enemyFactory->GetEnemies())
			{
				if (!enemy->IsExist())	continue;
				Move(enemy, 0.0f, moveY);
			}
			m_player->Movement({ 0.0f,moveY + 0.5f });
			m_correction.y += moveY;
			moveY *= -1.0f;
			m_add.y += moveY;
		}
		else
		{
			m_player->Movement({ 0.0f,0.1f });
			m_updateFunc = &GameplayingScene::NormalUpdat;
			m_isScreenMoveDown = false;
			m_map->EnemyPos();
			return;
		}
	}
	//右に移動
	else if (m_isScreenMoveWidth)
	{
		m_soundVolume++;
		ChangeVolumeSoundMem(SoundManager::GetInstance().GetBGMVolume() - m_soundVolume, m_BgmH);
		ChangeVolumeSoundMem(m_soundVolume, m_bossBgm);
		//プレイヤーの左座標がフィールドの左座標よりも大きいとき
		//if (m_playerPosBottom > Game::kMapScreenLeftX)
		//端についていないときは移動できる
		if((m_map->GetMapEventParam(m_add.x + Game::kMapScreenRightX, m_add.y - Game::kMapScreenTopY) != MapEvent_screen &&
			m_map->GetMapEventParam(m_add.x + Game::kMapScreenRightX, m_add.y + Game::kMapScreenBottomY - 1.0f) != MapEvent_screen))
		{
			m_map->Movement({ moveX,0.0f });
			//MoveEnemy(moveX, 0.0f);
			for (auto& enemy : m_enemyFactory->GetEnemies())
			{
				if (!enemy->IsExist())	continue;
				Move(enemy, moveX, 0.0f);
			}
			m_player->Movement({ moveX + 0.5f,0.0f });
			moveX *= -1.0f;
			m_add.x += moveX;
		}
		else
		{
			//ボス戦
			m_isBoss = true;
			m_isScreenMoveWidth = false;
			StopSoundMem(m_BgmH);
			m_updateFunc = &GameplayingScene::NormalUpdat;
			return;
		}
	}
}

//画面のフェードアウト
void GameplayingScene::FadeOutUpdat(const InputState& input)
{
	m_fadeValue = 255 * m_fadeTimer / kFadeInterval;
	ChangeVolumeSoundMem(SoundManager::GetInstance().GetBGMVolume() - m_fadeValue, m_BgmH);
	ChangeVolumeSoundMem(SoundManager::GetInstance().GetBGMVolume() - m_fadeValue, m_bossBgm);

	if(++m_fadeTimer == kFadeInterval)
	{
		switch (m_crea)
		{
		case 0:
			m_manager.ChangeScene(new GameclearScene(m_manager, m_player));
			return;
		case 1:
			m_manager.ChangeScene(new GameoverScene(m_manager,m_player));
		default:
			return;
		}
	}
}


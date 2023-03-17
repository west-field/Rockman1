#include "EnemyMoveLeft.h"
#include <DxLib.h>
#include "../Util/DrawFunctions.h"
#include "../game.h"
#include "../Game/HpBar.h"
#include "../Game/Player.h"
#include "../Game/ItemFactory.h"
#include "../Util/Sound.h"

namespace
{
	constexpr int kUpDownTouchAttackPower = 4;//接触した時の攻撃力
	
	//エネミーアニメーション
	constexpr float kEnemyMoveSpeed = -4.0f;//エネミーの移動速度
	constexpr int kSize = 96;//画像サイズX
	constexpr float kDrawScall = 0.3f;//拡大率
	constexpr int kAnimFrameNum = 4;//アニメーション枚数
	constexpr int kAnimFrameSpeed = 20;//アニメーションスピード

	//爆発アニメーション
	constexpr int kBurstImgWidth = 32;//画像サイズX
	constexpr int kBurstImgHeight = 32;//画像サイズY
	constexpr float kBurstDrawScale = 1.0f;//拡大率
	constexpr int kBurstAnimNum = 8;//アニメーション枚数
	constexpr int kBurstAnimSpeed = 5;//アニメーションスピード
	
}

EnemyMoveLeft::EnemyMoveLeft(std::shared_ptr<Player>player, const Position2 pos, int handle, int burstH, std::shared_ptr<ShotFactory> sFactory, std::shared_ptr<ItemFactory> itFactory):
	EnemyBase(player,pos,sFactory,itFactory),m_updateFunc(&EnemyMoveLeft::NormalUpdate),m_drawFunc(&EnemyMoveLeft::NormalDraw)
{
	m_idx = 0;
	m_handle = handle;
	m_burstHandle = burstH;
	m_rect = { pos, { static_cast<int>(kSize * Game::kScale * kDrawScall),static_cast<int>(kSize * Game::kScale * kDrawScall) } };
	
	m_hp->MaxHp(1);
}

EnemyMoveLeft::~EnemyMoveLeft()
{

}

void EnemyMoveLeft::Update()
{
	(this->*m_updateFunc)();
}

void EnemyMoveLeft::Draw()
{
	(this->*m_drawFunc)();
}

int EnemyMoveLeft::TouchAttackPower() const
{
	return kUpDownTouchAttackPower;
}

void EnemyMoveLeft::Damage(int damage)
{
	m_hp->Damage(damage);

	if (m_hp->GetHp() == 0)
	{
		SoundManager::GetInstance().Play(SoundId::EnemyBurst);
		m_updateFunc = &EnemyMoveLeft::BurstUpdate;
		m_drawFunc = &EnemyMoveLeft::BurstDraw;
		m_idx = 0;
		if (GetRand(100) % 3 == 0)
		{
			m_itemFactory->Create(ItemType::Heal, m_rect.center);//回復アイテム
		}
		return;
	}
	SoundManager::GetInstance().Play(SoundId::EnemyHit);
}

bool EnemyMoveLeft::IsCollidable() const
{
	//BurstUpdateの時は当たらない
	return (m_updateFunc != &EnemyMoveLeft::BurstUpdate);
}

void EnemyMoveLeft::NormalUpdate()
{
	Movement({ kEnemyMoveSpeed ,0.0f});//エネミー移動
	m_idx = (m_idx + 1) % (kAnimFrameSpeed * kAnimFrameNum);
}

void EnemyMoveLeft::NormalDraw()
{
	int imgX = (m_idx / kAnimFrameSpeed) * kSize;
	my::MyDrawRectRotaGraph(static_cast<int>(m_rect.center.x), static_cast<int>(m_rect.center.y),
		imgX, 0, kSize, kSize, kDrawScall * Game::kScale, 0.0f, m_handle, true, m_isLeft);
#ifdef _DEBUG
	m_rect.Draw(0xff0000);
#endif
}

void EnemyMoveLeft::BurstUpdate()
{
	m_idx++;
	if (m_idx == kBurstAnimNum * kBurstAnimSpeed)
	{
		m_isExist = false;
	}
}

void EnemyMoveLeft::BurstDraw()
{
	int imgX = (m_idx / kBurstAnimSpeed) * kBurstImgWidth;

	my::MyDrawRectRotaGraph(static_cast<int>(m_rect.center.x), static_cast<int>(m_rect.center.y),
		imgX, 0, kBurstImgWidth, kBurstImgHeight, kBurstDrawScale * Game::kScale, 0.0f, m_burstHandle, true, false);
}

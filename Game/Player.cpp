#include "Player.h"
#include <DxLib.h>
#include "../game.h"
#include "../Util/DrawFunctions.h"
#include "HpBar.h"

namespace
{
	constexpr int kSize = 5;

	//プレイヤーグラフィック
	constexpr int kGraphSizeWidth = 32;		//サイズ
	constexpr int kGraphSizeHeight = 32;	//サイズ
	constexpr float kDrawScale = 2.0f;		//拡大率
	constexpr int kMoveFrameNum = 4;	//歩く時のアニメーション枚数
	constexpr int kFrameNum = 2;		//そのほかのアニメーション枚数
	constexpr int kFrameSpeed = 10;		//アニメーションスピード

	constexpr int kBurstFrameNum = 5;
	constexpr int kBurstFrameSpeed = 5;

	constexpr int ultimate_frame = 120;//無敵時間 2秒

}

Player::Player(Position2 pos, std::shared_ptr<HpBar>hp):m_updateFunc(&Player::NormalUpdate),m_drawFunc(&Player::NormalDraw),m_hp(hp)
{
	m_drawScale = kDrawScale;
	m_rect.center = pos;
	m_rect.size = { static_cast<int>(kGraphSizeWidth /** kDrawScale*/ - kSize) ,static_cast<int>(kGraphSizeHeight /** kDrawScale*/ - kSize)};
	//m_handle = my::MyLoadGraph(L"Data/Retro-Lines-Player-transparent.png");
	m_handle = my::MyLoadGraph(L"Data/players blue x1.png");
}

Player::~Player()
{
}

void Player::Update()
{
	if (!m_isExist)	return;
	(this->*m_updateFunc)();
}

void Player::Draw()
{
	if (!m_isExist)	return;
	(this->*m_drawFunc)();
}

void Player::Movement(Vector2 vec)
{
	m_rect.center += vec;
}

const Rect& Player::GetRect() const
{
	return m_rect;
}

void Player::Action(ActionType type)
{
	if (m_idxX == 0 || m_idxY == 0 || m_idxY == 1)
	{
		switch (type)
		{
		case ActionType::grah_idle:
			m_idxY = 0;
			break;
		case ActionType::grah_walk:
			m_idxY = 1;
			break;
		case ActionType::grah_jump:
			m_idxY = 2;
			break;
		case ActionType::grah_attack:
			m_idxY = 3;
			break;
		case ActionType::grah_death:
			m_idxY = 4;
			m_ultimateTimer = 0;
			break;
		default:
			m_idxY = 0;
			break;
		}
	}
	
}

bool Player::IsCollidable() const
{
	return (m_updateFunc == &Player::NormalUpdate) && m_ultimateTimer == 0;
}

void Player::Damage(int damage)
{
	/*m_updateFunc = &Player::BurstUpdate;
	m_drawFunc = &Player::BurstDraw;*/
	m_ultimateTimer = ultimate_frame;
	m_hp->Damage(damage);
}

void Player::ScaleEnlarge(float scale)
{
	if (m_drawScale >= 5.0f)	return;
	m_drawScale += scale;
}

void Player::NormalUpdate()
{
	float chipSizeHalf = Game::ChipSize / 2;
	//画面の左端についたらそれ以上動けないようにする
	if (m_rect.center.x < Game::kMapScreenLeftX + chipSizeHalf)
	{
		m_rect.center.x = Game::kMapScreenLeftX + chipSizeHalf;
	}
	//画面の右端についたらそれ以上動けないようにする
	else if (m_rect.center.x > Game::kMapScreenRightX - chipSizeHalf)
	{
		m_rect.center.x = Game::kMapScreenRightX - chipSizeHalf;
	}
	//画面の上端についたらそれ以上動けないように
	if (m_rect.center.y < Game::kMapScreenTopY + chipSizeHalf)
	{
		m_rect.center.y = Game::kMapScreenTopY + chipSizeHalf;
	}
	//画面の下端についたらそれ以上動けないように
	else if (m_rect.center.y > Game::kMapScreenBottomY - chipSizeHalf)
	{
		m_rect.center.y = Game::kMapScreenBottomY - chipSizeHalf;
	}

	if (m_frame++ > kFrameSpeed)
	{
		switch (m_idxY)
		{
		case 0:
			m_idxX = (m_idxX + 1) % (1);
			break;
		case 1:
			m_idxX = (m_idxX + 1) % (2);
			break;
		case  2:
			m_idxX = (m_idxX + 1) % (4);
			break;
		case  3:
			m_idxX = (m_idxX + 1) % (4);
			break;
		case  4:
			m_idxX = (m_idxX + 1) % (6);
			if (m_idxX == 0)
			{
				m_isExist = false;
			}
			break;
		}
		if (m_idxX == 0)
		{
			m_idxY = 0;
		}
		
		m_frame = 0;
	}

	//0よりも小さかったら0を返す
	if (--m_ultimateTimer <= 0)
	{
		m_ultimateTimer = 0;
	}
}

void Player::NormalDraw()
{
	if ((m_ultimateTimer / 10) % 2 == 1)
	{
		return;
	}
	//プレイヤーを表示
	my::MyDrawRectRotaGraph(static_cast<int>(m_rect.center.x), static_cast<int>(m_rect.center.y- kGraphSizeHeight/2),
		m_idxX * kGraphSizeWidth, m_idxY * kGraphSizeHeight, kGraphSizeWidth, kGraphSizeHeight, m_drawScale, 0.0f, m_handle, true, m_isLeft);
#ifdef _DEBUG
	m_rect.Draw(0xaaffaa);

	DrawFormatString(0, 0, 0xaaaaaa, L"X%d", m_idxX);
	DrawFormatString(0, 20, 0xaaaaaa, L"Y%d", m_idxY);
#endif
}

void Player::BurstUpdate()
{
}

void Player::BurstDraw()
{
}

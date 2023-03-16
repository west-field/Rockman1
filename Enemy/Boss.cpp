#include "Boss.h"
#include <DxLib.h>
#include "../Util/DrawFunctions.h"
#include "../Game/ShotFactory.h"
#include "../Game/Player.h"
#include "../Game/HpBar.h"
#include "../game.h"
#include "../Util/Sound.h"
namespace
{
	constexpr int kCutManTouchAttackPower = 4;//接触した時の攻撃力

	constexpr float kSpeed = 4.0f;	//移動スピード
	constexpr int kGraphNum = 13;	//グラフィックの数
	constexpr int kGraphSizeWidth = 2600 / kGraphNum;		//サイズ
	constexpr int kGraphSizeHeight = 400;	//サイズ
	constexpr float kDrawScale = 0.5f;		//拡大率
	constexpr int kAnimFrameSpeed = 5;	//グラフィックアニメーションスピード
	constexpr int kSizeX = 30;	//グラフィックサイズ
	constexpr int kSizeY = kSizeX;

	constexpr int kJumpInterval = 100;

	//ジャンプ
	constexpr float kGravity = 0.8f;//重力
	constexpr float kJumpAcc = -10.0f;//ジャンプ力
	//通常爆発アニメーション
	constexpr int burst_img_width = 32;//画像サイズX
	constexpr int burst_img_height = 32;//画像サイズY
	constexpr float burst_draw_scale = 1.0f;//拡大率
	constexpr int burst_frame_num = 8;//アニメーション枚数
	constexpr int burst_frame_speed = 5;//アニメーションスピード
	//ボス爆発アニメーション
	constexpr int boss_burst_img_width = 100;//画像サイズX
	constexpr int boss_burst_img_height = 100;//画像サイズY
	constexpr float boss_burst_draw_scale = 2.0f;//拡大率
	constexpr int boss_burst_frame_num = 61;//アニメーション枚数
	constexpr int boss_burst_frame_speed = 1;//アニメーションスピード
}

Boss::Boss(std::shared_ptr<Player>player, const Position2& pos, int handle, int bossBurstH, int burstH, std::shared_ptr<ShotFactory> sFactory, std::shared_ptr<ItemFactory> itFactory, std::shared_ptr<HpBar>hp) :
	EnemyBase(player, pos, sFactory,itFactory), updateFunc(&Boss::StopUpdate), m_drawFunc(&Boss::NormalDraw),
	m_shotFrame(0), m_JumpFrame(kJumpInterval)
{
	m_hp = hp;
	m_isLeft = true;
	m_handle = handle;
	m_bossBurstH = bossBurstH;
	m_burstHandle = burstH;
	m_rect = { {pos.x,pos.y - 8.0f},
		{static_cast<int>(kGraphSizeWidth * Game::kScale * kDrawScale / 2) - 20,static_cast<int>(kGraphSizeHeight * Game::kScale * kDrawScale) - 20} };
	//m_vec = { kSpeed, kJumpAcc };
}

Boss::~Boss()
{
	DeleteGraph(m_bossBurstH);
}

void Boss::Update()
{
	if (!m_isExist)return;
	(this->*updateFunc)();
}

void Boss::Draw()
{
	if (!m_isExist)return;
	(this->*m_drawFunc)();
}

void Boss::Movement(Vector2 vec)
{
	if (!m_isExist)	return;
	//画面と一緒に移動
	m_rect.center += vec;
}

void Boss::EnemyMovement(Vector2 vec)
{
	if (!m_isExist)	return;
	//ジャンプしているとき
	m_rect.center.y += vec.y;
	if (m_isJump)
	{
		m_rect.center.x += vec.x;
	}
}

int Boss::TouchAttackPower() const
{
	return kCutManTouchAttackPower;
}

void Boss::Damage(int damage)
{
	m_hp->Damage(damage);

	if (m_hp->GetHp() == 0)
	{
		SoundManager::GetInstance().Play(SoundId::EnemyBurst);
		updateFunc = &Boss::BurstUpdate;
		m_drawFunc = &Boss::BurstDraw;
		m_idx = 0;
		return;
	}
	SoundManager::GetInstance().Play(SoundId::EnemyHit);
}

bool Boss::IsCollidable() const
{
	return (updateFunc != &Boss::BurstUpdate) && m_ultimateTimer == 0;
}

void Boss::MoveUpdate()
{
	//左を向いていたら方向を変える
	if (m_isLeft) m_vec.x *= -1.0f;
	//ジャンプさせる
	m_vec.y = kJumpAcc - (GetRand(100) % 5);
	updateFunc = &Boss::JumpUpdate;
	return;
}

void Boss::StopUpdate()
{
	if (m_animFrame++ > kAnimFrameSpeed)
	{
		m_idx = (m_idx + 1) % kGraphNum;
		m_animFrame = 0;
	}

	if (--m_ultimateTimer <= 0)
	{
		m_ultimateTimer = 0;
	}

	//1秒ごとに弾を打つ
	if (m_frame++ == 60)
	{
		m_frame = 0;
		m_isOnDamage = false;

		//HPが3分の2を切ったら二回攻撃する
		if (m_hp->GetHp() <= (m_hp->GetMaxHp() / 3) * 2)
		{
			updateFunc = &Boss::TwoShotUpdate;
			return;
		}
		else
		{
			updateFunc = &Boss::OneShotUpdate;
			return;
		}
	}
	////弾を打たないときは移動する
	//else if(m_JumpFrame++ >= kJumpInterval)
	//{
	//	m_JumpFrame = 0;
	//	
	//	//左を向いていたら方向を変える
	//	if (m_isLeft) m_vec.x *= -1.0f;
	//	//ジャンプさせる
	//	//m_vec.y = kJumpAcc - (GetRand(100) % 5);
	//	m_posTemp = m_rect.center.y + ( kJumpAcc - (GetRand(100) % 5));
	//	updateFunc = &Boss::JumpUpdate;
	//	return;
	//}
}

void Boss::JumpUpdate()
{
	m_rect.center.y += m_vec.y;

	if (m_rect.center.y <= m_posTemp)
	{
		updateFunc = &Boss::DownUpdate;
		return;
	}
}

void Boss::DownUpdate()
{
	m_vec.y += kGravity;//下に落ちる

	m_rect.center.x += m_vec.x;

	if (!m_isJump)
	{
		//プレイヤーの方向にジャンプ
		if (m_player->GetRect().GetCenter().x > m_rect.center.x)
		{
			m_isLeft = false;
		}
		else
		{
			m_isLeft = true;
		}
		updateFunc = &Boss::StopUpdate;
		return;
	}
}

void Boss::OneShotUpdate()
{
	//自機狙い弾を作る　自機狙いベクトル=終点(プレイヤー座標)　-　始点(敵機自身の座標)
	auto vel = m_player->GetRect().GetCenter() - m_rect.center;

	if (vel.SQLength() == 0.0f)
	{
		vel = { -2.0f,0.0f };//敵機の時期が重なっているときは既定の方向　真左に飛ばしておく
	}
	else
	{
		vel.Normalize();
	}
	//ショットを一回打つ
	m_shotFactory->Create(ShotType::ShotBattery, m_rect.center, vel, m_isLeft, false);

	//次の指示を待つ
	m_JumpFrame = 0;
	updateFunc = &Boss::StopUpdate;
	return;
}

void Boss::TwoShotUpdate()
{
	//自機狙い弾を作る　自機狙いベクトル=終点(プレイヤー座標)　-　始点(敵機自身の座標)
	auto vel = m_player->GetRect().GetCenter() - m_rect.center;

	if (vel.SQLength() == 0.0f)
	{
		vel = { -2.0f,0.0f };//敵機の時期が重なっているときは既定の方向　真左に飛ばしておく
	}
	else
	{
		vel.Normalize();
		vel *= 2.0f;
	}
	//ショットを二回打つ
	if (m_shotFrame-- % 10 == 0)
	{
		m_shotFactory->Create(ShotType::ShotBattery, m_rect.center, vel, m_isLeft, false);
	}

	if (m_shotFrame <= 0)
	{
		m_shotFrame = 20;
		//次の指示を待つ
		m_JumpFrame = 0;
		updateFunc = &Boss::StopUpdate;
		return;
	}
}

void Boss::NormalDraw()
{
	//無敵時間点滅させる
	if ((m_ultimateTimer / 10) % 2 == 1)	return;

	int img = m_idx * kGraphSizeWidth;
	my::MyDrawRectRotaGraph(static_cast<int>(m_rect.center.x), static_cast<int>(m_rect.center.y),
		img, 0, kGraphSizeWidth, kGraphSizeHeight, kDrawScale * Game::kScale, 0.0f, m_handle, true, m_isLeft);
#ifdef _DEBUG
	DrawFormatString(static_cast<int>(m_rect.center.x), static_cast<int>(m_rect.center.y), 0xffffff, L"%d", m_idx);
	m_rect.Draw(0xaaffaa);
#endif
}

void Boss::BurstUpdate()
{
	m_idx += 1;
	if (m_idx == (boss_burst_frame_num * 2) * boss_burst_frame_speed)
	{
		m_isExist = false;
	}
}

void Boss::BurstDraw()
{

	int animNum = (m_idx / burst_frame_speed);
	int imgX = (animNum % burst_frame_num) * burst_img_width;
	int imgY = 0;
	my::MyDrawRectRotaGraph(static_cast<int>(m_rect.center.x + 20), static_cast<int>(m_rect.center.y + 50),
		imgX, imgY, burst_img_width, burst_img_height, burst_draw_scale * Game::kScale, 0.0f, m_burstHandle, true, false);
	my::MyDrawRectRotaGraph(static_cast<int>(m_rect.center.x - 40), static_cast<int>(m_rect.center.y + 50),
		imgX, imgY, burst_img_width, burst_img_height, burst_draw_scale * Game::kScale, 0.0f, m_burstHandle, true, false);
	my::MyDrawRectRotaGraph(static_cast<int>(m_rect.center.x + 20), static_cast<int>(m_rect.center.y - 50),
		imgX, imgY, burst_img_width, burst_img_height, burst_draw_scale * Game::kScale, 0.0f, m_burstHandle, true, false);
	my::MyDrawRectRotaGraph(static_cast<int>(m_rect.center.x - 40), static_cast<int>(m_rect.center.y - 50),
		imgX, imgY, burst_img_width, burst_img_height, burst_draw_scale * Game::kScale, 0.0f, m_burstHandle, true, false);

	animNum = (m_idx / boss_burst_frame_speed);
	if (animNum >= boss_burst_frame_num)
	{
		animNum -= boss_burst_frame_num;
	}
	imgX = animNum % 8 * boss_burst_img_width;
	imgY = animNum / 8 * boss_burst_img_height;
	my::MyDrawRectRotaGraph(static_cast<int>(m_rect.center.x), static_cast<int>(m_rect.center.y),
		imgX, imgY, boss_burst_img_width, boss_burst_img_height, boss_burst_draw_scale * Game::kScale, 0.0f, m_bossBurstH, true, false);
#ifdef _DEBUG
	DrawFormatStringF(m_rect.center.x, m_rect.center.y, 0x000000, L"%d", animNum);
	DrawFormatStringF(m_rect.center.x, m_rect.center.y - 20, 0x000000, L"X%d,Y%d", imgX / boss_burst_img_width, imgY / boss_burst_img_height);
#endif
}
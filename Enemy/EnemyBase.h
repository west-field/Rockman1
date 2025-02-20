#pragma once
#include "../Game/Object.h"
#include <memory>
#include "../Util/Geometry.h"

class Player;			//プレイヤー
class HpBar;			//HPバー
class ShotFactory;		//ショット
class ItemFactory;		//アイテム

/// <summary>
/// 敵基底クラス
/// </summary>
class EnemyBase : public Object
{
public:
	EnemyBase(std::shared_ptr<Player>player,const Position2 pos, int handle, int burstH,
		std::shared_ptr<ShotFactory> sFactory, std::shared_ptr<ItemFactory> itFactory);
	virtual ~EnemyBase();

	virtual void Update() = 0;	//更新
	virtual void Draw() = 0;	//表示

	/// <summary>
	/// 当たり判定対象かどうか
	/// </summary>
	/// <returns>true:当たる false:当たらない</returns>
	virtual bool IsCollidable()const = 0;

	/// <summary>
	/// ダメージを受けた
	/// </summary>
	/// <param name="damage">ダメージ量</param>
	virtual void Damage(int damage) = 0;

	/// <summary>
	/// 接触した時の攻撃
	/// </summary>
	/// <returns>接触した時の攻撃力</returns>
	virtual int TouchAttackPower()const;


	/// <summary>
	/// 移動量を返す
	/// </summary>
	/// <returns>移動</returns>
	Vector2 GetVec()const;

	/// <summary>
	/// 現在のHPを返す
	/// </summary>
	/// <returns>現在のHP</returns>
	int GetHp() const;

	/// <summary>
	/// マップチップを入手する
	/// </summary>
	/// <param name="chipId">マップチップ</param>
	void GetChip(int chipId);
protected:
	std::shared_ptr<Player> m_player;				//プレイヤー
	std::shared_ptr<ShotFactory> m_shotFactory;		//ショット
	std::shared_ptr<ItemFactory> m_itemFactory;		//アイテム

	Vector2 m_vec;			//移動速度

	int m_burstHandle;		//爆発画像ハンドル

	int m_chipId;			//マップチップID
	bool m_isOnDamage;		//ダメージを受けたかどうか

	int m_touchDamagePower;	//当たった時の攻撃力
};


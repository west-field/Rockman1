#pragma once
#include "../Util/Geometry.h"
#include "../Shot/ShotBase.h"
#include <map>
#include <list>
#include <memory>

using ShotList_t = std::list<std::shared_ptr<ShotBase>>;

/// <summary>
/// 弾工場
/// </summary>
class ShotFactory
{
public:
	ShotFactory();
	virtual ~ShotFactory();

	void Update();//更新
	void Draw();//表示
	/// <summary>
	/// 画面が移動したら同じ分だけ移動する
	/// </summary>
	/// <param name="vel">移動量</param>
	void Movement(Vector2 vel);

	/// <summary>
	/// 弾の生成
	/// </summary>
	/// <param name="type">作成する弾の種類</param>
	/// <param name="pos">作成者の位置</param>
	/// <param name="vel">移動量</param>
	/// <param name="isleft">左を向いているのか</param>
	/// <param name="isPlayer">プレイヤーが撃った弾か</param>
	void Create(ShotType type, const Position2& pos, const Vector2& vel, bool isleft, bool isPlayer);

	/// <summary>
	/// 当たり判定用にすべての敵の弾への参照
	/// </summary>
	/// <returns>敵の弾リスト</returns>
	const ShotList_t& GetShot()const;
	
private:
	std::map<ShotType, int> m_shotHTable;//ロードする画像のテーブル
	ShotList_t m_shots;//弾リスト
};


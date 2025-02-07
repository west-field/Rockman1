#pragma once
#include <unordered_map>

enum class SoundId
{
	Gameclear,		//ゲームクリア音
	Gameover,		//ゲームオーバー音

	Cursor,			//カーソル移動音
	Determinant,	//決定ボタン
	BlockMove,		//ブロック接触音
	MenuOpen,		//メニューを開く時の音
	PlayerJump,		//プレイヤージャンプ音
	EnemyJump,		//エネミージャンプ音
	//ItemGet,		//アイテムゲット音 itemGet.wav
	Recovery,		//回復音

	//攻撃音
	PlayeyShot,		//プレイヤーの弾発射音
	PlayeyHit,	//プレイヤーが攻撃を受けた

	EnemyShot,		//敵の弾発射音
	EnemyHit,	//敵が攻撃を受けたときの音
	EnemyBurst,	//敵が死んだときの音

	SoundId_Max
};

class SoundManager
{
private:
	SoundManager();
	
	//コピーも代入も禁止する
	SoundManager(const SoundManager&) = delete;
	void operator= (const SoundManager&) = delete;

	/// <summary>
	/// サウンドハンドルを取得
	/// </summary>
	/// <param name="id">サウンド名</param>
	/// <param name="fileName">サウンドファイル名</param>
	/// <returns>サウンドハンドル</returns>
	int LoadSoundFile(SoundId id,const wchar_t* fileName);
	
	/// <summary>
	/// サウンド情報のロード
	/// </summary>
	void LoadSoundConfig();

	std::unordered_map<SoundId, int> m_nameAndHandleTable;//サウンドハンドル

	//変更したサウンド情報をファイルに書き込む
	struct SoundConfigInfo
	{
		unsigned short volumeSE;//0〜255
		unsigned short volumeBGM;//0〜255
	};

	int m_volumeSE = 255;//初期SE音量
	int m_volumeBGM = 255;//初期BGM音量
public:
	~SoundManager();
	/// <summary>
	/// SoundManager使用者はGetInstance()を通した参照からしか利用できない
	/// </summary>
	/// <returns>実体の参照を返す</returns>
	static SoundManager& GetInstance()
	{
		static SoundManager instance;//唯一の実体
		return instance;//それの参照を返す
	}

	/// <summary>
	/// サウンド情報のセーブ
	/// </summary>
	void SaveSoundConfig();

	/// <summary>
	/// 指定のサウンドを鳴らす
	/// </summary>
	/// <param name="name">サウンド名</param>
	void Play(SoundId id, int volume = 200);
	/// <summary>
	/// BGMを鳴らす
	/// </summary>
	/// <param name="soundH">サウンドハンドル</param>
	void PlayBGM(int soundH);

	/// <summary>
	/// SEのボリュームを設定する
	/// </summary>
	/// <param name="volume">音量</param>
	void SetSEVolume(int volume);
	/// <summary>
	/// SEのボリュームを取得する
	/// </summary>
	/// <returns>SEボリューム</returns>
	int GetSEVolume()const;

	/// <summary>
	/// BGMのボリュームを設定する
	/// </summary>
	/// <param name="volume">音量</param>
	/// <param name="soundH">サウンドハンドル</param>
	void SetBGMVolume(int volume,int soundH);
	/// <summary>
	/// BGMのボリュームを取得する
	/// </summary>
	/// <returns>BGMボリューム</returns>
	int GetBGMVolume()const;

	/// <summary>
	/// サウンドを止める
	/// </summary>
	/// <param name="id">サウンド名</param>
	void StopBgm(SoundId id);
};



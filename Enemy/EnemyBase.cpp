#include "EnemyBase.h"
#include "../Game/HpBar.h"
#include "../Util/Sound.h"

EnemyBase::EnemyBase(std::shared_ptr<Player>player, const Position2& pos, std::shared_ptr<ShotFactory> sFactory):
    m_player(player), m_rect(pos, {}),m_shotFactory(sFactory)
{
    m_hp = std::make_shared<HpBar>();
}

EnemyBase::~EnemyBase()
{
}

const Rect& EnemyBase::GetRect() const
{
    return m_rect;
}

void EnemyBase::Damage(int damage)
{
    m_hp->Damage(damage);
    if (m_hp->GetHp() == 0)
    {
        m_isExist = false;
    }
}

bool EnemyBase::IsExist() const
{
    return m_isExist;
}

void EnemyBase::OnDamage()
{
    m_isOnDamage = true;
}

/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include "flag.h"

CFlag::CFlag(CGameWorld *pGameWorld, int Team)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_FLAG)
{
	m_Team = Team;
	m_ProximityRadius = ms_PhysSize;
	m_pCarryingCharacter = NULL;
	m_GrabTick = 0;

	Reset();
}

void CFlag::Reset()
{
	m_pCarryingCharacter = NULL;
	m_AtStand = 1;
	m_Pos = m_StandPos;
	m_Vel = vec2(0,0);
	m_GrabTick = 0;
}

void CFlag::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, m_Team, sizeof(CNetObj_Flag));
	if(!pFlag)
		return;

	pFlag->m_X = (int)m_Pos.x;
	pFlag->m_Y = (int)m_Pos.y;
	pFlag->m_Team = m_Team;
}

void CFlag::Tick()
{
	if (!m_pCarryingCharacter)
	{
		//if (!m_AtStand)
		{
			bool XHold = absolute<float>(m_Vel.x) < 0.0001f;
			bool Grounded = false;

			if(GameServer()->Collision()->CheckPoint(m_Pos.x+ms_PhysSize/2, m_Pos.y+ms_PhysSize/2+5))
				Grounded = true;
			if(GameServer()->Collision()->CheckPoint(m_Pos.x-ms_PhysSize/2, m_Pos.y+ms_PhysSize/2+5))
				Grounded = true;
			if (!XHold)
				m_Vel.x *= (Grounded?g_Config.m_SvFlagFrictionGround:g_Config.m_SvFlagFrictionAir) / 100.0f;
			m_Vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
			GameServer()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(ms_PhysSize, ms_PhysSize), FLAGCOL_ELAST);
		}
	}
	else if (m_pCarryingCharacter->IsAlive())
	{
		m_Pos = m_pCarryingCharacter->m_Pos;
	}

}

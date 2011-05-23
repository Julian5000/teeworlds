#include <engine/shared/config.h>

#include <game/generated/protocol.h>

#include <game/server/player.h>
#include <game/server/gameworld.h>
#include <game/server/entities/flag.h>
#include <game/server/gamemodes/ddmincommon.h>

#include "mod.h"

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "FLAGDRAG";
	m_GameFlags = GAMEFLAG_TEAMS;
	m_apFlags[0] = m_apFlags[1] = 0;
	m_aFlagRegLvl[0] = m_aFlagRegLvl[1] = -1;
	m_LastBroadcast = 0;
	for(int z = 0; z < MAX_CLIENTS; ++z)
	{
		m_aFlagDropTick[z] = 0;
		m_aaBroadcasts[z][0]='\0';
	}
}
	
CGameControllerMOD::~CGameControllerMOD()
{
}

void CGameControllerMOD::DoBroadcast()
{
	if (m_LastBroadcast + TICKSPEED < TICK)
	{
		for(int z=0; z < MAX_CLIENTS; ++z)
		{
			if (GameServer()->m_apPlayers[z])
				GameServer()->SendBroadcast(m_aaBroadcasts[z], z);
		}
		m_LastBroadcast = TICK;
	}
}

void CGameControllerMOD::Tick()
{
	int isr;
	CFlag *Flag;
	CCharacter *Chr;

	DoTeamScoreWincheck();
	
	CCharacterCore::SetFlagHooking(g_Config.m_SvFlagHook);
	IGameController::Tick();

	DoBroadcast();

	for(int z = 0; z < 2; ++z)
	{
		if (m_apFlags[z]->m_pCarryingCharacter)
		{
			Chr = m_apFlags[z]->m_pCarryingCharacter;
			if (!Chr->IsAlive())
			{
				if (Chr->KilledByWorld()) // let it bounce up for others to have a chance to rescue it
					m_apFlags[z]->m_Vel = vec2(0.0f, -20.0f);
				if (Chr->GetPlayer())
				{
					m_aFlagDropTick[Chr->GetPlayer()->GetCID()] = TICK;
					UpdateBroadcast(Chr->GetPlayer()->GetCID(), "");
				}
				m_apFlags[z]->m_pCarryingCharacter = 0;

			}
			else if (m_apFlags[z]->m_pCarryingCharacter->GetPlayer()->m_FlagDrop)
			{
				vec2 Direction = Chr->GetInputDir();
				m_apFlags[z]->m_Vel = Chr->GetCore()->m_Vel + Direction * (float)CFG(FlagDropVel);
				m_aFlagDropTick[Chr->GetPlayer()->GetCID()] = TICK;
				UpdateBroadcast(Chr->GetPlayer()->GetCID(), "");
				m_apFlags[z]->m_pCarryingCharacter = 0;
			}
			else if (GameServer()->Collision()->GetCollisionAt(m_apFlags[z]->m_Pos.x, m_apFlags[z]->m_Pos.y) == TILE_FINISH)
			{
				m_aTeamscore[z]++;
				char aBuf[128];
				str_format(aBuf, sizeof aBuf, "%s flag delivered, %s team scores! (Red: %d, Blue :%d)",
						z?"Blue":"Red", z?"Blue":"Red", m_aTeamscore[0], m_aTeamscore[1]);
				GameServer()->SendChat(-1, -2, aBuf);
				m_apFlags[z]->Reset();
				UpdateBroadcast(Chr->GetPlayer()->GetCID(), "");
			}
		}
		else
		{
			int a;
 			if((((a=GameServer()->Collision()->GetCollisionAt(m_apFlags[z]->m_Pos.x, m_apFlags[z]->m_Pos.y))
				<= 5 && (a&CCollision::COLFLAG_DEATH))) || m_apFlags[z]->GameLayerClipped(m_apFlags[z]->m_Pos))
			{
				m_apFlags[z]->Reset();
				char aBuf[32];
				str_format(aBuf, sizeof aBuf, "%s flag returned to base!", z?"Blue":"Red");
				GameServer()->SendChat(-1, -2, aBuf);
			}
		}
	}
	bool NoPlayer = true;
	for(int z = 0; z < MAX_CLIENTS; ++z)
	{
		CPlayer *P = GameServer()->m_apPlayers[z];
		if (P)
		{
			NoPlayer = false;
			P->m_FlagDrop = false;
		}
	}
	if (NoPlayer)
	{
		m_apFlags[0]->Reset();
		m_apFlags[1]->Reset();

	}

	for(int z = 0; z < 2; ++z)
	{
		CCharacter *apPrx[MAX_CLIENTS];
		Flag = m_apFlags[z];
		int Num = GameServer()->m_World.FindEntities(Flag->m_Pos, CCharacter::ms_PhysSize - CFG(GrabRadDecr),
				(CEntity**)apPrx, sizeof apPrx, CGameWorld::ENTTYPE_CHARACTER);

		for(int y = 0; y < Num; ++y)
		{
			Chr = apPrx[y];
			if (!Chr->IsAlive() || Chr->GetPlayer()->GetTeam() == TEAM_SPECTATORS ||
					Flag->m_pCarryingCharacter == Chr ||
					m_apFlags[z^1]->m_pCarryingCharacter == Chr ||
					m_aFlagDropTick[Chr->GetPlayer()->GetCID()] + CFG(FlagDropCooldown) > TICK ||
					(isr=GameServer()->Collision()->IntersectLine(Flag->m_Pos,Chr->m_Pos,NULL,NULL)))
				continue;

			if (Flag->m_pCarryingCharacter)
			{ // pass on
				UpdateBroadcast(Flag->m_pCarryingCharacter->GetPlayer()->GetCID(), "");
				m_aFlagDropTick[Flag->m_pCarryingCharacter->GetPlayer()->GetCID()] =
						TICK;
			}
			Flag->m_AtStand = 0;
			Flag->m_pCarryingCharacter = Chr;
			UpdateBroadcast(Flag->m_pCarryingCharacter->GetPlayer()->GetCID(),
					Chr->GetPlayer()->GetTeam() == z ? "Your flag, deliver it! (say /drop to drop )"
					                                 : "Hostile flag, destroy or take it backwards! (say /drop to drop)  ");
			Flag->m_GrabTick = TICK;
			break;
		}
	}
}

void CGameControllerMOD::UpdateBroadcast(int ClientId, const char *pMsg)
{
	str_copy(m_aaBroadcasts[ClientId], pMsg, MAX_BROADCAST);
}

void CGameControllerMOD::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);
}

bool CGameControllerMOD::OnEntity(int Index, vec2 Pos)
{
	bool res = IGameController::OnEntity(Index, Pos);
	if (res)
		return true;
	res = false;
	//flags get placed at flag stands, if provided. if not, at team spawn, if provided. if not, at regular spawn.
	if (Index == ENTITY_FLAGSTAND_RED || Index == ENTITY_FLAGSTAND_BLUE)
	{
		int Team = Index == ENTITY_FLAGSTAND_RED ? TEAM_RED : TEAM_BLUE;
		if (!m_apFlags[Team])
		{
			m_apFlags[Team] = new CFlag(&GameServer()->m_World, Team);
			GameServer()->m_World.InsertEntity(m_apFlags[Team]);
			res = true;
		}
		m_aFlagRegLvl[Team] = 2;
		m_apFlags[Team]->m_StandPos = m_apFlags[Team]->m_Pos = Pos;
	}
	else if (Index == ENTITY_SPAWN_RED || Index == ENTITY_SPAWN_BLUE)
	{
		int Team = Index == ENTITY_SPAWN_RED ? TEAM_RED : TEAM_BLUE;
		if (!m_apFlags[Team])
		{
			m_apFlags[Team] = new CFlag(&GameServer()->m_World, Team);
			GameServer()->m_World.InsertEntity(m_apFlags[Team]);
			m_apFlags[Team]->m_StandPos = m_apFlags[Team]->m_Pos = Pos;
			m_aFlagRegLvl[Team] = 1;
			res = true;
		}
		else if (m_aFlagRegLvl[Team] < 1)
		{
			m_apFlags[Team]->m_StandPos = m_apFlags[Team]->m_Pos = Pos;
			m_aFlagRegLvl[Team] = 1;
		}
	}
	else if (Index == ENTITY_SPAWN)
	{
		for(int Team = 0; !res && Team < 2; ++Team)
			if (!m_apFlags[Team])
			{
				m_apFlags[Team] = new CFlag(&GameServer()->m_World, Team);
				GameServer()->m_World.InsertEntity(m_apFlags[Team]);
				m_apFlags[Team]->m_StandPos = m_apFlags[Team]->m_Pos = Pos;
				m_aFlagRegLvl[Team] = 0;
				res = true;
			}
			else if (m_aFlagRegLvl[Team] < 0)
			{
				m_apFlags[Team]->m_StandPos = m_apFlags[Team]->m_Pos = Pos;
				m_aFlagRegLvl[Team] = 0;
			}
	}
	if (m_apFlags[0] && m_apFlags[1])
		CCharacterCore::SetFlags(m_apFlags[0], m_apFlags[1]);
	return res;
}

void CGameControllerMOD::OnCharacterSpawn(class CCharacter *pChr)
{
	IGameController::OnCharacterSpawn(pChr);
}

int CGameControllerMOD::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	int res = IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);
	m_aaBroadcasts[pVictim->GetPlayer()->GetCID()][0]='\0';
	return res;
}

bool CGameControllerMOD::CanBeMovedOnBalance(int Cid)
{
	bool res = IGameController::CanBeMovedOnBalance(Cid);
	return res;
}

void CGameControllerMOD::OnPlayerInfoChange(class CPlayer *pP)
{
	IGameController::OnPlayerInfoChange(pP);
}

bool CGameControllerMOD::CanSpawn(int Cid, vec2 *pPos)
{
	bool res = IGameController::CanSpawn(Cid, pPos);
	return res;
}

const char* CGameControllerMOD::GetTeamName(int Team)
{
	const char* res = IGameController::GetTeamName(Team);
	return res;
}

int CGameControllerMOD::GetAutoTeam(int NotThisId)
{
	int res = IGameController::GetAutoTeam(NotThisId);
	return res;
}

bool CGameControllerMOD::CanJoinTeam(int Team, int NotThisId)
{
	bool res = IGameController::CanJoinTeam(Team, NotThisId);
	return res;
}

void CGameControllerMOD::PostReset()
{
	IGameController::PostReset();
}


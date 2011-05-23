#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include <game/server/gamecontroller.h>
#include <game/server/gamemodes/ddmincommon.h>

class CGameControllerMOD : public IGameController
{
private:
	class CFlag *m_apFlags[2];
	int m_aFlagDropTick[MAX_CLIENTS];
	int m_aFlagRegLvl[2];//for compat w/ maps w/o flag stands.
	char m_aaBroadcasts[MAX_CLIENTS][MAX_BROADCAST];
	int m_LastBroadcast;
	void UpdateBroadcast(int ClientId, const char *pMsg);
	void DoBroadcast();
public:
	CGameControllerMOD(class CGameContext *pGameServer);
	virtual ~CGameControllerMOD();

	virtual void Tick();

	virtual void Snap(int SnappingClient);
	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual int OnCharacterDeath(class CCharacter *pVictim,
			class CPlayer *pKiller, int Weapon);
	virtual bool OnEntity(int Index, vec2 Pos);
	virtual bool CanBeMovedOnBalance(int Cid);
	virtual void OnPlayerInfoChange(class CPlayer *pP);
	virtual bool CanSpawn(int Cid, vec2 *pPos);
	virtual const char *GetTeamName(int Team);
	virtual int GetAutoTeam(int NotThisId);
	virtual bool CanJoinTeam(int Team, int NotThisId);
	virtual void PostReset();
};

#endif

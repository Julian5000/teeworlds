/*
 * SingleElimTournament.cpp
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#include <set>

#include <cstring>

#include <base/system.h>

#include "../../common.h"
#include "../../Util.h"
#include "../../Factory.h"
#include "../../Lineup.h"
#include "../../Team.h"
#include "../../Player.h"
#include "../../match/Match.h"

#include "SingleElimTournament.h"

#define forset(SET,K,IT) for(std::set<K>::const_iterator IT=(SET)->begin();IT != (SET)->end();)
#define forset_auto(SET,K,IT) for(std::set<K>::const_iterator IT=(SET)->begin();IT != (SET)->end();++IT)
#define formap(MAP,K,V,IT) for(std::map<K,V>::const_iterator IT=(MAP)->begin();IT != (MAP)->end();)
#define formap_auto(MAP,K,V,IT) for(std::map<K,V>::const_iterator IT=(MAP)->begin();IT != (MAP)->end();++IT)
#define forvec(VEC,K,IT) for(std::vector<K>::const_iterator IT=(VEC)->begin();IT != (VEC)->end();)
#define forvec_auto(VEC,K,IT) for(std::vector<K>::const_iterator IT=(VEC)->begin();IT != (VEC)->end();++IT)

#define formatches(IT) forvec(&_matches,Match*,IT)
#define formatches_auto(IT) forvec_auto(&_matches,Match*,IT)
#define forteams(IT) formap(&_teams,int,Team*,IT)
#define forteams_auto(IT) formap_auto(&_teams,int,Team*,IT)
#define forplayers(IT) formap(&_players,int,Player*,IT)
#define forplayers_auto(IT) formap_auto(&_players,int,Player*,IT)

namespace tour
{

    int getArenaForSlot(int slot);

    SingleElimTournament::SingleElimTournament() :
        Tournament(), _players(), _teams(), _matches(), _matchinfo(), _loserTeams(), _teamsPerMatch(2)/*TODO remove hardcode*/,
                _leaveQueue(), _forceTestPending(false)
    {
        initPropagateMap();
        reset();
    }

    SingleElimTournament::~SingleElimTournament()
    {
        reset();
    }

    void SingleElimTournament::reset()
    {
        _started = _finished = _forceTestPending = false;
        _winnerTeam = -1;
        forplayers(it) delete it++->second;
        forteams(it) delete it++->second;
        formatches(it) delete *(it++);
        formap(&_matchinfo,Match*,MatchInfo*,it) delete it++->second;
        _matches.clear();_players.clear();_teams.clear();
        _matchinfo.clear();_loserTeams.clear();_leaveQueue.clear();
    }

    bool SingleElimTournament::start(class Lineup* lineup)
    {
        static std::vector<int> all_tids;
        if (_started || lineup->countDistinctTeams() < 2) return false;
        setupTeams(lineup);
        SV("done setting up teams, dumping");if (VERBOSE) dump();
        all_tids.clear();
        forteams(it) all_tids.push_back(it++->second->id());

        Util::shuffleIV(&all_tids);
        int num_teams = all_tids.size(), slot = 0;
        Match *new_match = NULL;
        MatchInfo *new_match_info = NULL;
        for (int z = 0; z < num_teams; ++z) {
            if ((z % _teamsPerMatch) == 0) {
                if (new_match) new_match->start(getArenaForSlot(new_match_info->slot));
                new_match = Factory::allocMatch();
                new_match_info = new MatchInfo(new_match, slot++);
                _matches.push_back(new_match);
                _matchinfo[new_match] = new_match_info;
            }
            new_match->addTeam(_teams[all_tids[z]]);
            _teams[all_tids[z]]->setNoobStyle(0);
        }
        if (new_match) new_match->start(getArenaForSlot(new_match_info->slot));
        _started = true;
        SV("done setting up initial matches, and _started, dumping");if (VERBOSE) dump();
        return true;
    }

    void SingleElimTournament::tick()
    {
        static std::set<Match *> fin_matches;
        static int tmp_tids[MAX_CLIENTS];
        if (!_started || _finished) return;

        if (!_leaveQueue.empty()) {
            // this ensures a maximum of 1 leaves per tick, which simplifies stuff
            performLeave(_leaveQueue.front());
            _leaveQueue.pop_front();
        }

        fin_matches.clear();
        formatches_auto(it_matches) {
            Match *match = *it_matches;
            if (match->hasStarted() && !match->hasFinished()) {
                match->tick();
            } else if (_forceTestPending && !match->hasStarted()) {
                DV("enforced pending test on match in slot %i", _matchinfo[match]->slot);
                if (!waitsForMatches(match)) {
                    D("starting a match due to (double) leave event (slot: %i, arena: %i)", _matchinfo[match]->slot,getArenaForSlot(_matchinfo[match]->slot));
                    match->start(getArenaForSlot(_matchinfo[match]->slot));
                    SV("a match started, dumping match"); if (VERBOSE) match->dump();
                }
            }
            if (match->hasFinished()) fin_matches.insert(match);
        }
        _forceTestPending = false;
        forset_auto(&fin_matches,Match*,it_finmatches) {
            Match *fin_match = *it_finmatches;
            MatchInfo *fin_match_info = _matchinfo[fin_match];
            DV("a match finished (slot %i), dumping match",_matchinfo[fin_match]->slot); if (VERBOSE) fin_match->dump();
            for (std::vector<Match*>::iterator it_matches = _matches.begin(); it_matches != _matches.end(); ++it_matches) {
                if ((*it_matches) == fin_match) {
                    _matchinfo.erase(fin_match);
                    _matches.erase(it_matches);
                    SV("deregistered match");
                    break;
                }
            }

            int num = fin_match->getLoser(tmp_tids, sizeof(tmp_tids));
            for (int z = 0; z < num; ++z) _loserTeams.push_back(tmp_tids[z]);

            int slot = fin_match_info->slot;
            int prop_slot = getProp(slot);

            if (slot == 14) { // win  (match slot 14 is the final in this tournament)
                if (fin_match->getWinner(&_winnerTeam, 1) != 1) {
                    S("nasty stuff happening, dump.");
                    dump();
                    _winnerTeam = -1;
                }
            } else if (prop_slot >= 0 && prop_slot <= 14) {
                int win_team;
                fin_match->getWinner(&win_team, 1);
                if (win_team < 0 || !_teams.count(win_team)) {
                    _forceTestPending = true;//unlikely, but possible
                } else {
                    Match *new_match = findMatchBySlot(prop_slot);
                    if (!new_match) {
                        _matches.push_back(new_match = Factory::allocMatch());
                        _matchinfo[new_match] = new MatchInfo(new_match, prop_slot);
                        DV("spawned a new match (slot %i), dumping match",prop_slot);if (VERBOSE) new_match->dump();
                    }

                    num = fin_match->getWinner(tmp_tids, sizeof(tmp_tids));
                    for (int z = 0; z < num; ++z) {
                        if (_teams.count(tmp_tids[z])) {
                            new_match->addTeam(_teams[tmp_tids[z]]);
                            DV("added team %i to recently spawned match",tmp_tids[z]);
                            _teams[tmp_tids[z]]->setNoobStyle(0);
                        }
                    }
                    if (!waitsForMatches(new_match)) {
                        new_match->start(getArenaForSlot(prop_slot));
                        DV("started a match (slot %i), dumping match",prop_slot);if (VERBOSE) new_match->dump();
                    }
                }
            }
            delete fin_match;delete fin_match_info;
        }
        if (_matches.empty()) {
            SV("no more matches, tournament finished");
            _finished = true;
        }
    }

    void SingleElimTournament::rankPort() const
    {
        if (_teams.count(_winnerTeam))
            Util::teleportTeam(_teams.find(_winnerTeam)->second, Util::getPosRank(1));
        forvec_auto(&_loserTeams,int,it)
            if (_teams.count(*it)) Util::teleportTeam(_teams.find(*it)->second, Util::getPosRank(2));
    }

    bool SingleElimTournament::hasStarted() const
    {
        return _started;
    }

    bool SingleElimTournament::hasFinished() const
    {
        return _finished;
    }

    int SingleElimTournament::getPlayerMsg(char *dest, int cid) const
    {
        if (!_started) {
            strcpy(dest, "select teams (if any)! tournament will start soon!");
        } else {
            if (_finished) {
                strcpy(dest, "tournament over, epic action!");
            } else if (_players.count(cid)) {
                Player *p = _players.find(cid)->second;
                Team *t = p->getTeam();
                Match *m = findMatchByTeam(t->id());
                if (m) m->getTeamMsg(dest, t);
                else strcpy(dest, "you're out, bad luck! maybe next time?");
            } else {
                strcpy(dest, "welcome, newb. wait for the next tournament to begin!");
            }
        }
        return strlen(dest);
    }

    void SingleElimTournament::onNoobStyle(int cid)
    {
        if (_started && !_finished && _players.count(cid))
            _players[cid]->getTeam()->incNoobStyle();
    }

    void SingleElimTournament::onLeave(int cid)
    {
        if (_started && !_finished) {
            _leaveQueue.push_back(cid);
            DV("queued leave event for cid %i",cid);
        }
    }

    void SingleElimTournament::onFreeze(int cid)
    {
    }

    void SingleElimTournament::onUnfreeze(int cid)
    {
    }

    void SingleElimTournament::onFail(int failpid, int killpid)
    {
        if (_started && !_finished && _players.count(failpid))
            _players[failpid]->setFailFlag(true);
    }

    void SingleElimTournament::dump()
    {
        fprintf(stderr, "<<<<<<<<<<<<<<<~se tour dump~>>>>>>>>>>>>>>>>\n");
        fprintf(stderr, "--------players:\n");
        forplayers(it) it++->second->dump();
        fprintf(stderr, "--------teams:\n");
        forteams(it) it++->second->dump();
        fprintf(stderr, "--------matches/matchinfo:\n");
        formatches_auto(it) {
            Match *m = *it;
            fprintf(stderr, "\tmatch in slot %i:\n", _matchinfo[m]->slot);
            m->dump();
            if (!m->hasStarted())
                fprintf(stderr, "\t\t%swaiting matches\n", waitsForMatches(m) ? "" : "no ");
        }
        fprintf(stderr, "winner: %i, losers: ", _winnerTeam);
        for (std::vector<int>::const_iterator it = _loserTeams.begin(); it != _loserTeams.end(); ++it)
            fprintf(stderr, "%i,", *it);
        fprintf(stderr, "\nstarted: %i, finished: %i, ftp: %i, teamsPerMatch: %i\n", _started, _finished, _forceTestPending, _teamsPerMatch);
        fprintf(stderr, "<<<<<<<<<<<<~end of se tour dump~>>>>>>>>>>>>\n");
    }

    bool SingleElimTournament::waitsForMatches(Match *m) const
    {
        static std::set<int> pend_vec, tmp_vec;
        int slot = _matchinfo.find(m)->second->slot;
        bool has_waiting = false;
        pend_vec.clear();
        formatches(it_matches) {
            if ((*it_matches)->countTeams() > 0)
                pend_vec.insert(_matchinfo.find(*(it_matches++))->second->slot);
            else {
                int win_tid = -1;
                (*it_matches)->getWinner(&win_tid,1);
                if (_teams.count(win_tid)) {
                    /* this should fix the alone-in-arena problem,
                     * which occurs when 2 matches timeout in the same
                     * tick. for now. */
                    pend_vec.insert(_matchinfo.find(*(it_matches++))->second->slot);
                } else {
                    ++it_matches;
                }
            }
        }
        /*rolled up recursion, this is like a BFS*/
        tmp_vec.clear();
        while (!pend_vec.empty()) {
            forset(&pend_vec,int,it)
                tmp_vec.insert(*(it++));
            pend_vec.clear();
            forset(&tmp_vec,int,it) {
                int p = getProp(*(it++));
                if (p >= 0 && p <= 14) pend_vec.insert(p);
                if (p == slot) {
                    has_waiting = true;
                    pend_vec.clear();
                    break;
                }
            }
            tmp_vec.clear();
        }
        return has_waiting;
    }


    void SingleElimTournament::performLeave(int cid)
    {
        if (_started && !_finished && _players.count(cid)) {
            DV("handling leave (id %i)",cid);
            Player *p = _players[cid];
            Team *t = p->getTeam();
            if (t) {
                bool drop_team = t->countMembs() == 1;
                if (drop_team) {
                    DV("will drop team %i",t->id());
                    Match *m = findMatchByTeam(t->id());
                    if (m) {
                        SV("dropping team from match");
                        m->dropTeam(t->id());
                    }
                }
                SV("dropping player from team + unregister/dealloc");
                t->dropMemb(p->id());
                _players.erase(p->id());
                if (drop_team) _teams.erase(t->id());
                delete p;
                if (drop_team) delete t;
            } else {
                SV("no team for known player? nasty stuff happening");
                dump();
            }
        } else {
            DV("will not handle leave event for id %i (%i, %i, %zu)",cid,_started,_finished,_players.count(cid));
        }
    }

    Match *SingleElimTournament::findMatchBySlot(int s) const
    {
        formatches_auto(it)
            if (_matchinfo.count(*it) && _matchinfo.find(*it)->second->slot == s) return *it;
        return NULL;
    }

    Match *SingleElimTournament::findMatchByTeam(int tid) const
    {
        formatches_auto(it) if ((*it)->hasTeam(tid)) return *it;
        return NULL;
    }

    void SingleElimTournament::setupTeams(Lineup *lup)
    {
        static std::set<int> tids_in_use;
        tids_in_use.clear();
        for (int z = 0; z < MAX_CLIENTS; ++z) {
            int selected_team = lup->getTeamSelection(z);
            if (selected_team >= 0) {
                _players[z] = new Player(z);
                Team *team = (_teams.count(selected_team)) ? _teams[selected_team] : (_teams[selected_team] = new Team(selected_team));
                team->addMemb(_players[z]);
                _players[z]->setTeam(team);
                tids_in_use.insert(selected_team);
            } else if (selected_team == TEAMSELECT_SOLO) {
                _players[z] = new Player(z);
            }
        }
        for (int z = 0; z < MAX_CLIENTS; ++z) {
            if (lup->getTeamSelection(z) == TEAMSELECT_SOLO) {
                int tid = -1;
                while (tids_in_use.count(++tid));
                tids_in_use.insert(tid);
                Team *t = _teams[tid] = new Team(tid);
                t->addMemb(_players[z]);
                _players[z]->setTeam(t);
            }
        }
    }

    void SingleElimTournament::initPropagateMap() {
        _propagateMap[0] = _propagateMap[1] = 8;
        _propagateMap[2] = _propagateMap[3] = 9;
        _propagateMap[4] = _propagateMap[5] = 10;
        _propagateMap[6] = _propagateMap[7] = 11;

        _propagateMap[8] = _propagateMap[9] = 12;
        _propagateMap[10] = _propagateMap[11] = 13;

        _propagateMap[12] = _propagateMap[13] = 14;

        _propagateMap[14] = -1;
    }

    int getArenaForSlot(int slot)
    {
        if (slot >= 0 && slot <= 7) return slot;
        if (slot >= 8 && slot <= 11) return (slot - 8) << 1;
        switch (slot) {
            case 12:
                return 1;
            case 13:
                return 5;
            case 14:
                return 3;
        }
        return -1;
    }

}

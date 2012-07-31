/*
 * SingleElimTournament.h
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#ifndef SINGLEELIMTOURNAMENT_H_
#define SINGLEELIMTOURNAMENT_H_

#include <map>
#include <vector>
#include <list>

#include "../../Factory.h"

#include "../Tournament.h"

namespace tour
{

    class SingleElimTournament: public Tournament
    {
        private:
            std::map<int, class Player*> _players;
            std::map<int, class Team*> _teams;
            std::vector<class Match*> _matches;
            std::map<class Match*, class MatchInfo*> _matchinfo;//maps ptr -> matchinfo

            int _winnerTeam;
            std::vector<int> _loserTeams;

            bool _started, _finished;
            int _teamsPerMatch; //only 1st round
            int _propagateMap[15];
            std::list<int> _leaveQueue;
            bool _forceTestPending;

            //SingleElimTournament();

            int getProp(int slot) const {return _propagateMap[slot];}
            void initPropagateMap();

            class Match *findMatchBySlot(int s) const;
            class Match *findMatchByTeam(int s) const;

            void setupTeams(Lineup *lup);
            void performLeave(int cid);
            bool waitsForMatches(Match *m) const;
        public:
            SingleElimTournament();
            virtual ~SingleElimTournament();
            virtual void reset();
            virtual bool start(class Lineup*);
            virtual void tick();
            virtual void rankPort() const;
            virtual bool hasStarted() const;
            virtual bool hasFinished() const;
            virtual int getPlayerMsg(char *dest, int cid) const;

            virtual void onNoobStyle(int cid);
            virtual void onLeave(int cid);
            virtual void onFreeze(int cid);
            virtual void onUnfreeze(int cid);
            virtual void onFail(int failpid, int killpid);

            virtual void dump();

            //template<typename T> friend class Tournament* Factory::createTournamentInstance();
    };

    class MatchInfo
    {
        public:
            class Match *match;
            int slot;
            MatchInfo() :
                match(0), slot(-1)
            {}
            MatchInfo(class Match*m, int s) :
                match(m), slot(s)
            {}
    };

}

#endif /* SINGLEELIMTOURNAMENT_H_ */

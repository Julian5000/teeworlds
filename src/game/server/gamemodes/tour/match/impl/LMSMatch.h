/*
 * LMSMatch.h
 *
 *  Created on: Oct 15, 2010
 *      Author: fisted
 */

#ifndef LMSMATCH_H_
#define LMSMATCH_H_

#include <map>
#include <vector>

#include "../../Factory.h"

#include "../Match.h"

namespace tour
{

    class LMSMatch: public Match
    {
        private:
            std::map<int, class Team*> _teams;
            std::map<int, class TeamInfo*> _teaminfo;
            bool _started, _finished;
            int _arena;
            int _timeleft;
            std::vector<int> _losers;
            int _winner;
            int _failLimit;
            int _lastLeadTid;
            int _tmp_orderc;

            //LMSMatch();

            void allToSpawn() const;
        public:
            LMSMatch();
            virtual ~LMSMatch();

            virtual bool start(int arena);

            virtual void tick();

            virtual void addTeam(class Team*);
            virtual int countTeams() const;
            virtual void dropTeam(int tid);
            virtual bool hasTeam(int tid);

            virtual bool hasStarted() const;
            virtual bool hasFinished() const;


            virtual int getWinner(int *dest, int max) const;
            virtual int getLoser(int *dest, int max) const;

            virtual int getTeamMsg(char *dest, Team* t) const;

            virtual void dump();

            //template<typename T> friend class Match* Factory::createMatchInstance();
    };

    class TeamInfo
    {
        public:
            class Team* team;
            int order;
            int fail;
            TeamInfo() :
                team(0), order(-1), fail(0)
            {}
            TeamInfo(class Team *t, int order) :
                team(t), order(order), fail(0)
            {}
    };

}

#endif /* LMSMATCH_H_ */

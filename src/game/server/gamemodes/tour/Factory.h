/*
 * Factory.h
 *
 *  Created on: Oct 14, 2010
 *      Author: fisted
 */

#ifndef FACTORY_H_
#define FACTORY_H_

#include <map>
#include <string>

#define MAXMATCHNAME 128
#define MAXTOURNAME 128

namespace tour
{

    class Factory
    {
        private:
            //static std::map<std::string, class Tournament*(*)()> _tourMap;
            //static std::map<std::string, class Match*(*)()> _matchMap;
            static char _defaultMatchStyle[MAXMATCHNAME + 1];
            static char _defaultTournamentStyle[MAXTOURNAME + 1];

            //template<typename T> static class Tournament* createTournamentInstance();
            //template<typename T> static class Match* createMatchInstance();
        public:
            static void init();
            static void registerStuff();
            static class Tournament *allocTournament();
            static class Tournament *allocCustomTournament(const char*classname);
            static class Match *allocMatch();
            static class Match *allocCustomMatch(const char*classname);
    };
}
#endif /* FACTORY_H_ */

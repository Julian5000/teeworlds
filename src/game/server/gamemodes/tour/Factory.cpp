/*
 * Factory.cpp
 *
 *  Created on: Oct 14, 2010
 *      Author: fisted
 */

#include <cstring>
#include <string>

#include "match/impl/LMSMatch.h"
#include "tour/impl/SingleElimTournament.h"

#include "Factory.h"

//#define REGTOUR(NAME) _tourMap[ #NAME ] = &createTournamentInstance<NAME>;
//#define REGMATCH(NAME) _matchMap[ #NAME ] = &createMatchInstance<NAME>;

namespace tour
{
    //std::map<std::string, Tournament*(*)()> Factory::_tourMap;
    //std::map<std::string, Match*(*)()> Factory::_matchMap;
    char Factory::_defaultMatchStyle[MAXMATCHNAME + 1];
    char Factory::_defaultTournamentStyle[MAXTOURNAME + 1];
    void Factory::init()
    {
        strcpy(_defaultMatchStyle, "LMSMatch");
        _defaultMatchStyle[MAXMATCHNAME] = '\0';
        strcpy(_defaultTournamentStyle, "SingleElimTournament");
        _defaultTournamentStyle[MAXTOURNAME] = '\0';
        registerStuff();

    }

    void Factory::registerStuff()
    {
        //REGTOUR(SingleElimTournament);
        //REGMATCH(LMSMatch);
    }

    Tournament *Factory::allocTournament()
    {
        return allocCustomTournament(0);
    }

    Tournament *Factory::allocCustomTournament(const char*tourname)
    {
    	return new SingleElimTournament;
        //std::string s(tourname ? tourname : _defaultTournamentStyle);
        //return (_tourMap.count(s)) ? _tourMap[s]() : NULL;
    }

    Match *Factory::allocMatch()
    {
        return allocCustomMatch(0);
    }

    Match *Factory::allocCustomMatch(const char*matchname)
    {
    	return new LMSMatch;
        //std::string s(matchname ? matchname : _defaultMatchStyle);
        //return (_matchMap.count(s)) ? _matchMap[s]() : NULL;
    }

    /*template<typename T> class Tournament* Factory::createTournamentInstance()
    {
        return new T;
    }

    template<typename T> class Match* Factory::createMatchInstance()
    {
        return new T;
    }*/
}

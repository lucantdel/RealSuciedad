#include "parsers.h"
#include <string>

void skipDelims(std::string_view& sv)
{
    const char* delims = " ()";
    size_t pos = sv.find_first_not_of(delims);
    if (pos == std::string_view::npos) {
        sv = {};
    } else {
        sv.remove_prefix(pos);
    }
}

std::string_view nextToken(std::string_view& sv)
{
    skipDelims(sv);
    if (sv.empty()) return {};

    size_t end = sv.find_first_of(" ()");
    std::string_view tok = sv.substr(0, end);
    if (end == std::string_view::npos) {
        sv = {};
    } else {
        sv.remove_prefix(end);
    }
    return tok;
}

bool parseObjectInfo(std::string_view msg,
                      std::string_view objectTag,
                      ObjectInfo& out)
{
    size_t pos = msg.find(objectTag);
    if (pos == std::string_view::npos) {
        out = ObjectInfo{};
        return false;
    }

    std::string_view sv = msg.substr(pos + objectTag.size());

    // Extraer distancia y dirección
    auto distTok = nextToken(sv);
    auto dirTok  = nextToken(sv);

    if (distTok.empty() || dirTok.empty()) {
        out.visible = false;
        return false;
    }

    out.dist = std::stod(std::string(distTok));
    out.dir  = std::stod(std::string(dirTok));
    out.visible = true;
    return true;
}

void parseInitMsg(const std::string &msg, PlayerInfo &player)
{
    std::string_view sv = msg;

    auto cmdTok = nextToken(sv);

    auto sideTok = nextToken(sv);
    if (sideTok == "l")
        player.side = Side::Left;
    else if (sideTok == "r")
        player.side = Side::Right;
    else
        player.side = Side::Unknown;

    auto numberTok = nextToken(sv);
    player.number = std::stoi(std::string(numberTok));

    auto playmodeTok = nextToken(sv); 
    player.playMode = std::string(playmodeTok);

    auto position = calcKickOffPosition(player.number);
    player.initialPosition = position;
}

void parseSeeMsg(const std::string &msg, PlayerInfo &player)
{
    std::string_view sv = msg;

    auto cmdTok  = nextToken(sv);
    auto timeTok = nextToken(sv);
    player.see.time = std::stoi(std::string(timeTok));

    parseObjectInfo(msg, "(b)", player.see.ball);

    // Identificar porterías según el lado del jugador
    if (player.side == Side::Left) {
        // Nuestro lado es el izquierdo -> nuestra portería es g l, rival g r
        parseObjectInfo(msg, "(g l)", player.see.own_goal);
        parseObjectInfo(msg, "(g r)", player.see.opp_goal);
    } else if (player.side == Side::Right) {
        // Nuestro lado es el derecho -> nuestra portería es g r, rival g l
        parseObjectInfo(msg, "(g r)", player.see.own_goal);
        parseObjectInfo(msg, "(g l)", player.see.opp_goal);
    } 
}

void parseSenseMsg(const std::string &msg, PlayerInfo &player)
{
    // TODO: Implementar parsing completo de sense_body
}

void parseHearMsg(const std::string &msg, PlayerInfo &player)
{
    // TODO: Implementar parsing completo de hear
}

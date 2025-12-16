#include "parsers.h"
#include <cmath>
#include <sstream>

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

bool parseObjectInfo(std::string_view msg, std::string_view objectTag, ObjectInfo& out)
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

PlayMode mapRefereeTokenToPlayMode(std::string_view tok)
{
    std::cout << "[DEBUG] Mapping referee token to PlayMode: " << tok << std::endl;

    if (tok == "before_kick_off")           return PlayMode::BeforeKickOff;
    if (tok == "play_on")                   return PlayMode::PlayOn;

    if (tok == "kick_off_l")                return PlayMode::KickOff_Left;
    if (tok == "kick_off_r")                return PlayMode::KickOff_Right;

    if (tok == "kick_in_l")                 return PlayMode::KickIn_Left;
    if (tok == "kick_in_r")                 return PlayMode::KickIn_Right;

    if (tok == "corner_kick_l")             return PlayMode::Corner_Left;
    if (tok == "corner_kick_r")             return PlayMode::Corner_Right;

    if (tok == "goal_kick_l")               return PlayMode::GoalKick_Left;
    if (tok == "goal_kick_r")               return PlayMode::GoalKick_Right;

    if (tok.compare(0, 6, "goal_l") == 0)   return PlayMode::Goal_Left;
    if (tok.compare(0, 6, "goal_r") == 0)   return PlayMode::Goal_Right;

    return PlayMode::Unknown;
}

// Actualiza marcador a partir de tokens tipo goal_l_1 / goal_r_2
void updateScoreFromGoalToken(std::string_view tok, GameState &game)
{
    // Formato esperado: goal_l_3 o goal_r_1
    if (tok.compare(0, 5, "goal_") != 0)
        return;

    if (tok.size() < 7) // mínimo: g o a l _ x _ n
        return;

    char sideChar = tok[5]; // 'l' o 'r'
    if (tok[6] != '_')
        return;

    std::string_view numStr = tok.substr(7); // "3", "10", etc.
    int goalNumber = 0;
    try {
        goalNumber = std::stoi(std::string(numStr));
    } catch (...) {
        return;
    }

    if (sideChar == 'l')
        game.scoreLeft = goalNumber;
    else if (sideChar == 'r')
        game.scoreRight = goalNumber;
}

void parseInitMsg(const std::string &msg, PlayerInfo &player, GameState &gameState)
{
    std::string_view sv = msg;

    nextToken(sv); // Saltar "(init"

    auto sideTok = nextToken(sv);
    if (sideTok == "l")
        player.side = Side::Left;
    else if (sideTok == "r")
        player.side = Side::Right;
    else
        player.side = Side::Unknown;

    auto numberTok = nextToken(sv);
    player.number = std::stoi(std::string(numberTok));

    auto playModeTok = nextToken(sv); 
    gameState.playMode = mapRefereeTokenToPlayMode(std::string(playModeTok));

    auto position = calcKickOffPosition(player.number);
    player.initialPosition = position;
}

void parseSeeMsg(const std::string &msg, PlayerInfo &player)
{
    std::string_view sv = msg;

    nextToken(sv); // Saltar "(see"
    auto timeTok = nextToken(sv);
    player.see.time = std::stoi(std::string(timeTok));

    parseObjectInfo(msg, "(b)", player.see.ball);

    // Identificar porterías según el lado del jugador
    if (player.side == Side::Left) {
        // Nuestro lado es el izquierdo -> nuestra portería es g l, rival g r
        parseObjectInfo(msg, "(g l)", player.see.ownGoal);
        parseObjectInfo(msg, "(g r)", player.see.oppGoal);
    } else if (player.side == Side::Right) {
        // Nuestro lado es el derecho -> nuestra portería es g r, rival g l
        parseObjectInfo(msg, "(g r)", player.see.ownGoal);
        parseObjectInfo(msg, "(g l)", player.see.oppGoal);
    } 
}

void parseSenseMsg(const std::string &msg, PlayerInfo &player)
{
    // TODO: Implementar parsing completo de sense_body
}

void parseHearMsg(const std::string &msg, PlayerInfo &player, GameState &gameState)
{
    std::string_view sv = msg;

    nextToken(sv); // Saltar "(hear"

    auto timeTok = nextToken(sv);
    gameState.time = std::stoi(std::string(timeTok));

    auto sourceTok = nextToken(sv);
    if (!(sourceTok == "referee")) 
        return;

    auto messageTok = nextToken(sv);

    // 4.1) Actualizar marcador si es un gol
    updateScoreFromGoalToken(messageTok, gameState);

    gameState.playMode = mapRefereeTokenToPlayMode(std::string(messageTok));
}

std::vector<FlagInfo> parseVisibleFlags(const std::string &seeMsg)
{
    std::vector<FlagInfo> visibleFlags;
    const std::string &s = seeMsg;
    std::size_t start = 0;
    const std::size_t N = s.size();

    // Función auxiliar para eliminar los espacios en blanco al principio y al final de una cadena
    auto trim = [](std::string &str){
        std::size_t a = 0;
        while (a < str.size() && std::isspace((unsigned char)str[a])) ++a; // Eliminar espacios al principio
        std::size_t b = str.size();
        while (b > a && std::isspace((unsigned char)str[b-1])) --b; // Eliminar espacios al final
        str = str.substr(a, b-a);
    };

    // Bucle para recorrer todo el mensaje en busca de flags
    while (start < N) {
        // buscar la próxima ocurrencia de "(f" o "(g" o "(b"
        std::size_t pos_f = s.find("(f", start);
        std::size_t pos_g = s.find("(g", start);
        std::size_t pos_b = s.find("(b", start);

        // Determinar cuál de las posiciones es la más cercana
        std::size_t pos = std::string::npos;
        if (pos_f != std::string::npos) pos = pos_f;
        if (pos_g != std::string::npos && (pos == std::string::npos || pos_g < pos)) pos = pos_g;
        if (pos_b != std::string::npos && (pos == std::string::npos || pos_b < pos)) pos = pos_b;

        // Si no se encuentran flags, salimos del bucle
        if (pos == std::string::npos) break;

        // encontrar el cierre de ese paréntesis correspondiente (primer ')' tras pos)
        std::size_t close = s.find(')', pos);
        if (close == std::string::npos) break; // formato raro -> salir

        // nombre entre '(' y ')', por ejemplo "f c" o "f t l 40"
        std::string name = s.substr(pos + 1, close - (pos + 1)); // quitar '('
        trim(name);

        // validar nombre con FLAG_POSITIONS
        if (FLAG_POSITIONS.find(name) != FLAG_POSITIONS.end()) {
            // extraer números que siguen después de close
            // tomamos una ventana corta a partir de close+1 para parsear dist y dir
            std::size_t num_start = close + 1;
            // aumentar la ventana si hace falta, pero limitarla para eficiencia
            std::size_t len = std::min<std::size_t>(N - num_start, 128);
            std::string tail = s.substr(num_start, len); // Extraer la parte del mensaje que contiene los números
            std::istringstream iss(tail); // Convertir la parte extraída a un stream para parsear los números
            double dist = 0.0, dir = 0.0;
            // Si se pudieron leer correctamente los números de distancia y dirección
            if ( (iss >> dist >> dir) ) {
                FlagInfo fi;
                fi.name = name;
                fi.dist = dist;
                fi.dir  = dir;
                fi.visible = true;
                fi.pos = FLAG_POSITIONS.at(name);
                visibleFlags.push_back(fi);
            }
            // si no se pudieron leer números, ignoramos esta ocurrencia
        }

        // avanzar start para seguir buscando (evitar loops infinitos)
        start = close + 1;
    }

    return visibleFlags;
}
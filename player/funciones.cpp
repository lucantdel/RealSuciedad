#include "estructuras.h"
#include <MinimalSocket/udp/UdpSocket.h>
#include <cmath>
#include <thread>
#include <chrono>

#include "estructuras.h"
#include <MinimalSocket/udp/UdpSocket.h>
#include <iostream>
#include <cmath>
#include <map>

// Definición real
std::map<std::string, Point> FLAG_POSITIONS = {
    {"f c",      {0, 0}},
    {"f c t",    {0, 34}},
    {"f c b",    {0, -34}},
    {"f r t",    {52.5, 34}},
    {"f r b",    {52.5, -34}},
    {"f l t",    {-52.5, 34}},
    {"f l b",    {-52.5, -34}},
    // Resto de flags...
};

// ---- IMPLEMENTACIONES DE OPERADORES ----

std::ostream& operator<<(std::ostream& os, Side s)
{
    switch (s) {
        case Side::Left:    os << "Left"; break;
        case Side::Right:   os << "Right"; break;
        default:            os << "Unknown"; break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const Point& p)
{
    os << "Point(x=" << p.x << ", y=" << p.y << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const ObjectInfo& o)
{
    os << "ObjectInfo(dist=" << o.dist
       << ", dir=" << o.dir
       << ", visible=" << std::boolalpha << o.visible << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const SeeInfo& s)
{
    os << "SeeInfo(time=" << s.time
       << ", ball=" << s.ball
       << ", own_goal=" << s.own_goal
       << ", opp_goal=" << s.opp_goal
       << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const SenseInfo& s)
{
    os << "SenseInfo(time=" << s.time
       << ", speed=" << s.speed
       << ", stamina=" << s.stamina
       << ", headAngle=" << s.headAngle
       << ")";
    return os;
}

std::ostream& operator<<(std::ostream &os, const PlayerInfo &player) 
{
    os << "Player created (side: " << player.side
       << ", number: " << player.number
       << ", playmode: " << player.playMode
       << ", position: (" << player.initialPosition.x << ", " << player.initialPosition.y << "))";
    return os;
}

// ---- IMPLEMENTACIONES DE FUNCIONES ----


// Calcula la posición inicial en el campo según el número de dorsal (1-11)
Point calcKickOffPosition(int unum)
{
    static const Point positions[11] = {
        { -50.0,   0.0 },                                                       // 1: Portero
        { -40.0, -15.0 }, { -42.0,  -5.0 }, { -42.0,   5.0 }, { -40.0,  15.0 }, // 2-5: Defensa
        { -28.0, -10.0 }, { -22.0,   0.0 }, { -28.0,  10.0 },                   // 6-8: Medios
        { -15.0, -15.0 }, { -12.0,   0.0 }, { -15.0,  15.0 }                    // 9-11: Delanteros
    };

    if (unum < 1 || unum > 11) {
        return Point{0.0, 0.0};
    }

    return positions[unum - 1];
}

std::string receiveMsgFromServer(MinimalSocket::udp::Udp<true> &udp_socket, std::size_t message_max_size)
{
    auto received_message = udp_socket.receive(message_max_size);

    if (!received_message) {
        std::cerr << "Error receiving message from server" << std::endl;
        return "";
    }

    std::string received_message_content = received_message->received_message;
    std::cout << "Received message: " << received_message_content << std::endl;

    return received_message_content;
}

// ---------- HELPERS DE PARSING ----------

// Elimina espacios y paréntesis al inicio del string_view
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

// Extrae el siguiente token del string_view (palabra delimitada por espacios o paréntesis)
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

// Busca y parsea la información de un objeto específico en el mensaje see
// Retorna true si el objeto fue encontrado y parseado correctamente
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

// ---------- FUNCIONES DE PARSING ----------

// Parsea el mensaje de inicialización del servidor
// Ejemplo: (init l 1 before_kick_off) -> lado izquierdo, dorsal 1
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

// Parsea el mensaje de visión del servidor
// Ejemplo: (see 0 ... ((g r) 102.5 0) ... ((b) 49.4 0) ...)
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

// Parsea el mensaje de información sensorial interna del jugador
// Ejemplo: (sense_body 0 ... (stamina 8000 1 130600) (speed 0 0) (head_angle 0) ...)
void parseSenseMsg(const std::string &msg, PlayerInfo &player)
{
    // TODO: Implementar parsing completo de sense_body
}

// Función para parsear las flags de un mensaje "see"
std::vector<FlagInfo> parseVisibleFlags(const std::string &seeMsg) {
    std::vector<FlagInfo> visibleFlags;
    std::string_view sv = seeMsg;

    nextToken(sv); // Saltar "(see"
    nextToken(sv); // Saltar tiempo

    while (!sv.empty()) {
    auto tok = nextToken(sv);
    if (tok.empty()) break;

        // Detectar si es una flag (empieza con f o g o b)
        std::string flagName(tok);
        if (tok == "f" || tok == "g" || tok == "b") {
            // mirar siguiente token para completar nombre
            auto nextTok = nextToken(sv);
            if (!nextTok.empty() && nextTok != ")") {
                flagName += " " + std::string(nextTok);
                // para flags de 3 tokens (ej: f c b)
                if (FLAG_POSITIONS.find(flagName) == FLAG_POSITIONS.end()) {
                    auto nextTok2 = nextToken(sv);
                    if (!nextTok2.empty() && nextTok2 != ")") {
                        flagName += " " + std::string(nextTok2);
                    }
                }
            }
        }

        if (FLAG_POSITIONS.find(flagName) != FLAG_POSITIONS.end()) {
            try {
                double dist = std::stod(std::string(nextToken(sv)));
                double dir  = std::stod(std::string(nextToken(sv)));
                Point absolutePos = FLAG_POSITIONS[flagName];
                visibleFlags.push_back({flagName, dist, dir, true, absolutePos});
            } catch (const std::invalid_argument&) {
                continue; 
            }
        }
    }

    return visibleFlags;
}

// Función para obtener las dos flags más cercanas
std::pair<FlagInfo, FlagInfo> getTwoClosestFlags(const std::string &see_msg) {
    auto flags = parseVisibleFlags(see_msg);

    if (flags.size() < 2) {
        std::cerr << "No hay suficientes flags visibles para triangulación." << std::endl;
        return {FlagInfo{}, FlagInfo{}};
    }

    // Ordenar por distancia ascendente
    std::sort(flags.begin(), flags.end(), [](const FlagInfo &a, const FlagInfo &b) {
        return a.dist < b.dist;
    });

    // Devolver las dos primeras
    return {flags[0], flags[1]};
}

// ---------- FUNCIONES PARA ENVIAR COMANDOS ----------

// Envía el comando de inicialización al servidor
// Los puertos 7001 y 8001 se asignan como porteros
void sendInitCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, MinimalSocket::Port this_socket_port, std::string team_name)
{
    std::string init_msg;

    if (this_socket_port == 7001 || this_socket_port == 8001) {
        init_msg = "(init " + team_name + " (version 19) (goalie))";
    } else {
        init_msg = "(init " + team_name + " (version 19))";
    }

    std::cout << "Sending init message: " << init_msg << std::endl;
    udp_socket.sendTo(init_msg, server_udp);
    std::cout << "Init message sent" << std::endl;
}

// Envía el comando para posicionar al jugador en su ubicación inicial
void sendMoveCommand(PlayerInfo &player, MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp)
{
    std::string move_cmd =
        "(move " + std::to_string(player.initialPosition.x) +
        " "      + std::to_string(player.initialPosition.y) + ")";

    std::cout << "Sending move command: " << move_cmd << std::endl;
    udp_socket.sendTo(move_cmd, server_udp);
    std::cout << "Move command sent" << std::endl;
}

// ---------- FUNCIONES DE DECISIÓN ----------

// Decide la acción a realizar basándose en la información visual del jugador
std::string decideAction(const PlayerInfo &player)
{
    std::string action_cmd;

        // Si no ve el balón, girar para buscarlo
        if (!player.see.ball.visible) {
            action_cmd = "(turn 30)";
        } else {
            double ball_dist = player.see.ball.dist;
            double ball_dir  = player.see.ball.dir;

            // Balón lejos: orientarse y correr hacia él
            if (ball_dist > 1.0) {
                if (std::abs(ball_dir) > 10.0) {
                    // Ajustar orientación hacia el balón (limitado a ±60°)
                    double turnAngle = ball_dir;
                    if (turnAngle > 60.0) turnAngle = 60.0;
                    if (turnAngle < -60.0) turnAngle = -60.0;

                    action_cmd = "(turn " + std::to_string(turnAngle) + ")";
                } else {
                    // Ya orientado, correr hacia el balón
                    action_cmd = "(dash 90)";
                }
            } else {
                // Balón cerca: intentar chutar
                if (player.see.opp_goal.visible) {
                    double kickDir = player.see.opp_goal.dir;
                    action_cmd = "(kick 90 " + std::to_string(kickDir) + ")";
                } else {
                    // Si no ve la portería rival, girar para buscarla
                    action_cmd = "(turn 30)";
                }
            }
        }
    return action_cmd;
}

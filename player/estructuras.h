#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <MinimalSocket/udp/UdpSocket.h>
#include <MinimalSocket/core/Address.h>

// ---------- ESTRUCTURAS DE DATOS ---------- //

// Representa el lado del campo donde juega el equipo
enum class Side
{
    Left, Right, Unknown 
};

std::ostream& operator<<(std::ostream& os, Side s);


// Coordenadas 2D en el campo
struct Point
{
    double x{0.0};
    double y{0.0};
};

struct FlagInfo {
    std::string name;
    double dist;
    double dir;
    bool visible;
    Point pos; // coordenadas absolutas desde FLAG_POSITIONS
};

extern std::map<std::string, Point> FLAG_POSITIONS;

std::ostream& operator<<(std::ostream& os, const Point& p);

// Información sobre un objeto visto: distancia, dirección y visibilidad
struct ObjectInfo
{
    double dist{0.0};     // Distancia al objeto
    double dir{0.0};      // Dirección en grados
    bool visible{false};  // Si el objeto es visible
};

std::ostream& operator<<(std::ostream& os, const ObjectInfo& o);

// Información visual del jugador en un instante dado
struct SeeInfo
{
    int time{0};              // Tiempo de simulación
    ObjectInfo ball{};        // Información del balón
    ObjectInfo own_goal{};    // Información de portería propia
    ObjectInfo opp_goal{};    // Información de portería rival
};

std::ostream& operator<<(std::ostream& os, const SeeInfo& s);

// Información sensorial interna del jugador
struct SenseInfo 
{
    int time{0};              // Tiempo de simulación
    double speed{0.0};        // Velocidad actual
    double stamina{0.0};      // Resistencia actual
    double headAngle{0.0};    // Ángulo de la cabeza
};

std::ostream& operator<<(std::ostream& os, const SenseInfo& s);

// Información completa del jugador
struct PlayerInfo 
{
    std::string team{""};
    Side side{Side::Unknown};
    int number{-1};
    std::string playMode{""};
    SeeInfo see{};
    SenseInfo sense{};
    Point initialPosition{};  // Posición inicial asignada según el dorsal

    // posición absoluta
    float x_abs{0.0f};
    float y_abs{0.0f};
    float dir_abs{0.0f};
};

std::ostream& operator<<(std::ostream &os, const PlayerInfo &player);

// --------------- PROTOTIPOS DE FUNCIÓN ------------------

Point calcKickOffPosition(int unum);

std::string receiveMsgFromServer(
    MinimalSocket::udp::Udp<true>& udp_socket,
    std::size_t message_max_size
);

void skipDelims(std::string_view& sv);
std::string_view nextToken(std::string_view& sv);

bool parseObjectInfo(std::string_view msg,
                     std::string_view objectTag,
                     ObjectInfo& out);

void parseInitMsg(const std::string &msg, PlayerInfo &player);
void parseSeeMsg(const std::string &msg, PlayerInfo &player);
void parseSenseMsg(const std::string &msg, PlayerInfo &player);
std::vector<FlagInfo> parseVisibleFlags(const std::string &see_msg);
std::pair<FlagInfo, FlagInfo> getTwoBestFlags(const std::string &see_msg);

std::vector<Point> corteCircunferencias(
        float x1, float y1, float r1,
        float x2, float y2, float r2);
Point calcularPosicionJugador(const std::pair<FlagInfo,FlagInfo>& flags);


void sendInitCommand(MinimalSocket::udp::Udp<true> &udp_socket,
                     const MinimalSocket::Address &server_udp,
                     MinimalSocket::Port this_socket_port,
                     std::string team_name);

void sendMoveCommand(PlayerInfo &player,
                     MinimalSocket::udp::Udp<true> &udp_socket,
                     const MinimalSocket::Address &server_udp);

std::string decideAction(const PlayerInfo &player);

#endif // ESTRUCTURAS_H

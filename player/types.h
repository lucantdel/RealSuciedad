#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <iostream>
#include <MinimalSocket/udp/UdpSocket.h>


// Representa el lado del campo donde juega el equipo
enum class Side
{
    Left, Right, Unknown 
};

std::ostream& operator<<(std::ostream& os, Side s)
{
    switch (s) {
        case Side::Left:    os << "Left"; break;
        case Side::Right:   os << "Right"; break;
        default:            os << "Unknown"; break;
    }
    return os;
}

// Coordenadas 2D en el campo
struct Point
{
    double x{0.0};
    double y{0.0};
};

std::ostream& operator<<(std::ostream& os, const Point& p)
{
    os << "Point(x=" << p.x << ", y=" << p.y << ")";
    return os;
}

// Información sobre un objeto visto: distancia, dirección y visibilidad
struct ObjectInfo
{
    double dist{0.0};     // Distancia al objeto
    double dir{0.0};      // Dirección en grados
    bool visible{false};  // Si el objeto es visible
};

std::ostream& operator<<(std::ostream& os, const ObjectInfo& o)
{
    os << "ObjectInfo(dist=" << o.dist
       << ", dir=" << o.dir
       << ", visible=" << std::boolalpha << o.visible << ")";
    return os;
}

// Información visual del jugador en un instante dado
struct SeeInfo
{
    int time{0};              // Tiempo de simulación
    ObjectInfo ball{};        // Información del balón
    ObjectInfo own_goal{};    // Información de portería propia
    ObjectInfo opp_goal{};    // Información de portería rival
};

std::ostream& operator<<(std::ostream& os, const SeeInfo& s)
{
    os << "SeeInfo(time=" << s.time
       << ", ball=" << s.ball
       << ", own_goal=" << s.own_goal
       << ", opp_goal=" << s.opp_goal
       << ")";
    return os;
}

// Información sensorial interna del jugador
struct SenseInfo 
{
    int time{0};              // Tiempo de simulación
    double speed{0.0};        // Velocidad actual
    double stamina{0.0};      // Resistencia actual
    double headAngle{0.0};    // Ángulo de la cabeza
};

std::ostream& operator<<(std::ostream& os, const SenseInfo& s)
{
    os << "SenseInfo(time=" << s.time
       << ", speed=" << s.speed
       << ", stamina=" << s.stamina
       << ", headAngle=" << s.headAngle
       << ")";
    return os;
}

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
};

std::ostream& operator<<(std::ostream &os, const PlayerInfo &player) 
{
    os << "Player created (side: " << player.side
       << ", number: " << player.number
       << ", playmode: " << player.playMode
       << ", position: (" << player.initialPosition.x << ", " << player.initialPosition.y << "))";
    return os;
}
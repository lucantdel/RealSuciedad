#pragma once

#include <iostream>
#include <string>
#include <map>


// Representa el lado del campo donde juega el equipo
enum class Side
{
    Left, Right, Unknown 
};

inline std::ostream& operator<<(std::ostream& os, Side s)
{
    switch (s) {
        case Side::Left:    os << "Left"; break;
        case Side::Right:   os << "Right"; break;
        default:            os << "Unknown"; break;
    }
    return os;
}

// Representa el modo de juego actual
enum class PlayMode
{
    Unknown, BeforeKickOff, PlayOn,
    KickOff_Left, KickOff_Right,
    KickIn_Left, KickIn_Right,
    Corner_Left, Corner_Right,
    GoalKick_Left, GoalKick_Right,
    Goal_Left, Goal_Right
};

inline std::ostream& operator<<(std::ostream& os, PlayMode pm)
{
    switch (pm) {
        case PlayMode::BeforeKickOff:   os << "BeforeKickOff";  break;
        case PlayMode::PlayOn:          os << "PlayOn";         break;
        case PlayMode::KickOff_Left:    os << "KickOff_Left";   break;
        case PlayMode::KickOff_Right:   os << "KickOff_Right";  break;
        case PlayMode::KickIn_Left:     os << "KickIn_Left";    break;
        case PlayMode::KickIn_Right:    os << "KickIn_Right";   break;
        case PlayMode::Corner_Left:     os << "Corner_Left";    break;
        case PlayMode::Corner_Right:    os << "Corner_Right";   break;
        case PlayMode::GoalKick_Left:   os << "GoalKick_Left";  break;
        case PlayMode::GoalKick_Right:  os << "GoalKick_Right"; break;
        case PlayMode::Goal_Left:       os << "Goal_Left";      break;
        case PlayMode::Goal_Right:      os << "Goal_Right";     break;
        default:                        os << "Unknown";        break;
    }
    return os;
}

// Coordenadas 2D en el campo
struct Point
{
    double x{0.0};
    double y{0.0};
};

struct Zona {
    double x_min, x_max, y_min, y_max;
};

struct FlagInfo {
    std::string name;
    double dist;
    double dir;
    bool visible;
    Point pos; // coordenadas absolutas desde FLAG_POSITIONS
};

inline std::ostream& operator<<(std::ostream& os, const Point& p)
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

inline std::ostream& operator<<(std::ostream& os, const ObjectInfo& o)
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
    ObjectInfo ownGoal{};    // Información de portería propia
    ObjectInfo oppGoal{};    // Información de portería rival
};

inline std::ostream& operator<<(std::ostream& os, const SeeInfo& s)
{
    os << "SeeInfo(time=" << s.time
       << ", ball=" << s.ball
       << ", ownGoal=" << s.ownGoal
       << ", oppGoal=" << s.oppGoal
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

inline std::ostream& operator<<(std::ostream& os, const SenseInfo& s)
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
    SeeInfo see{};
    SenseInfo sense{};
    Point initialPosition{};  // Posición inicial asignada según el dorsal

    // posición absoluta
    float x_abs{0.0f};
    float y_abs{0.0f};
    float dir_abs{0.0f};
};

inline std::ostream& operator<<(std::ostream &os, const PlayerInfo &player) 
{
    os << "Player(team: " << player.team
       << ", side: " << player.side
       << ", number: " << player.number
       << ", initialPosition: (" << player.initialPosition.x << ", " << player.initialPosition.y << "))";
    return os;
}

// Estado global del partido (igual para todos los jugadores)
struct GameState
{
    int time{0};
    PlayMode playMode{PlayMode::Unknown};
    int scoreLeft{0};
    int scoreRight{0};
};


inline std::ostream& operator<<(std::ostream &os, const GameState &gameState) 
{
    os << "GameState(time: " << gameState.time
       << ", playMode: " << gameState.playMode
       << ", scoreLeft: " << gameState.scoreLeft
       << ", scoreRight: " << gameState.scoreRight
       << ")";
    return os;
}
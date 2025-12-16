#pragma once

#include "types.h"
#include "positions.h"
#include <string_view>

// Parsea el mensaje de inicialización del servidor
// Ejemplo: (init l 1 before_kick_off)
void parseInitMsg(const std::string &msg, PlayerInfo &player, GameState &gameState);

// Parsea el mensaje de visión del servidor
// Ejemplo: (see 0 ... ((g r) 102.5 0) ... ((b) 49.4 0) ...)
void parseSeeMsg(const std::string &msg, PlayerInfo &player);

// Parsea el mensaje de información sensorial interna del jugador
// Ejemplo: (sense_body 0 ... (stamina 8000 1 130600) (speed 0 0) (head_angle 0) ...)
void parseSenseMsg(const std::string &msg, PlayerInfo &player);

// Parsea el mensaje de audición del jugador
// Ejemplo: (hear 0 referee kick_off_l)
void parseHearMsg(const std::string &msg, PlayerInfo &player, GameState &gameState);

// Parsea y devuelve una lista de banderas visibles en el mensaje de visión
std::vector<FlagInfo> parseVisibleFlags(const std::string &see_msg);
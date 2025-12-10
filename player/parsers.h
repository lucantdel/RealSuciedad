#pragma once

#include <types.h>
#include <positions.h>

// Elimina espacios y paréntesis al inicio del string_view
void skipDelims(std::string_view& sv);

// Extrae el siguiente token del string_view (palabra delimitada por espacios o paréntesis)
std::string_view nextToken(std::string_view& sv);

// Busca y parsea la información de un objeto específico en el mensaje see
// Retorna true si el objeto fue encontrado y parseado correctamente
bool parseObjectInfo(std::string_view msg, std::string_view objectTag, ObjectInfo& out);

// Parsea el mensaje de inicialización del servidor
// Ejemplo: (init l 1 before_kick_off) -> lado izquierdo, dorsal 1
void parseInitMsg(const std::string &msg, PlayerInfo &player);

// Parsea el mensaje de visión del servidor
// Ejemplo: (see 0 ... ((g r) 102.5 0) ... ((b) 49.4 0) ...)
void parseSeeMsg(const std::string &msg, PlayerInfo &player);

// Parsea el mensaje de información sensorial interna del jugador
// Ejemplo: (sense_body 0 ... (stamina 8000 1 130600) (speed 0 0) (head_angle 0) ...)
void parseSenseMsg(const std::string &msg, PlayerInfo &player);

// Parsea el mensaje de audición del jugador
// Ejemplo: 
void parseHeatMsg(const std::string &msg, PlayerInfo &player);
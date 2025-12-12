#pragma once

#include "types.h"

// Decide la acción a realizar basándose en la información visual del jugador
std::string decideAction(const PlayerInfo &player, const GameState &gameState);
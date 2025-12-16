#pragma once

#include "types.h"
#include <vector>

// Definición flags y sus posiciones absolutas en el campo
inline static std::map<std::string, Point> FLAG_POSITIONS = {
    // Corners
    {"f l t", {-52.5, 34}},
    {"f l b", {-52.5, -34}},
    {"f r t", { 52.5, 34}},
    {"f r b", { 52.5,-34}},

    // Center line
    {"f c", {0, 0}},
    {"f c t", {0, 34}},
    {"f c b", {0,-34}},

    // Left line
    {"f l 0", {-52.5, 0}},
    {"f l t 10", {-52.5, 10}},
    {"f l t 20", {-52.5, 20}},
    {"f l t 30", {-52.5, 30}},
    {"f l b 10", {-52.5, -10}},
    {"f l b 20", {-52.5, -20}},
    {"f l b 30", {-52.5, -30}},

    // Right line
    {"f r 0", {52.5, 0}},
    {"f r t 10", {52.5, 10}},
    {"f r t 20", {52.5, 20}},
    {"f r t 30", {52.5, 30}},
    {"f r b 10", {52.5, -10}},
    {"f r b 20", {52.5, -20}},
    {"f r b 30", {52.5, -30}},

    // Top line
    {"f t 0", {0,34}},
    {"f t l 10", {-10, 34}},
    {"f t l 20", {-20, 34}},
    {"f t l 30", {-30, 34}},
    {"f t r 10", { 10, 34}},
    {"f t r 20", { 20, 34}},
    {"f t r 30", { 30, 34}},

    // Bottom line
    {"f b 0", {0,-34}},
    {"f b l 10", {-10,-34}},
    {"f b l 20", {-20,-34}},
    {"f b l 30", {-30,-34}},
    {"f b r 10", { 10,-34}},
    {"f b r 20", { 20,-34}},
    {"f b r 30", { 30,-34}}
};

Point calcKickOffPosition(int unum);

Zona definirZonaJugador(PlayerInfo &p);

std::pair<FlagInfo, FlagInfo> getTwoBestFlags(const std::string &see_msg);

std::vector<Point> corteCircunferencias(
        float x1, float y1, float r1,
        float x2, float y2, float r2);
Point calcularPosicionJugador(const std::pair<FlagInfo,FlagInfo>& flags, const Point& last_pos);

double normalizaAngulo(double ang);

double calcularOrientacion(const Point& mi_pos, const FlagInfo& flag);
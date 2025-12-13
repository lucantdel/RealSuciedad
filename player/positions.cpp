#include "positions.h"

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
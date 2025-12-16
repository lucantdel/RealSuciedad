#include "positions.h"
#include "parsers.h"
#include <cmath>

// Calcula la posición inicial en el campo según el número de dorsal (1-11)
Point calcKickOffPosition(int unum)
{
    static const Point positions[11] = {
        { -50.0,   0.0 },                                                       // 1: Portero
        { -42.0, -25.0 },{ -44.0,  -8.0 },{ -44.0,   8.0 },{ -42.0,  25.0 },    // 2-5: Defensa
        { -28.0,   0.0 },{ -20.0, -15.0 },{ -20.0,  15.0 },                     // 6-8: Medios
        { -5.0,  -20.0 },{ -10.0,    0.0 },{ -5.0,   20.0 }                      // 9-11: Delanteros
    };

    if (unum < 1 || unum > 11) {
        return Point{0.0, 0.0};
    }

    return positions[unum - 1];
}

//Zona de cada jugador según su número
Zona definirZonaJugador(PlayerInfo &p) {
    Zona zona{};

    switch(p.number) {
        case 1:
            zona = { -52.5, -40.0, -12.0, 12.0 };
            break;
        case 2:
            zona = { -52.5, -15.0, 10.0, 34.0 };
            break;
        case 3:
            zona = { -52.5, -20.0, 2.0, 20.0 };
            break;
        case 4:
            zona = { -52.5, -20.0, -20.0, -2.0 };
            break;
        case 5:
            zona = { -52.5, -15.0, -34.0, -10.0 };
            break;
        case 6:
            zona = { -35.0, 10.0, -15.0, 15.0 };
            break;
        case 7:
            zona = { -30.0, 25.0, 10.0, 34.0 };
            break;
        case 8:
            zona = { -30.0, 25.0, -34.0, -10.0 };
            break;
        case 9:
            zona = { -5.0, 52.5, 10.0, 34.0 };
            break;
        case 10:
            zona = { -10.0, 52.5, -15.0, 15.0 };
            break;
        case 11:
            zona = { -5.0, 52.5, -34.0, -10.0 };
            break;
        default:
            zona = { -52.5, 52.5, -34.0, 34.0 };
            break;
    }

    if (p.side == Side::Right) {
        // Invertir coordenadas X para el lado derecho
        double x_min = -zona.x_max;
        double x_max = -zona.x_min;
        zona.x_min = x_min;
        zona.x_max = x_max;
        double y_min = -zona.y_max;
        double y_max = -zona.y_min;
        zona.y_min = y_min;
        zona.y_max = y_max;
    }

    return zona;
}

// Función para obtener las dos mejores flags según la distancia y dirección
std::pair<FlagInfo, FlagInfo> getTwoBestFlags(const std::string &see_msg)
{
    // Parseamos el mensaje "see_msg" para obtener todas las flags visibles
    auto flags = parseVisibleFlags(see_msg);

    if (flags.size() < 2)
        return {FlagInfo{}, FlagInfo{}};

    // Eliminar duplicados: Si hay varias entradas de la misma flag, nos quedamos con la más cercana
    std::map<std::string, FlagInfo> unique;
    for (auto &f : flags)
        // Si la flag no está en el mapa o tiene una distancia menor, la agregamos
        if (!unique.count(f.name) || f.dist < unique[f.name].dist)
            unique[f.name] = f;

    // Convertimos las flags únicas en un vector
    std::vector<FlagInfo> v;
    for (auto &kv : unique)
        v.push_back(kv.second);

    if (v.size() < 2)
        return {FlagInfo{}, FlagInfo{}};

    // Inicializamos variables para encontrar las mejores dos flags
    bool found = false;
    double bestScore = 1e18;
    FlagInfo bestA, bestB;

    // Umbrales para filtrar distancias y ángulos pequeños
    const double MIN_DIST = 0.001;
    const double MIN_ANG  = 0.001;

    // Comparamos todas las combinaciones posibles de flags para encontrar las mejores
    for (size_t i = 0; i < v.size(); ++i) {
        for (size_t j = i + 1; j < v.size(); ++j) {

            const auto &A = v[i]; // Primera flag
            const auto &B = v[j]; // Segunda flag

            // Obtenemos las posiciones de las flags desde FLAG_POSITIONS
            auto pA = FLAG_POSITIONS.at(A.name);
            auto pB = FLAG_POSITIONS.at(B.name);

            // Calculamos la distancia entre las dos flags y la separación angular
            double dx = pB.x - pA.x;
            double dy = pB.y - pA.y;
            double baseLineDist = std::sqrt(dx*dx + dy*dy);
            double angSep = std::fabs(A.dir - B.dir);

            // Si las flags están demasiado cerca o casi en la misma dirección, las ignoramos
            if (baseLineDist < 1.0) continue;   // evita flags casi juntas
            if (angSep < 5.0) continue;         // evita flags casi en la misma dirección

            // Calculamos una "puntuación" basada en la distancia y la separación angular
            double score = (1.0 / (baseLineDist + MIN_DIST))
                         + (1.0 / (angSep + MIN_ANG));

            // Si encontramos una mejor puntuación (más baja), actualizamos las mejores flags
            if (!found || score < bestScore) {
                bestScore = score;
                bestA = A;
                bestB = B;
                found = true;
            }
        }
    }

    if (!found)
        return {FlagInfo{}, FlagInfo{}};

    return {bestA, bestB};
}

// Función para calcular los puntos de intersección de dos circunferencias dadas sus posiciones y radios
std::vector<Point> corteCircunferencias(
        float x1, float y1, float r1,
        float x2, float y2, float r2)
{
    std::vector<Point> res; // Vector para almacenar los puntos de intersección.

    // Calcular la distancia entre los centros de las circunferencias
    float dx = x2 - x1;
    float dy = y2 - y1;
    float d = std::sqrt(dx*dx + dy*dy);

    // Si la distancia es mayor que la suma de los radios o menor que la diferencia absoluta
    // entre los radios, o si las circunferencias están concéntricas (d == 0), no hay intersección válida.
    if (d > r1 + r2 || d < std::fabs(r1 - r2) || d == 0) {
        return res;
    }

    // Calcular los puntos de intersección
    float a = (r1*r1 - r2*r2 + d*d) / (2*d);
    float h = std::sqrt(r1*r1 - a*a); // Calculamos la distancia "h" que es la distancia entre el punto de intersección y el centro de cada circunferencia

    // Punto medio entre los dos centros
    float xm = x1 + a * dx / d;
    float ym = y1 + a * dy / d;

    // Calculamos las dos posibles intersecciones (puntos de intersección)
    float xs1 = xm + h * dy / d;
    float ys1 = ym - h * dx / d;
    float xs2 = xm - h * dy / d;
    float ys2 = ym + h * dx / d;

    res.push_back({xs1, ys1});
    res.push_back({xs2, ys2});
    return res;
}

// Función para calcular la posición de un jugador basada en las dos flags visibles
Point calcularPosicionJugador(const std::pair<FlagInfo,FlagInfo>& flags, const Point& last_pos)
{
    const auto& f1 = flags.first;
    const auto& f2 = flags.second;

    Point p1 = FLAG_POSITIONS[f1.name];
    Point p2 = FLAG_POSITIONS[f2.name];

    float x1 = p1.x, y1 = p1.y;
    float x2 = p2.x, y2 = p2.y;

    float r1 = f1.dist; // Distancia de la primera flag
    float r2 = f2.dist; // Distancia de la segunda flag

    double dx = x2 - x1, dy = y2 - y1;
    double d = std::sqrt(dx*dx + dy*dy); // Distancia entre las dos flags

    std::cout << "[DEBUG] Centers: ("<<x1<<","<<y1<<") r1="<<r1
              << " - ("<<x2<<","<<y2<<") r2="<<r2
              << " ; d="<<d << std::endl;

    auto puntos = corteCircunferencias(x1, y1, r1, x2, y2, r2);

    // Si no hay puntos de intersección, mantenemos la última posición conocida
    if (puntos.empty()) {
        return last_pos;
    }

    // Si hay solo un punto de intersección, devolvemos ese punto
    if (puntos.size() == 1) {
        return puntos[0];
    }

    auto& pA = puntos[0];
    auto& pB = puntos[1];

    // Elegir el punto más cercano a la posición previa del jugador
    double dA = hypot(pA.x - last_pos.x, pA.y - last_pos.y);
    double dB = hypot(pB.x - last_pos.x, pB.y - last_pos.y);

    return (dA < dB) ? pA : pB; // Devolvemos el punto más cercano
}

// Asegura que el ángulo esté entre -180 y 180
double normalizaAngulo(double ang){
    while (ang > 180) ang -= 360;
    while (ang < -180) ang += 360;
    return ang;
}

// Calcula la orientación absoluta hacia una flag (en grados)
double calcularOrientacion(const Point& mi_pos, const FlagInfo& flag)
{
    // Ángulo absoluto desde el jugador hacia la flag (Matemático CCW)
    Point pFlag = FLAG_POSITIONS[flag.name];
    double anguloAbsolutoAFlag = atan2(pFlag.y - mi_pos.y, pFlag.x - mi_pos.x) * 180.0 / M_PI;

    // Convertimos la dirección del servidor (CW) a dirección matemática (CCW)    
    double orientacion = anguloAbsolutoAFlag + flag.dir;

    return normalizaAngulo(orientacion); // Normalizamos el ángulo para que esté entre -180 y 180 grados
}


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

// Definición flags y sus posiciones absolutas en el campo
std::map<std::string, Point> FLAG_POSITIONS = {
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
std::vector<FlagInfo> parseVisibleFlags(const std::string &seeMsg)
{
    std::vector<FlagInfo> visibleFlags;
    const std::string &s = seeMsg;
    std::size_t start = 0;
    const std::size_t N = s.size();

    // Función auxiliar para eliminar los espacios en blanco al principio y al final de una cadena
    auto trim = [](std::string &str){
        std::size_t a = 0;
        while (a < str.size() && std::isspace((unsigned char)str[a])) ++a; // Eliminar espacios al principio
        std::size_t b = str.size();
        while (b > a && std::isspace((unsigned char)str[b-1])) --b; // Eliminar espacios al final
        str = str.substr(a, b-a);
    };

    // Bucle para recorrer todo el mensaje en busca de flags
    while (start < N) {
        // buscar la próxima ocurrencia de "(f" o "(g" o "(b"
        std::size_t pos_f = s.find("(f", start);
        std::size_t pos_g = s.find("(g", start);
        std::size_t pos_b = s.find("(b", start);

        // Determinar cuál de las posiciones es la más cercana
        std::size_t pos = std::string::npos;
        if (pos_f != std::string::npos) pos = pos_f;
        if (pos_g != std::string::npos && (pos == std::string::npos || pos_g < pos)) pos = pos_g;
        if (pos_b != std::string::npos && (pos == std::string::npos || pos_b < pos)) pos = pos_b;

        // Si no se encuentran flags, salimos del bucle
        if (pos == std::string::npos) break;

        // encontrar el cierre de ese paréntesis correspondiente (primer ')' tras pos)
        std::size_t close = s.find(')', pos);
        if (close == std::string::npos) break; // formato raro -> salir

        // nombre entre '(' y ')', por ejemplo "f c" o "f t l 40"
        std::string name = s.substr(pos + 1, close - (pos + 1)); // quitar '('
        trim(name);

        // validar nombre con FLAG_POSITIONS
        if (FLAG_POSITIONS.find(name) != FLAG_POSITIONS.end()) {
            // extraer números que siguen después de close
            // tomamos una ventana corta a partir de close+1 para parsear dist y dir
            std::size_t num_start = close + 1;
            // aumentar la ventana si hace falta, pero limitarla para eficiencia
            std::size_t len = std::min<std::size_t>(N - num_start, 128);
            std::string tail = s.substr(num_start, len); // Extraer la parte del mensaje que contiene los números
            std::istringstream iss(tail); // Convertir la parte extraída a un stream para parsear los números
            double dist = 0.0, dir = 0.0;
            // Si se pudieron leer correctamente los números de distancia y dirección
            if ( (iss >> dist >> dir) ) {
                FlagInfo fi;
                fi.name = name;
                fi.dist = dist;
                fi.dir  = dir;
                fi.visible = true;
                fi.pos = FLAG_POSITIONS.at(name);
                visibleFlags.push_back(fi);
            }
            // si no se pudieron leer números, ignoramos esta ocurrencia
        }

        // avanzar start para seguir buscando (evitar loops infinitos)
        start = close + 1;
    }

    return visibleFlags;
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

// ---------- FUNCIONES DE OPERACIONES ----------

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

Point calcularPosicionJugador(const std::pair<FlagInfo,FlagInfo>& flags)
{
    const auto& f1 = flags.first;
    const auto& f2 = flags.second;

    // Se obtienen las coordenadas absolutas de cada flag desde tu mapa
    Point p1 = FLAG_POSITIONS[f1.name];
    Point p2 = FLAG_POSITIONS[f2.name];

    float x1 = p1.x, y1 = p1.y; // Distancia desde el jugador a la primera flag
    float x2 = p2.x, y2 = p2.y; // Distancia desde el jugador a la segunda flag

    float r1 = f1.dist;
    float r2 = f2.dist;

    double dx = x2 - x1, dy = y2 - y1; // Distancia entre las dos flags
    double d = std::sqrt(dx*dx + dy*dy);

    std::cout << "[DEBUG] Centers: ("<<x1<<","<<y1<<") r1="<<r1
            << " - ("<<x2<<","<<y2<<") r2="<<r2
            << " ; d="<<d
            << " ; r1+r2="<< (r1+r2)
            << " ; |r1-r2|="<< std::fabs(r1-r2) << std::endl;

    // Calcular intersección de las circunferencias
    auto puntos = corteCircunferencias(x1, y1, r1, x2, y2, r2);

    if (puntos.size() == 0) {
        return {0,0};   // Si no hay intersección válida
    }

    // Se elige la que esté dentro del campo
    auto& pA = puntos[0];
    auto& pB = puntos[1];

    bool Ainside = (std::fabs(pA.x) <= 57.5 && std::fabs(pA.y) <= 39); // Verifica si el punto A está dentro del campo
    bool Binside = (std::fabs(pB.x) <= 57.5 && std::fabs(pB.y) <= 39); // Verifica si el punto B está dentro del campo

    if (Ainside && !Binside) return pA; // Si A está dentro y B no, retornamos pA
    if (!Ainside && Binside) return pB; // Si B está dentro y A no, retornamos pB

    return pA;
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

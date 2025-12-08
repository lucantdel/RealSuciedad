#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <MinimalSocket/udp/UdpSocket.h>
#include <chrono>
#include <thread>

// using namespace std;

// ---------- ESTRUCTURAS DE DATOS ----------

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

struct ObjectInfo
{
    double dist{0.0};
    double dir{0.0};
    bool visible{false};
};

std::ostream& operator<<(std::ostream& os, const ObjectInfo& o)
{
    os << "ObjectInfo(dist=" << o.dist
       << ", dir=" << o.dir
       << ", visible=" << std::boolalpha << o.visible << ")";
    return os;
}

struct SeeInfo
{
    int time{0};
    ObjectInfo ball{};
    ObjectInfo own_goal{};
    ObjectInfo opp_goal{};
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

struct SenseInfo 
{
    int time{0};
    double speed{0.0};
    double stamina{0.0};
    double headAngle{0.0};
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

struct PlayerInfo 
{
    std::string team{""};
    Side side{Side::Unknown};
    int number{-1};
    std::string playMode{""};
    SeeInfo see{};
    SenseInfo sense{};
    Point initialPosition{}; 
};

std::ostream& operator<<(std::ostream &os, const PlayerInfo &player) 
{
    os << "Player created (side: " << player.side
       << ", number: " << player.number
       << ", playmode: " << player.playMode
       << ", position: (" << player.initialPosition.x << ", " << player.initialPosition.y << "))";
    return os;
}

// ---------- FUNCIONES AUXILIARES ----------

Point calcKickOffPosition(int unum)
{
    static const Point positions[11] = {
        { -50.0,   0.0 },                                                       // Portero
        { -40.0, -15.0 }, { -42.0,  -5.0 }, { -42.0,   5.0 }, { -40.0,  15.0 }, // Defensa
        { -28.0, -10.0 }, { -22.0,   0.0 }, { -28.0,  10.0 },                   // Medios
        { -15.0, -15.0 }, { -12.0,   0.0 }, { -15.0,  15.0 }                    // Delanteros
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

void skipDelims(std::string_view& sv)
{
    // Salta espacios y paréntesis
    const char* delims = " ()";
    size_t pos = sv.find_first_not_of(delims);
    if (pos == std::string_view::npos) {
        sv = {};
    } else {
        sv.remove_prefix(pos);
    }
}

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

    // Dos tokens numéricos: dist y dir
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

void parseInitMsg(const std::string &msg, PlayerInfo &player)
{
    // (init l 1 before_kick_off)

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

void parseSeeMsg(const std::string &msg, PlayerInfo &player)
{
    // EJEMPLO: (see 0 ... ((g r) 102.5 0) ... ((G) 2.5 180) ... ((b) 49.4 0) ...)

    std::string_view sv = msg;

    auto cmdTok  = nextToken(sv); // "see"
    auto timeTok = nextToken(sv); // tiempo
    player.see.time = std::stoi(std::string(timeTok));

    // Balón
    parseObjectInfo(msg, "(b)", player.see.ball);

    // Porterías
    if (player.side == Side::Left) {
        // Nuestro lado es el izquierdo -> nuestra portería es g l, rival g r
        parseObjectInfo(msg, "(g l)", player.see.own_goal);
        parseObjectInfo(msg, "(g r)", player.see.opp_goal);
    } else if (player.side == Side::Right) {
        // Nuestro lado es el derecho -> nuestra portería es g r, rival g l
        parseObjectInfo(msg, "(g r)", player.see.own_goal);
        parseObjectInfo(msg, "(g l)", player.see.opp_goal);
    } else {
        // Si aún no sabemos el lado, mejor intentar ambas como opp_goal
        parseObjectInfo(msg, "(g l)", player.see.opp_goal);
        parseObjectInfo(msg, "(g r)", player.see.opp_goal);
    }
}

void parseSenseMsg(const std::string &msg, PlayerInfo &player)
{
    // EJEMPLO: (sense_body 0 ... (stamina 8000 1 130600) (speed 0 0) (head_angle 0) ...)

    // Por implementar
}

// ---------- FUNCIONES PARA ENVIAR COMANDOS ----------

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

void sendMoveCommand(PlayerInfo &player, MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp)
{
    std::string move_cmd =
        "(move " + std::to_string(player.initialPosition.x) +
        " "      + std::to_string(player.initialPosition.y) + ")";

    std::cout << "Sending move command: " << move_cmd << std::endl;

    udp_socket.sendTo(move_cmd, server_udp);

    std::cout << "Move command sent" << std::endl;
}

// ---------- FUNCIONES DE DECISIÓN (Think-Act) ----------

std::string decideAction(const PlayerInfo &player)
{
    std::string action_cmd;

        // 1) Si no veo el balón, giro para buscarlo
        if (!player.see.ball.visible) {
            action_cmd = "(turn 30)";
        } else {
            double ball_dist = player.see.ball.dist;
            double ball_dir  = player.see.ball.dir;

            // 2) Balón lejos: orientarse y correr
            if (ball_dist > 1.0) {
                // Si el ángulo es grande, primero giramos
                if (std::abs(ball_dir) > 10.0) {
                    double turnAngle = ball_dir;

                    // Limitar el giro para no pedir algo extremo
                    if (turnAngle > 60.0) turnAngle = 60.0;
                    if (turnAngle < -60.0) turnAngle = -60.0;

                    action_cmd = "(turn " + std::to_string(turnAngle) + ")";
                } else {
                    // Ya más o menos orientados → correr hacia el balón
                    action_cmd = "(dash 90)";
                }
            } else {
                // 3) Balón cerca: chutar
                if (player.see.opp_goal.visible) {
                    double kickDir = player.see.opp_goal.dir;
                    action_cmd = "(kick 90 " + std::to_string(kickDir) + ")";
                } else {
                    // No vemos portería rival: chutar hacia delante
                    // action_cmd = "(kick 80 0)";

                    // Alternativamente, podríamos girar un poco para buscar la portería
                    action_cmd = "(turn 30)";
                }
            }
        }
    return action_cmd;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <team-name> <this-port>" << std::endl;
        return 1;
    }

    std::string team_name = argv[1];
    MinimalSocket::Port this_socket_port = std::stoi(argv[2]);

    std::cout << "Creating a UDP socket on local port " << this_socket_port << std::endl;

    MinimalSocket::udp::Udp<true> udp_socket(
        this_socket_port,
        MinimalSocket::AddressFamily::IP_V4
    );

    std::cout << "Socket created" << std::endl;

    bool success = udp_socket.open();
    if (!success) {
        std::cout << "Error opening socket" << std::endl;
        return 1;
    }

    MinimalSocket::Address server_address{"127.0.0.1", 6000};

    sendInitCommand(udp_socket, server_address, this_socket_port, team_name);

    std::size_t message_max_size = 1000;
    std::cout << "Waiting for an init message from server..." << std::endl;

    auto received_message = udp_socket.receive(message_max_size);

    if (!received_message) {
        std::cerr << "Error receiving message from server" << std::endl;
        return 1;
    }

    std::string received_message_content = received_message->received_message;
    std::cout << "Received message: " << received_message_content << std::endl;

    // Dirección efectiva desde la que el servidor nos ha enviado este mensaje.
    // Normalmente será también 6000, pero usamos la que nos llega para ser
    // más correctos a nivel de red.
    MinimalSocket::Address other_sender_udp = received_message->sender;
    MinimalSocket::Address server_udp{"127.0.0.1", other_sender_udp.getPort()};

    // Creamos el jugador y parseamos el mensaje 'init'
    PlayerInfo player;
    parseInitMsg(received_message_content, player);
    std::cout << player << std::endl;

    // Colocar al jugador en el campo según su dorsal y lado
    sendMoveCommand(player, udp_socket, server_udp);

    // Mantenemos el proceso vivo para poder ver al jugador en el monitor
    // y para que posteriormente puedas añadir el bucle Sense-Think-Act.
    while(true) {
        auto msg = receiveMsgFromServer(udp_socket, message_max_size);

        bool shouldAct = false;

        if (msg.rfind("(see", 0) == 0) {
            parseSeeMsg(msg, player);
            std::cout << "[DEBUG] " << player.see << std::endl;
            shouldAct = true;
        } else if (msg.rfind("(sense_body", 0) == 0) {
            parseSenseMsg(msg, player);
        } 

        if (shouldAct) {
            std::string action_cmd = decideAction(player);
            if (!action_cmd.empty()) {
                std::cout << "Sending command: " << action_cmd << std::endl;
                udp_socket.sendTo(action_cmd + '\0', server_udp);
            }
        }

        // Pequeña pausa para no saturar al servidor
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}

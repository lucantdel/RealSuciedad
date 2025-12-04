#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include <MinimalSocket/udp/UdpSocket.h>
#include <chrono>
#include <thread>

// ---------------------------------------------------------------------------
// Point
// ---------------------------------------------------------------------------
// Representa un punto en el campo en coordenadas (x, y).
// Se usará para guardar la posición inicial deseada del jugador y
// más adelante será útil para el modelo del mundo.
// ---------------------------------------------------------------------------
struct Point {
    double x{0.0};
    double y{0.0};
};

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------
// Representa la información básica que conocemos de nuestro jugador a partir
// del mensaje de inicialización:
//
//   (init l 1 before_kick_off)
//
// Campos:
//   - side: 'l' (left) o 'r' (right), lado del campo asignado.
//   - number: dorsal (uniform number) asignado por el servidor [1..11].
//   - playmode: modo de juego (before_kick_off, play_on, etc.) en el init.
//   - position: posición inicial deseada en el campo (x, y).
// ---------------------------------------------------------------------------

struct ObjectInfo {
    double distance;
    double direction;
    bool seen{false};
    char side; // puede sobrar
};

struct World {
    ObjectInfo ball;
    ObjectInfo goal_l;
    ObjectInfo goal_r;
    // vector<Player> players;
};

struct Player {
    char side{'?'};
    int number{-1};
    string playmode;
    Point position;  // posición inicial que nosotros decidimos para él
    World world;

    // -----------------------------------------------------------------------
    // parse_init_msg
    // -----------------------------------------------------------------------
    // Parsea un mensaje de tipo:
    //
    //   (init l 1 before_kick_off)
    //
    // y rellena los campos 'side', 'number' y 'playmode' del propio Player.
    //
    // Notas:
    //   - Este parser es específico del mensaje 'init', no es genérico
    //     para todas las S-expresiones del servidor.
    // -----------------------------------------------------------------------
    Player& parse_init_msg(const std::string &msg) {
        // Creamos una copia local 's' para poder recortarla sin modificar 'msg'
        std::string s = msg;

        if (s.empty()) {
            std::cerr << "Mensaje vacío en parse_init_msg\n";
            return *this;
        }

        // 1. Limpiamos paréntesis inicial y la parte desde el primer ')'
        //    Ejemplo de entrada: "(init l 1 before_kick_off)"
        if (!s.empty() && s.front() == '(') {
            s.erase(0, 1); // quitamos '('
        }
        auto pos_paren = s.find(')');
        if (pos_paren != std::string::npos) {
            s.erase(pos_paren); // eliminamos desde ')' hasta el final
        }
        // Ahora 's' debería ser: "init l 1 before_kick_off"

        size_t pos;

        // --- PASO 1: Ignorar la palabra "init" ---
        pos = s.find(' ');
        if (pos == std::string::npos) {
            std::cerr << "Formato inesperado (no se encuentra espacio tras 'init')\n";
            return *this;
        }
        s = s.substr(pos + 1);
        // Ahora s: "l 1 before_kick_off"

        // --- PASO 2: Parsear SIDE (l/r) ---
        pos = s.find(' ');
        if (pos == std::string::npos) {
            std::cerr << "Formato inesperado parseando side\n";
            return *this;
        }
        {
            std::string val = s.substr(0, pos);
            if (!val.empty()) {
                this->side = val[0]; // 'l' o 'r'
            }
            s = s.substr(pos + 1);
        }
        // Ahora s: "1 before_kick_off"

        // --- PASO 3: Parsear NUMBER (dorsal) ---
        pos = s.find(' ');
        if (pos == std::string::npos) {
            std::cerr << "Formato inesperado parseando number\n";
            return *this;
        }
        {
            std::string val = s.substr(0, pos);
            this->number = std::stoi(val); // dorsal del jugador
            s = s.substr(pos + 1);
        }
        // Ahora s: "before_kick_off"

        // --- PASO 4: Parsear PLAYMODE ---
        // Lo que queda en 's' es el playmode completo
        this->playmode = s;

        return *this;
    }
};

// Operador para imprimir información del Player de forma legible
std::ostream& operator<<(std::ostream &os, const Player &player) {
    os << "Player created (side: " << player.side
       << ", number: " << player.number
       << ", playmode: " << player.playmode
       << ", position: (" << player.position.x << ", " << player.position.y << "))";
    return os;
}

// ---------------------------------------------------------------------------
// place_initial_position
// ---------------------------------------------------------------------------
// Dado:
//   - Player player  -> contiene 'side' y 'number' correctos.
//   - udp_socket     -> socket UDP para enviar comandos al servidor.
//   - server_udp     -> dirección efectiva del servidor.
//
// Calcula la posición inicial deseada según el dorsal del jugador en una
// formación 4-4-2 y envía un comando:
//
//   (move x y)
//
// para colocar a ese jugador en el campo.
// ---------------------------------------------------------------------------
void place_initial_position(Player &player,
                            MinimalSocket::udp::Udp<true> &udp_socket,
                            const MinimalSocket::Address &server_udp)
{
    Point initial_pos;

    // Asignamos posición en función del dorsal
    switch (player.number) {
        case 1:  initial_pos = {-50,   0};  break;   // Portero
        // Defensa (4)
        case 2:  initial_pos = {-40, -20};  break;   // LD
        case 3:  initial_pos = {-40,  -7};  break;   // DCD
        case 4:  initial_pos = {-40,   7};  break;   // DCI
        case 5:  initial_pos = {-40,  20};  break;   // LI
        // Medios (4)
        case 6:  initial_pos = {-25, -20};  break;   // ED
        case 7:  initial_pos = {-25,  -7};  break;   // MCD
        case 8:  initial_pos = {-25,   7};  break;   // MCI
        case 9:  initial_pos = {-25,  20};  break;   // EI
        // Delanteros (2)
        case 10: initial_pos = {-10, -10};  break;   // DC derecho
        case 11: initial_pos = {-10,  10};  break;   // DC izquierdo
        // Si por algún motivo el dorsal no está entre 1 y 11,
        // lo dejamos en el centro del campo.
        default:
            initial_pos.x = 0.0;
            initial_pos.y = 0.0;
            break;
    }

    // Guardamos la posición calculada en el propio Player
    player.position = initial_pos;

    // Construimos el comando (move x y) como S-Expression
    std::string move_cmd =
        "(move " + std::to_string(player.position.x) +
        " "      + std::to_string(player.position.y) + ")";

    std::cout << "Enviando comando de colocación inicial: " << move_cmd << std::endl;

    // Enviamos el comando al servidor
    udp_socket.sendTo(move_cmd, server_udp);
}

void skip_spaces(std::string_view& in) {
    while (!in.empty() && std::isspace(in.front()))
        in.remove_prefix(1);
}

bool consume_char(std::string_view& in, char c) {
    if (!in.empty() && in.front() == c) {
        in.remove_prefix(1);
        return true;
    }
    return false;
}

bool consume_literal(std::string_view& in, std::string_view lit) {
    if (in.substr(0, lit.size()) == lit) {
        in.remove_prefix(lit.size());
        return true;
    }
    return false;
}

bool parse_number(std::string_view& in, double& out) {
    skip_spaces(in);
    size_t i = 0;

    // detectar parte del número
    while (i < in.size() && (isdigit(in[i]) || in[i]=='+' || in[i]=='-' || in[i]=='.'))
        i++;

    if (i == 0) return false;

    std::string temp(in.substr(0, i));  // copia mínima solo del número
    out = std::stod(temp);
    in.remove_prefix(i);
    return true;
}


void parse_see_msg(std::string_view &msg, World &world){
    if(msg.find("(b)")){
        cout << "Entra" << endl;
        // EJEMPLO: (ball 20 -10))
        double dist, dir;
        consume_literal(msg, "(b) ");
        cout << "Mensaje tras quitar (b)" << msg << endl;
        parse_number(msg, dist);
        skip_spaces(msg);
        parse_number(msg, dir);

        world.ball.distance = dist;
        world.ball.direction = dir;
        world.ball.seen = true;
    } else if(msg.find("(g")) {

    }
}

void handle_msg(std::string_view &msg, Player p) {
    if(msg.find("(see")) {
        // EJEMPLO: (see 10 ((b) 20 -10))
        double time;
        consume_literal(msg, "(see ");
        parse_number(msg, time);
        skip_spaces(msg);
        parse_see_msg(msg, p.world);
    }
    // else if(msg.find("(sense_body"))
    //     parse_sense_msg(msg);
}

// void parse_sense_msg(std::string_view &msg){
//     // EJEMPLO: ()
// }

string decide_action(const Player& player) {
    //¿Veo el balón?
    if (!player.world.ball.seen) {
        return ""; // no hacer nada
    }
    // 2) Si el balón está desviado más de 5°, girar hacia él
    if (abs(player.world.ball.direction) > 5.0) {
        double turn_value = player.world.ball.direction;
        if (turn_value > 60) turn_value = 60;
        if (turn_value < -60) turn_value = -60;
        return "(turn " + to_string(turn_value) + ")";
    }

    // 3) Si estoy cerca del balón (< 0.7 m), chutar
    if (player.world.ball.distance < 0.7) {
        return "(kick 100 0)";
    }

    // 4) En cualquier otro caso, correr hacia él
    return "(dash 80)";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
// Parámetros:
//   argv[1] -> nombre del equipo
//   argv[2] -> puerto local de este proceso (para el socket UDP)
//
// Flujo:
//   1. Crear socket UDP y abrirlo.
//   2. Enviar comando (init ...) al servidor (el primer puerto se marca
//      como (goalie)).
//   3. Recibir la respuesta del servidor y parsear el mensaje 'init'.
//   4. Calcular posición inicial según dorsal y lado, enviando (move x y).
//   5. Mantener el proceso vivo a la espera de entrada para no terminar
//      inmediatamente (posteriormente aquí irá el loop Sense-Think-Act).
// ---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // Comprobamos número de argumentos
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <team-name> <this-port>" << endl;
        return 1;
    }

    // Nombre del equipo y puerto local de este jugador
    string team_name = argv[1];
    MinimalSocket::Port this_socket_port = std::stoi(argv[2]);

    cout << "Creating a UDP socket on local port " << this_socket_port << endl;

    // Creamos el socket UDP:
    //   - true -> bloqueante (por ahora)
    //   - AddressFamily::IP_V4 -> coherente con 127.0.0.1
    MinimalSocket::udp::Udp<true> udp_socket(
        this_socket_port,
        MinimalSocket::AddressFamily::IP_V4
    );

    cout << "Socket created" << endl;

    bool success = udp_socket.open();
    if (!success) {
        cout << "Error opening socket" << endl;
        return 1;
    }

    // Dirección del servidor de RoboCup (rcssserver), puerto 6000 por defecto
    MinimalSocket::Address server_address{"127.0.0.1", 6000};

    // Construimos el mensaje init con el formato correcto:
    //   (init <team_name> (version 19))
    // Si es el primer jugador (por ejemplo puerto 8001), lo marcamos como goalie.
    std::string init_msg;
    if (this_socket_port == 7001 || this_socket_port == 8001) {
        // Portero
        init_msg = "(init " + team_name + " (version 19) (goalie))";
    } else {
        init_msg = "(init " + team_name + " (version 19))";
    }

    cout << "Sending init message: " << init_msg << endl;
    udp_socket.sendTo(init_msg, server_address);
    cout << "Init message sent" << endl;

    // -----------------------------------------------------------------------
    // Recepción del mensaje de init desde el servidor
    // -----------------------------------------------------------------------
    std::size_t message_max_size = 1000;
    cout << "Waiting for an init message from server..." << endl;

    // Recibimos el primer mensaje del servidor.
    // Ejemplo de entrada: "(init l 1 before_kick_off)"
    // NOTA: en un cliente más robusto, aquí haríamos un bucle leyendo
    //       server_param, player_param, etc., hasta encontrar el (init ...).
    auto received_message = udp_socket.receive(message_max_size);

    if (!received_message) {
        cerr << "Error receiving message from server" << endl;
        return 1;
    }

    std::string received_message_content = received_message->received_message;
    cout << "Received message: " << received_message_content << endl;

    // Dirección efectiva desde la que el servidor nos ha enviado este mensaje.
    // Normalmente será también 6000, pero usamos la que nos llega para ser
    // más correctos a nivel de red.
    MinimalSocket::Address other_sender_udp = received_message->sender;
    MinimalSocket::Address server_udp{
        "127.0.0.1",
        other_sender_udp.getPort()
    };

    // Creamos el jugador y parseamos el mensaje 'init'
    Player player;
    player.parse_init_msg(received_message_content);
    cout << player << endl;

    // -----------------------------------------------------------------------
    // Colocar al jugador en el campo según su dorsal y lado
    // -----------------------------------------------------------------------
    place_initial_position(player, udp_socket, server_udp);

    // -----------------------------------------------------------------------
    // Mantenemos el proceso vivo para poder ver al jugador en el monitor
    // y para que posteriormente puedas añadir el bucle Sense-Think-Act.
    // -----------------------------------------------------------------------
    while(true) {
        // SENSE
        // Recibir mensaje
        auto received_message = udp_socket.receive(message_max_size);

        if (!received_message) {
            cerr << "Error receiving message from server" << endl;
            return 1;
        }

        std::string_view received_message_content = received_message->received_message;
        cout << "Received message: " << received_message_content << endl;

        // Manejar mensaje
        handle_msg(received_message_content, player);
        

        // Think
        string action = decide_action(player);
        cout << "Accion pensada: " << action << endl;

        // Act
        udp_socket.sendTo(action, server_udp);
    }

    return 0;
}

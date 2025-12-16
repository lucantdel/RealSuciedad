#include "types.h"
#include "parsers.h"
#include "positions.h"
#include "decisions.h"
#include "net.h"
#include <MinimalSocket/udp/UdpSocket.h>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{
    // Validar argumentos de línea de comandos
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <team-name> <this-port>" << std::endl;
        return 1;
    }

    std::string team_name = argv[1];
    MinimalSocket::Port this_socket_port = std::stoi(argv[2]);

    std::cout << "Creating a UDP socket on local port " << this_socket_port << std::endl;

    // Crear socket UDP para comunicación con el servidor
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

    // Dirección del servidor rcssserver (puerto estándar 6000)
    MinimalSocket::Address server_address{"127.0.0.1", 6000};

    sendInitCommand(udp_socket, server_address, this_socket_port, team_name);

    std::size_t message_max_size = 1000;
    std::cout << "Waiting for an init message from server..." << std::endl;

    // Recibir respuesta del servidor con la asignación de lado y dorsal
    auto received_message = udp_socket.receive(message_max_size);

    if (!received_message) {
        std::cerr << "Error receiving message from server" << std::endl;
        return 1;
    }

    std::string received_message_content = received_message->received_message;
    std::cout << "Received message: " << received_message_content << std::endl;

    // Usar el puerto específico del servidor para las comunicaciones posteriores
    MinimalSocket::Address other_sender_udp = received_message->sender;
    MinimalSocket::Address server_udp{"127.0.0.1", other_sender_udp.getPort()};

    // Parsear el mensaje de inicialización y configurar el jugador
    PlayerInfo player;
    player.team = team_name;

    GameState game_state;

    parseInitMsg(received_message_content, player, game_state);
    std::cout << player << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    sendMoveCommand(udp_socket, server_udp, player);

    // Bucle principal: recibir mensajes del servidor y actuar
    while(true) {
        auto msg = receiveMsgFromServer(udp_socket, message_max_size);

        bool shouldAct = false;

        if (msg.rfind("(see", 0) == 0) {
            std::cout << "Received message: " << msg << std::endl;
            parseSeeMsg(msg, player);
            std::cout << "[DEBUG] " << player.see << std::endl;
            // Obtener las dos mejores banderas para calcular la posición
            auto [flag1, flag2] = getTwoBestFlags(msg);
            std::cout << "Flag1: " << flag1.name << " dist=" << flag1.dist
                << " dir=" << flag1.dir
                << " pos=(" << flag1.pos.x << "," << flag1.pos.y << ")" << std::endl;

            std::cout << "Flag2: " << flag2.name << " dist=" << flag2.dist
                << " dir=" << flag2.dir
                << " pos=(" << flag2.pos.x << "," << flag2.pos.y << ")" << std::endl;

            if (!flag1.name.empty() && !flag2.name.empty()) {

                // Calcular la posición del jugador a partir de las dos banderas
                std::pair<FlagInfo, FlagInfo> flags = {flag1, flag2};
                Point last = {player.x_abs, player.y_abs};

                Point pos = calcularPosicionJugador(flags, last);

                // Actualizar la posición del jugador
                player.x_abs = pos.x;
                player.y_abs = pos.y;

                // Calcular la orientación (dirección) del jugador usando la bandera más cercana
                player.dir_abs = calcularOrientacion(pos, flag1); 

                std::cout << "[INFO] Pos: (" << pos.x << ", " << pos.y 
                          << ") | Dir: " << player.dir_abs << "º" << std::endl;

                // Comprobar si el jugador está dentro de su zona permitida
                Zona z = definirZonaJugador(player);
                if (player.x_abs >= z.x_min && player.x_abs <= z.x_max &&
                    player.y_abs >= z.y_min && player.y_abs <= z.y_max)
                {
                    std::cout << "Jugador " << player.number
                              << " está dentro de su zona permitida.\n";
                }
                else
                {
                    std::cout << "Jugador " << player.number
                              << " está fuera de su zona permitida.\n";
                }
            }
            shouldAct = true;  // Actuar después de recibir información visual
        // } else if (msg.rfind("(sense_body", 0) == 0) {
        //     parseSenseMsg(msg, player);
        //     std::cout << "[DEBUG] " << player.sense << std::endl;
        } else if (msg.rfind("(hear", 0) == 0) {
            std::cout << "Received message: " << msg << std::endl;
            parseHearMsg(msg, player, game_state);
            std::cout << "[DEBUG] " << game_state << std::endl;
        }

        if (shouldAct) {
            std::string action_cmd = decideAction(player, game_state);
            if (!action_cmd.empty()) {
                sendActionCommand(udp_socket, server_udp, action_cmd);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}

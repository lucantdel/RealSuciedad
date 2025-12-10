#pragma once

#include "types.h"
#include <MinimalSocket/udp/UdpSocket.h>

// Recibe un mensaje del servidor a través del socket UDP
std::string receiveMsgFromServer(MinimalSocket::udp::Udp<true> &udp_socket, std::size_t message_max_size);

// Envía un comando al servidor a través del socket UDP añadiendo un carácter nulo al final 
// para evitar "(warning message_not_null_terminated)"
void sendCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, const std::string &cmd);

// Envía el comando de inicialización al servidor
// Los puertos 7001 y 8001 se asignan como porteros
void sendInitCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, MinimalSocket::Port this_socket_port, std::string team_name);

// Envía el comando para activar la recepción de mensajes de audio
void sendEarCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp);

// Envía el comando para posicionar al jugador en su ubicación inicial
void sendMoveCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, PlayerInfo &player);

// Envía el comando de acción decidido al servidor
void sendActionCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, const std::string &action_cmd);
#pragma once

#include <types.h>

std::string receiveMsgFromServer(MinimalSocket::udp::Udp<true> &udp_socket, std::size_t message_max_size);

// Envía el comando de inicialización al servidor
// Los puertos 7001 y 8001 se asignan como porteros
void sendInitCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, MinimalSocket::Port this_socket_port, std::string team_name);

// Envía el comando para activar la recepción de mensajes de audio
void sendEarCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp);

// Envía el comando para posicionar al jugador en su ubicación inicial
void sendMoveCommand(PlayerInfo &player, MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp);
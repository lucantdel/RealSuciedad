#include "net.h"
#include <iostream>

std::string receiveMsgFromServer(MinimalSocket::udp::Udp<true> &udp_socket, std::size_t message_max_size)
{
    auto received_message = udp_socket.receive(message_max_size);

    if (!received_message) {
        std::cerr << "Error receiving message from server" << std::endl;
        return "";
    }

    std::string received_message_content = received_message->received_message;
    // std::cout << "Received message: " << received_message_content << std::endl;

    return received_message_content;
}

void sendCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, const std::string &cmd)
{
    udp_socket.sendTo(cmd + '\0', server_udp);
}

void sendInitCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, MinimalSocket::Port this_socket_port, std::string team_name)
{
    std::string init_msg;

    if (this_socket_port == 7001 || this_socket_port == 8001) {
        init_msg = "(init " + team_name + " (version 19) (goalie))";
    } else {
        init_msg = "(init " + team_name + " (version 19))";
    }

    std::cout << "Sending init message: " << init_msg << std::endl;
    sendCommand(udp_socket, server_udp, init_msg);
    std::cout << "Init message sent" << std::endl;
}

void sendMoveCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, PlayerInfo &player)
{
    std::string move_cmd =
        "(move " + std::to_string(player.initialPosition.x) +
        " "      + std::to_string(player.initialPosition.y) + ")";

    std::cout << "Sending move command: " << move_cmd << std::endl;
    sendCommand(udp_socket, server_udp, move_cmd);
    std::cout << "Move command sent" << std::endl;
}

void sendActionCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp, const std::string &action_cmd) 
{
    std::cout << "Sending action command: " << action_cmd << std::endl;
    sendCommand(udp_socket, server_udp, action_cmd);
    std::cout << "Action command sent" << std::endl;
}
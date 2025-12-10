#include "net.h"

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

void sendEarCommand(MinimalSocket::udp::Udp<true> &udp_socket, const MinimalSocket::Address &server_udp)
{
    std::string hear_msg = "(ear on)";
    
    std::cout << "Sending hear message: " << hear_msg << std::endl;
    udp_socket.sendTo(hear_msg, server_udp);
    std::cout << "Hear message sent" << std::endl;
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
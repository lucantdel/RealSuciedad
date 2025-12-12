#include "decisions.h"
#include <cmath>

std::string playOnDecision(const PlayerInfo &player)
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
            if (player.see.oppGoal.visible) {
                double kickDir = player.see.oppGoal.dir;
                action_cmd = "(kick 90 " + std::to_string(kickDir) + ")";
            } else {
                // Si no ve la portería rival, girar para buscarla
                action_cmd = "(turn 30)";
            }
            // action_cmd = "(kick 90 0)";  // Chutar hacia adelante (para comprobar corners y saques de banda)
        }
    }
    return action_cmd;
}

std::string decideAction(const PlayerInfo &player, const GameState &gameState)
{
    if (gameState.playMode == PlayMode::PlayOn) {
        return playOnDecision(player);
    }
    
    return "";
}
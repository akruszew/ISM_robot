#ifndef ROBOT_APP_H
#define ROBOT_APP_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INIT, IDLE, FOLLOW, LOST, STOPPED, FAULT
} RobotState;

typedef enum {
    EVENT_NONE, EVENT_INIT_OK, EVENT_INIT_KO,
    EVENT_START, EVENT_STOP, EVENT_ACK,
    EVENT_LINE_LOST, EVENT_LINE_FOUND, EVENT_FAULT
} RobotEvent;

/* Démarre INIT. Retour : résultat de l'initialisation matérielle.
   Sur false, le prochain robot_app_run() traite INIT_KO et entre FAULT.
   Appeler robot_app_run() même après un échec, sans autoriser de mouvement. */
bool robot_app_init(uint32_t now_us);

/* Un passage de super-loop ; pas d'attente active. */
void robot_app_run(uint32_t now_us);

/* Lecture seule du mode logique, pas du mouvement physique. */
RobotState robot_app_get_state(void);

/* Ordres reçus dans l'application, pas depuis une ISR dans cet exemple. */
void robot_app_request_start(void);
void robot_app_request_stop(void);
void robot_app_request_ack(void);
void robot_app_signal_fault(void);

#endif

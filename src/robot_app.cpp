#include "robot_app.h"
#include "robot_actions.h"
#include "instrumentation.h"
#include <stddef.h>

/* Types C, mémoire fixe : aucun objet C++ ni allocation dynamique. */
typedef struct {
    bool fault, init_ko, stop, init_ok, ack;
    bool line_lost, line_found, start;
} PendingEvents;

typedef void (*Action)(uint32_t now_us);

static RobotState state = INIT;
static PendingEvents pending_events = {0};
static bool hardware_ready = false;
static bool line_present = false;
static bool line_valid = false;
static bool fault_latched = false;
static bool lost_timer_armed = false;
static uint32_t lost_since_us = 0;
static uint32_t last_speed_us = 0;
static uint32_t last_telemetry_us = 0;

static const uint32_t SPEED_PERIOD_US = 50000UL;
static const uint32_t TELEMETRY_PERIOD_US = 100000UL;
/* Valeur pédagogique à valider sur le robot, pas une exigence de sûreté. */
static const uint32_t LOST_TIMEOUT_US = 5000000UL;

static const uint32_t TEST_DURATION_US = 10000000UL;
static bool instrumentation_ok = true;
static uint32_t start_us;
static bool acquisition_active = false;

/* Le parent RUNNING contient FOLLOW et LOST.
   En LOST, le robot est arrêté : ce n'est pas un test de mouvement. */
static bool is_in_running_state(RobotState s)
{
    return s == FOLLOW || s == LOST;
}

/* Pas de rattrapage : prochaine échéance à partir de l'exécution réelle.
   Préconditions : period_us > 0, pointeur valide, moins d'un tour de compteur. */
static bool period_is_due(uint32_t now_us, uint32_t *last_us,
                          uint32_t period_us)
{
    if ((uint32_t)(now_us - *last_us) < period_us) return false;
    *last_us = now_us;
    return true;
}

static void clear_pending_events(void)
{
    const PendingEvents empty = {0};
    pending_events = empty; /* Copie de structure autorisée en C. */
}

/* Un seul événement pris ; les autres restent en attente, sauf annulations.
   Aucun producteur ISR : sinon protéger les échanges, pas volatile seul. */
static RobotEvent take_event(void)
{
    if (pending_events.fault) {
        clear_pending_events();
        return EVENT_FAULT;
    }
    if (pending_events.init_ko) {
        clear_pending_events();
        return EVENT_INIT_KO;
    }
    if (pending_events.stop) {
        pending_events.stop = false;
        pending_events.start = false;
        pending_events.ack = false; /* Exiger un nouvel ACK après STOP. */
        return EVENT_STOP;
    }
    if (pending_events.init_ok) {
        pending_events.init_ok = false;
        return EVENT_INIT_OK;
    }
    if (pending_events.ack) {
        pending_events.ack = false;
        return EVENT_ACK;
    }
    if (pending_events.line_lost) {
        pending_events.line_lost = false;
        return EVENT_LINE_LOST;
    }
    if (pending_events.line_found) {
        pending_events.line_found = false;
        return EVENT_LINE_FOUND;
    }
    if (pending_events.start) {
        pending_events.start = false;
        return EVENT_START;
    }
    return EVENT_NONE;
}

static void latch_fault(uint32_t now_us)
{
    (void)now_us;
    fault_latched = true;
}

/* Actions du parent : exécutées seulement si on franchit RUNNING. */
static void enter_running(uint32_t now_us)
{
    (void)now_us;
    robot_reset_controllers();
}

static void exit_running(uint32_t now_us)
{
    (void)now_us;
    /* Rien à libérer. state == FOLLOW autorise déjà la commande.
       Ne pas ajouter un control_enabled qui duplique l'état. */
}

static void enter_state(RobotState s, uint32_t now_us)
{
    switch (s) {
    case INIT:
        hardware_ready = robot_hw_init();
        line_present = false;
        line_valid = false;
        if (hardware_ready) {
            fault_latched = false;
            pending_events.init_ok = true;
        } else {
            pending_events.init_ko = true;
        }
        break;
    case IDLE:
    case STOPPED:
        robot_stop_all();
        break;
    case FOLLOW:
        /* La super-loop, pas entry, exécutera la commande périodique. */
        robot_reset_controllers();
        break;
    case LOST:
        robot_stop_all();
        lost_since_us = now_us;
        lost_timer_armed = true;
        break;
    case FAULT:
        fault_latched = true;
        robot_stop_all();
        robot_report_fault(); /* Arrêter d'abord, diagnostiquer ensuite. */
        break;
    }
}

static void exit_state(RobotState s, uint32_t now_us)
{
    (void)now_us;
    if (s == LOST) lost_timer_armed = false;
}

/* Appeler seulement après validation du déclencheur et de la garde.
   next doit être une valeur valide de RobotState.
   Une transition externe réflexive exécute bien exit/entry. */
static void transition_to(RobotState next, Action effect, uint32_t now_us)
{
    exit_state(state, now_us);
    if (is_in_running_state(state) && !is_in_running_state(next)) {
        exit_running(now_us);
    }
    if (effect != NULL) effect(now_us);
    if (!is_in_running_state(state) && is_in_running_state(next)) {
        enter_running(now_us);
    }
    state = next;
    enter_state(state, now_us);
}

static bool lost_timer_expired(uint32_t now_us)
{
    return lost_timer_armed &&
           (uint32_t)(now_us - lost_since_us) >= LOST_TIMEOUT_US;
}

/* Uniquement sur un changement d'observation valide.
   Annuler l'événement opposé devenu périmé : ici, dernière observation retenue. */
static void update_line_observation(bool present)
{
    if (!line_valid || present != line_present) {
        pending_events.line_found = present;
        pending_events.line_lost = !present;
    }
    line_present = present;
    line_valid = true;
}

/* A COMPLETER PAR LES ETUDIANTS.
   La version distribuée ne permet pas le départ ; les actions fournies
   maintiennent les sorties en arrêt.
   Les helpers et les réactions communes sont déjà fournis. */
static void run_statechart(uint32_t now_us)
{
    const RobotEvent event = take_event();

    if ((event == EVENT_FAULT || event == EVENT_INIT_KO) && state != FAULT) {
        transition_to(FAULT, latch_fault, now_us);
        return;
    }
    if (event == EVENT_STOP && is_in_running_state(state)) {
        transition_to(STOPPED, NULL, now_us);
        return;
    }

    switch (state) {
    case INIT:
        if (event == EVENT_INIT_OK) transition_to(IDLE, NULL, now_us);
        break;
    case IDLE:
        /* TODO : START [line_valid && line_present] -> FOLLOW. */
        if(event == EVENT_START && line_present) transition_to(FOLLOW,NULL,now_us);
        break;
    case FOLLOW:
        /* TODO : LINE_LOST -> LOST. */
        break;
    case LOST:
        /* after(T) prioritaire si retour et expiration coïncident. */
        if (lost_timer_expired(now_us)) {
            transition_to(STOPPED, NULL, now_us);
        } else {
            /* TODO : LINE_FOUND [line_valid && line_present] -> FOLLOW. */
        }
        break;
    case STOPPED:
        /* TODO : ACK -> IDLE (un nouveau START sera nécessaire). */
        break;
    case FAULT:
        /* TODO : ACK [robot_fault_resolved()] -> INIT.
           Vider les événements périmés avant la réinitialisation. */
        break;
    }
}

bool robot_app_init(uint32_t now_us)
{
    clear_pending_events();
    state = INIT;
    fault_latched = false;
    lost_timer_armed = false;
    last_speed_us = now_us;
    last_telemetry_us = now_us;
    enter_state(INIT, now_us);
    perf_reset();
    return hardware_ready;
}

RobotState robot_app_get_state(void) { return state; }
void robot_app_request_start(void) { pending_events.start = true; }
void robot_app_request_stop(void) { pending_events.stop = true; }
void robot_app_request_ack(void) { pending_events.ack = true; }
void robot_app_signal_fault(void) { pending_events.fault = true; }

void robot_app_run(uint32_t now_us)
{
    if(!acquisition_active){
        start_us = now_us;
        acquisition_active = true;
    }
    const bool due_speed = period_is_due(now_us, &last_speed_us, SPEED_PERIOD_US);
    if(!perf_tic(PERF_APP)) instrumentation_ok = false;
    if (due_speed && hardware_ready) {
        bool present;
        if(!perf_mark(PERF_SPEED_INTERVAL)) instrumentation_ok = false;
        
        if (!robot_measure_speed(now_us)) robot_app_signal_fault();
        if (robot_read_line(&present)) {
            update_line_observation(present);
        } else {
            line_valid = false;
            robot_app_signal_fault();
        }
    }

    /* STOP / FAULT sont traités avant toute commande de mouvement. */
    run_statechart(now_us);
    if (due_speed && state == FOLLOW) {
        perf_tic(PERF_CONTROL);
        if (!robot_run_control(now_us)) {
            robot_app_signal_fault();
            run_statechart(now_us); /* Réaction immédiate à l'erreur de commande. */
        }
        perf_toc(PERF_CONTROL);
    }

    /* Même en FAULT, la boucle continue pour recevoir les demandes ACK. */
    if (period_is_due(now_us, &last_telemetry_us, TELEMETRY_PERIOD_US)) {
        perf_tic(PERF_TELEMETRY);
        
        /* uncomment to increase the load of the telemetry */
        robot_run_telemetry(now_us);    
        // for (uint16_t i =0; i<10;i++)  robot_run_telemetry(now_us);    
        // for (uint16_t i =0; i<100;i++)  robot_run_telemetry(now_us);    
        perf_toc(PERF_TELEMETRY);
    }

    if(!perf_toc(PERF_APP)) instrumentation_ok = false;
    if(now_us-start_us > TEST_DURATION_US)
    {
        robot_stop_all();
        perf_print_report();
        while(1){
            /* fin de la mesure */
        }
    }
}

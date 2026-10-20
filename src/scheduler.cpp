#include "scheduler.h"
#include "Arduino.h" // pour micros()
#include "robot_app.h"

#define TASK_COUNT 2

Task tasks[TASK_COUNT] = {
    {robot_app_run_control, 50000UL,0,true,0}, //Commande
    {robot_app_run_telemetry, 100000UL,0,true,0} //telemetrie
};

static bool scheduler_ready = false;

bool scheduler_init(uint32_t now_us){
    for (uint8_t i =0;i<TASK_COUNT;i++){
        if(tasks[i].function == NULL) return false;
        if(tasks[i].period_us > 2147483648UL) return false; //2^31
        if(tasks[i].period_us== 0) return false;
        tasks[i].next_release_us = now_us+tasks[i].period_us;
        tasks[i].missed_releases = 0;
    }
    scheduler_ready = true;
    return true;
}

void scheduler_run(TaskFunction service){
    if(!scheduler_ready) return;

    for (uint8_t i =0;i<TASK_COUNT;i++){
        if(service!= NULL) service(micros());
        if(tasks[i].enabled){
            const uint32_t now_us = micros();
            // si elle est due, l'appeler UNE SEULE FOIS avec cette date reelle ;
            if(time_reached(now_us,tasks[i].next_release_us)){
                tasks[i].function(now_us);
                /*relire l'heure apres son retour ; avancer depuis l'ancienne echeance
                 *jusqu'a la premiere echeance STRICTEMENT future a cette heure de fin ; */
                const uint32_t after_us = micros();
                tasks[i].next_release_us+=tasks[i].period_us;
                while(time_reached(after_us,tasks[i].next_release_us)) 
                {
                    tasks[i].next_release_us+=tasks[i].period_us;
                    /* compter les echeances intermediaires abandonnees, sans depasser
                    *      UINT32_MAX. Ne jamais rejouer des commandes en rafale.*/ 
                    if(tasks[i].missed_releases<UINT32_MAX) tasks[i].missed_releases++;
                }
            }
        }

    }
}



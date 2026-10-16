#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Ordonnanceur cooperatif minimal — contrat etudiant
 *
 * Integration :
 *   - placer ce fichier dans include/ ;
 *   - implementer les fonctions dans src/scheduler.cpp (API Arduino) ;
 *   - definir dans ce .cpp une table statique de taches, dans l'ordre :
 *       acquisition-commande :  50000 us ;
 *       telemetrie           : 100000 us.
 *     Les fonctions applicatives raccordees a cette table doivent etre
 *     accessibles depuis scheduler.cpp (pas des fonctions static d'un autre
 *     fichier). Aucun changement des interfaces de robot_actions.h n'est requis.
 *
 * Ce module gere les declenchements, pas les autorisations de mouvement.
 * Le statechart reste dans la couche application, hors de la table periodique.
 * Le comptage des encodeurs reste dans de courtes ISR.
 */

/*
 * now_us : date reelle de debut de la tache, en microsecondes modulo 2^32.
 * Ce n'est PAS son echeance theorique : prevue a 50 ms, demarree a 53 ms,
 * la tache recoit 53000.
 *
 * Une tache retourne sans attente volontaire et termine en un temps maitrise.
 * Dans acquisition-commande, transmettre le meme now_us a la mesure, au
 * statechart et au PI. Cette sequence doit rester courte. La telemetrie utilise
 * uniquement les valeurs memorisees, sans nouveau prelevement des ticks.
 */
typedef void (*TaskFunction)(uint32_t now_us);

typedef struct {
    TaskFunction function;
    uint32_t period_us;       /* 0 < periode < 2^31 us. */
    uint32_t next_release_us; /* Echeance theorique suivante, modulo 2^32. */
    bool enabled;
    uint32_t missed_releases; /* Echeances abandonnees ; compteur saturant. */
} Task;

/*
 * Helper FOURNI : vrai lorsque now_us atteint ou depasse deadline_us.
 * Une comparaison directe now_us >= deadline_us echoue au passage par zero.
 *
 * La soustraction non signee est modulo 2^32 : sous l'hypothese d'un ecart
 * strictement inferieur a une demi-plage, la moitie basse correspond a une
 * echeance atteinte, la moitie haute a une echeance encore future.
 * L'ecart exact de 2^31 est ambigu et exclu du contrat.
 */
static inline bool time_reached(uint32_t now_us, uint32_t deadline_us)
{
    return (uint32_t)(now_us - deadline_us) < 0x80000000UL;
}

/*
 * A IMPLEMENTER — initialise l'ordonnanceur avec la date reelle now_us.
 *
 * Verifier toutes les entrees : fonction non nulle, periode non nulle et
 * strictement inferieure a 2^31 us, meme pour une tache desactivee.
 * Si la table est valide :
 *   - next_release_us = now_us + period_us pour chaque tache ;
 *   - missed_releases = 0 ; conserver les indicateurs enabled ;
 *   - rendre l'ordonnanceur pret et retourner true.
 * En cas d'erreur, retourner false et laisser l'ordonnanceur non pret.
 *
 * Les premiers appels sont donc prevus une periode apres l'initialisation.
 * L'application verifie le retour ; en cas d'echec, elle maintient les moteurs
 * arretes et signale le defaut. Cette fonction ne commande pas les moteurs.
 */
bool scheduler_init(uint32_t now_us);

/*
 * A IMPLEMENTER — un parcours de la table, sans boucle d'attente.
 *
 * Si l'ordonnanceur n'est pas pret : ne rien faire, ni appeler service.
 * Sinon, pour CHAQUE entree du tableau, dans l'ordre :
 *   1. si service != NULL, appeler service avec une date micros() fraiche,
 *      meme si la tache est desactivee ou pas encore due ;
 *   2. si la tache est active, relire micros() puis tester son echeance ;
 *   3. si elle est due, l'appeler UNE SEULE FOIS avec cette date reelle ;
 *   4. relire l'heure apres son retour ; avancer depuis l'ancienne echeance
 *      jusqu'a la premiere echeance STRICTEMENT future a cette heure de fin ;
 *   5. compter les echeances intermediaires abandonnees, sans depasser
 *      UINT32_MAX. Ne jamais rejouer des commandes en rafale.
 *
 * Exemples (periode 50 ms, echeance servie 50 ms) :
 *   fin a  54 ms : prochain appel prevu a 100 ms ; 0 abandon ;
 *   fin a 170 ms : prochain appel prevu a 200 ms ; 2 abandons (100, 150) ;
 *   fin a 100 ms : prochain appel prevu a 150 ms ; 1 abandon (100).
 * Un petit retard de demarrage n'est pas, a lui seul, un abandon.
 *
 * service est un callback applicatif court : recevoir les commandes sans
 * attente et traiter le statechart. Le scheduler n'en connait pas les etats.
 * La tache acquisition-commande traite aussi le statechart apres l'acquisition,
 * avant toute commande, puis immediatement apres une erreur de commande.
 * Elle ne commande qu'en FOLLOW et preserve le calcul du Delta t reel ainsi
 * que la regle existante du premier appel reserve a la datation apres reset.
 *
 * Cette fonction n'est ni reentrante ni preemptive : une tache longue retarde
 * les suivantes et le traitement de STOP/FAULT. Elle ne garantit pas un arret
 * materiel sur.
 */
void scheduler_run(TaskFunction service);

/*
 * FOURNI OU EN EXTENSION — active/desactive une tache par son indice (base 0).
 * Retourne false si l'ordonnanceur n'est pas pret ou si l'indice est invalide.
 *
 * Desactivation : aucun nouvel appel de cette tache.
 * Passage inactive -> active : nouvelle phase, prochaine echeance fixee a
 * micros() + period_us ; pas de rattrapage du temps desactive.
 * Repetition du meme etat : ne pas decaler l'echeance.
 * Conserver missed_releases ; seule l'initialisation remet ce compteur a zero.
 *
 * Appeler entre deux scheduler_run(), jamais depuis une tache, le callback ou
 * une ISR. Activer une tache n'autorise pas le mouvement : le statechart decide.
 */
bool scheduler_set_enabled(size_t index, bool enabled);

/*
 * Hypotheses et restrictions communes :
 *   - scheduler_run() est appele regulierement depuis la boucle principale,
 *     JAMAIS depuis une ISR ; les interruptions restent normalement actives ;
 *   - periodes, retards, et delai de l'echeance servie a la fin de la tache
 *     restent strictement inferieurs a 2^31 us (environ 35,8 min) ;
 *   - ne pas appeler scheduler_run(), scheduler_init() ou modifier la table
 *     pendant un parcours ; ne pas changer les periodes en fonctionnement ;
 *   - pas d'allocation dynamique ni d'affichage dans le coeur du scheduler ;
 *   - mesurer les durees et intervalles avec le module d'instrumentation.
 *
 * Une ISR longue retarde les encodeurs, la base de temps et la liaison serie.
 * Si un timer est utilise, son ISR signale seulement le travail a effectuer ;
 * la boucle principale execute les taches.
 *
 * Validation demandee : avant/a l'echeance, deux taches dues, retard important,
 * date reelle transmise, passage de l'horloge par zero, activation/desactivation
 * si implementee, STOP/FAULT et comparaison instrumentee avant/apres.
 * Commencer avec des fonctions factices et une horloge simulee, moteurs coupes.
 */

#endif /* SCHEDULER_H */

#ifndef INSTRUMENTATION_H
#define INSTRUMENTATION_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ajouter des identifiants avant PERF_COUNT si nécessaire.
 * Employer des identifiants distincts pour les durées et les intervalles. */
typedef enum {
    PERF_APP,
    PERF_CONTROL,
    PERF_TELEMETRY,
    PERF_SPEED_INTERVAL,
    PERF_COUNT
} PerfId;

typedef struct {
    uint32_t count;
    uint32_t last_us;
    uint32_t min_us;
    uint32_t max_us;
    uint64_t total_us;
} PerfStats;

/* Réinitialise les statistiques, les modes et les mesures en cours.
 * À appeler avant un nouvel essai, jamais entre un tic et son toc. */
void perf_reset(void);

/* Démarre une mesure de durée avec micros().
 * false : identifiant invalide, tic déjà actif ou identifiant utilisé par mark.
 * Sur false, aucune modification. */
bool perf_tic(PerfId id);

/* Termine et enregistre la durée depuis tic.
 * false : identifiant invalide, absence de tic ou compteur de mesures plein.
 * Si le compteur est plein, la mesure en cours est terminée mais non ajoutée.
 * Une interruption survenue entre tic et toc fait partie du temps mesuré. */
bool perf_toc(PerfId id);

/* Mesure les intervalles entre deux appels successifs avec le même id.
 * Le premier appel établit seulement la référence et retourne true.
 * false : identifiant invalide, id utilisé par tic/toc ou compteur plein.
 * Si le compteur est plein, la référence temporelle est quand même avancée. */
bool perf_mark(PerfId id);

/* Copie les statistiques ; true uniquement si au moins une mesure existe.
 * false : id invalide, out == NULL ou aucune mesure ; sortie inchangée.
 * Moyenne (us) = (double)total_us / count, uniquement si count > 0. */
bool perf_get_stats(PerfId id, PerfStats *out);

/* Affiche un bilan CSV sur Serial, en microsecondes.
 * Prérequis : Serial.begin(...) effectué par l'application.
 * Appeler après l'arrêt du robot et la fin de l'acquisition des statistiques.
 * Peut bloquer ; ne doit pas être inclus dans les mesures à interpréter. */
void perf_print_report(void);

/* Contrat commun :
 * - appels depuis la boucle principale uniquement, jamais depuis une ISR ;
 * - aucun affichage dans tic, toc, mark, reset ou get_stats ;
 * - pas d'allocation dynamique ; un emplacement statique par identifiant ;
 * - un id est réservé au premier mode choisi jusqu'au prochain perf_reset ;
 * - emboîtement possible entre ids distincts, pas de récursion avec le même id ;
 * - durée réelle entre deux dates < un tour complet de micros()
 *   (environ 71,6 min pour un compteur 32 bits en microsecondes) ;
 * - mesures observées avec le surcoût de l'instrumentation et la résolution
 *   de micros(), pas des garanties de pire cas.
 */

#ifdef __cplusplus
}
#endif

#endif

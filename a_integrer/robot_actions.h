#ifndef ROBOT_ACTIONS_H
#define ROBOT_ACTIONS_H

#include <stdbool.h>
#include <stdint.h>

/* Adaptation aux pilotes : à raccorder à la base matérielle fournie.
   Aucun de ces appels ne doit allouer ni attendre activement.
   Les fichiers de test proposent seulement des simulations. */

/* Initialiser/vérifier les pilotes. Laisser les sorties en arrêt.
   Sur échec : arrêter les moteurs déjà initialisés AVANT de retourner. */
bool robot_hw_init(void);

/* Arrêt des DEUX roues selon la politique matérielle validée.
   Ne pas accéder aux broches d'une instance non initialisée. */
void robot_stop_all(void);

/* Réinitialiser les DEUX états indépendants de correcteur. */
void robot_reset_controllers(void);

/* Diagnostic après l'arrêt ; appelé une seule fois à l'entrée de FAULT.
   Si le port série est utilisé, son blocage éventuel reste à maîtriser. */
void robot_report_fault(void);

/* Tester la cause réelle, pas seulement le booléen de défaut mémorisé. */
bool robot_fault_resolved(void);

/* Mesurer les deux vitesses avec les deltas et le dt réel ; garder les résultats.
   true : deux mesures disponibles ; false : au moins une mesure invalide. */
bool robot_measure_speed(uint32_t now_us);

/* Capteur numérique : convertir LOW en ligne détectée, puis fournir
   la présence validée par la géométrie (au moins une voie dans le TP).
   false : lecture indisponible, sortie inchangée ; 11 n'est pas false API. */
bool robot_read_line(bool *present);

/* Mettre à jour les deux PI et vérifier les deux commandes moteur.
   Utiliser uniquement les mesures déjà mémorisées. Retour false sur erreur. */
bool robot_run_control(uint32_t now_us);

/* Publier les valeurs mémorisées et leur validité, sans prélever les ticks. */
void robot_run_telemetry(uint32_t now_us);

#endif

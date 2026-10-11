#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>

/* Type opaque : représentation privée définie dans motor.cpp. */
typedef struct Motor Motor;

/* Identifiants des deux moteurs du robot. */
typedef enum {
    MOTOR_LEFT,
    MOTOR_RIGHT
} MotorId;

/* Modes d'arrêt ; le freinage doit être validé sur le pont réel. */
typedef enum {
    MOTOR_COAST,  /* Roue libre : pas d'arrêt mécanique immédiat. */
    MOTOR_BRAKE   /* Freinage selon le comportement du pont en H. */
} MotorStopMode;

/**
 * Retourne l'instance correspondant à l'identifiant.
 *
 * Identifiant invalide : NULL.
 * L'instance appartient au pilote et reste valide pendant l'exécution.
 * Aucun accès matériel ni allocation ; ne pas libérer l'instance.
 * Obtenir l'instance ne signifie pas qu'elle est initialisée.
 */
Motor *motor_get(MotorId id);

/**
 * Vérifie la configuration et initialise les sorties en roue libre.
 *
 * Retourne true si l'instance est initialisée à la sortie.
 * Déjà initialisée : true, sans modifier son état ni sa commande.
 * Instance NULL ou configuration invalide : false.
 * initialized n'est positionné qu'après une initialisation réussie.
 *
 * Une réussite ne confirme ni l'alimentation ni le fonctionnement
 * électrique du moteur.
 */
bool motor_init(Motor *motor);

/**
 * Applique une consigne de tension signée, en volts.
 *
 * Précondition : instance obtenue par motor_get() et initialisée.
 * Une tension finie hors plage est saturée à ±9 V : retour true.
 * Une commande nulle place le moteur en roue libre.
 *
 * NaN ou infini : COAST sur l'instance initialisée, puis retour false.
 * Instance NULL ou non initialisée : false, sans accès aux sorties.
 * Mémorise la consigne saturée appliquée ; en COAST, mémorise 0 V.
 *
 * true confirme le traitement de la commande, pas une tension mesurée.
 * Aucun affichage : l'application traite l'erreur et le diagnostic.
 */
bool motor_set_voltage(
    Motor *motor,
    float voltage_v);

/**
 * Demande l'arrêt selon le mode COAST ou BRAKE.
 *
 * Préconditions : instance obtenue par motor_get(), initialisée,
 * et mode valide.
 * Mémorise une consigne appliquée de 0 V.
 * Instance NULL ou non initialisée : aucune action.
 *
 * COAST ne garantit pas un arrêt immédiat ; BRAKE dépend du matériel.
 * Aucun diagnostic ni retour de confirmation électrique.
 */
void motor_stop(
    Motor *motor,
    MotorStopMode mode);

/**
 * Lit la consigne appliquée mémorisée, en volts.
 *
 * Retourne true et écrit dans *voltage_v si l'instance est initialisée.
 * En COAST, 0 V est une valeur valide.
 * Instance NULL, non initialisée ou sortie NULL : false,
 * sans modifier la valeur de sortie.
 *
 * Utiliser la sortie uniquement si le retour vaut true.
 * Aucun effet de bord : ni commande moteur, ni diagnostic.
 * La valeur retournée n'est pas une mesure électrique.
 */
bool motor_get_applied_voltage(
    const Motor *motor,
    float *voltage_v);

#endif /* MOTOR_H */
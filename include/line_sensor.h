
#ifndef LINE_SENSOR_H
#define LINE_SENSOR_H

/**
 * Observation des deux voies du capteur de ligne.
 * Les champs masquent la polarité électrique :
 * true = ligne détectée (GPIO à 0), false = fond blanc (GPIO à 1).
 *
 * Cette observation ne fournit pas directement une erreur de suivi.
 * Son interprétation appartient à l'application.
 */
typedef struct {
    bool left_detected;   /* Ligne détectée par la voie gauche. */
    bool right_detected;  /* Ligne détectée par la voie droite. */
} LineReading;

/**
 * Initialise les deux entrées numériques du capteur de ligne.
 *
 * @return true si l'initialisation logicielle a réussi.
 *         false si la configuration est invalide.
 *
 * Une initialisation répétée retourne true sans modifier l'état.
 * La réussite ne garantit ni le branchement ni le fonctionnement
 * électrique du capteur.
 *
 * À appeler depuis l'application, pas depuis une ISR.
 */
bool line_sensor_init(void);

/**
 * Lit les deux voies et convertit leur niveau électrique :
 * 0 = ligne détectée ; 1 = fond blanc.
 *
 * @param reading Sortie : observation des voies gauche et droite.
 *
 * @return true si la lecture a été effectuée.
 *         false si reading est NULL ou si le pilote n'est pas initialisé.
 *
 * Sur false : la sortie reste inchangée.
 * Sur true  : les deux champs sont renseignés ; utiliser la sortie
 *            uniquement dans ce cas.
 *
 * Deux voies sur fond blanc constituent une observation valide,
 * pas une erreur d'API. L'application décide de la perte de ligne.
 *
 * Les deux GPIO sont lus successivement, pas simultanément.
 * Aucun effet de bord sur les moteurs ou la machine à états.
 * À appeler depuis l'application, pas depuis une ISR.
 */
bool line_sensor_read(LineReading *reading);

#endif
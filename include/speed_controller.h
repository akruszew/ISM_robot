
#ifndef SPEED_CONTROLLER_H
#define SPEED_CONTROLLER_H

typedef struct {
  /*PI Control gains */
  float kp; 
  float ki;

  /* Internal state */
  float integral; 

  /* Control signal limits (for anti-windup) */
  float out_min; 
  float out_max;
} SpeedController;


void speed_controller_init(SpeedController *controller);

/**
 * Calcule la commande et met à jour l'état du correcteur.
 *
 * @param controller  Correcteur propre à une roue, déjà initialisé.
 * @param reference   Consigne signée, en ticks/s.
 * @param measurement Vitesse mesurée signée, en ticks/s.
 * @param dt          Durée réellement écoulée, en secondes.
 * @param voltage_v   Sortie : commande signée et saturée, en V.
 *
 * @return true si le calcul a réussi.
 *         false si un pointeur est NULL, si une entrée ou un paramètre
 *         est invalide, ou si le calcul produit une valeur non finie.
 *
 * Sur false : sortie et état du correcteur inchangés.
 * Sur true  : sortie écrite et état du correcteur mis à jour.
 *
 * Aucun accès matériel. L'application décide de la réaction sur erreur.
 */
bool speed_controller_update(
    SpeedController *controller,
    float reference,
    float measurement,
    float dt,
    float *voltage_v);

#endif
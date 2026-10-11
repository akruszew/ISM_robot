#ifndef ENCODER_H
#define ENCODER_H

#include <stdbool.h>
#include <stdint.h>

/* Type opaque : représentation définie dans encoder.cpp. */
typedef struct Encoder Encoder;

/* Identifiants des deux encodeurs du robot. */
typedef enum {
    ENCODER_LEFT,
    ENCODER_RIGHT
} EncoderId;

/*
 * Retourne l'adresse de l'instance correspondant à id.
 *
 * Identifiant invalide : retourne NULL.
 * N'alloue rien et n'initialise pas le matériel.
 * L'instance appartient au pilote et reste disponible pendant
 * toute l'exécution : ne pas la libérer.
 */
Encoder *encoder_get(EncoderId id);

/*
 * Configure les entrées A/B et l'interruption de l'encodeur.
 * À la première initialisation, met le compteur à zéro.
 *
 * Retourne true si l'initialisation logicielle réussit.
 * Si déjà initialisé : retourne true sans remettre à zéro
 * le compteur ni modifier la configuration.
 *
 * Retourne false si encoder est NULL ou si une erreur de
 * configuration est détectée, notamment une broche A ne
 * permettant pas l'interruption requise.
 *
 * Ne garantit pas le bon fonctionnement électrique du capteur.
 */
bool encoder_init(Encoder *encoder);

/*
 * Lit atomiquement le compteur signé, sans le remettre à zéro.
 * La valeur représente les comptes accumulés depuis la dernière
 * remise à zéro, y compris un prélèvement avec reset.
 *
 * Retourne true et écrit la valeur dans *ticks si l'instance
 * est initialisée et les deux pointeurs sont non NULL.
 *
 * Sinon : retourne false et laisse la sortie inchangée.
 */
bool encoder_get_ticks(
    const Encoder *encoder,
    int32_t *ticks);

/*
 * Prélève l'incrément signé et remet le compteur à zéro.
 * La copie et la remise à zéro forment une seule section atomique.
 *
 * Retourne true et écrit l'incrément dans *delta_ticks si
 * l'instance est initialisée et les deux pointeurs sont non NULL.
 *
 * Sinon : retourne false, sans modifier la sortie ni le compteur.
 *
 * Cette lecture consomme les comptes accumulés :
 * prévoir un seul consommateur par encodeur.
 */
bool encoder_get_and_reset_delta(
    Encoder *encoder,
    int32_t *delta_ticks);

/*
 * Remet atomiquement le compteur à zéro.
 * Les comptes accumulés avant cette opération sont perdus.
 *
 * Retourne true si l'instance est initialisée.
 * Retourne false si encoder est NULL ou non initialisé.
 */
bool encoder_reset(Encoder *encoder);

#endif /* ENCODER_H */
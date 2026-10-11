#include "encoder.h"
#include "config.h"
#include <Arduino.h>
#include <util/atomic.h>

/* Représentation privée : configuration et état par encodeur. */
struct Encoder {
    uint8_t pin_a;
    uint8_t pin_b;
    volatile int32_t ticks;
    bool initialized;
};

/* Instances appartenant au pilote, sans allocation dynamique. */
static Encoder encoders[2] = {
    { ENCODER_LEFT_VA,  ENCODER_LEFT_VB,  0, false },
    { ENCODER_RIGHT_VA, ENCODER_RIGHT_VB, 0, false }
};

/*
 * Compter un front montant de A.
 * La convention de signe doit être vérifiée pour chaque roue.
 */
static void encoder_count_edge(Encoder *encoder)
{
    if (digitalRead(encoder->pin_b) == HIGH) {
        encoder->ticks++;
    } else {
        encoder->ticks--;
    }
}

/* attachInterrupt() attend une fonction sans paramètre. */
static void encoder_left_isr(void)
{
    encoder_count_edge(&encoders[0]);
}

static void encoder_right_isr(void)
{
    encoder_count_edge(&encoders[1]);
}

Encoder *encoder_get(EncoderId id)
{
    switch (id) {
    case ENCODER_LEFT:
        return &encoders[0];

    case ENCODER_RIGHT:
        return &encoders[1];

    default:
        return NULL;
    }
}

bool encoder_init(Encoder *encoder)
{
    if (encoder == NULL) {
        return false;
    }

    /* Déjà initialisé : conserver le compteur et la configuration. */
    if (encoder->initialized) {
        return true;
    }

    /* Associer l'instance à son ISR, sans dupliquer l'initialisation. */
    void (*isr)(void);

    if (encoder == &encoders[0]) {
        isr = encoder_left_isr;
    } else if (encoder == &encoders[1]) {
        isr = encoder_right_isr;
    } else {
        return false;
    }

    /* Vérifier les broches avant tout accès matériel. */
    if (encoder->pin_a >= NUM_DIGITAL_PINS ||
        encoder->pin_b >= NUM_DIGITAL_PINS ||
        encoder->pin_a == encoder->pin_b) {
        return false;
    }

    const int interrupt_number =
        digitalPinToInterrupt(encoder->pin_a);

    if (interrupt_number == NOT_AN_INTERRUPT) {
        return false;
    }

    pinMode(encoder->pin_a, INPUT_PULLUP);
    pinMode(encoder->pin_b, INPUT_PULLUP);

    /*
     * Mettre le compteur à zéro et installer l'ISR sans laisser
     * une interruption s'intercaler pendant l'initialisation.
     */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        encoder->ticks = 0;

        attachInterrupt(
            interrupt_number,
            isr,
            RISING);

        /* En dernier, après configuration du matériel. */
        encoder->initialized = true;
    }

    return true;
}

bool encoder_get_ticks(
    const Encoder *encoder,
    int32_t *ticks)
{
    if (encoder == NULL || ticks == NULL) {
        return false;
    }

    if (!encoder->initialized) {
        return false;
    }

    int32_t result;

    /* Lecture cohérente d'un entier 32 bits sur AVR 8 bits. */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        result = encoder->ticks;
    }

    *ticks = result;
    return true;
}

bool encoder_get_and_reset_delta(
    Encoder *encoder,
    int32_t *delta_ticks)
{
    if (encoder == NULL || delta_ticks == NULL) {
        return false;
    }

    if (!encoder->initialized) {
        return false;
    }

    int32_t result;

    /* Copie et remise à zéro : une seule section atomique. */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        result = encoder->ticks;
        encoder->ticks = 0;
    }

    *delta_ticks = result;
    return true;
}

bool encoder_reset(Encoder *encoder)
{
    if (encoder == NULL || !encoder->initialized) {
        return false;
    }

    /* Les comptes accumulés sont volontairement abandonnés. */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        encoder->ticks = 0;
    }

    return true;
}
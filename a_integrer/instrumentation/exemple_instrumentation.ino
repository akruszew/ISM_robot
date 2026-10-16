/* Exemple autonome de l'API, sans moteur ni capteur.
 * Copier instrumentation.h et instrumentation.cpp dans le dossier du sketch.
 * Ouvrir le moniteur série à 115200 bauds.
 * La charge ci-dessous est SYNTHETIQUE : elle ne remplace pas les mesures
 * du programme robot. Aucun delay() n'est utilisé pendant l'essai.
 */
#include "instrumentation.h"

static bool acquisition_active = false;
static uint32_t start_us = 0;
static uint32_t last_release_us = 0;
static volatile uint32_t result = 0;
static bool instrumentation_ok = true;

static const uint32_t TEST_DURATION_US = 5000000UL;
static const uint32_t CONTROL_PERIOD_US = 50000UL;

static void workload(void)
{
    /* Remplacer par le traitement à observer, en respectant sa sécurité. */
    for (uint16_t i = 0; i < 128; ++i) {
        result = result * 33UL + (uint32_t)i;
    }
}

void setup(void)
{
    Serial.begin(115200);
    perf_reset();
    start_us = (uint32_t)micros();
    last_release_us = start_us;
    acquisition_active = true;
}

void loop(void)
{
    if (!acquisition_active) return;

    const uint32_t now_us = (uint32_t)micros();
    if ((uint32_t)(now_us - start_us) >= TEST_DURATION_US) {
        acquisition_active = false;
        /* Sur le robot : arrêter les deux moteurs AVANT ce bilan,
           et ne pas sortir du service des commandes de sécurité. */
        Serial.println(F("Bilan apres 5 s ; toutes les durees sont en us."));
        Serial.print(F("Instrumentation valide : "));
        Serial.println(instrumentation_ok ? F("oui") : F("non"));
        perf_print_report();
        return;
    }

    const bool app_started = perf_tic(PERF_APP);
    if (!app_started) instrumentation_ok = false;

    if ((uint32_t)(now_us - last_release_us) >= CONTROL_PERIOD_US) {
        last_release_us = now_us; /* Pas de rafale de rattrapage. */

        if (!perf_mark(PERF_SPEED_INTERVAL)) instrumentation_ok = false;
        const bool control_started = perf_tic(PERF_CONTROL);
        if (!control_started) instrumentation_ok = false;
        workload();
        if (control_started && !perf_toc(PERF_CONTROL)) {
            instrumentation_ok = false;
        }
    }

    if (app_started && !perf_toc(PERF_APP)) instrumentation_ok = false;
}

#include "instrumentation.h"
#include <Arduino.h>
#include <stddef.h>

typedef enum {
    PERF_MODE_NONE,
    PERF_MODE_DURATION,
    PERF_MODE_INTERVAL
} PerfMode;

typedef struct {
    PerfStats stats;
    PerfMode mode;
    uint32_t reference_us;
    bool active;
} PerfSlot;

static PerfSlot slots[PERF_COUNT] = {0};

static bool valid_id(PerfId id)
{
    return (unsigned int)id < (unsigned int)PERF_COUNT;
}

static bool record_sample(PerfSlot *slot, uint32_t value_us)
{
    PerfStats *stats = &slot->stats;

    if (stats->count == UINT32_MAX) {
        return false;
    }

    if (stats->count == 0) {
        stats->min_us = value_us;
        stats->max_us = value_us;
    } else {
        if (value_us < stats->min_us) stats->min_us = value_us;
        if (value_us > stats->max_us) stats->max_us = value_us;
    }

    stats->last_us = value_us;
    stats->total_us += (uint64_t)value_us;
    stats->count++;
    /* Le total tient sur 64 bits : au plus UINT32_MAX échantillons,
       chacun inférieur ou égal à UINT32_MAX microsecondes. */
    return true;
}

void perf_reset(void)
{
    for (unsigned int i = 0; i < (unsigned int)PERF_COUNT; ++i) {
        const PerfSlot empty = {0};
        slots[i] = empty;
    }
}

bool perf_tic(PerfId id)
{
    if (!valid_id(id)) return false;

    PerfSlot *slot = &slots[(unsigned int)id];
    if (slot->mode == PERF_MODE_INTERVAL || slot->active) return false;

    slot->mode = PERF_MODE_DURATION;
    slot->reference_us = (uint32_t)micros();
    slot->active = true;
    return true;
}

bool perf_toc(PerfId id)
{
    if (!valid_id(id)) return false;

    PerfSlot *slot = &slots[(unsigned int)id];
    if (slot->mode != PERF_MODE_DURATION || !slot->active) return false;

    const uint32_t now_us = (uint32_t)micros();
    const uint32_t elapsed_us = now_us - slot->reference_us;
    slot->active = false;
    return record_sample(slot, elapsed_us);
}

bool perf_mark(PerfId id)
{
    if (!valid_id(id)) return false;

    PerfSlot *slot = &slots[(unsigned int)id];
    if (slot->mode == PERF_MODE_DURATION) return false;

    const uint32_t now_us = (uint32_t)micros();
    if (slot->mode == PERF_MODE_NONE) {
        slot->mode = PERF_MODE_INTERVAL;
        slot->reference_us = now_us;
        return true;
    }

    const uint32_t elapsed_us = now_us - slot->reference_us;
    slot->reference_us = now_us;
    return record_sample(slot, elapsed_us);
}

bool perf_get_stats(PerfId id, PerfStats *out)
{
    if (!valid_id(id) || out == NULL) return false;

    const PerfSlot *slot = &slots[(unsigned int)id];
    if (slot->stats.count == 0) return false;

    *out = slot->stats;
    return true;
}

static const char *label_for(unsigned int id)
{
    switch ((PerfId)id) {
    case PERF_APP:            return "APP";
    case PERF_CONTROL:        return "CONTROL";
    case PERF_TELEMETRY:      return "TELEMETRY";
    case PERF_SPEED_INTERVAL: return "SPEED_INTERVAL";
    default:                 return "CUSTOM";
    }
}

void perf_print_report(void)
{
    Serial.println(F("id,label,type,count,last_us,min_us,max_us,mean_us"));

    for (unsigned int i = 0; i < (unsigned int)PERF_COUNT; ++i) {
        const PerfSlot *slot = &slots[i];
        if (slot->mode == PERF_MODE_NONE) continue;

        Serial.print(i);
        Serial.print(',');
        Serial.print(label_for(i));
        Serial.print(',');
        if (slot->mode == PERF_MODE_DURATION) {
            Serial.print(F("duration"));
        } else {
            Serial.print(F("interval"));
        }
        Serial.print(',');
        Serial.print((unsigned long)slot->stats.count);
        Serial.print(',');

        if (slot->stats.count == 0) {
            Serial.println(F("NA,NA,NA,NA"));
            continue;
        }

        Serial.print((unsigned long)slot->stats.last_us);
        Serial.print(',');
        Serial.print((unsigned long)slot->stats.min_us);
        Serial.print(',');
        Serial.print((unsigned long)slot->stats.max_us);
        Serial.print(',');
        const double mean_us = (double)slot->stats.total_us
                             / (double)slot->stats.count;
        Serial.println(mean_us, 2);
    }
}

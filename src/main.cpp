#include <Arduino.h>
#include "motor.h"
#include "encoder.h"

/* static in global scope means private to this module */
static const uint32_t SPEED_PERIOD_US = 50000;
static const uint32_t TELEMETRY_PERIOD_US =100000;

static Motor *motor_left;
static Motor *motor_right;

struct WheelSpeed
{
  Encoder *encoder;
  uint32_t previous_sample_us;
  float speed_ticks_s;
  bool valid;
};

static WheelSpeed left_speed = {NULL, 0, 0.0f, false};
static WheelSpeed right_speed = {NULL, 0, 0.0f, false};

static uint32_t last_speed_us = 0;
static uint32_t last_telemetry_us = 0;
static bool ready = false;

/* Prélèvement des ticks et calcul avec la durée réellement écoulée. */
static void update_speed(WheelSpeed *wheel, const uint32_t now_us)
{
  const uint32_t dt_us = now_us - wheel->previous_sample_us;

  if (dt_us == 0)
  {
    wheel->valid = false;
    return;
  }

  int32_t delta_ticks;

  if (!encoder_get_and_reset_delta(wheel->encoder, &delta_ticks))
  {
    wheel->valid = false;
    return;
  }

  /* Le calcul reste hors de la section atomique du pilote. */
  wheel->speed_ticks_s =
      (float)delta_ticks * 1000000.0f / (float)dt_us;

  wheel->previous_sample_us = now_us;
  wheel->valid = true;
}

void run_speed(const uint32_t now_us)
{
  update_speed(&left_speed, now_us);
  update_speed(&right_speed, now_us);
}

/* Publier une valeur uniquement si la mesure est valide. */
static void print_speed(const WheelSpeed *wheel)
{
  if (wheel->valid)
  {
    Serial.print(wheel->speed_ticks_s, 1);
  }
  else
  {
    Serial.print("NA");
  }
}

static void print_voltage(const Motor *motor)
{
  float voltage_v;
  if (motor_get_applied_voltage(motor, &voltage_v))
  {
    Serial.print(voltage_v);
  }
  else
  {
    Serial.print("NA");
  }
}

static void run_telemetry(void) // static = fonction privée du module
{
  Serial.print(millis());
  Serial.print(',');

  print_voltage(motor_left);
  Serial.print(',');

  print_voltage(motor_right);
  Serial.print(',');

  print_speed(&left_speed);
  Serial.print(',');

  print_speed(&right_speed);
  Serial.println();
}

static bool period_is_due(
    uint32_t now_us,
    uint32_t *last_release_us,
    uint32_t period_us)
{
  if ((uint32_t)(now_us - *last_release_us) < period_us)
    return false;

  // pas de rafale de rattrapage
  *last_release_us = now_us;
  return true;
}

float update_voltage_sequence(uint32_t time_us)
{
  if (time_us < 1000000UL)
  {
    return 0.0f;
  }

  if (time_us < 2000000UL)
  {
    return 5.0f;
  }

  return 0.0f;
}

static void stop_on_fault(const char *message)
{
  /* Arrêter les moteurs initialisés avant tout affichage. */
  motor_stop(motor_left, MOTOR_COAST);
  motor_stop(motor_right, MOTOR_COAST);

  ready = false;
  Serial.println(message);
}

static bool run_voltage_sequence(uint32_t time_us)
{
  const float voltage_v = update_voltage_sequence(time_us);

  /* Évaluer les deux appels, sans court-circuit logique. */
  const bool left_ok =
      motor_set_voltage(motor_left, voltage_v);

  const bool right_ok =
      motor_set_voltage(motor_right, -voltage_v);

  return left_ok && right_ok;
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello");
  motor_left = motor_get(MOTOR_LEFT);
  motor_right = motor_get(MOTOR_RIGHT);
  const bool motor_left_ok = motor_init(motor_left);
  const bool motor_right_ok = motor_init(motor_right);

  if (!motor_left_ok || !motor_right_ok)
  {
    stop_on_fault("Erreur : initialisation des moteurs");
    return;
  }

  left_speed.encoder = encoder_get(ENCODER_LEFT);
  right_speed.encoder = encoder_get(ENCODER_RIGHT);
  const bool left_ok = encoder_init(left_speed.encoder);
  const bool right_ok = encoder_init(right_speed.encoder);

  if (!left_ok || !right_ok)
  {

    stop_on_fault("Erreur : initialisation des encodeurs");
    return;
  }

  /* Commencer les fenêtres de mesure après l'initialisation. */
  const bool left_reset_ok = encoder_reset(left_speed.encoder);
  const bool right_reset_ok = encoder_reset(right_speed.encoder);

  if (!left_reset_ok || !right_reset_ok)
  {
    stop_on_fault("Erreur : reset des encodeurs");
    return;
  }

  const uint32_t start_us = micros();

  left_speed.previous_sample_us = start_us;
  right_speed.previous_sample_us = start_us;
  last_speed_us = start_us;
  last_telemetry_us = start_us;

  // Telemetry
  Serial.println("temps_ms,consigne_gauche_V,consigne_droite_V,"
                 "vitesse_gauche_ticks_s,vitesse_droite_ticks_s");
  ready = true;
}

void loop()
{
  if (!ready)
  {
    return;
  }

  // put your main code here, to run repeatedly:
  const uint32_t now_us = micros();

  if (period_is_due(now_us, &last_speed_us, SPEED_PERIOD_US))
  {
    run_speed(now_us);
    if (!left_speed.valid || !right_speed.valid)
    {
      stop_on_fault("Erreur : mesure de vitesse");
      return;
    }

    if (!run_voltage_sequence(now_us))
    {
      stop_on_fault("Erreur : commande moteur");
      return;
    }
  }

  if (period_is_due(now_us, &last_telemetry_us, TELEMETRY_PERIOD_US))
  {
    run_telemetry();
  }
}

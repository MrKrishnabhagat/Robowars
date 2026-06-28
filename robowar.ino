// ================== RECEIVER INPUT ==================
#define CH2_PIN 34   // Turn (Aileron)
#define CH1_PIN 35   // Throttle
#define CH3_PIN 32   // Weapon speed
#define CH4_PIN 13   // Invert controls (e.g., SWB or SWC)
#define CH5_PIN 33   // Weapon kill (SWA)
#define CH6_PIN 27   // Drive kill (SWD)

// ================== OUTPUT ==========================
#define LEFT_MOTOR 25
#define RIGHT_MOTOR 26
#define WEAPON_ESC 14

// ================== PWM SETUP =======================
#define PWM_FREQ 50
#define PWM_RES 16

// ================== VARIABLES =======================
int ch1, ch2, ch3, ch4, ch5, ch6;

// ================== READ CHANNEL ====================
int readChannel(int pin) {
  int val = pulseIn(pin, HIGH, 25000);
  if (val < 900 || val > 2100) return 1500; // failsafe center
  return val;
}

// ================== WRITE PPM =======================
void writePPM(uint8_t pin, int pulseWidth) {
  uint32_t duty = map(pulseWidth, 1000, 2000, 3276, 6553);
  ledcWrite(pin, duty);
}

void setup() {
  Serial.begin(115200);

  pinMode(CH1_PIN, INPUT);
  pinMode(CH2_PIN, INPUT);
  pinMode(CH3_PIN, INPUT);
  pinMode(CH4_PIN, INPUT);
  pinMode(CH5_PIN, INPUT);
  pinMode(CH6_PIN, INPUT);

  ledcAttach(LEFT_MOTOR, PWM_FREQ, PWM_RES);
  ledcAttach(RIGHT_MOTOR, PWM_FREQ, PWM_RES);
  ledcAttach(WEAPON_ESC, PWM_FREQ, PWM_RES);

  // ===== SAFE START =====
  writePPM(LEFT_MOTOR, 1500);   // stop drive
  writePPM(RIGHT_MOTOR, 1500);
  writePPM(WEAPON_ESC, 1000);   // weapon OFF

  delay(3000); // ESC arming time
}

void loop() {
  // ===== READ RECEIVER =====
  ch1 = readChannel(CH1_PIN);
  ch2 = readChannel(CH2_PIN);
  ch3 = readChannel(CH3_PIN);
  ch4 = readChannel(CH4_PIN);
  ch5 = readChannel(CH5_PIN);
  ch6 = readChannel(CH6_PIN);

  // ===== SWITCHES =====
  bool invertMode = (ch4 > 1500); // True when switch is flipped
  bool weaponKill = (ch5 > 1500);
  bool driveKill  = (ch6 > 1500);

  // =====================================================
  // 🚗 DRIVE CONTROL (TANK MIXING)
  // =====================================================
  if (driveKill) {
    writePPM(LEFT_MOTOR, 1500);
    writePPM(RIGHT_MOTOR, 1500);
  } else {
    int throttle = map(ch2, 1000, 2000, 500, -500);
    int turn     = map(ch1, 1000, 2000, -500, 500);

    // --- INVERT LOGIC ---
    if (invertMode) {
      throttle = -throttle; // Forward becomes back, back becomes forward
      // Turn remains untouched! The physical flip auto-corrects steering.
    }

    // Deadband to prevent motor whine when sticks are centered
    if (abs(throttle) < 40) throttle = 0;
    if (abs(turn) < 40) turn = 0;

    int left  = throttle + turn;
    int right = throttle - turn;

    left  = constrain(left, -500, 500);
    right = constrain(right, -500, 500);

    // Scale to 90% to allow steering overhead at full throttle
    left  *= 0.9;
    right *= 0.9;

    int leftPPM  = map(left,  -500, 500, 1000, 2000);
    int rightPPM = map(right, -500, 500, 1000, 2000);

    writePPM(LEFT_MOTOR, leftPPM);
    writePPM(RIGHT_MOTOR, rightPPM);
  }

  // =====================================================
  // 🔪 WEAPON CONTROL (ESC)
  // =====================================================
  if (weaponKill) {
    writePPM(WEAPON_ESC, 1000);
  } else {
    int weaponPPM = map(ch3, 1000, 2000, 1000, 2000);
    if (weaponPPM < 1050) weaponPPM = 1000;
    writePPM(WEAPON_ESC, weaponPPM);
  }

  // ===== DEBUG =====
  Serial.print("DriveKill: "); Serial.print(driveKill);
  Serial.print(" | WeaponKill: "); Serial.print(weaponKill);
  Serial.print(" | Invert: "); Serial.print(invertMode);
  Serial.print(" | Throttle: "); Serial.print(ch2);
  Serial.print(" | Turn: "); Serial.print(ch1);
  Serial.print(" | Weapon: "); Serial.println(ch3);
}

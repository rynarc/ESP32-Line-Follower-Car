/*!
 * MindPlus
 * firebeetleesp32e
 *
 * Line Follower + Tanjakan Kayu
 * Flow:
 *   0. Tunggu tombol
 *   1. Maju lurus 1500ms (start) dengan speed line follower
 *   2. Ikuti garis
 *   3. Garis hilang -> berhenti 1 detik
 *   4. Maju lurus (FORWARD_MS) abaikan garis
 *   5. Ikuti garis lagi sampai hilang -> berhenti (selesai)
 */
#include <line_tracker.h>
LineTracker line_tracker;
#include <five_line_tracker_v3.h>
emakefun::FiveLineTracker line_tracker_v3;

// ============================================================
// Global Variables
// ============================================================
volatile float bw_threshold;
volatile float last_direction;

int programState = 0;

const int LOST_CONFIRM = 8;
int lost_count = 0;

// Lama maju lurus melewati tanjakan (tanpa peduli garis), dalam milidetik.
const unsigned long FORWARD_MS = 1200;
unsigned long forward_start_time = 0;

// Lama maju lurus di AWAL sebelum mulai ikut garis, dalam milidetik.
const unsigned long START_FORWARD_MS = 500;
unsigned long start_forward_time = 0;

// ============================================================
// Function Declarations
// ============================================================
void LineFollow(float speed, float adjust_speed);
void Forward(float speed);
void StopAllMotors();
bool ReadSensors(bool &s0, bool &s1, bool &s2, bool &s3, bool &s4);
bool LineDetected();

// ============================================================
// Setup
// ============================================================
void setup() {
	pinMode(D5, INPUT);
	Wire.begin();
	line_tracker_v3.Initialize();
	Serial.begin(9600);

	bw_threshold = 500;
	last_direction = 0;
	lost_count = 0;
	programState = 0;
}

// ============================================================
// Loop
// ============================================================
void loop() {
	switch (programState) {

		// ---------- State 0: tunggu tombol untuk mulai ----------
		case 0:
			if ((digitalRead(D5)) == 0) {
				delay(300);
				lost_count = 0;
				start_forward_time = millis();
				programState = 5;          // ke maju lurus awal dulu
			}
			break;

		// ---------- State 5: maju lurus 1500ms di awal ----------
		case 5:
			if (millis() - start_forward_time < START_FORWARD_MS) {
				Forward(150);              // maju lurus, speed sama seperti line follower
			} else {
				programState = 1;          // selesai -> mulai ikut garis
			}
			break;

		// ---------- State 1: ikuti garis sampai hilang ----------
		case 1: {
			if (!LineDetected()) {
				lost_count++;
				if (lost_count >= LOST_CONFIRM) {
					// Garis hilang -> berhenti 1 detik, lalu maju
					StopAllMotors();
					delay(1000);
					lost_count = 0;
					forward_start_time = millis();
					programState = 2;
					break;
				}
			} else {
				lost_count = 0;
			}

			LineFollow(150, 125);
			break;
		}

		// ---------- State 2: maju lurus (abaikan garis) ----------
		case 2:
			if (millis() - forward_start_time < FORWARD_MS) {
				Forward(130);
			} else {
				lost_count = 0;
				programState = 3;
			}
			break;

		// ---------- State 3: ikuti garis lagi sampai hilang -> berhenti ----------
		case 3: {
			if (!LineDetected()) {
				lost_count++;
				if (lost_count >= LOST_CONFIRM) {
					// Garis hilang -> selesai, berhenti
					StopAllMotors();
					programState = 4;
					break;
				}
			} else {
				lost_count = 0;
			}

			LineFollow(150, 125);
			break;
		}

		// ---------- State 4: selesai (diam) ----------
		case 4:
			StopAllMotors();
			break;
	}
}

// ============================================================
// ReadSensors - baca 5 sensor sekaligus
// ============================================================
bool ReadSensors(bool &s0, bool &s1, bool &s2, bool &s3, bool &s4) {
	s0 = line_tracker_v3.AnalogValue(0) > bw_threshold;
	s1 = line_tracker_v3.AnalogValue(1) > bw_threshold;
	s2 = line_tracker_v3.AnalogValue(2) > bw_threshold;
	s3 = line_tracker_v3.AnalogValue(3) > bw_threshold;
	s4 = line_tracker_v3.AnalogValue(4) > bw_threshold;
	return (s0 || s1 || s2 || s3 || s4);
}

// ============================================================
// LineDetected - true kalau minimal 1 sensor mengenai garis
// ============================================================
bool LineDetected() {
	bool s0, s1, s2, s3, s4;
	return ReadSensors(s0, s1, s2, s3, s4);
}

// ============================================================
// LineFollow - Core line following algorithm (dengan belokan tajam)
// ============================================================
void LineFollow(float speed, float adjust_speed) {
	bool s0, s1, s2, s3, s4;
	ReadSensors(s0, s1, s2, s3, s4);

	if (s2 && !s1 && !s3) {                 // tengah -> lurus
		line_tracker.setMotorSpeed(1, (speed * -1));
		line_tracker.setMotorSpeed(2, (speed * -1));
		last_direction = 0;
	}
	else if (s2 && s3 && !s1) {             // sedikit ke kanan
		line_tracker.setMotorSpeed(1, (adjust_speed * -1));
		line_tracker.setMotorSpeed(2, (speed * -1));
		last_direction = 1;
	}
	else if (s2 && s1 && !s3) {             // sedikit ke kiri
		line_tracker.setMotorSpeed(1, (speed * -1));
		line_tracker.setMotorSpeed(2, (adjust_speed * -1));
		last_direction = 2;
	}
	else if (s3 && !s2 && !s1) {            // koreksi kanan
		line_tracker.setMotorSpeed(1, 0);
		line_tracker.setMotorSpeed(2, (speed * -1));
		last_direction = 1;
	}
	else if (s1 && !s2 && !s3) {            // koreksi kiri
		line_tracker.setMotorSpeed(1, (speed * -1));
		line_tracker.setMotorSpeed(2, 0);
		last_direction = 2;
	}
	else if (s4) {                          // belok tajam kanan (putar balik kanan)
		line_tracker.setMotorSpeed(1, speed);
		line_tracker.setMotorSpeed(2, (speed * -1));
		last_direction = 1;
	}
	else if (s0) {                          // belok tajam kiri (putar balik kiri)
		line_tracker.setMotorSpeed(1, (speed * -1));
		line_tracker.setMotorSpeed(2, speed);
		last_direction = 2;
	}
	else if (s1 && s2 && s3) {              // garis lebar -> lurus
		line_tracker.setMotorSpeed(1, (speed * -1));
		line_tracker.setMotorSpeed(2, (speed * -1));
	}
	else {
		// Tidak ada sensor di garis (sesaat) -> teruskan ke arah terakhir
		if (last_direction == 1) {          // terakhir belok kanan
			line_tracker.setMotorSpeed(1, speed);
			line_tracker.setMotorSpeed(2, (speed * -1));
		}
		else if (last_direction == 2) {     // terakhir belok kiri
			line_tracker.setMotorSpeed(1, (speed * -1));
			line_tracker.setMotorSpeed(2, speed);
		}
		else {                              // terakhir lurus
			line_tracker.setMotorSpeed(1, (speed * -1));
			line_tracker.setMotorSpeed(2, (speed * -1));
		}
	}
}

// ============================================================
// Forward - maju lurus (untuk lewat tanjakan / cari garis)
// ============================================================
void Forward(float speed) {
	line_tracker.setMotorSpeed(1, (speed * -1));
	line_tracker.setMotorSpeed(2, (speed * -1));
}

// ============================================================
// StopAllMotors
// ============================================================
void StopAllMotors() {
	line_tracker.setMotorBrake(1);
	line_tracker.setMotorBrake(2);
	delay(10);
}
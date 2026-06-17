/*!
 * MindPlus / firebeetleesp32e
 * Line Follower - PD control + motor trim + bobot sensor diperbaiki
 * s0 = kanan ... s4 = kiri ; garis hitam = nilai tinggi
 * Bobot disesuaikan jarak fisik sensor (s0/s4 lebih jauh dari tetangganya)
 */
#include <line_tracker.h>
LineTracker line_tracker;
#include <five_line_tracker_v3.h>
emakefun::FiveLineTracker line_tracker_v3;

float bw_threshold  = 500;
float base_speed    = 1000;
float Kp            = 330;
float Kd            = 750;
float motor2_trim   = 1.04;

float last_error    = 0;
int   lost_count    = 0;
int   lost_limit    = 30;

int programState = 0;

void StopAllMotors() {
	line_tracker.setMotorBrake(1);
	line_tracker.setMotorBrake(2);
}

void setup() {
	pinMode(D5, INPUT);
	Wire.begin();
	line_tracker_v3.Initialize();
	Serial.begin(9600);
	programState = 0;
}

void loop() {
	switch (programState) {

		case 0:
			if (digitalRead(D5) == 0) {
				delay(300);
				programState = 1;
			}
			break;

		case 1: {
			bool s0 = line_tracker_v3.AnalogValue(0) > bw_threshold; // kanan ujung
			bool s1 = line_tracker_v3.AnalogValue(1) > bw_threshold;
			bool s2 = line_tracker_v3.AnalogValue(2) > bw_threshold; // tengah
			bool s3 = line_tracker_v3.AnalogValue(3) > bw_threshold;
			bool s4 = line_tracker_v3.AnalogValue(4) > bw_threshold; // kiri ujung

			int count = s0 + s1 + s2 + s3 + s4;

			float error;

			if (count == 0) {
				lost_count++;
				if (lost_count >= lost_limit) {
					StopAllMotors();
					programState = 2;
					break;
				}
				error = last_error;
			} else {
				lost_count = 0;
				float weighted = (s0 * 3) + (s1 * 2) + (s2 * 0) + (s3 * -2) + (s4 * -3);
				error = weighted / count;
			}

			// PD control
			float derivative = error - last_error;
			float turn       = (Kp * error) + (Kd * derivative);
			last_error       = error;

			// Trim hanya pada base_speed motor kanan
			float left  = base_speed + turn;
			float right = (base_speed * motor2_trim) - turn;

			// maju = negatif (tidak ada clamp, motor support sampai 3000)
			line_tracker.setMotorSpeed(1, left  * -1);
			line_tracker.setMotorSpeed(2, right * -1);
			break;
		}

		case 2:
			break;
	}
}
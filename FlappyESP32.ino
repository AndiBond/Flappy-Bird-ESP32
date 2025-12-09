/*   Flappy ESP32 — MIT License (Extended)
   Author: AndiBond
   License: MIT with Disclaimer of Liability

   You are allowed to use, copy, modify, and distribute this software freely
   under the following conditions:

   1. The entire source code must retain this header with license information.
   2. The software is provided "as is", without any warranties, 
      express or implied, including but not limited to warranties 
      of merchantability, fitness for a particular purpose, and non-infringement.
   3. Neither the author nor the copyright holders shall be liable for any 
      direct, indirect, incidental, special, punitive, or consequential 
      damages arising from the use of this software or inability to use it.

   Use of this code is at your own risk.
*/

#include <SH1106Wire.h>
#include <Preferences.h>

#define BUTTON_PIN      23
#define BOOT_BUTTON_PIN 0
#define BUZZER_PIN      18

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64

#define TUBE_DISTANCE   32
#define TUBE_WIDTH      6
#define PATH_WIDTH      30

// SH1106: SDA = 21, SCL = 22
SH1106Wire display(0x3c, 21, 22);
Preferences preferences;

// Game state
float tubeX[4];
int bottomTubeHeight[4];
bool hasScored[4];

unsigned int score = 0;
unsigned int highScore = 0;
unsigned int gameState = 0;  // 0 - start, 1 - play, 2 - score

bool isFlyingUp = false;
bool isBuzzerOn = false;

float birdX = 20.0;
float birdY = 28.0;
float speed = 0.6;

unsigned long keyPressTime = 0;

const int BIRD_W = 8;
const int BIRD_H = 8;

// ------------------------------------------------------

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  preferences.begin("Flappy", false);
  highScore = preferences.getUInt("highScore", 0);
  preferences.end();

  display.init();
  display.flipScreenVertically();  // как и в SSD1306 версии

  for (int i = 0; i < 4; i++) {
    tubeX[i] = SCREEN_WIDTH + i * TUBE_DISTANCE;
    bottomTubeHeight[i] = random(8, SCREEN_HEIGHT - PATH_WIDTH - 8);
    hasScored[i] = false;
  }

  randomSeed(analogRead(34));
}

// ------------------------------------------------------

void loop() {
  display.clear();

  // -------------------- START SCREEN --------------------
  if (gameState == 0) {
    birdY = 28.0;
    score = 0;
    speed = 0.6;

    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "Flappy ver 1.0");

    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 44, "Press to start");
    display.drawString(0, 54, "BOOT: reset score");

    for (int i = 0; i < 4; i++) {
      tubeX[i] = SCREEN_WIDTH + (i + 1) * TUBE_DISTANCE;
      bottomTubeHeight[i] = random(8, SCREEN_HEIGHT - PATH_WIDTH - 8);
      hasScored[i] = false;
    }

    if (digitalRead(BUTTON_PIN) == LOW) {
      gameState = 1;
      delay(80);
      keyPressTime = millis();
      isFlyingUp = true;
      isBuzzerOn = true;
    }
  }

  // -------------------- GAMEPLAY --------------------
  else if (gameState == 1) {
    display.setFont(ArialMT_Plain_10);
    display.drawString(3, 0, String(score));

    if (digitalRead(BUTTON_PIN) == LOW) {
      keyPressTime = millis();
      isFlyingUp = true;
      isBuzzerOn = true;
    }

    display.fillRect((int)birdX, (int)birdY, BIRD_W, BIRD_H);

    for (int i = 0; i < 4; i++) {
      display.fillRect((int)tubeX[i], 0, TUBE_WIDTH, bottomTubeHeight[i]);
      display.fillRect((int)tubeX[i], bottomTubeHeight[i] + PATH_WIDTH,
                       TUBE_WIDTH,
                       SCREEN_HEIGHT - bottomTubeHeight[i] - PATH_WIDTH);
    }

    for (int i = 0; i < 4; i++) {
      tubeX[i] -= speed;

      if (tubeX[i] + TUBE_WIDTH < 0) {
        tubeX[i] = SCREEN_WIDTH;
        bottomTubeHeight[i] = random(8, SCREEN_HEIGHT - PATH_WIDTH - 8);
        hasScored[i] = false;
      }

      if (!hasScored[i] && tubeX[i] + TUBE_WIDTH < birdX) {
        score++;
        hasScored[i] = true;

        if (score % 10 == 0) speed += 0.15;
      }
    }

    if (millis() - keyPressTime > 80) isFlyingUp = false;
    if (millis() - keyPressTime > 12) isBuzzerOn = false;

    if (isFlyingUp) birdY -= 1.0;
    else birdY += 1.0;

    digitalWrite(BUZZER_PIN, isBuzzerOn ? HIGH : LOW);

    if (birdY < 0 || birdY + BIRD_H > SCREEN_HEIGHT) endGameSequence();

    for (int i = 0; i < 4; i++) {
      int tx1 = (int)tubeX[i];
      int tx2 = tx1 + TUBE_WIDTH;
      int bx1 = (int)birdX;
      int bx2 = bx1 + BIRD_W;

      if (!(bx2 < tx1 || bx1 > tx2)) {
        if (birdY < bottomTubeHeight[i] ||
            birdY + BIRD_H > bottomTubeHeight[i] + PATH_WIDTH) {
          endGameSequence();
        }
      }
    }

    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  }

  // -------------------- SCORE SCREEN --------------------
  else {
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "Your score: " + String(score));
    display.drawString(0, 20, "High score: " + String(highScore));

    display.setFont(ArialMT_Plain_10);
    display.drawString(32, 44, "Press to restart");
    display.drawString(5, 54, "BOOT: reset score");

    if (digitalRead(BUTTON_PIN) == LOW) {
      gameState = 0;
      delay(200);
    }

    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
      highScore = 0;
      preferences.begin("Flappy", false);
      preferences.putUInt("highScore", highScore);
      preferences.end();
      delay(200);
    }
  }

  display.display();
  delay(20);
}

// ------------------------------------------------------

void endGameSequence() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(120);
    digitalWrite(BUZZER_PIN, LOW);  delay(80);
  }

  if (score > highScore) {
    highScore = score;
    preferences.begin("Flappy", false);
    preferences.putUInt("highScore", highScore);
    preferences.end();
  }

  gameState = 2;
}
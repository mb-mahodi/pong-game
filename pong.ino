/*
  Enhanced Pong Game with 4-Button Controls
  Requires Adafruit_SSD1306 and Adafruit_GFX libraries
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Pin Definitions ---
#define UP_BUTTON     2
#define DOWN_BUTTON   3
#define PAUSE_BUTTON  4
#define SPEED_BUTTON  5
#define BUZZER_PIN    11

// --- OLED Setup ---
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Game Constants & Variables ---
const uint8_t PADDLE_HEIGHT = 12;
const uint8_t SCORE_LIMIT   = 9;

const uint8_t MCU_X    = 8;
const uint8_t PLAYER_X = 118;

const unsigned long PADDLE_RATE_NORMAL = 40;
const unsigned long PADDLE_RATE_TURBO  = 20;
unsigned long paddle_rate = PADDLE_RATE_NORMAL;

const unsigned long INITIAL_BALL_RATE = 20;
const unsigned long MIN_BALL_RATE     = 8;
unsigned long current_ball_rate        = INITIAL_BALL_RATE;

unsigned long ball_update;
unsigned long paddle_update;

bool game_over = false;
bool win       = false;
bool is_paused = false;
bool turbo_mode = false;

uint8_t player_score = 0;
uint8_t mcu_score    = 0;

int16_t ball_x = 64, ball_y = 26;
int8_t  ball_dir_x = 1, ball_dir_y = 1;

uint8_t mcu_y    = 20;
uint8_t player_y = 20;

bool last_pause_state = HIGH;
bool last_speed_state = HIGH;

void playerPaddleTone() { tone(BUZZER_PIN, 300, 20); }
void mcuPaddleTone()    { tone(BUZZER_PIN, 250, 20); }
void wallTone()         { tone(BUZZER_PIN, 180, 15); }
void player_scoreTone() { tone(BUZZER_PIN, 500, 100); }
void mcu_scoreTone()    { tone(BUZZER_PIN, 120, 100); }

void drawCourt() {
    display.drawRect(0, 0, 128, 54, WHITE);
    for (uint8_t i = 2; i < 52; i += 4) {
        display.drawFastVLine(64, i, 2, WHITE);
    }
}

void resetBall() {
    ball_x = 64;
    ball_y = 26;
    ball_dir_x = (ball_dir_x > 0) ? -1 : 1;
    ball_dir_y = (millis() % 2 == 0) ? 1 : -1;
    current_ball_rate = INITIAL_BALL_RATE;
}

void setup() {
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    
    pinMode(UP_BUTTON, INPUT_PULLUP);
    pinMode(DOWN_BUTTON, INPUT_PULLUP);
    pinMode(PAUSE_BUTTON, INPUT_PULLUP);
    pinMode(SPEED_BUTTON, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(25, 25);
    display.print(F("PONG DELUXE"));
    display.display();
    delay(1500);

    display.clearDisplay();
    drawCourt();
    display.display();

    ball_update = millis();
    paddle_update = ball_update;
}

void loop() {
    unsigned long time = millis();

    bool pause_btn = digitalRead(PAUSE_BUTTON);
    if (pause_btn == LOW && last_pause_state == HIGH) {
        is_paused = !is_paused;
        delay(50);
    }
    last_pause_state = pause_btn;

    bool speed_btn = digitalRead(SPEED_BUTTON);
    if (speed_btn == LOW && last_speed_state == HIGH) {
        turbo_mode = !turbo_mode;
        paddle_rate = turbo_mode ? PADDLE_RATE_TURBO : PADDLE_RATE_NORMAL;
        delay(50);
    }
    last_speed_state = speed_btn;

    if (is_paused) {
        display.fillRect(40, 22, 48, 15, BLACK);
        display.drawRect(40, 22, 48, 15, WHITE);
        display.setCursor(47, 26);
        display.print(F("PAUSE"));
        display.display();
        return;
    }

    bool update_needed = false;

    if (time > ball_update) {
        int16_t new_x = ball_x + ball_dir_x;
        int16_t new_y = ball_y + ball_dir_y;

        if (new_y <= 1 || new_y >= 52) {
            wallTone();
            ball_dir_y = -ball_dir_y;
            new_y += ball_dir_y;
        }

        if (new_x <= MCU_X + 1 && ball_x > MCU_X && new_y >= mcu_y && new_y <= mcu_y + PADDLE_HEIGHT) {
            mcuPaddleTone();
            ball_dir_x = 1;
            int8_t hit_pos = new_y - (mcu_y + PADDLE_HEIGHT / 2);
            if (hit_pos < 0) ball_dir_y = -1;
            else if (hit_pos > 0) ball_dir_y = 1;

            if (current_ball_rate > MIN_BALL_RATE) current_ball_rate--;
            new_x = MCU_X + 2;
        }

        if (new_x >= PLAYER_X - 1 && ball_x < PLAYER_X && new_y >= player_y && new_y <= player_y + PADDLE_HEIGHT) {
            playerPaddleTone();
            ball_dir_x = -1;
            int8_t hit_pos = new_y - (player_y + PADDLE_HEIGHT / 2);
            if (hit_pos < 0) ball_dir_y = -1;
            else if (hit_pos > 0) ball_dir_y = 1;

            if (current_ball_rate > MIN_BALL_RATE) current_ball_rate--;
            new_x = PLAYER_X - 2;
        }

        if (new_x <= 0) {
            player_scoreTone();
            player_score++;
            resetBall();
            new_x = ball_x;
            new_y = ball_y;
        } else if (new_x >= 127) {
            mcu_scoreTone();
            mcu_score++;
            resetBall();
            new_x = ball_x;
            new_y = ball_y;
        }

        if (player_score >= SCORE_LIMIT || mcu_score >= SCORE_LIMIT) {
            win = (player_score > mcu_score);
            game_over = true;
        }

        display.drawPixel(ball_x, ball_y, BLACK);
        display.drawPixel(new_x, new_y, WHITE);
        ball_x = new_x;
        ball_y = new_y;

        ball_update += current_ball_rate;
        update_needed = true;
    }

    if (time > paddle_update) {
        paddle_update += paddle_rate;

        display.drawFastVLine(MCU_X, mcu_y, PADDLE_HEIGHT, BLACK);
        display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, BLACK);

        uint8_t half_paddle = PADDLE_HEIGHT / 2;
        if (mcu_y + half_paddle < ball_y && mcu_y + PADDLE_HEIGHT < 53) {
            mcu_y++;
        } else if (mcu_y + half_paddle > ball_y && mcu_y > 1) {
            mcu_y--;
        }

        if (digitalRead(UP_BUTTON) == LOW && player_y > 1) {
            player_y--;
        }
        if (digitalRead(DOWN_BUTTON) == LOW && (player_y + PADDLE_HEIGHT) < 53) {
            player_y++;
        }

        display.drawFastVLine(MCU_X, mcu_y, PADDLE_HEIGHT, WHITE);
        display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, WHITE);

        update_needed = true;
    }

    if (update_needed) {
        drawCourt();

        if (game_over) {
            display.clearDisplay();
            display.setCursor(35, 25);
            display.print(win ? F("YOU WIN!!") : F("GAME OVER"));
            display.display();
            delay(3000);

            player_score = 0;
            mcu_score = 0;
            game_over = false;
            resetBall();
            display.clearDisplay();
            drawCourt();
        }

        display.setTextColor(WHITE, BLACK);
        display.setCursor(20, 55);
        display.print(F("CPU:"));
        display.print(mcu_score);

        display.setCursor(75, 55);
        display.print(F("YOU:"));
        display.print(player_score);

        if (turbo_mode) {
            display.setCursor(58, 55);
            display.print(F("T"));
        } else {
            display.setCursor(58, 55);
            display.print(F(" "));
        }

        display.display();
    }
}
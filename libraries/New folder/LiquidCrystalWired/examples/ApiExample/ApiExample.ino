/*
 * Copyright (C) 2020 Hendrik van Essen
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "Arduino.h"
#include "LiquidCrystalWired.h"

#define LCD_ADDRESS (0x7c >> 1)

#define ROW_COUNT   2
#define COL_COUNT   16

byte customCharHeart[] = {
        0b00000,
        0b00000,
        0b01010,
        0b11111,
        0b11111,
        0b01110,
        0b00100,
        0b00000,
};

LiquidCrystalWired lcd = LiquidCrystalWired(
        ROW_COUNT, COL_COUNT, FONT_SIZE_5x8, BITMODE_8_BIT);

void setup() {
    Serial.begin(115200);
    lcd.begin(LCD_ADDRESS, &Wire);

    lcd.turnOn();
    lcd.setCustomSymbol(CUSTOM_SYMBOL_1, customCharHeart);
}

void loop() {

    // 1. custom symbol
    lcd.print("Hello world! ");
    lcd.printCustomSymbol(CUSTOM_SYMBOL_1);
    delay(1000);

    // 2.a scroll right with text insertion following LEFT_TO_RIGHT
    lcd.clear();
    lcd.setTextInsertionMode(LEFT_TO_RIGHT);
    lcd.print("scroll right");
    delay(500);

    for (int i = 0; i < 4; i++) {
        lcd.scrollDisplayRight();
        delay(500);
    }

    // 2.b scroll left with text insertion following RIGHT_TO_LEFT
    lcd.clear();
    lcd.setTextInsertionMode(RIGHT_TO_LEFT);
    lcd.setCursorPosition(0, COL_COUNT - 1);
    lcd.print("tfel llorcs");
    delay(500);

    for (int i = 0; i < 5; i++) {
        lcd.scrollDisplayLeft();
        delay(500);
    }
    lcd.setTextInsertionMode(LEFT_TO_RIGHT);

    // 3. turning on/off display
    lcd.clear();
    lcd.print("turning off...");
    delay(1000);
    lcd.turnOff();
    delay(1000);

    lcd.clear();
    lcd.print("turned on again");
    lcd.turnOn();
    delay(1000);

    // 4. autoscroll
    lcd.clear();

    if (ROW_COUNT > 1) {
        lcd.setCursorPosition(1, 0);
        lcd.print("This is also a very long line");
    }

    lcd.setCursorPosition(0, COL_COUNT);
    lcd.setAutoScrollEnabled(true);

    String longLine = "This is a very long line";
    for (int i = 0; i < longLine.length(); i++) {
        lcd.print(longLine.charAt(i));
        delay(250);
    }
    lcd.setAutoScrollEnabled(false);
    delay(2000);

    // 5. return home
    lcd.returnHome();
    delay(1000);

    // 6. cursor blinking
    lcd.clear();
    lcd.setCursorBlinkingEnabled(true);
    delay(2000);

    for (int i = 0; i < COL_COUNT; i++) {

        if (i == (int) (COL_COUNT / 2)) {
            lcd.setCursorBlinkingEnabled(false);
        }

        if (i == (int) (COL_COUNT / 2) + 2) {
            lcd.setCursorBlinkingEnabled(true);
        }

        lcd.setCursorPosition(0, i);
        delay(500);
    }
    delay(1000);
    lcd.returnHome();
    delay(500);
    lcd.setCursorBlinkingEnabled(false);

    // 7. show and move cursor
    lcd.setCursorVisible(true);
    lcd.clear();
    delay(1000);
    for (int i = 0; i <= 9; i++) {
        lcd.print(i);
        delay(250);
    }

    lcd.returnHome();
    delay(100);
    for(int i = 0; i < 50; i++) {
        lcd.moveCursorRight();
        delay(100);
    }
    for(int i = 50; i > 0 ; i--) {
        lcd.moveCursorLeft();
        delay(100);
    }

    lcd.setCursorVisible(false);
    lcd.clear();

    /* 8. write 10 characters on the next line (if available), because 40
     * characters per line is the maximum */
    if (ROW_COUNT > 1) {
        lcd.clear();
        lcd.setTextInsertionMode(LEFT_TO_RIGHT);
        for (int i = 0; i < 50; i++) {
            if (i % 2 == 0) {
                lcd.print("A");
            }
            else {
                lcd.print("B");
            }

            delay(100);
        }

        lcd.clear();
        lcd.setTextInsertionMode(RIGHT_TO_LEFT);
        lcd.setCursorPosition(0, COL_COUNT - 1);
        for (int i = 0; i < 50; i++) {
            if (i % 2 == 0) {
                lcd.print("X");
            }
            else {
                lcd.print("Y");
            }

            delay(100);
        }
    }

    lcd.clear();

    // 9. progress bar
    lcd.setProgressBarEnabled(true);
    lcd.setCursorPosition(0, 0);
    lcd.print("Progress: ");

    for (int j = 0; j <= 100; j++) {

        lcd.setCursorPosition(0, 10);

        #ifdef __AVR__
            char string_rep[6];
            sprintf(string_rep, "%d %%", j);
            lcd.print(string_rep);
        #else
            // on an esp32 you can use printf instead
            lcd.printf("%d %%", j);
        #endif

        lcd.setProgress(j);
        delay(100);
    }
    lcd.setProgressBarEnabled(false);
    lcd.clear();
}

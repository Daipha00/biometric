/*
 * Copyright (C) 2020 Hendrik van Essen
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef LIQUID_CRYSTAL_WIRED_H
#define LIQUID_CRYSTAL_WIRED_H

#include <Wire.h>

// Commands (page 18, table 3)

/**
 * Clears entire display and sets cursor to position (0, 0).
 * ATTENTION: Also changes BIT_ENTRY_MODE_INCREMENT to 1
 */
#define CMD_CLEAR_DISPLAY   0x01

/**
 * Sets cursor to position (0, 0) and resets display to original position
 * before any shift operations.
 * .
 */
#define CMD_RETURN_HOME 0x02

/**
 * Sets cursor move direction and specifies display shift.
 */
#define CMD_ENTRY_MODE_SET  0x04

/**
 * Sets entire display on/off, cursor on/off, and blinking of cursor position
 * character on/off.
 */
#define CMD_DISPLAY_CONTROL 0x08

/**
 * Moves cursor and shifts display.
 */
#define CMD_CURSOR_DISPLAY_SHIFT    0x10

/**
 * Sets interface data length, number of display lines and character font size.
 */
#define CMD_FUNCTION_SET    0x20

/**
 * Sets CGRAM address.
 */
#define CMD_SET_CGRAM_ADDR  0x40

/**
 * Sets DDRAM address.
 */
#define CMD_SET_DDRAM_ADDR  0x80



// Bits for CMD_ENTRY_MODE_SET (page 16, section 3)

/**
 * 0 = Decrement cursor after insertion
 * 1 = Increment cursor after insertion
 */
#define BIT_ENTRY_MODE_INCREMENT    1

/**
 * 0 = No automated display scroll
 * 1 = Automated display scroll
 */
#define BIT_ENTRY_MODE_AUTOINCREMENT    0



// Bits for CMD_DISPLAY_CONTROL (page 16, section 4)

/**
 * 0 = Display off
 * 1 = Display on
 */
#define BIT_DISPLAY_CONTROL_DISPLAY 2

/**
 * 0 = Cursor off
 * 1 = Cursor on
 */
#define BIT_DISPLAY_CONTROL_CURSOR  1

/**
 * 0 = Cursor blinking off
 * 1 = Cursor blinking on
 */
#define BIT_DISPLAY_CONTROL_CURSOR_BLINKING 0



// Bits for CMD_CURSOR_DISPLAY_SHIFT (page 17, section 5)

/**
 * 0 = Shift the cursor position
 * 1 = Scroll the display content
 */
#define BIT_CURSOR_DISPLAY_SHIFT_SELECTION  3

/**
 * 0 = Shift to the left
 * 1 = Shift to the right
 */
#define BIT_CURSOR_DISPLAY_SHIFT_DIRECTION  2



// Bits for CMD_FUNCTION_SET (page 17, section 6)

/**
 * 0 = 4 bit interface data length
 * 1 = 8 bit interface data length
 */
#define BIT_FUNCTION_SET_BITMODE    4

/**
 * 0 = Single line
 * 1 = Two lines
 */
#define BIT_FUNCTION_SET_LINECOUNT  3

/**
 * 0 = 5x8 dots per character
 * 1 = 5x10 dots per character
 */
#define BIT_FUNCTION_SET_FONTSIZE   2



// Bits for constrol byte (page 12)

/**
 * @brief 0 = Last control byte
 *        1 = Another control byte follows data byte
 */
#define BIT_CONTROL_BYTE_CO 7

/**
 * @brief 0 = data byte interpreted as command
 *        1 = data byte interpreted as data
 */
#define BIT_CONTROL_BYTE_RS 6



/**
 * Keys for custom symbols.
 */
enum CustomSymbol {
    CUSTOM_SYMBOL_1 = 0,
    CUSTOM_SYMBOL_2 = 1,
    CUSTOM_SYMBOL_3 = 2,
    CUSTOM_SYMBOL_4 = 3,
    CUSTOM_SYMBOL_5 = 4,
    CUSTOM_SYMBOL_6 = 5,
    CUSTOM_SYMBOL_7 = 6,
    CUSTOM_SYMBOL_8 = 7
};

/**
 * Defines the direction of the text insertion. Starting from the cursor, either
 * increment the column of the cursor position after insertion (LEFT_TO_RIGHT),
 * or decrement the current column of the cursor position after insertion
 * (RIGHT_TO_LEFT).
 */
enum TextInsertionMode { LEFT_TO_RIGHT, RIGHT_TO_LEFT };

/**
 * Size of a character of the display in dots/pixels.
 */
enum FontSize { FONT_SIZE_5x8, FONT_SIZE_5x10 };

/**
 * Bit mode for the display.
 */
enum BitMode { BITMODE_4_BIT, BITMODE_8_BIT };

class LiquidCrystalWired : public Print {

public:

    /**
     * Constructor for LiquidCrystalWired.
     *
     * @param rowCount  Number of rows of the connected display
     * @param colCount  Number of columns of the connected display
     * @param fontSize  Size of a character of the connected display
     * @param bitMode   Bitmode of the connected display
     */
    LiquidCrystalWired(
            uint8_t rowCount, uint8_t colCount,
            FontSize fontSize, BitMode bitMode);

    /**
     * Initialization.
     *
     * @param deviceAddress I2C address of the used display controller
     * @param wire          Reference to TwoWire for I2C communication
     */
    void begin(uint8_t deviceAddress, TwoWire *wire);

    /**
     * Turn on the display.
     */
    void turnOn();

    /**
     * Turn off the display.
     */
    void turnOff();

    /**
     * Clear the display and set the cursor to position (0, 0).
     * ATTENTION: Also changes to setTextInsertionMode(LEFT_TO_RIGHT)
     */
    void clear();

    /**
     * Reset cursor position to (0, 0) and scroll display to original position.
     */
    void returnHome();

    /**
     * Enable or disable automated scrolling.
     *
     * @param enabled   Enable or disable
     */
    void setAutoScrollEnabled(bool enabled);

    /**
     * Enable or disable cursor blinking.
     *
     * @param enabled   Enable or disable
     */
    void setCursorBlinkingEnabled(bool enabled);

    /**
     * Show or hide the cursor.
     *
     * @param visible   Show or hide
     */
    void setCursorVisible(bool visible);

    /**
     * Move the cursor to a given position.
     *
     * @param row   Row of the new cursor position (starting at 0)
     * @param col   Column of the new cursor position (starting at 0)
     */
    void setCursorPosition(uint8_t row, uint8_t col);

    /**
     * Set the direction from which the text is inserted, starting from the cursor.
     *
     * @param mode  Insertion mode
     */
    void setTextInsertionMode(TextInsertionMode mode);

    /**
     * Move the cursor one unit to the left. When the cursor passes the 40th
     * character of the first line and a second line is available, the cursor
     * will move to the second line.
     */
    void moveCursorLeft();

    /**
     * Move the cursor one unit to the right. When the cursor passes the 40th
     * character of the first line and a second line is available, the cursor
     * will move to the second line.
     *
     * NOTE: The cursor respects the setting for the insertion mode and is set
     *       to (1, 0) for LEFT_TO_RIGHT and to (1, COL_MAX) for RIGHT_TO_LEFT.
     */
    void moveCursorRight();

    /**
     * Scroll the entire display content (all lines) one unit to the left.
     *
     * NOTE: The cursor respects the setting for the insertion mode and is set
     *       to (1, 0) for LEFT_TO_RIGHT and to (1, COL_MAX) for RIGHT_TO_LEFT.
     */
    void scrollDisplayLeft();

    /**
     * Scroll the entire display content (all lines) one unit to the right.
     */
    void scrollDisplayRight();

    /*
     * Create a custom symbol.
     * Useful link: https://maxpromer.github.io/LCD-Character-Creator/
     *
     * @param customSymbol  Key to which a custom symbol should be assigned
     * @param charmap       Bitmap definition of the custom symbol
     * */
    void setCustomSymbol(CustomSymbol customSymbol, uint8_t charmap[]);

    /*
     * Print a custom symbol by key reference.
     *
     * @param customSymbol  Key of the custom symbol to be printed
     * */
    void printCustomSymbol(CustomSymbol customSymbol);

    /*
     * Enable or disable the progress bar. When enabled, the last five custom
     * symbols are reserved to display the progress bar (CUSTOM_SYMBOL_4 to
     * CUSTOM_SYMBOL_8) and can't be used. Assignments via setCustomSymbol() to
     * these keys will be ignored. The given line will be reserved completely
     * for the progress bar. Any text written to that line will be overwritten
     * by the progress bar on an update.
     *
     * NOTE: Auto scroll will be disabled and the display will be scrolled to
     * its original position. Don't use scrolling when using the
     * progressbar, otherwise it won't display correctly.
     *
     * NOTE: Text insertion mode will be set to LEFT_TO_RIGHT.
     *
     * @param enabled   Enable or disable
     * */
    void setProgressBarEnabled(bool enabled);

    /*
     * Set the row for displaying the progress bar. Defaults to the last row,
     * according to the given row count in the constructor.
     *
     * @param row    Row where the progress bar is displayed
     * */
    void setProgressBarRow(uint8_t row);

    /*
     * Set the progress of the progress bar and draw the update.
     *
     * NOTE: This function changes the cursor position. You will have to use
     * setCursorPosition in order to return to your required cursor position.
     *
     * @param progress  Progress in percentage (0.0 to 100.0)
     * */
    void setProgress(float progress);

    /*
     * Implement virtual function write(uint8_t value) from Arduino's Print class
     *
     * @param value     Data to write
     * */
    virtual size_t write(uint8_t value);

private:

    /**
     * Write a data byte to the device.
     *
     * @param value Data byte to write
     */
    int data(uint8_t value);

    /**
     * Write a command byte with it's arguments (modified bits) to the device.
     *
     * @param value Command byte to write
     */
    int command(uint8_t value);

    /**
     * Write a command or data byte to the device.
     *
     * @param data_byte     Byte to write
     * @param is_cmd        Whether byte should be interpreted as data or command
     */
    int write(uint8_t data_byte, bool is_cmd);

    /**
     * Write data to the device.
     *
     * @param data   Data to write
     * @param len    Length of the data
     */
    int deviceWrite(uint8_t *data, uint8_t len);

    /**
     * Initialize controller for progress bar feature.
     *
     * @param row    Row where the progress bar is displayed
     */
    void initProgressBar(uint8_t row);

    /**
     * I2C address of device.
     */
    uint8_t _deviceAddress;

    /**
     * Number of rows of the connected display.
     */
    uint8_t _rowCount;

    /**
     * Number of columns of the connected display.
     */
    uint8_t _colCount;

    /**
     * Size of a character of the connected display.
     */
    uint8_t _fontSize;

    /**
     * Bit mode of the connected display.
     */
    uint8_t _bitMode;

    /**
     * Current value of DISPLAY_CONTROL.
     */
    uint8_t _currDisplayControl;

    /**
     * Current value of ENTRY_MODE_SET.
     */
    uint8_t _currEntryModeSet;

    /**
     * Object for I2C communication.
     */
    TwoWire *_wire;

    /**
     * Whether progress bar feature is enabled or not.
     */
    bool _progressBarEnabled = false;

    /**
     * The line where the progress bar should be displayed on.
     */
    uint8_t _progressBarRow = 0;

    /**
     * 1 x 8 bar for progress bar.
     */
    byte customCharProgressBar1[8] = {
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
    };

    /**
     * 2 x 8 bar for progress bar.
     */
    byte customCharProgressBar2[8] = {
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
    };

    /**
     * 3 x 8 bar for progress bar.
     */
    byte customCharProgressBar3[8] = {
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
    };

    /**
     * 4 x 8 bar for progress bar.
     */
    byte customCharProgressBar4[8] = {
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
    };

    /**
     * 5 x 8 bar for progress bar.
     */
    byte customCharProgressBar5[8] = {
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
    };
};

#endif //LIQUID_CRYSTAL_WIRED_H

# Driver for AiP31068 based controllers for Arduino

This library was developed and tested on an esp32 with a AiP31068L controller,
but should work for every other board as well. The AiP31068L is an intermediate
controller making it possible to speak to a HD44780 compatible display via I2C.

The API is incompatible to existing APIs like LiquidCrystal, because I don't
like the API of them. In fact it was one of the reasons to write my own version.

## Installation

### Arduino IDE

#### Installation using the Library Manager (IDE 1.6.2 and later)
In the Arduino IDE, simply click on:\
`[Sketch] -> Include Library -> Manage Libraries`\
or\
`[Tools] -> Manage Libraries`\
Then search for `LiquidCrystalWired` to locate the library and install it.

#### Installation prior to IDE 1.6.2
Because the Library Manager is not available on versions prior to 1.6.2, you
would have to clone this repository or download the .zip file to install the
library manually. This is strongly discouraged as it can lead to several
problems. You should really do yourself a favour and update your Arduino IDE to
the latest version and install the library using the Library Manager instead.

For generic information about Arduino libraries and how to install them consult
the Arduino Libraries page: https://www.arduino.cc/en/Guide/Libraries

### Other platforms
There isn't a sophisticated installation support for other platforms yet (which
doesn't mean that the library won't work). As long as Arduino libraries are
supported, simply clone this repository according to the platforms guidelines.

## API overview:
```cpp
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
    void begin(uint16_t deviceAddress, TwoWire *wire);

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
     * NOTE: This function changes the cursor position. You must use
     * setCursorPosition to return to your desired cursor position.
     *
     * @param progress  Progress in percentage (0.0 to 100.0)
     * */
    void setProgress(float progress);
```

There is an application called `ApiExample` in `examples`, where you can have a
look how the API works and how it's intended to be used.

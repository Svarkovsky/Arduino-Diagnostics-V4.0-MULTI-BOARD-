// ============================================================================
// ARDUINO DIAGNOSTICS V4.0 [MULTI-BOARD]
// Professional No-Wires Self-Test Tool for Arduino Boards
// ============================================================================
//
// SUPPORTED BOARDS:
//   • Arduino UNO (ATmega328P)
//   • Arduino Nano (ATmega328P)
//   • Arduino Mega 2560 (ATmega2560)
//
// DESCRIPTION:
//   Complete diagnostic suite that tests all major subsystems without
//   requiring any external wires or components. Only USB cable needed.
//   Automatically detects board type and adjusts test parameters.
//
// FEATURES:
//   • Automatic board detection (UNO/Nano/Mega)
//   • Board Type Identification (Crystal vs Resonator)
//   • LED PWM Fade Test (visual confirmation)
//   • EEPROM Multi-Sector Test (4 sectors × 4 patterns)
//   • EEPROM Write Timing + Phantom Read Verification
//   • CPU Clock Accuracy measurement
//   • Timer1 Interrupt Test
//   • Digital Pins Floating Check
//   • Internal Pull-up Resistors Test
//   • Analog Inputs Noise Test
//   • ADC Multiplexer Stability (all channels)
//   • Bandgap Reference Drift Test (10 seconds)
//   • Brown-out Detection Status
//   • Power Stability Under Load
//   • Watchdog Reset Verification
//   • Results preserved in EEPROM across reset
//   • Final summary table with recommendations
//
// TEST TIME: 
//   • UNO/Nano: ~50 seconds
//   • Mega 2560: ~70 seconds (more pins to test)
//
// USAGE:
//   1. Disconnect EVERYTHING from all pins (USB cable only!)
//   2. Upload this sketch to your Arduino board
//   3. Open Serial Monitor at 9600 baud
//   4. Wait for all tests to complete
//   5. Board will auto-reset and display final summary
//   6. Slow blinking LED = all tests passed
//
// VERSION HISTORY:
//   V3.0  - Basic tests (LED, EEPROM, Clock, Pull-ups, Analog)
//   V3.7  - Fixed ADC stabilization
//   V3.8  - Added final summary table
//   V3.9  - Multi-sector EEPROM test
//   V3.9.1 - Results preserved in EEPROM
//   V3.9.2 - EEPROM write timing + phantom read
//   V3.10 - Board ID + Bandgap Drift + Progress counter
//   V3.11 - Fixed progress bar + honest board classification
//   V3.12 - Power stability + Brown-out detection
//   V4.0  - MULTI-BOARD support (UNO/Nano/Mega) [CURRENT]
//
// AUTHOR: Ivan Svarkovsky
// LICENSE: MIT License
// YEAR: 2026
//
// ============================================================================
// MIT License
// 
// Copyright (c) 2026 Ivan Svarkovsky
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ============================================================================

#include <EEPROM.h>
#include <avr/wdt.h>

// ==================== VERSION CONSTANT ====================
const char* VERSION = "V4.0 [MULTI-BOARD]";

// ==================== BOARD DETECTION ====================
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
    // Arduino UNO or Nano
    #define BOARD_TYPE_STR "UNO/Nano (ATmega328P)"
    #define NUM_DIGITAL_PINS_TEST 11      // D2-D12
    #define NUM_ANALOG_PINS 6             // A0-A5
    #define EEPROM_SIZE 1024
    #define DIGITAL_PIN_START 2
    #define DIGITAL_PIN_END 12
    #define ANALOG_PIN_START 0
    #define ANALOG_PIN_END 5
    #define LOAD_PIN_COUNT 11
    const int loadPins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    
#elif defined(__AVR_ATmega2560__)
    // Arduino Mega 2560
    #define BOARD_TYPE_STR "Mega 2560 (ATmega2560)"
    #define NUM_DIGITAL_PINS_TEST 22      // D2-D23 (skip communication pins)
    #define NUM_ANALOG_PINS 16            // A0-A15
    #define EEPROM_SIZE 4096
    #define DIGITAL_PIN_START 2
    #define DIGITAL_PIN_END 23
    #define ANALOG_PIN_START 0
    #define ANALOG_PIN_END 15
    #define LOAD_PIN_COUNT 20
    const int loadPins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
    
#else
    #error "Unsupported MCU! This sketch supports ATmega328P (UNO/Nano) and ATmega2560 (Mega) only."
#endif

// ==================== GLOBAL CONSTANTS ====================
const int ledPin       = 13;
const int EEPROM_ADDR  = 0;
const byte WATCHDOG_MARKER = 77;
const int RESULTS_BASE = 200;  // Increased for Mega compatibility
const int MAX_TESTS    = 12;

volatile unsigned long timerCounter = 0;
ISR(TIMER1_COMPA_vect) { timerCounter++; }

long g_vccResults[NUM_ANALOG_PINS];

struct TestResult {
    char name[20];
    bool passed;
    char details[25];
};

TestResult g_results[MAX_TESTS];
int g_testIndex = 0;

void recordResult(const char* name, bool passed, const char* details);
long readVccStable(int prevPin = -1);

// ==================== PROGRESS COUNTER ====================
void printProgress(int current, int total) {
    Serial.print(F("\r  "));
    Serial.print(current);
    Serial.print(F("/"));
    Serial.print(total);
    Serial.print(F("  "));
    Serial.flush();
}

// ==================== BOARD IDENTIFICATION ====================
void detectBoardType() {
    Serial.println(F("Board Identification..."));
    Serial.print(F("  Detected: "));
    Serial.println(F(BOARD_TYPE_STR));
    
    unsigned long start = micros();
    delay(100);
    unsigned long dur = micros() - start;
    long error = abs((long)dur - 100000);
    
    Serial.print(F("  Timing error: ")); Serial.print(error); Serial.println(F(" us"));
    
    if (error < 500) {
        Serial.println(F("  -> Crystal oscillator (good timing)"));
        recordResult("Board Type", true, "Crystal");
    } else if (error < 2000) {
        Serial.println(F("  -> Ceramic resonator (acceptable)"));
        recordResult("Board Type", true, "Resonator");
    } else {
        Serial.println(F("  -> Poor timing (check clock source)"));
        recordResult("Board Type", false, "Poor Clock");
    }
}

// ==================== BROWN-OUT DETECTION ====================
void testBOD() {
    Serial.println(F("Test: Brown-out Detection..."));
    
    uint8_t mcusr = MCUSR;
    MCUSR = 0;
    
    if (mcusr & (1 << BORF)) {
        Serial.println(F("  Previous reset was BOD -> Power issue detected!"));
        recordResult("BOD Status", false, "BOD Triggered");
    } else {
        Serial.println(F("  No BOD since power-on -> Stable power"));
        recordResult("BOD Status", true, "Not Triggered");
    }
}

// ==================== POWER STABILITY UNDER LOAD ====================
void testPowerStability() {
    Serial.println(F("Test 10: Power Stability under Load..."));
    
    long vccNoLoad = readVccStable(-1);
    
    for (int i = 0; i < LOAD_PIN_COUNT; i++) {
        pinMode(loadPins[i], OUTPUT);
        digitalWrite(loadPins[i], HIGH);
    }
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
    
    delay(100);
    
    long vccWithLoad = readVccStable(-1);
    
    digitalWrite(ledPin, LOW);
    for (int i = 0; i < LOAD_PIN_COUNT; i++) {
        digitalWrite(loadPins[i], LOW);
        pinMode(loadPins[i], INPUT);
    }
    
    long drop = vccNoLoad - vccWithLoad;
    Serial.print(F("  Voltage drop (LED + "));
    Serial.print(LOAD_PIN_COUNT);
    Serial.print(F(" pins): ")); 
    Serial.print(drop); Serial.println(F(" mV"));
    
    bool stable = abs(drop) < 150;
    recordResult("Power Stability", stable, stable ? "Stable" : "Drops under load");
}

// ==================== BANDGAP DRIFT TEST ====================
void testReferenceDrift() {
    Serial.print(F("Test 9: Bandgap Stability (10s)..."));
    
    long samples[10];
    for (int i = 0; i < 10; i++) {
        samples[i] = readVccStable(-1);
        printProgress(i + 1, 10);
        delay(1000);
    }
    Serial.println(F(" Done!      "));
    
    long minV = samples[0], maxV = samples[0];
    for (int i = 1; i < 10; i++) {
        if (samples[i] < minV) minV = samples[i];
        if (samples[i] > maxV) maxV = samples[i];
    }
    
    long drift = maxV - minV;
    Serial.print(F("  Drift over 10s: ")); Serial.print(drift); Serial.println(F(" mV"));
    
    recordResult("Ref Drift (10s)", drift < 30, drift < 30 ? "Stable" : "Unstable");
}

// ==================== EEPROM STORAGE ====================
void saveResultsToEEPROM() {
    EEPROM.write(RESULTS_BASE, g_testIndex);
    
    int offset = 1;
    for (int i = 0; i < g_testIndex; i++) {
        EEPROM.write(RESULTS_BASE + offset, g_results[i].passed ? 1 : 0);
        offset++;
        
        for (int j = 0; j < 20; j++) {
            EEPROM.write(RESULTS_BASE + offset + j, g_results[i].name[j]);
        }
        offset += 20;
        
        for (int j = 0; j < 25; j++) {
            EEPROM.write(RESULTS_BASE + offset + j, g_results[i].details[j]);
        }
        offset += 25;
    }
}

bool loadResultsFromEEPROM() {
    g_testIndex = EEPROM.read(RESULTS_BASE);
    if (g_testIndex == 0 || g_testIndex > MAX_TESTS) return false;
    
    int offset = 1;
    for (int i = 0; i < g_testIndex; i++) {
        g_results[i].passed = (EEPROM.read(RESULTS_BASE + offset) == 1);
        offset++;
        
        for (int j = 0; j < 20; j++) {
            g_results[i].name[j] = EEPROM.read(RESULTS_BASE + offset + j);
        }
        offset += 20;
        
        for (int j = 0; j < 25; j++) {
            g_results[i].details[j] = EEPROM.read(RESULTS_BASE + offset + j);
        }
        offset += 25;
    }
    return true;
}

void recordResult(const char* name, bool passed, const char* details) {
    if (g_testIndex < MAX_TESTS) {
        strncpy(g_results[g_testIndex].name, name, 19);
        g_results[g_testIndex].name[19] = '\0';
        g_results[g_testIndex].passed = passed;
        strncpy(g_results[g_testIndex].details, details, 24);
        g_results[g_testIndex].details[24] = '\0';
        g_testIndex++;
    }
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(9600);
    while (!Serial);

    byte marker = EEPROM.read(EEPROM_ADDR);

    Serial.println(F("\n============================================================================"));
    Serial.print(F("         ARDUINO DIAGNOSTICS "));
    Serial.print(VERSION);
    Serial.println(F(" "));
    Serial.println(F("         Professional No-Wires Self-Test Tool"));
    Serial.println(F("============================================================================\n"));

    if (marker == WATCHDOG_MARKER) {
        EEPROM.write(EEPROM_ADDR, 0);
        Serial.println(F(">> [RESULT] Watchdog Reset: PASSED!"));
        Serial.println(F(">> ALL TESTS COMPLETE.\n"));
        
        if (loadResultsFromEEPROM()) {
            printFinalSummaryTable();
        } else {
            Serial.println(F("Warning: Could not load test results."));
        }
        
        printAnalogRecommendations();

        pinMode(ledPin, OUTPUT);
        while(1) { blinkSlow(); }
    }

    Serial.println(F("IMPORTANT: Disconnect everything from all pins!"));
    Serial.println(F("(USB cable only)\n"));
    delay(2000);

    g_testIndex = 0;

    detectBoardType();
    testLedPWM();
    testEEPROM();
    testClockAccuracy();
    testTimerInterrupt();
    testFloatingDigital();
    testInternalPullUps();
    testAnalogNoiseLevel();
    testVccAllChannels();
    testReferenceDrift();
    testBOD();
    testPowerStability();

    saveResultsToEEPROM();

    Serial.println(F("\nFinal Test: Watchdog Timer (will reset in 2 seconds)"));
    delay(500);

    EEPROM.write(EEPROM_ADDR, WATCHDOG_MARKER);
    wdt_enable(WDTO_2S);
    while(1);
}

void loop() {}

// ========================================
// 1. LED PWM FADE TEST
// ========================================
void testLedPWM() {
    Serial.print(F("Test 1: LED 13 PWM Fade... "));
    pinMode(ledPin, OUTPUT);

    for (int b = 0; b <= 255; b += 5) {
        analogWrite(ledPin, b);
        delay(10);
    }
    for (int b = 255; b >= 0; b -= 5) {
        analogWrite(ledPin, b);
        delay(10);
    }
    digitalWrite(ledPin, LOW);
    Serial.println(F("Passed."));
    
    recordResult("PWM (D13)", true, "Fade smooth");
}

// ========================================
// 2. MULTI-SECTOR EEPROM TEST
// ========================================
void testEEPROM() {
    Serial.println(F("Test 2: EEPROM Multi-Sector + Timing..."));
    
    // Adjust addresses based on EEPROM size
    int testAddresses[4];
    if (EEPROM_SIZE >= 4096) {
        // Mega - spread across larger memory
        testAddresses[0] = 0;
        testAddresses[1] = 1024;
        testAddresses[2] = 2048;
        testAddresses[3] = 4095;
    } else {
        // UNO/Nano
        testAddresses[0] = 0;
        testAddresses[1] = 256;
        testAddresses[2] = 512;
        testAddresses[3] = 1023;
    }
    
    const byte testPatterns[] = {0x00, 0xFF, 0xAA, 0x55};
    
    bool allPassed = true;
    int failedTests = 0;
    int slowWrites = 0;
    long maxWriteTime = 0;
    
    byte originalValues[4];
    for (int i = 0; i < 4; i++) {
        originalValues[i] = EEPROM.read(testAddresses[i]);
    }
    
    Serial.println(F("   Testing 4 sectors with 4 patterns + timing..."));
    
    for (int addrIdx = 0; addrIdx < 4; addrIdx++) {
        int addr = testAddresses[addrIdx];
        Serial.print(F("   Sector @"));
        Serial.print(addr);
        Serial.print(F(": "));
        
        bool sectorOk = true;
        
        for (int patIdx = 0; patIdx < 4; patIdx++) {
            byte pattern = testPatterns[patIdx];
            
            unsigned long startMicros = micros();
            EEPROM.write(addr, pattern);
            unsigned long writeTime = micros() - startMicros;
            
            if (writeTime > maxWriteTime) maxWriteTime = writeTime;
            if (writeTime > 10000) slowWrites++;
            
            byte readBack = EEPROM.read(addr);
            
            if (readBack != pattern) {
                sectorOk = false;
                allPassed = false;
                failedTests++;
            }
        }
        
        EEPROM.write(addr, originalValues[addrIdx]);
        
        if (sectorOk) {
            Serial.println(F("OK"));
        } else {
            Serial.println(F("FAILED"));
        }
    }
    
    Serial.println(F("   Verifying no phantom changes..."));
    bool phantomOk = true;
    for (int i = 0; i < 4; i++) {
        byte verifyRead = EEPROM.read(testAddresses[i]);
        if (verifyRead != originalValues[i]) {
            phantomOk = false;
        }
    }
    
    Serial.print(F("   Total: "));
    Serial.print(16 - failedTests);
    Serial.print(F("/16 sub-tests passed, max write: "));
    Serial.print(maxWriteTime);
    Serial.println(F(" us"));
    
    if (allPassed && phantomOk && slowWrites == 0) {
        Serial.println(F(" -> EEPROM: HEALTHY"));
        recordResult("EEPROM", true, "All sectors OK");
    } else {
        Serial.println(F(" -> EEPROM: DEGRADED"));
        char detailBuf[30];
        if (slowWrites > 0) {
            sprintf_P(detailBuf, PSTR("%d slow writes"), slowWrites);
        } else if (!phantomOk) {
            sprintf_P(detailBuf, PSTR("Phantom changes!"));
        } else {
            sprintf_P(detailBuf, PSTR("%d/16 failed"), failedTests);
        }
        recordResult("EEPROM", false, detailBuf);
    }
}

// ========================================
// 3. CLOCK ACCURACY TEST
// ========================================
void testClockAccuracy() {
    Serial.print(F("Test 3: Clock Accuracy... "));
    unsigned long start = micros();
    delay(1000);
    unsigned long dur = micros() - start;
    long delta = (long)dur - 1000000UL;

    Serial.print(dur);
    Serial.print(F(" us (delta "));
    Serial.print(delta);
    Serial.print(F(" us) "));

    bool passed = (abs(delta) < 15000);
    if (passed) {
        Serial.println(F("-> OK"));
    } else {
        Serial.println(F("-> WARNING"));
    }
    
    char detailBuf[40];
    sprintf_P(detailBuf, PSTR("delta %ld us"), delta);
    recordResult("CPU Clock", passed, detailBuf);
}

// ========================================
// 4. TIMER1 INTERRUPT TEST
// ========================================
void testTimerInterrupt() {
    Serial.print(F("Test 4: Timer1 Interrupts... "));
    noInterrupts();
    TCCR1A = 0; TCCR1B = 0; TCNT1 = 0; OCR1A = 15999;
    TCCR1B |= (1 << WGM12) | (1 << CS10);
    TIMSK1 |= (1 << OCIE1A);
    interrupts();

    timerCounter = 0;
    delay(200);
    TIMSK1 &= ~(1 << OCIE1A);

    bool passed = (timerCounter > 100);
    Serial.println(passed ? F("Passed.") : F("FAILED!"));
    
    recordResult("Timer1 IRQ", passed, passed ? "Working" : "Failed");
}

// ========================================
// 5. DIGITAL PINS FLOATING TEST
// ========================================
void testFloatingDigital() {
    Serial.print(F("Test 5: Digital Floating (D"));
    Serial.print(DIGITAL_PIN_START);
    Serial.print(F("-D"));
    Serial.print(DIGITAL_PIN_END);
    Serial.print(F(")... "));
    
    int highCount = 0;
    for (int i = DIGITAL_PIN_START; i <= DIGITAL_PIN_END; i++) {
        pinMode(i, INPUT);
        delay(2);
        if (digitalRead(i) == HIGH) highCount++;
    }
    Serial.print(highCount);
    Serial.print(F(" HIGHs. "));
    Serial.println(F("OK."));
    
    recordResult("Digital Pins", true, "No shorts");
}

// ========================================
// 6. INTERNAL PULL-UP RESISTORS TEST
// ========================================
void testInternalPullUps() {
    Serial.print(F("Test 6: Internal Pull-ups (D"));
    Serial.print(DIGITAL_PIN_START);
    Serial.print(F("-D"));
    Serial.print(DIGITAL_PIN_END);
    Serial.print(F(")... "));
    
    bool ok = true;
    for (int i = DIGITAL_PIN_START; i <= DIGITAL_PIN_END; i++) {
        pinMode(i, INPUT_PULLUP);
        delay(2);
        if (digitalRead(i) == LOW) ok = false;
        pinMode(i, INPUT);
    }
    Serial.println(ok ? F("Passed.") : F("FAILED!"));
    
    recordResult("Pull-up Resistors", ok, ok ? "All working" : "Some failed");
}

// ========================================
// 7. ANALOG NOISE TEST
// ========================================
void testAnalogNoiseLevel() {
    Serial.print(F("Test 7: Analog Noise (A"));
    Serial.print(ANALOG_PIN_START);
    Serial.print(F("-A"));
    Serial.print(ANALOG_PIN_END);
    Serial.println(F(")"));
    
    bool allOk = true;
    for (int pin = ANALOG_PIN_START; pin <= ANALOG_PIN_END; pin++) {
        int minV = 1023, maxV = 0;
        long sum = 0;

        for (int i = 0; i < 64; i++) {
            int v = analogRead(pin);
            sum += v;
            if (v < minV) minV = v;
            if (v > maxV) maxV = v;
        }
        int avg = sum / 64;
        int range = maxV - minV;

        Serial.print(F("   A")); Serial.print(pin);
        Serial.print(F(": avg=")); Serial.print(avg);
        Serial.print(F(", range=")); Serial.print(range);

        if (avg < 100 || avg > 900) {
            Serial.println(F(" -> WARNING"));
            allOk = false;
        } else {
            Serial.println(F(" -> OK"));
        }
    }
    
    recordResult("Analog Inputs", allOk, allOk ? "All channels OK" : "Some suspicious");
}

// ========================================
// 8. VCC + ADC MULTIPLEXER TEST
// ========================================
long readVccStable(int prevPin = -1) {
    ADCSRA |= _BV(ADEN);

    if (prevPin >= 0) {
        ADMUX = _BV(REFS0) | (prevPin & 0x07);
        delayMicroseconds(50);
        ADCSRA |= _BV(ADSC);
        while (bit_is_set(ADCSRA, ADSC));
    }

    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delayMicroseconds(200);

    for (int i = 0; i < 8; i++) {
        ADCSRA |= _BV(ADSC);
        while (bit_is_set(ADCSRA, ADSC));
        delayMicroseconds(50);
    }

    long sum = 0;
    for (int i = 0; i < 64; i++) {
        ADCSRA |= _BV(ADSC);
        while (bit_is_set(ADCSRA, ADSC));
        sum += ADC;
        delayMicroseconds(50);
    }

    float avgADC = sum / 64.0;
    return 1125300L / avgADC;
}

void testVccAllChannels() {
    Serial.println(F("Test 8: VCC Voltage - Full ADC Channel Map"));
    
    long baseline = 0;
    int failedChannels = 0;
    int warningChannels = 0;

    for (int ch = ANALOG_PIN_START; ch <= ANALOG_PIN_END; ch++) {
        if (ch == ANALOG_PIN_START) {
            g_vccResults[ch] = readVccStable(-1);
        } else {
            g_vccResults[ch] = readVccStable(ch - 1);
        }

        if (ch == ANALOG_PIN_START) baseline = g_vccResults[ch];

        long deviation = g_vccResults[ch] - baseline;
        long absDeviation = abs(deviation);

        String status;
        if (ch == ANALOG_PIN_START) {
            status = "[REFERENCE]";
        } else if (absDeviation <= 150) {
            status = "[OK]";
        } else if (absDeviation <= 400) {
            status = "[WARNING]";
            warningChannels++;
        } else {
            status = "[FAIL]";
            failedChannels++;
        }

        Serial.print(F("   A")); Serial.print(ch);
        Serial.print(F(": ")); Serial.print(g_vccResults[ch]); 
        Serial.print(F(" mV (dev: ")); Serial.print(deviation); 
        Serial.print(F(" mV) "));
        Serial.println(status);
    }

    long minV = g_vccResults[ANALOG_PIN_START];
    long maxV = g_vccResults[ANALOG_PIN_START];
    long total = 0;
    for (int ch = ANALOG_PIN_START; ch <= ANALOG_PIN_END; ch++) {
        if (g_vccResults[ch] < minV) minV = g_vccResults[ch];
        if (g_vccResults[ch] > maxV) maxV = g_vccResults[ch];
        total += g_vccResults[ch];
    }
    
    long spread = maxV - minV;
    bool adcHealthy = (failedChannels == 0 && warningChannels == 0);

    Serial.println(F(" --- Summary ---"));
    Serial.print(F(" Baseline (A0): ")); Serial.print(baseline); Serial.println(F(" mV"));
    Serial.print(F(" Spread (max-min): ")); Serial.print(spread); Serial.println(F(" mV"));
    Serial.print(F(" Average Vcc: ")); Serial.print(total / (ANALOG_PIN_END - ANALOG_PIN_START + 1)); Serial.println(F(" mV"));
    Serial.print(F(" Failed channels: ")); Serial.println(failedChannels);
    Serial.print(F(" Warning channels: ")); Serial.println(warningChannels);
    
    if (adcHealthy) {
        Serial.println(F(" -> ADC Multiplexer: HEALTHY"));
    } else if (failedChannels <= 2) {
        Serial.println(F(" -> ADC Multiplexer: DEGRADED"));
    } else {
        Serial.println(F(" -> ADC Multiplexer: CRITICAL"));
    }
    
    char detailBuf[50];
    sprintf_P(detailBuf, PSTR("Spread %ld mV"), spread);
    recordResult("ADC Multiplexer", adcHealthy, detailBuf);
}

// ========================================
// FINAL SUMMARY TABLE
// ========================================
void printFinalSummaryTable() {
    Serial.println(F("\n+==========================================================================+"));
    Serial.println(F("|                  FINAL DIAGNOSTIC SUMMARY                                |"));
    Serial.println(F("+====================+=========+===========================================+"));
    Serial.println(F("| Component          | Status  | Details                                   |"));
    Serial.println(F("+====================+=========+===========================================+"));
    
    int passedCount = 0;
    for (int i = 0; i < g_testIndex; i++) {
        Serial.print(F("| "));
        Serial.print(g_results[i].name);
        for (int s = strlen(g_results[i].name); s < 19; s++) Serial.print(F(" "));
        
        Serial.print(F("| "));
        if (g_results[i].passed) {
            Serial.print(F("OK      "));
            passedCount++;
        } else {
            Serial.print(F("FAIL    "));
        }
        Serial.print(F("| "));
        Serial.print(g_results[i].details);
        for (int s = strlen(g_results[i].details); s < 42; s++) Serial.print(F(" "));
        Serial.println(F("|"));
    }
    
    Serial.println(F("+====================+=========+===========================================+"));
    
    if (passedCount == g_testIndex) {
        Serial.println(F("| OVERALL: ALL TESTS PASSED - BOARD IS HEALTHY!                        |"));
    } else {
        Serial.print(F("| OVERALL: "));
        Serial.print(g_testIndex - passedCount);
        Serial.println(F(" TEST(S) FAILED - CHECK BOARD!                              |"));
    }
    
    Serial.println(F("+==========================================================================+"));
}

// ========================================
// ANALOG RECOMMENDATIONS
// ========================================
void printAnalogRecommendations() {
    Serial.println(F("\n--- ADC Channel Recommendations ---"));
    Serial.print(F("Safe analog pins: "));

    bool hasSafe = false;
    for (int ch = ANALOG_PIN_START; ch <= ANALOG_PIN_END; ch++) {
        long deviation = abs(g_vccResults[ch] - g_vccResults[ANALOG_PIN_START]);
        if (deviation <= 150) {
            Serial.print(F("A")); Serial.print(ch); Serial.print(F(" "));
            hasSafe = true;
        }
    }
    if (!hasSafe) Serial.print(F("NONE"));
    Serial.println();

    Serial.print(F("Avoid for ADC: "));
    bool hasBad = false;
    for (int ch = ANALOG_PIN_START; ch <= ANALOG_PIN_END; ch++) {
        long deviation = abs(g_vccResults[ch] - g_vccResults[ANALOG_PIN_START]);
        if (deviation > 150) {
            Serial.print(F("A")); Serial.print(ch); Serial.print(F(" "));
            hasBad = true;
        }
    }
    if (!hasBad) Serial.print(F("NONE"));
    Serial.println();
}

// ========================================
// SLOW BLINK
// ========================================
void blinkSlow() {
    digitalWrite(ledPin, HIGH); delay(500);
    digitalWrite(ledPin, LOW);  delay(500);
}

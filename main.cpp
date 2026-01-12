


#include <stdio.h>
#include "EmbSysLib.h"
#include "ReportHandler.h"
#include "config.h"
#include <stdint.h>
#include <math.h>

// =====================================================
// UHR-FUNKTION
// =====================================================
static void tickClock(uint8_t &h, uint8_t &m, uint8_t &s)
{
    s++;
    if(s >= 60)
    {
        s = 0;
        m++;
        if(m >= 60)
        {
            m = 0;
            h++;
            if(h >= 24) { h = 0; }
        }
    }
}

// =====================================================
// MAIN
// =====================================================
int main(void)
{
    terminal.printf("\r\n\nFitnessband gestartet\r\n");

    EmbSysLib::Hw::I2Cmaster::Device i2cMPU6050(i2cBus, 0xD0);

    BYTE id;
    BYTE ax_h, ax_l, ay_h, ay_l, az_h, az_l;

    // MPU6050 aufwecken
    i2cMPU6050.write((BYTE)0x6B, (BYTE)0x00);

    // Uhrzeit aus __TIME__
    const char *t = __TIME__;
    uint8_t hour   = (uint8_t)((t[0] - '0') * 10 + (t[1] - '0'));
    uint8_t minute = (uint8_t)((t[3] - '0') * 10 + (t[4] - '0'));
    uint8_t second = (uint8_t)((t[6] - '0') * 10 + (t[7] - '0'));

    // Schrittzähler
    long stepCount = 0;
    int32_t prevMag = 0;
    uint16_t loopsSinceLastStep = 0;

    // Tagesziel (änderbar per Tasten)
    long DAILY_STEP_GOAL = 15; // <<< GEÄNDERT >>> (nicht const)

    // Display Wechsel
    const uint8_t DISPLAY_SWITCH_INTERVAL = 5;
    uint8_t viewTimer = 0;
    bool showGoalView = false;

    const uint32_t LOOP_DELAY = 2600000;

    // Kalorien: Basis + kleiner Bonus bei intensiver Bewegung
    uint32_t intensityAvg = 0;
    uint32_t extraPoints  = 0;
    const uint32_t EXTRA_POINTS_PER_KCAL = 5000;

    // -------------------------------------------------
    // Flacker-Fix: clear nur bei View-Wechsel
    // -------------------------------------------------
    bool lastView = false;
    bool firstDraw = true;

    while(1)
    {
        // -------------------------------------------------
        // <<< NEU >>> Ziel mit Tasten einstellen: 1 Klick = 1x +/-5
        // PullUp (InPU): gedrückt = 0
        static bool prevA = false;
        static bool prevB = false;

        bool aDown = (btn_A == 0);
        bool bDown = (btn_B == 0);

        bool aPressed = (aDown && !prevA);
        bool bPressed = (bDown && !prevB);

        prevA = aDown;
        prevB = bDown;

        if(bPressed)
        {
            DAILY_STEP_GOAL += 5;
            if(DAILY_STEP_GOAL > 50000) DAILY_STEP_GOAL = 50000; // optional
            firstDraw = true; // damit Anzeige sofort sauber neu gerendert wird
        }

        if(aPressed)
        {
            if(DAILY_STEP_GOAL >= 5) DAILY_STEP_GOAL -= 5;
            else DAILY_STEP_GOAL = 0; // optional
            firstDraw = true;
        }
        // -------------------------------------------------

        led_A = btn_A;

        // WHO_AM_I (Debug)
        i2cMPU6050.read((BYTE)0x75, &id);

        // Beschleunigungswerte
        i2cMPU6050.read((BYTE)0x3B, &ax_h);
        i2cMPU6050.read((BYTE)0x3C, &ax_l);

        i2cMPU6050.read((BYTE)0x3D, &ay_h);
        i2cMPU6050.read((BYTE)0x3E, &ay_l);

        i2cMPU6050.read((BYTE)0x3F, &az_h);
        i2cMPU6050.read((BYTE)0x40, &az_l);

        int16_t ax = ((int16_t)ax_h << 8) | ax_l;
        int16_t ay = ((int16_t)ay_h << 8) | ay_l;
        int16_t az = ((int16_t)az_h << 8) | az_l;

        // Magnitude = sqrt(ax^2 + ay^2 + az^2)
        int64_t mag2 =
            (int64_t)ax * ax +
            (int64_t)ay * ay +
            (int64_t)az * az;

        int32_t mag = (int32_t)sqrt((double)mag2);

        int32_t diff = mag - prevMag;
        prevMag = mag;

        // Intensität = |diff| + Glättung
        uint32_t intensity = (uint32_t)((diff >= 0) ? diff : -diff);
        intensityAvg = (intensityAvg * 7 + intensity) / 8;

        // Schritt-Erkennung
        const int32_t STEP_RISE_MIN = 600;
        const uint16_t MIN_LOOP_INTERVAL = 1; // (0.5 wäre 0)

        bool stepDetected = false;

        if(diff > STEP_RISE_MIN && loopsSinceLastStep > MIN_LOOP_INTERVAL)
        {
            stepCount++;
            loopsSinceLastStep = 0;
            stepDetected = true;
        }
        else
        {
            if(loopsSinceLastStep < 60000)
                loopsSinceLastStep++;
        }

        // Kalorien (realistisch): Basis 1 kcal pro 25 Schritte + kleiner Bonus
        long baseCalories = stepCount / 25;

        if(stepDetected)
        {
            const uint32_t INTENSITY_BASE = 350;
            if(intensityAvg > INTENSITY_BASE)
            {
                extraPoints += (intensityAvg - INTENSITY_BASE) / 10;
            }
        }

        long extraCalories = (long)(extraPoints / EXTRA_POINTS_PER_KCAL);
        long calories = baseCalories + extraCalories;

        bool goalReached = (stepCount >= DAILY_STEP_GOAL);

        // Zeitstring
        char timeStr[16];
        sprintf(timeStr, "%02u:%02u:%02u", hour, minute, second);

        // View-Wechsel
        viewTimer++;
        if(viewTimer >= DISPLAY_SWITCH_INTERVAL)
        {
           showGoalView = !showGoalView;
           viewTimer = 0;
           firstDraw = true; // damit beim Umschalten sauber gecleart wird
        }

        // -------------------------------------------------
        // Display ohne Flackern:
        // - clear nur beim ersten Mal oder beim View-Wechsel
        // -------------------------------------------------
        if(firstDraw || (showGoalView != lastView))
        {
            screen.clear();
            lastView = showGoalView;
            firstDraw = false;

            screen.drawText(0, 0, "Time %s", timeStr);

            if(!showGoalView)
            {
                screen.drawText(0, 20, "Steps: %6ld", stepCount);
                screen.drawText(0, 40, "Kcal : %6ld", calories);
            }
            else
            {
                screen.drawText(0, 30, "Ziel: %ld / %ld      ", stepCount, DAILY_STEP_GOAL);

                if(goalReached)
                    screen.drawText(0, 45, "Ziel erreicht");
            }

            screen.refresh();
        }
        else
        {
            screen.drawText(0, 0,  "Time %s", timeStr);

            if(!showGoalView)
            {
                screen.drawText(0, 20, "Steps: %6ld", stepCount);
                screen.drawText(0, 40, "Kcal : %6ld", calories);
            }
            else
            {
                screen.drawText(0, 30, "Ziel: %ld / %ld      ", stepCount, DAILY_STEP_GOAL);

                if(goalReached)
                    screen.drawText(0, 45, "Ziel erreicht");
                else
                    screen.drawText(0, 45, "            ");
            }

            screen.refresh();
        }
        // -------------------------------------------------

        terminal.printf("Time=%s Steps=%ld Kcal=%ld diff=%ld intAvg=%lu ID=0x%02X View=%s Goal=%ld\r\n",
                        timeStr, stepCount, calories, (long)diff, (unsigned long)intensityAvg,
                        id, showGoalView ? "ZIEL" : "STD", DAILY_STEP_GOAL);

      //  for(volatile uint32_t i = 0; i < LOOP_DELAY; i++) {}
        for(volatile uint32_t i = 0; i < LOOP_DELAY; i++)
        {
            // alle paar Iterationen Buttons abfragen (damit kurzer Klick erkannt wird)
            if((i % 500) == 0)
            {
                bool aDown2 = (btn_A == 0);
                bool bDown2 = (btn_B == 0);

                bool aPressed2 = (aDown2 && !prevA);
                bool bPressed2 = (bDown2 && !prevB);

                prevA = aDown2;
                prevB = bDown2;

                if(bPressed2)
                {
                    DAILY_STEP_GOAL += 5;
                    if(DAILY_STEP_GOAL > 50000) DAILY_STEP_GOAL = 50000;
                    firstDraw = true;
                }

                if(aPressed2)
                {
                    if(DAILY_STEP_GOAL >= 5) DAILY_STEP_GOAL -= 5;
                    else DAILY_STEP_GOAL = 0;
                    firstDraw = true;
                }
            }
        }
        tickClock(hour, minute, second);
    }
}

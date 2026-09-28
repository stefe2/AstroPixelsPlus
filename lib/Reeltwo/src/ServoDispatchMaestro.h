#ifndef ServoDispatchMaestro_h
#define ServoDispatchMaestro_h

#include "ServoDispatch.h"
#include "core/SetupEvent.h"
#include "core/AnimatedEvent.h"

// Enable debug messages on Serial Monitor (uncomment to enable)
// #define SERVO_DEBUG 1

// Maestro Device ID (configured in Maestro Control Center - Serial Settings tab)
#ifndef MAESTRO_DEVICE_ID
#define MAESTRO_DEVICE_ID 0x01  // Device Number = 1
#endif

/**
  * \ingroup Dome
  *
  * \class ServoDispatchMaestro
  *
  * \brief Servo control via Pololu Maestro 24-channel controller using Serial interface.
  *
  * Implements ServoDispatch interface to control servos through a Pololu Maestro
  * controller connected via hardware serial (Serial1). Uses Pololu Protocol for
  * full control including servo disable capability.
  *
  * \tparam numServos Number of servos (up to 24 for Maestro 24-channel)
  */
template<uint16_t numServos>
class ServoDispatchMaestro : public ServoDispatch, public SetupEvent, public AnimatedEvent
{
public:
    /**
      * Constructor
      *
      * \param serial Pointer to HardwareSerial (typically Serial1)
      * \param settings Array of ServoSettings in PROGMEM
      */
    ServoDispatchMaestro(HardwareSerial* serial, const ServoSettings* settings)
        : fSerial(serial)
    {
        // Load settings from PROGMEM
        for (uint16_t i = 0; i < numServos; i++)
        {
            ServoSettings setting;
            memcpy_P(&setting, &settings[i], sizeof(setting));
            
            fSettings[i].fPin = setting.pinNum;
            fSettings[i].fStart = setting.startPulse;
            fSettings[i].fEnd = setting.endPulse;
            fSettings[i].fGroup = setting.group;
            fSettings[i].fNeutral = (setting.startPulse + setting.endPulse) / 2;
            fSettings[i].fMinimum = min(setting.startPulse, setting.endPulse);
            fSettings[i].fMaximum = max(setting.startPulse, setting.endPulse);
            
            // Initialize servo state
            fServos[i].fCurrentPos = fSettings[i].fNeutral;
            fServos[i].fTargetPos = fSettings[i].fNeutral;
            fServos[i].fStartPos = fSettings[i].fNeutral;
            fServos[i].fMoveStartTime = 0;
            fServos[i].fMoveDuration = 0;
            fServos[i].fStopTime = 0;
            fServos[i].fEasing = nullptr;  // No easing by default (will be set via setEasingMethod)
            fServos[i].fActive = false;
            fServos[i].fMoving = false;
        }
    }

    /**
      * Initialize Serial connection to Maestro
      */
    virtual void setup() override
    {
        if (fSerial != nullptr)
        {
            // Serial already initialized in main setup()
            DEBUG_PRINTLN("ServoDispatchMaestro: Ready");
        }
    }

    /**
      * Update servo positions (called in loop)
      */
    virtual void setSequenceActive(bool active) override
    {
        fSequenceActive = active;
        if (active)
        {
            // Cancel any pending per-servo stop timers when a sequence starts
            for (uint16_t i = 0; i < numServos; i++)
                fServos[i].fStopTime = 0;
        }
    }

    virtual void animate() override
    {
        uint32_t currentTime = millis();
        
        for (uint16_t i = 0; i < numServos; i++)
        {
            // Per-servo auto-stop: cut PWM after servo reaches target (only outside sequences)
            if (!fSequenceActive && fServos[i].fStopTime != 0 && currentTime >= fServos[i].fStopTime)
            {
                fServos[i].fStopTime = 0;
                disable(i);
                continue;
            }

            // Skip if not moving or disabled
            if (!fServos[i].fMoving || !fServos[i].fActive) continue;
            
            // Check if we should start the movement (startDelay elapsed)
            if (currentTime < fServos[i].fMoveStartTime) continue;
            
            // Calculate elapsed time since movement started
            uint32_t elapsed = currentTime - fServos[i].fMoveStartTime;
            
            // Movement complete?
            if (elapsed >= fServos[i].fMoveDuration)
            {
                fServos[i].fCurrentPos = fServos[i].fTargetPos;
                fServos[i].fMoving = false;
                setPWM(i, fServos[i].fTargetPos);
                // Schedule PWM cut after 700ms (only if not in a sequence)
                if (!fSequenceActive)
                    fServos[i].fStopTime = currentTime + 700;
                continue;
            }
            
            // Calculate completion (0.0 to 1.0)
            float completion = (float)elapsed / (float)fServos[i].fMoveDuration;
            
            // Apply easing function if available
            if (fServos[i].fEasing != nullptr)
            {
                completion = fServos[i].fEasing(completion);
            }
            
            // Interpolate position
            int32_t startPos = fServos[i].fStartPos;
            int32_t targetPos = fServos[i].fTargetPos;
            int32_t delta = targetPos - startPos;
            uint16_t currentPos = startPos + (uint16_t)(delta * completion);
            
            // Update position if changed
            if (currentPos != fServos[i].fCurrentPos)
            {
                fServos[i].fCurrentPos = currentPos;
                setPWM(i, currentPos);
            }
        }
    }

    // =========================================================================
    // ServoDispatch pure virtual methods - Getters
    // =========================================================================

    virtual uint16_t getNumServos() override
    {
        return numServos;
    }

    virtual uint8_t getPin(uint16_t num) override
    {
        return (num < numServos) ? fSettings[num].fPin : 0;
    }

    virtual uint16_t getStart(uint16_t num) override
    {
        return (num < numServos) ? fSettings[num].fStart : 1500;
    }

    virtual uint16_t getEnd(uint16_t num) override
    {
        return (num < numServos) ? fSettings[num].fEnd : 1500;
    }

    virtual uint16_t getMinimum(uint16_t num) override
    {
        return (num < numServos) ? fSettings[num].fMinimum : 1000;
    }

    virtual uint16_t getMaximum(uint16_t num) override
    {
        return (num < numServos) ? fSettings[num].fMaximum : 2000;
    }

    virtual uint16_t getNeutral(uint16_t num) override
    {
        return (num < numServos) ? fSettings[num].fNeutral : 1500;
    }

    virtual uint32_t getGroup(uint16_t num) override
    {
        return (num < numServos) ? fSettings[num].fGroup : 0;
    }

    virtual uint16_t currentPos(uint16_t num) override
    {
        return (num < numServos) ? fServos[num].fCurrentPos : 1500;
    }

    virtual uint16_t scaleToPos(uint16_t num, float scale) override
    {
        if (num >= numServos) return 1500;
        
        uint16_t start = fSettings[num].fStart;
        uint16_t end = fSettings[num].fEnd;
        
        // Clamp scale to 0.0-1.0
        if (scale < 0.0) scale = 0.0;
        if (scale > 1.0) scale = 1.0;
        
        return start + (uint16_t)((end - start) * scale);
    }

    virtual bool isActive(uint16_t num) override
    {
        return (num < numServos) ? fServos[num].fActive : false;
    }

    // =========================================================================
    // ServoDispatch pure virtual methods - Control
    // =========================================================================

    virtual void disable(uint16_t num) override
    {
        if (num >= numServos) return;
        
        // IMPORTANT: Disable first to prevent animate() from sending more positions
        fServos[num].fActive = false;
        fServos[num].fMoving = false;
        
        // Wait 250ms to ensure Maestro buffer is fully processed before sending disable
        // delay(250);  // Commented out - may not be needed
        
        // Use Set PWM command to truly disable servo (no PWM signal)
        if (fSerial != nullptr)
        {
            fSerial->write(0xAA);                   // Start byte
            fSerial->write(MAESTRO_DEVICE_ID);      // Device ID
            fSerial->write(0x60);                   // Command: Set PWM (0x60 = disable)
            fSerial->write(num);                    // Channel
            fSerial->write(0x00);                   // On time low = 0 (OFF)
            fSerial->write(0x00);                   // On time high = 0 (OFF)
            
            #ifdef SERVO_DEBUG
            DEBUG_PRINT("Maestro: Disable Chan ");
            DEBUG_PRINT(num);
            DEBUG_PRINTLN(" (Set PWM=0)");
            #endif
        }
    }

    virtual void setServo(uint16_t num, uint8_t pin, uint16_t startPulse, uint16_t endPulse, 
                          uint16_t neutralPulse, uint32_t group) override
    {
        if (num < numServos)
        {
            fSettings[num].fPin = pin;
            fSettings[num].fStart = startPulse;
            fSettings[num].fEnd = endPulse;
            fSettings[num].fNeutral = neutralPulse;
            fSettings[num].fGroup = group;
            fSettings[num].fMinimum = min(startPulse, endPulse);
            fSettings[num].fMaximum = max(startPulse, endPulse);
        }
    }

    /**
      * Send Set Target command to Maestro using Pololu Protocol
      *
      * \param num Servo channel (0-23 for Maestro 24)
      * \param targetMicros Target position in microseconds (typically 1000-2000)
      */
    virtual void setPWM(uint16_t num, uint16_t targetMicros) override
    {
        if (num >= numServos || fSerial == nullptr)
            return;
        
        // Pololu Protocol: Set Target
        // 6 bytes: 0xAA, device_id, 0x04 (Set Target), channel, target_low, target_high
        // Target units: quarter-microseconds (multiply µs by 4)
        uint16_t target = targetMicros * 4;
        
        fSerial->write(0xAA);                       // Start byte
        fSerial->write(MAESTRO_DEVICE_ID);          // Device ID (configured to 1)
        fSerial->write(0x04);                       // Command: Set Target
        fSerial->write(num);                        // Channel (0-23)
        fSerial->write(target & 0x7F);              // Target low 7 bits
        fSerial->write((target >> 7) & 0x7F);       // Target high 7 bits
        
        // Update current position for tracking
        // Note: fCurrentPos is updated by animate() during movement
        // Only update directly here for instant moves (moveTime=0)
        // fServos[num].fCurrentPos = targetMicros;  // Removed - handled by animate()
        fServos[num].fActive = true;
        
    #ifdef SERVO_DEBUG
        DEBUG_PRINT("Maestro: Chan ");
        DEBUG_PRINT(num);
        DEBUG_PRINT(" -> ");
        DEBUG_PRINT(targetMicros);
        DEBUG_PRINTLN("µs");
    #endif
    }

    virtual void stop() override
    {
        // IMPORTANT: Disable all first to prevent animate() from sending more positions
        for (uint16_t i = 0; i < numServos; i++)
        {
            fServos[i].fMoving = false;
            fServos[i].fActive = false;
        }
        
        // Wait 250ms to ensure Maestro buffer is fully processed before sending disable
        // delay(250);  // Commented out - may not be needed
        
        // Use Set PWM command to truly disable all servos
        if (fSerial != nullptr)
        {
            for (uint16_t i = 0; i < numServos; i++)
            {
                fSerial->write(0xAA);               // Start byte
                fSerial->write(MAESTRO_DEVICE_ID);  // Device ID
                fSerial->write(0x60);               // Command: Set PWM (0x60 = disable)
                fSerial->write(i);                  // Channel
                fSerial->write(0x00);               // On time low = 0 (OFF)
                fSerial->write(0x00);               // On time high = 0 (OFF)
            }
        }
        
        #ifdef SERVO_DEBUG
        DEBUG_PRINTLN("Maestro: All servos disabled (Pololu target=0)");
        #endif
    }

    // =========================================================================
    // ServoDispatch pure virtual methods - Protected movement methods (stubs)
    // =========================================================================

protected:
    virtual void _moveServoToPulse(uint16_t num, uint32_t startDelay, uint32_t moveTime, 
                                   uint16_t startPos, uint16_t pos) override
    {
        if (num >= numServos) return;
        
        // Skip only if servo is active AND already at target position.
        // If fActive=false (servo was disabled by stop()), fCurrentPos may be stale
        // (e.g. speed=0 sequences set fCurrentPos instantly without physical movement),
        // so we must always re-send the command.
        if (fServos[num].fActive && fServos[num].fCurrentPos == pos)
        {
            return;
        }
        
        // Immediate move if moveTime is 0
        if (moveTime == 0)
        {
            fServos[num].fCurrentPos = pos;
            fServos[num].fTargetPos = pos;
            fServos[num].fMoving = false;
            setPWM(num, pos);
            return;
        }
        
        // Setup animated movement
        // Use ACTUAL current position, not startPos parameter which might be stale
        fServos[num].fStartPos = fServos[num].fCurrentPos;
        fServos[num].fTargetPos = pos;
        fServos[num].fMoveStartTime = millis() + startDelay;
        fServos[num].fMoveDuration = moveTime;
        fServos[num].fMoving = true;
        fServos[num].fActive = true;
        fServos[num].fStopTime = 0;  // Cancel any pending stop timer
        
        #ifdef SERVO_DEBUG
        DEBUG_PRINT("Maestro animate: Chan ");
        DEBUG_PRINT(num);
        DEBUG_PRINT(" from ");
        DEBUG_PRINT(startPos);
        DEBUG_PRINT(" to ");
        DEBUG_PRINT(pos);
        DEBUG_PRINT(" over ");
        DEBUG_PRINT(moveTime);
        DEBUG_PRINT("ms (delay: ");
        DEBUG_PRINT(startDelay);
        DEBUG_PRINTLN("ms)");
        #endif
    }

    virtual void _moveServosToPulse(uint32_t servoGroupMask, uint32_t startDelay, 
                                    uint32_t moveTimeMin, uint32_t moveTimeMax, uint16_t pos) override
    {
        for (uint16_t i = 0; i < numServos; i++)
        {
            if (fSettings[i].fGroup & servoGroupMask)
            {
                // Random moveTime between Min and Max for natural movement
                uint32_t moveTime = (moveTimeMin == moveTimeMax) ? moveTimeMin : 
                                    random(moveTimeMin, moveTimeMax + 1);
                _moveServoToPulse(i, startDelay, moveTime, currentPos(i), pos);
            }
        }
    }

    virtual void _moveServosByPulse(uint32_t servoGroupMask, uint32_t startDelay, 
                                    uint32_t moveTimeMin, uint32_t moveTimeMax, int16_t pos) override
    {
        for (uint16_t i = 0; i < numServos; i++)
        {
            if (fSettings[i].fGroup & servoGroupMask)
            {
                uint16_t newPos = currentPos(i) + pos;
                // Random moveTime between Min and Max
                uint32_t moveTime = (moveTimeMin == moveTimeMax) ? moveTimeMin : 
                                    random(moveTimeMin, moveTimeMax + 1);
                _moveServoToPulse(i, startDelay, moveTime, currentPos(i), newPos);
            }
        }
    }

    virtual void _moveServoSetToPulse(uint32_t servoGroupMask, uint32_t servoSetMask, 
                                      uint32_t startDelay, uint32_t moveTimeMin, uint32_t moveTimeMax,
                                      uint16_t onPos, uint16_t offPos) override
    {
        for (uint16_t i = 0; i < numServos; i++)
        {
            if (fSettings[i].fGroup & servoGroupMask)
            {
                uint16_t targetPos = (fSettings[i].fGroup & servoSetMask) ? onPos : offPos;
                // Random moveTime between Min and Max for natural movement
                uint32_t moveTime = (moveTimeMin == moveTimeMax) ? moveTimeMin : 
                                    random(moveTimeMin, moveTimeMax + 1);
                _moveServoToPulse(i, startDelay, moveTime, currentPos(i), targetPos);
            }
        }
    }

    virtual void _moveServoSetByPulse(uint32_t servoGroupMask, uint32_t servoSetMask, 
                                      uint32_t startDelay, uint32_t moveTimeMin, uint32_t moveTimeMax,
                                      int16_t onPos, int16_t offPos) override
    {
        for (uint16_t i = 0; i < numServos; i++)
        {
            if (fSettings[i].fGroup & servoGroupMask)
            {
                int16_t delta = (fSettings[i].fGroup & servoSetMask) ? onPos : offPos;
                uint16_t newPos = currentPos(i) + delta;
                // Random moveTime between Min and Max
                uint32_t moveTime = (moveTimeMin == moveTimeMax) ? moveTimeMin : 
                                    random(moveTimeMin, moveTimeMax + 1);
                _moveServoToPulse(i, startDelay, moveTime, currentPos(i), newPos);
            }
        }
    }

    virtual void _moveServosTo(uint32_t servoGroupMask, uint32_t startDelay, 
                               uint32_t moveTimeMin, uint32_t moveTimeMax, float pos) override
    {
        for (uint16_t i = 0; i < numServos; i++)
        {
            if (fSettings[i].fGroup & servoGroupMask)
            {
                uint16_t targetPos = scaleToPos(i, pos);
                // Random moveTime between Min and Max for natural movement
                uint32_t moveTime = (moveTimeMin == moveTimeMax) ? moveTimeMin : 
                                    random(moveTimeMin, moveTimeMax + 1);
                _moveServoToPulse(i, startDelay, moveTime, currentPos(i), targetPos);
            }
        }
    }

    virtual void _moveServoSetTo(uint32_t servoGroupMask, uint32_t servoSetMask, 
                                 uint32_t startDelay, uint32_t moveTimeMin, uint32_t moveTimeMax,
                                 float onPos, float offPos, 
                                 float (*onEasingMethod)(float), float (*offEasingMethod)(float)) override
    {
        for (uint16_t i = 0; i < numServos; i++)
        {
            if (fSettings[i].fGroup & servoGroupMask)
            {
                bool isOn = (fSettings[i].fGroup & servoSetMask);
                float targetScale = isOn ? onPos : offPos;
                uint16_t targetPos = scaleToPos(i, targetScale);
                
                // Set easing method if provided
                float (*easingMethod)(float) = isOn ? onEasingMethod : offEasingMethod;
                if (easingMethod != nullptr)
                {
                    fServos[i].fEasing = easingMethod;
                }
                
                // Random moveTime between Min and Max for natural movement
                uint32_t moveTime = (moveTimeMin == moveTimeMax) ? moveTimeMin : 
                                    random(moveTimeMin, moveTimeMax + 1);
                _moveServoToPulse(i, startDelay, moveTime, currentPos(i), targetPos);
            }
        }
    }

    virtual void _setServoEasingMethod(uint16_t num, float (*easingMethod)(float completion)) override
    {
        if (num < numServos && easingMethod != nullptr)
        {
            fServos[num].fEasing = easingMethod;
        }
    }

    virtual void _setServosEasingMethod(uint32_t servoGroupMask, float (*easingMethod)(float completion)) override
    {
        for (uint16_t i = 0; i < numServos; i++)
        {
            if (fSettings[i].fGroup & servoGroupMask)
            {
                _setServoEasingMethod(i, easingMethod);
            }
        }
    }

    virtual void _setEasingMethod(float (*easingMethod)(float completion)) override
    {
        for (uint16_t i = 0; i < numServos; i++)
        {
            _setServoEasingMethod(i, easingMethod);
        }
    }

private:
    HardwareSerial* fSerial;
    
    struct Settings {
        uint8_t fPin;
        uint16_t fStart;
        uint16_t fEnd;
        uint16_t fNeutral;
        uint16_t fMinimum;
        uint16_t fMaximum;
        uint32_t fGroup;
    };
    
    struct ServoState {
        uint16_t fCurrentPos;
        uint16_t fTargetPos;
        uint16_t fStartPos;        // Position de départ pour l'interpolation
        uint32_t fMoveStartTime;
        uint16_t fMoveDuration;
        uint32_t fStopTime;        // millis() when servo PWM should be cut (0 = no pending stop)
        float (*fEasing)(float);
        bool fActive;
        bool fMoving;
    };
    
    Settings fSettings[numServos];
    ServoState fServos[numServos];
    bool fSequenceActive = false;   // True while ServoSequencer is running a sequence
};

#endif

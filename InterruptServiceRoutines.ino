/*
 *  These are the interrupt routines to handle the pressing of the buttons and the rotation of the 
 *  knob. We must be very careful what we do in here as interrupt routines block. We communicate with
 *  the main code by means of flags.
 */

// Interrupt called when the rotary switch is turned
// Each rotation step creates three bytes 123 anti-clockwise and 213 clockwise
void rotarySwitchService( void )
{
    static bool inService = false;
    static short inputByte = 0;
    static enumState lastState = Idle;
    static ULONG ttime = MAX_ULONG ;
    static byte buff[ 3 ] = { 0 };
    static ULONG lbuff = 0;
    
    if ( inService == true ) // Ensure we dont get called  again while servicing. Shouldn't happen but I've been caught before!
        return;
    else
        inService = true; 
         
    if (( millis() - ttime )  > 100 ) // Helps to detect stray pulses by invalidating the buffer after a timeout
        inputByte = 0;
        
    delay( 30 ); // Debounce
    
    short a = (PIND & 0xC0 ) >> 6; // Read the input pins for a button press
    if ( a == 0 )
    {
        inService = false;
        return;
    }
    
    switch( inputByte ) 
    {
        case 0: // First pulse
            buff[ 0 ] = a;
            ++inputByte;
            ttime = millis(); // Save the time that the first pulse arrived
            break;
        case 1: // Second pulse
             buff[ 1 ] = a;
            ++inputByte;
           break;
        case 2: // Third pulse
           buff[ 2 ] = a;
           inputByte = 0;
           ttime = millis(); // Reset the time
           if (( buff[ 0 ] == 0x01 ) && (buff[ 1 ] == 0x02 ) && ( buff[ 2 ] == 0x03 ))
           {
                if ( slowRateAdjust <= ( SLOWRATEMAXIMUM - SLOWRATEINCREMENT) )
                {
                    slowRateAdjust += SLOWRATEINCREMENT;
                    feedRateChanged = true; 
                }
                 
           }
           else 
           if (( buff[ 0 ] == 0x02 ) && (buff[ 1 ] == 0x01 ) && ( buff[ 2 ] == 0x03 ))
           {
                if ( slowRateAdjust >= SLOWRATEINCREMENT )
                {
                    slowRateAdjust -= SLOWRATEINCREMENT;   
                    feedRateChanged = true; 
                }
           }               
           else // Out of sync
           {
                inService = false;
               return;
           }

           break;       
    }
    inService = false; // Re-enable this function
}

// Interrupt routine called when a button is pressed
void buttonService()
{
    static bool inService = false; // Ensure we dont get called  again while servicing. Shouldn't happen.
    static short inputByte = 0;
    static enumState lastState = Idle;
    if ( inService == true )
        return;
    else
        inService = true;
        
    delay( 10 ); // Debounce
    
    short a = (PINB ^ 0x3F ); // Read the input pins for a button press
    //Serial.println(a, HEX);
 
    if  ( a == inputByte ) // Same key still pressed
    {
        inService = false;
        return;
    }

    // OK, so we have a valid button press/release 
    if ( a == 0 )  // A button has been released
    {
        if ( inputByte != 0 )
        {  
            switch ( inputByte )
            {
                case 0x01: // Switch button released
                            currentState = ButtonPressed;
                            // Serial.println( "Switch released" );
                            break;
                case 0x02: // Stop button released
                           // Serial.println( "Stop released" );
                            currentState = Stopping;
                           break;
                 case 0x04: // Fast left button released
                            currentState = Stopping;
                           break;
                case 0x08: // Slow left button released
                            currentState = Stopping;
                           break;
                case 0x10: // Slow right button released
                           currentState = Stopping;
                            break;
                case 0x20: // Fast left button released
                            currentState = Stopping;
                           break;
                default:    // More than one button released
                            // Serial.println( inputByte, HEX );
                            currentState = Stopping;
                           break;               
            }  
            shouldBeStepping = false;
            inputByte = 0;          
            inService = false;
            return;   
        }
    }
    else
    {   // Button pressed
        switch ( a )
        {
            case 0x01: // Switch button pressed
                        break;
            case 0x02: // Stop button pressed
                       currentState = Stopping;
                       break;
             case 0x04: // Fast left button pressed
                       if ( currentState == Idle )
                            currentState = MovingRightFast;
                        break;
            case 0x08: // Slow left button pressed
                        if ( currentState == Idle )
                            currentState = MovingRightSlow;
                       break;
            case 0x10: // Slow right button pressed
                       if ( currentState == Idle )
                            currentState = MovingLeftSlow;
                         break;
            case 0x20: // Fast left button pressed
                        if ( currentState == Idle )
                            currentState = MovingLeftFast;
                       break;
            default:    // More than one button pressed
                        // Serial.println( "Multiple pushed" );
                       break;               
        }
        inputByte = a;
       shouldBeStepping = true;
       if ( currentState == lastState )
       {
            inService = false;
            return;
       }
    }
    inService = false;
}


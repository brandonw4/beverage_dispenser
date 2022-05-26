Error Codes:
100 - Motor Timeout 
    DESCRIPTION:
        The motor has run for longer than the timeout value set (variable: const MOTOR_TIMEOUT_MILLIS). 
    CAUSES:
        This could be due to the plumbing/motor needing to be primed. (See priming instructions.)
        This could be due to an empty bottle.
        This could be due to a blocked line or a dead motor.
    FIX:
        Check above causes, and run again

101 - Motor/Bottle Out of Stock
    DESCRIPTION:
        The motor/bottle is empty or out of stock. (variable: bool bottle_status[])
    CAUSES:
        This can be triggered manually in the admin menu.
        This can be triggered if the motor timed out in the past. (Error 100)
    FIX:
        Check for Error 100 causes, then reset in admin menu.

102 - No Cup Detected

103 - Failed Authentication IMPLEMENT MESSAGES

    
    
/*
* The VF3AM is unique, the tcase uses a mechanical electrical hybrid system. 
* The tcase has a mechanical shifter that selects between High, Neutral, and Low range.
* The buttons inside the dash (diff lock, 4wd) are electrical inputs to control module.
* The module, based on the inputs, runs a motor which moves a mechanical sleeve inside the 
* transfer case to engage the different modes. The modes are in order, 2wd, awd, 4wd.
* Once a position is reached securely, a digital signal is sent back to the module from a seperate
* sensor, this allows the controller moduule to know when to stop the motor, and also to know which mode is currently engaged.
*/

typedef struct {
    int position_2wd; // This will be 1 when 2wd is engaged, 0 otherwise
    int position_awd; // This will be 1 when awd/4wd (4wd open) is engaged, 0 otherwise
    int position_4wd; // This will be 1 when 4wd (4wd locked) is engaged, 0 otherwise
    int differential_lock; // This will be 1 when the differential lock button is pressed (inside cabin)
    int mode_select; // This will be 1 when 4WD mode button on the shifter is pressed
    int low_range; /* This is an input from the transfer to let the MCU know that the shifter is in the LOW position, 
    immediately put the vehicle in 4WD  (locked) mode to protect the gears. */
    int neutral_safety_switch; /* This is an input from the transmission to let the MCU know that the transmission
    is in the NEUTRAL position, does not serve a real purpose since lo-range is mechanically actuated anyways*/
} inputs;

typedef struct {
    int target_mode; // This will be 0 for 2wd, 1 for awd, and 2 for 4wd
    int motor_direction; // This will be 2 for forward, 1 for reverse, and 0 for stop
    int motor_speed; // This will be a value from 0 to 100 representing the speed of the motor
} outputs;

void watchdog_setup();
void poll_mode(inputs *input_values);
void poll_buttons(inputs *input_values);
void update_outputs(inputs *input_values, outputs *output_values);

int main(){
    inputs input_values = {0, 0, 0, 0, 0, 0, 0};
    outputs output_values = {0, 0, 0};
    watchdog_setup();

    while(1){
        poll_mode(&input_values);
        poll_buttons(&input_values);
        update_outputs(&input_values, &output_values);
    }
    return 0;
}

void watchdog_setup(){
    /* Watchdog timer so that the system resets if for some reaason the 
    MCU gets stuck. Once reset, the default mode will be AWD */
}

void poll_mode(inputs *input_values){
    //mode select = button poll
    //diff lock = button poll
    
    input_values->mode_select = input_values->mode_select + input_values->differential_lock; //diff lock can be either 0 or 1, the mode select is also 0 or 1, so range is 0->2
}
void poll_buttons(inputs *input_values){
    
}

void update_outputs(inputs *input_values, outputs *output_values){
    /* The combinations:  Mode |             Condition                  | Output
                          2WD  | 4WD button OFF, Center Diff button OFF | 2WD Sensor HIGH
                          AWD  | 4WD button ON, Center Diff button OFF  | AWD Sensor HIGH
                          4WD  | 4WD button ON, Center Diff button ON   | 4WD Sensor HIGH
                        4WD LO | Low range button ON                    | 4WD Sensor HIGH
                        */
    switch(input_values->low_range){
        case 0:
        /* The normal case, do not force 4WD Locked*/
            switch(input_values->mode_select){
                case 0:
                /* 2WD Mode */
                    if(input_values->position_awd || input_values->position_4wd){
                        // If the system is currently in AWD or 4WD, we need to reverse the motor to get back to 2WD
                        output_values->motor_direction = 1; // Move in reverse
                        output_values->motor_speed = 50; // Set a moderate speed
                        if(!input_values->position_2wd){
                            output_values->motor_direction = 1; // Enable the motor
                        }
                        while(!input_values->position_2wd){
                            poll_mode(input_values); // Keep polling the mode to update the position sensors
                            poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
                        }
                        output_values->motor_direction = 0; // Disable the motor once we are in 2WD
                    }
                    else{
                        // If we are already in 2WD, do nothing
                        output_values->motor_direction = 0; // Ensure the motor is disabled
                    }
                break;

                case 1:
                /* AWD Mode */
                    if(input_values->position_4wd){
                        // If the system is currently in 4WD, we need to reverse the motor to get back to AWD
                        if(!input_values->position_awd){
                            output_values->motor_direction = 1; // Enable the motor
                        }
                        while(!input_values->position_awd){ // Wait until we are out of 4WD and in AWD
                            poll_mode(input_values); // Keep polling the mode to update the position sensors
                            poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
                        }
                        output_values->motor_direction = 0; // Disable the motor once we are in AWD
                    }
                    else if(input_values->position_2wd){
                        // If the system is currently in 2WD, we need to move the motor forward
                        if(!input_values->position_awd){
                        output_values->motor_direction = 1; // Enable the motor
                        }
                        while(!input_values->position_awd){
                            poll_mode(input_values); // Keep polling the mode to update the position sensors
                            poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
                        }
                        output_values->motor_direction = 0; // Disable the motor once the position is reached
                    }
                break;

                case 2:
                /* 4WD Mode */
                    output_values->target_mode = 2;
                    if(!input_values->position_4wd){
                        output_values->motor_direction = 2; // Move forward only if not already in 4WD
                    }
                    output_values->motor_speed = 50; // Set a moderate speed
                    while(!input_values->position_4wd){
                        poll_mode(input_values); // Keep polling the mode to update the position sensors
                        poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
                    }
                    output_values->motor_direction = 0; // Disable the motor once the position is reached
                break;

            }
        break;

        case 1:
        /* Force 4WD Locked due to Low Range */
            if(input_values->position_2wd){
                // If the system is currently in 2WD, we need to move the motor forward
                output_values->motor_direction = 2; // Enable the motor
            }
            else if(input_values->position_awd){
                // If the system is currently in AWD, we need to move the motor forward
                output_values->motor_direction = 2; // Enable the motor
            }
            output_values->target_mode = 2;
            if(!input_values->position_4wd){
                output_values->motor_direction = 2; // Move forward only if not already in 4WD
            }
            while(!input_values->position_4wd){
                poll_mode(input_values); // Keep polling the mode to update the position sensors
                poll_buttons(input_values); // Keep polling the buttons to update the mode select and low range
            }
            output_values->motor_direction = 0; // Disable the motor once the position is reached
        break;
    }

}
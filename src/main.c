#include "main.h"

/*
* The VF3AM is unique, the tcase uses a mechanical electrical hybrid system. 
* The tcase has a mechanical shifter that selects between High, Neutral, and Low range.
* The buttons inside the dash (diff lock, 4wd) are electrical inputs to control module.
* The module, based on the inputs, runs a motor which moves a mechanical sleeve inside the 
* transfer case to engage the different modes. The modes are in order, 2wd, awd, 4wd.
* Once a position is reached securely, a digital signal is sent back to the module from a seperate
* sensor, this allows the controller moduule to know when to stop the motor, and also to know which mode is currently engaged.
*/

extern volatile uint8_t machine_mode;
extern volatile uint8_t *run_target;
extern volatile uint8_t *p_run_targe;
int main(){
    machine_mode = 0;
    run_target = 0;
    inputs input_values = {0, 0, 0, 0, 0, 0, 0};
    outputs output_values = {0, 0, 50};
    state current_state = {0, 0};
    watchdog_setup();

    while(1){
        switch(machine_mode){
            case POLL:
                poll_mode(&input_values);
                update_outputs(&input_values, &output_values, &current_state, &p_run_target);
                break;
            case RUN:
                run_motor(run_target, &input_values);
        }
    }
    return 0;
}
#ifdef AT32F421K8U7
#include "at32f421.h"
#endif
#ifdef AT32F415K8U7
#include "at32f415.h"
#endif


//#include "systick.h"
#include <stdio.h>
#include "main.h"

#include "targets.h"
#include "signal.h"
#include "dshot.h"
#include "phaseouts.h"
#include "eeprom.h"
#include "sounds.h"
#include "ADC.h"
#ifdef USE_SERIAL_TELEMETRY
#include "serial_telemetry.h"
#endif
#include "IO.h"
#include "comparator.h"
#include "functions.h"
#include "peripherals.h"
#include "common.h"
#include "firmwareversion.h"



uint16_t comp_change_time = 0;


//firmware build options !! fixed speed and duty cycle modes are not to be used with sinusoidal startup !!

//#define FIXED_DUTY_MODE  // bypasses signal input and arming, uses a set duty cycle. For pumps, slot cars etc
//#define FIXED_DUTY_MODE_POWER 10     // 0-100 percent not used in fixed speed mode

//#define FIXED_SPEED_MODE  // bypasses input signal and runs at a fixed rpm using the speed control loop PID
//#define FIXED_SPEED_MODE_RPM  1000  // intended final rpm , ensure pole pair numbers are entered correctly in config tool.

//#define BRUSHED_MODE         // overrides all brushless config settings, enables two channels for brushed control
//#define GIMBAL_MODE     // also sinusoidal_startup needs to be on, maps input to sinusoidal angle.

//===========================================================================
//=============================  Defaults =============================
//===========================================================================

uint8_t drive_by_rpm = 0;
uint32_t MAXIMUM_RPM_SPEED_CONTROL = 12000;
uint32_t MINIMUM_RPM_SPEED_CONTROL = 2000;

 //assign speed control PID values values are x10000
 fastPID speedPid = {      //commutation speed loop time
 		.Kp = 10,
 		.Ki = 0,
 		.Kd = 100,
 		.integral_limit = 10000,
 		.output_limit = 50000
 };

 fastPID currentPid = {   // 1khz loop time
 		.Kp = 800,
 		.Ki = 0,
 		.Kd = 1000,
 		.integral_limit = 20000,
 		.output_limit = 100000
 };

 fastPID stallPid = {          //1khz loop time
 		.Kp = 2,
 		.Ki = 0,
 		.Kd = 50,
 		.integral_limit = 10000,
 		.output_limit = 50000
 };

uint16_t target_e_com_time_high;
uint16_t target_e_com_time_low;

char eeprom_layout_version = 2;
char dir_reversed = 0;
char comp_pwm = 1;
char VARIABLE_PWM = 1;
char bi_direction = 0;
char stuck_rotor_protection = 1;	// Turn off for Crawlers
char brake_on_stop = 0;
uint16_t stop_counter_for_alloff=0; //the counter for pwm off for 0 input
char stall_protection = 0;
char use_sin_start = 0;
char TLM_ON_INTERVAL = 0;
uint8_t telemetry_interval_ms = 30;
uint8_t TEMPERATURE_LIMIT = 255;  // degrees 255 to disable
char advance_level = 2;			// 7.5 degree increments 0 , 7.5, 15, 22.5)
uint16_t motor_kv = 2000;
char motor_poles = 14;
uint16_t CURRENT_LIMIT = 202;
uint8_t sine_mode_power = 5;
char drag_brake_strength = 10;		// Drag Brake Power when brake on stop is enabled
uint8_t driving_brake_strength = 10;
uint8_t dead_time_override = DEAD_TIME;
char sine_mode_changeover_thottle_level = 5;	// Sine Startup Range
uint16_t stall_protect_target_interval = TARGET_STALL_PROTECTION_INTERVAL;
char USE_HALL_SENSOR = 0;
uint16_t enter_sine_angle = 180;

//============================= Servo Settings ==============================
uint16_t servo_low_threshold = 1100;	// anything below this point considered 0
uint16_t servo_high_threshold = 1900;	// anything above this point considered 2000 (max)
uint16_t servo_neutral = 1500;
uint8_t servo_dead_band = 100;

//========================= Battery Cuttoff Settings ========================
char LOW_VOLTAGE_CUTOFF = 0;		// Turn Low Voltage CUTOFF on or off
uint16_t low_cell_volt_cutoff = 330;	// 3.3volts per cell

//=========================== END EEPROM Defaults ===========================

//typedef struct __attribute__((packed)) {
//  uint8_t version_major;
//  uint8_t version_minor;
//  char device_name[12];
//} firmware_info_s;

//firmware_info_s __attribute__ ((section(".firmware_info"))) firmware_info = {
//  version_major: VERSION_MAJOR,
//  version_minor: VERSION_MINOR,
//  device_name: FIRMWARE_NAME
//};

uint8_t EEPROM_VERSION;
//move these to targets folder or peripherals for each mcu
char RC_CAR_REVERSE = 0;   // have to set bidirectional, comp_pwm off and stall protection off, no sinusoidal startup
uint16_t ADC_CCR = 30;
uint16_t current_angle = 90;
uint16_t desired_angle = 90;


//assign current control PID values

//PID speedPid = {
//		.Kp = 0.001,
//		.Ki = 0,
//		.Kd = 0.010,
//		.integral_limit = 1,
//		.output_limit = 5
//};
//
//PID currentPid = {
//		.Kp = 0.04,
//		.Ki = 0,
//		.Kd = 0.05,
//		.integral_limit = 2,
//		.output_limit = 10
//};
//
//PID stallPid = {
//		.Kp = 0.0002,
//		.Ki = 0.00000001,
//		.Kd = 0.05,
//		.integral_limit = 1,
//		.output_limit = 5
//};
char boot_up_tune_played = 0;
uint16_t target_e_com_time = 0;
int16_t Speed_pid_output;
char use_speed_control_loop = 0;
float input_override = 0;
int16_t	use_current_limit_adjust = 2000;
char use_current_limit = 0;
float stall_protection_adjust = 0;

uint32_t MCU_Id = 0;
uint32_t REV_Id = 0;

uint16_t armed_timeout_count;
uint16_t reverse_speed_threshold = 1500;
uint8_t desync_happened = 0;
char maximum_throttle_change_ramp = 1;
int target_rampratio_mul10=0;
int targetmax_rpm=0;

char crawler_mode = 0;  // no longer used //
uint16_t velocity_count = 0;
uint16_t velocity_count_threshold = 75;

char low_rpm_throttle_limit = 1;

uint16_t low_voltage_count = 0;
uint16_t telem_ms_count;

char VOLTAGE_DIVIDER = TARGET_VOLTAGE_DIVIDER;     // 100k upper and 10k lower resistor in divider
uint16_t battery_voltage;  // scale in volts * 10.  1260 is a battery voltage of 12.60
char cell_count = 0;
char brushed_direction_set = 0;

uint16_t tenkhzcounter = 0;
float consumed_current = 0;
uint16_t smoothed_raw_current = 0;
uint16_t actual_current = 0;

char lowkv = 0;

uint16_t min_startup_duty = 120;
uint16_t sin_mode_min_s_d = 120;
char bemf_timeout = 10;

char startup_boost = 50;
char reversing_dead_band = 1;

uint16_t low_pin_count = 0;


char fast_accel = 1;
uint16_t last_duty_cycle = 0;
char play_tone_flag = 0;

typedef enum
{
  GPIO_PIN_RESET = 0U,
  GPIO_PIN_SET
}GPIO_PinState;

uint16_t startup_max_duty_cycle = 300 + DEAD_TIME;
uint16_t minimum_duty_cycle = DEAD_TIME;
uint16_t stall_protect_minimum_duty = DEAD_TIME;
char desync_check = 0;
char low_kv_filter_level = 20;

uint16_t tim1_arr = TIM1_AUTORELOAD;         // current auto reset value
uint16_t TIMER1_MAX_ARR = TIM1_AUTORELOAD;      // maximum auto reset register value
int VARIABLE_PWM_MIN_ARR= TIM1_AUTORELOAD / 2;
uint16_t duty_cycle_maximum = TIM1_AUTORELOAD;     //restricted by temperature or low rpm throttle protect
uint16_t low_rpm_level  = 20;        // thousand erpm used to set range for throttle resrictions
uint16_t high_rpm_level = 70;      //
uint16_t throttle_max_at_low_rpm  = 400;
uint16_t throttle_max_at_high_rpm = TIM1_AUTORELOAD;

uint16_t commutation_intervals[6] = {0};
uint32_t average_interval = 0;
uint32_t last_average_interval;
int e_com_time;

uint16_t ADC_smoothed_input = 0;
uint8_t degrees_celsius;
uint16_t converted_degrees;
uint8_t temperature_offset;
uint16_t ADC_raw_temp;
uint16_t ADC_raw_volts;
uint16_t ADC_raw_current;
uint16_t ADC_raw_input;
uint8_t adc_counter = 0;
char send_telemetry = 0;
char telemetry_done = 0;
char prop_brake_active = 0;

uint8_t eepromBuffer[176] ={0};

char dshot_telemetry = 0;

uint8_t last_dshot_command = 0;

char old_routine = 0;
int comm_timeout=50000;


uint16_t adjusted_input = 0;

#define TEMP30_CAL_VALUE            ((uint16_t*)((uint32_t)0x1FFFF7B8))
#define TEMP110_CAL_VALUE           ((uint16_t*)((uint32_t)0x1FFFF7C2))

uint16_t smoothedinput = 0;
const uint8_t numReadings = 30;     // the readings from the analog input
uint8_t readIndex = 0;              // the index of the current reading
int total = 0;
uint16_t readings[30];

uint8_t bemf_timeout_happened = 0;
uint8_t changeover_step = 5;
uint8_t filter_level = 5;
uint8_t running = 0;
uint16_t advance = 0;
uint8_t advancedivisor = 6;
char rising = 1;


int inner_step=1;
uint16_t last_adjusted_duty_cycle=0;
void setPwmRatio(){
	if(!crawler_mode){
		int bbemf_timeout_count=bemf_timeout_happened;
		//bbemf_timeout_count=8;
		int maxduty;
		if(bbemf_timeout_count>=4){
			maxduty=300;
		}else if(bbemf_timeout_count>=2){
			maxduty=600;
		}else if(bbemf_timeout_count>=1){
			maxduty=1200;
		}else{
			maxduty=2000;
		}
		maxduty=maxduty*tim1_arr/2000;
		if(last_adjusted_duty_cycle>maxduty){
			last_adjusted_duty_cycle=maxduty;
		}
	}
	
#if defined(USE_INNER_STEP)
	uint32_t targetduty;	
	if(inner_step<0){
		targetduty = (uint32_t)last_adjusted_duty_cycle*95/100;
	}else if(inner_step<=1){
		targetduty = (uint32_t)last_adjusted_duty_cycle*90/100;
		++inner_step;
	}else {
		targetduty = (uint32_t)last_adjusted_duty_cycle;
	}
	TMR1->c1dt = targetduty;
	TMR1->c2dt = targetduty;
	TMR1->c3dt = targetduty;
#else
	TMR1->c1dt = last_adjusted_duty_cycle;
	TMR1->c2dt = last_adjusted_duty_cycle;
	TMR1->c3dt = last_adjusted_duty_cycle;
#endif	
}

////Space Vector PWM ////////////////
//const int pwmSin[] ={128, 132, 136, 140, 143, 147, 151, 155, 159, 162, 166, 170, 174, 178, 181, 185, 189, 192, 196, 200, 203, 207, 211, 214, 218, 221, 225, 228, 232, 235, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 248, 249, 250, 250, 251, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 253, 252, 252, 251, 250, 250, 249, 248, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 248, 249, 250, 250, 251, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 253, 252, 252, 251, 250, 250, 249, 248, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 235, 232, 228, 225, 221, 218, 214, 211, 207, 203, 200, 196, 192, 189, 185, 181, 178, 174, 170, 166, 162, 159, 155, 151, 147, 143, 140, 136, 132, 128, 124, 120, 116, 113, 109, 105, 101, 97, 94, 90, 86, 82, 78, 75, 71, 67, 64, 60, 56, 53, 49, 45, 42, 38, 35, 31, 28, 24, 21, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 6, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 6, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 24, 28, 31, 35, 38, 42, 45, 49, 53, 56, 60, 64, 67, 71, 75, 78, 82, 86, 90, 94, 97, 101, 105, 109, 113, 116, 120, 124};

////Sine Wave PWM ///////////////////
int16_t pwmSin[] = {180,183,186,189,193,196,199,202,
		205,208,211,214,217,220,224,227,
		230,233,236,239,242,245,247,250,
		253,256,259,262,265,267,270,273,
		275,278,281,283,286,288,291,293,
		296,298,300,303,305,307,309,312,
		314,316,318,320,322,324,326,327,
		329,331,333,334,336,337,339,340,
		342,343,344,346,347,348,349,350,
		351,352,353,354,355,355,356,357,
		357,358,358,359,359,359,360,360,
		360,360,360,360,360,360,360,359,
		359,359,358,358,357,357,356,355,
		355,354,353,352,351,350,349,348,
		347,346,344,343,342,340,339,337,
		336,334,333,331,329,327,326,324,
		322,320,318,316,314,312,309,307,
		305,303,300,298,296,293,291,288,
		286,283,281,278,275,273,270,267,
		265,262,259,256,253,250,247,245,
		242,239,236,233,230,227,224,220,
		217,214,211,208,205,202,199,196,
		193,189,186,183,180,177,174,171,
		167,164,161,158,155,152,149,146,
		143,140,136,133,130,127,124,121,
		118,115,113,110,107,104,101,98,
		95,93,90,87,85,82,79,77,
		74,72,69,67,64,62,60,57,
		55,53,51,48,46,44,42,40,
		38,36,34,33,31,29,27,26,
		24,23,21,20,18,17,16,14,
		13,12,11,10,9,8,7,6,
		5,5,4,3,3,2,2,1,
		1,1,0,0,0,0,0,0,
		0,0,0,1,1,1,2,2,
		3,3,4,5,5,6,7,8,
		9,10,11,12,13,14,16,17,
		18,20,21,23,24,26,27,29,
		31,33,34,36,38,40,42,44,
		46,48,51,53,55,57,60,62,
		64,67,69,72,74,77,79,82,
		85,87,90,93,95,98,101,104,
		107,110,113,115,118,121,124,127,
		130,133,136,140,143,146,149,152,
		155,158,161,164,167,171,174,177};


//int sin_divider = 2;
int16_t phase_A_position;
int16_t phase_B_position;
int16_t phase_C_position;
uint16_t step_delay  = 100;
char stepper_sine = 0;
char forward = 1;
uint16_t gate_drive_offset = DEAD_TIME;

uint8_t stuckcounter = 0;
uint16_t k_erpm;
uint16_t e_rpm;      // electrical revolution /100 so,  123 is 12300 erpm

uint16_t adjusted_duty_cycle;

uint8_t bad_count = 0;
uint8_t bad_count_threshold = CPU_FREQUENCY_MHZ / 24;
uint8_t dshotcommand;
uint16_t armed_count_threshold = 1000;

char armed = 0;
uint16_t zero_input_count = 0;

uint16_t input = 0;
uint16_t newinput =0;
char inputSet = 0;
char dshot = 0;
char servoPwm = 0;
uint32_t zero_crosses;

uint8_t zcfound = 0;

uint8_t bemfcounter;
uint8_t min_bemf_counts_up = TARGET_MIN_BEMF_COUNTS;
uint8_t min_bemf_counts_down = TARGET_MIN_BEMF_COUNTS;

uint16_t lastzctime;
uint16_t thiszctime;

uint16_t duty_cycle = 0;
char step = 1;
uint16_t commutation_interval = 12500;
uint16_t waitTime = 0;
uint16_t signaltimeout = 0;
uint8_t ubAnalogWatchdogStatus = RESET;


void checkForHighSignal(){
changeToInput();
	gpio_mode_QUICK(INPUT_PIN_PORT, GPIO_MODE_INPUT, GPIO_PULL_DOWN, INPUT_PIN);
delayMicros(100);
for(int i = 0 ; i < 1000; i ++){
	 if(!((INPUT_PIN_PORT->idt & INPUT_PIN))){  // if the pin is low for 5 checks out of 100 in  100ms or more its either no signal or signal. jump to application
		 low_pin_count++;
	 }
     delayMicros(10);
}
gpio_mode_QUICK(INPUT_PIN_PORT, GPIO_MODE_MUX, GPIO_PULL_NONE, INPUT_PIN);
	 if(low_pin_count > 5){
		 return;      // its either a signal or a disconnected pin
	 }else{
		allOff();
		NVIC_SystemReset();
	 }
}



float doPidCalculations(struct fastPID *pidnow, int actual, int target){

	pidnow->error = actual - target;
	pidnow->integral = pidnow->integral + pidnow->error*pidnow->Ki + pidnow->last_error*pidnow->Ki;
	if(pidnow->integral > pidnow->integral_limit){
		pidnow->integral = pidnow->integral_limit;
	}
	if(pidnow->integral < -pidnow->integral_limit){
		pidnow->integral = -pidnow->integral_limit;
	}

	pidnow->derivative = pidnow->Kd * (pidnow->error - pidnow->last_error);
	pidnow->last_error = pidnow->error;

	pidnow->pid_output = pidnow->error*pidnow->Kp + pidnow->integral + pidnow->derivative;


	if (pidnow->pid_output>pidnow->output_limit){
		pidnow->pid_output = pidnow->output_limit;
	}if(pidnow->pid_output <-pidnow->output_limit){
		pidnow->pid_output = -pidnow->output_limit;
	}
	return pidnow->pid_output;

}


void loadEEpromSettings(){
	   read_flash_bin( eepromBuffer , EEPROM_START_ADD , 176);

	   if(eepromBuffer[17] == 0x01){
	 	  dir_reversed =  1;
	   }else{
		   dir_reversed = 0;
	   }
	   if(eepromBuffer[18] == 0x01){
	 	  bi_direction = 1;
	   }else{
		  bi_direction = 0;
	   }
	   if(eepromBuffer[19] == 0x01){
	 	  use_sin_start = 1;
	 //	 min_startup_duty = sin_mode_min_s_d;
	   }
	   if(eepromBuffer[20] == 0x01){
	  	  comp_pwm = 1;
	    }else{
	    	comp_pwm = 0;
	    }
	   if(eepromBuffer[21] == 0x01){
		   VARIABLE_PWM = 1;
	    }else{
	    	VARIABLE_PWM = 0;
	    }
	   if(eepromBuffer[22] == 0x01){
		   stuck_rotor_protection = 1;
	    }else{
	    	stuck_rotor_protection = 0;
	    }
	   if(eepromBuffer[23] < 4){
		   advance_level = eepromBuffer[23];
	    }else{
	    	advance_level = 2;  // * 7.5 increments
	    }

	   if(eepromBuffer[24] < 49 && eepromBuffer[24] > 7){
		   if(eepromBuffer[24] < 49 && eepromBuffer[24] > 23){
			   TIMER1_MAX_ARR = map (eepromBuffer[24], 24, 144, TIM1_AUTORELOAD,TIM1_AUTORELOAD/6);
		   }
		   if(eepromBuffer[24] < 24 && eepromBuffer[24] > 11){
			   TIMER1_MAX_ARR = map (eepromBuffer[24], 12, 24, TIM1_AUTORELOAD *2 ,TIM1_AUTORELOAD);
		   }
		   if(eepromBuffer[24] < 12 && eepromBuffer[24] > 7){
			   TIMER1_MAX_ARR = map (eepromBuffer[24], 7, 16, TIM1_AUTORELOAD *3 ,TIM1_AUTORELOAD/2*3);
		   }
		   TMR1->pr  = TIMER1_MAX_ARR;
		   throttle_max_at_high_rpm = TIMER1_MAX_ARR;
		   duty_cycle_maximum = TIMER1_MAX_ARR;
	    }else{
	    	tim1_arr = TIM1_AUTORELOAD;
	    	TMR1->pr = tim1_arr;
	    }

	   if(eepromBuffer[25] < 151 && eepromBuffer[25] > 49){
	   min_startup_duty = (eepromBuffer[25]/2 + DEAD_TIME) * TIMER1_MAX_ARR / 2000;
	   minimum_duty_cycle = (eepromBuffer[25]/ 4 + DEAD_TIME/4) * TIMER1_MAX_ARR / 2000 ;
	   stall_protect_minimum_duty = minimum_duty_cycle+10;
	    }else{
	    	min_startup_duty = 150;
	    	minimum_duty_cycle = (min_startup_duty / 2) + 10;
	    }
      motor_kv = (eepromBuffer[26] * 40) + 20;
      motor_poles = eepromBuffer[27];
	   if(eepromBuffer[28] == 0x01){
		   brake_on_stop = 1;
	    }else{
	    	brake_on_stop = 0;
	    }
	   if(eepromBuffer[29] == 0x01){
		   stall_protection = 1;
	    }else{
	    	stall_protection = 0;
	    }
	   setVolume(5);
	   if(eepromBuffer[1] > 0){             // these commands weren't introduced until eeprom version 1.

		   if(eepromBuffer[30] > 11){
			   setVolume(5);
		   }else{
			   setVolume(eepromBuffer[30]);
		   }
		   if(eepromBuffer[31] == 0x01){
			   TLM_ON_INTERVAL = 1;
		   }else{
			   TLM_ON_INTERVAL = 0;
		   }
		   servo_low_threshold = (eepromBuffer[32]*2) + 750; // anything below this point considered 0
		   servo_high_threshold = (eepromBuffer[33]*2) + 1750;;  // anything above this point considered 2000 (max)
		   servo_neutral = (eepromBuffer[34]) + 1374;
		   servo_dead_band = eepromBuffer[35];

		   if(eepromBuffer[36] == 0x01){
			   LOW_VOLTAGE_CUTOFF = 1;
		   }else{
			   LOW_VOLTAGE_CUTOFF = 0;
		   }

		   low_cell_volt_cutoff = eepromBuffer[37] + 250; // 2.5 to 3.5 volts per cell range
		   if(eepromBuffer[38] == 0x01){
			   RC_CAR_REVERSE = 1;
		   }else{
			   RC_CAR_REVERSE = 0;
		   }
		   if(eepromBuffer[39] == 0x01){
#ifdef HAS_HALL_SENSORS
			   USE_HALL_SENSOR = 1;
#else
			   USE_HALL_SENSOR = 0;
#endif
		   }else{
			   USE_HALL_SENSOR = 0;
		   }
	   if(eepromBuffer[40] > 4 && eepromBuffer[40] < 26){            // sine mode changeover 5-25 percent throttle
       sine_mode_changeover_thottle_level = eepromBuffer[40];
	   }
	   if(eepromBuffer[41] > 0 && eepromBuffer[41] < 11){        // drag brake 1-10
       drag_brake_strength = eepromBuffer[41];
	   }
	   
	   if(eepromBuffer[42] > 0 && eepromBuffer[42] < 10){        // motor brake 1-9
       driving_brake_strength = eepromBuffer[42];
	   dead_time_override = DEAD_TIME + (150 - (driving_brake_strength * 10));
	   if(dead_time_override > 200){
	   dead_time_override = 200;
	   }
	   min_startup_duty = eepromBuffer[25] + dead_time_override;
	   minimum_duty_cycle = eepromBuffer[25]/2 + dead_time_override;
	   throttle_max_at_low_rpm  = throttle_max_at_low_rpm + dead_time_override;
	   startup_max_duty_cycle = startup_max_duty_cycle  + dead_time_override;
	//   TIMER_CCHP(TIMER0)|= TIMER_CCHP_DTCFG & dead_time_override;
	   }
	   
	   if(eepromBuffer[43] >= 70 && eepromBuffer[43] <= 140){ 
	   TEMPERATURE_LIMIT = eepromBuffer[43];
	   
	   }
	   
	   if(eepromBuffer[44] > 0 && eepromBuffer[44] < 100){
	   CURRENT_LIMIT = eepromBuffer[44] * 2;
	   use_current_limit = 1;
	   
	   }
	   if(eepromBuffer[45] > 0 && eepromBuffer[45] < 11){ 
	   sine_mode_power = eepromBuffer[45];
	   }
	   /*if(eepromBuffer[46] >= 0 && eepromBuffer[46] < 10){
		   switch (eepromBuffer[46]){
		   case AUTO_IN:
			   dshot= 0;
			   servoPwm = 0;
			   EDT_ARMED = 1;
			   break;
		   case DSHOT_IN:
			   dshot = 1;
			   EDT_ARMED = 1;
			   break;
		   case SERVO_IN:
			   servoPwm = 1;
			   break;
		   case SERIAL_IN:
			   break;
		   case EDTARM:
			   EDT_ARM_ENABLE = 1;
			   EDT_ARMED = 0;
			   dshot = 1;
			   break;
		   };
	   }else{
		   dshot = 0;
		   servoPwm = 0;
		   EDT_ARMED = 1;
	   }*/
       if(motor_kv < 300){
		   low_rpm_throttle_limit = 0;
	   }
		if((motor_kv < 1500)|| (stall_protection)){
			min_bemf_counts_up = TARGET_MIN_BEMF_COUNTS *2;
			min_bemf_counts_down = TARGET_MIN_BEMF_COUNTS *2;
		}
	   low_rpm_level  = motor_kv / 100 / (32 / motor_poles);
	   high_rpm_level = motor_kv / 17 / (32/motor_poles);
	   }
	   reverse_speed_threshold =  map(motor_kv, 300, 3000, 2500 , 1250);
	if(!comp_pwm){
		bi_direction = 0;
	}
	
	VARIABLE_PWM_MIN_ARR=map(motor_kv, 2800, 4800, TIMER1_MAX_ARR/2 , TIMER1_MAX_ARR/3);



}

void saveEEpromSettings(){    

   eepromBuffer[1] = eeprom_layout_version;
   if(dir_reversed == 1){
	   eepromBuffer[17] = 0x01;
   }else{
	   eepromBuffer[17] = 0x00;
   }
   if(bi_direction == 1){
	   eepromBuffer[18] = 0x01;
      }else{
    	  eepromBuffer[18] = 0x00;
      }
   if(use_sin_start == 1){
	   eepromBuffer[19] = 0x01;
      }else{
    	  eepromBuffer[19] = 0x00;
      }

   if(comp_pwm == 1){
	   eepromBuffer[20] = 0x01;
      }else{
    	  eepromBuffer[20] = 0x00;
      }
   if(VARIABLE_PWM == 1){
	   eepromBuffer[21] = 0x01;
      }else{
    	  eepromBuffer[21] = 0x00;
      }
   if(stuck_rotor_protection == 1){
	   eepromBuffer[22] = 0x01;
      }else{
    	  eepromBuffer[22] = 0x00;
      }
   eepromBuffer[23] = advance_level;
   save_flash_nolib(eepromBuffer, 176, EEPROM_START_ADD);
}



void getSmoothedInput() {

		total = total - readings[readIndex];
		readings[readIndex] = commutation_interval;
		total = total + readings[readIndex];
		readIndex = readIndex + 1;
		if (readIndex >= numReadings) {
			readIndex = 0;
		}
		smoothedinput = total / numReadings;


}

void getBemfState(){
	uint8_t current_state = 0;
#ifdef MCU_AT415
	 current_state = !(CMP->ctrlsts1_bit.cmp1value);  // polarity reversed
#else
    current_state = !(CMP->ctrlsts_bit.cmpvalue);  // polarity reversed
#endif
    if (rising){
    	if (current_state){
    		bemfcounter++;
    		}else{
    		bad_count++;
    		if(bad_count > bad_count_threshold){
    		bemfcounter = 0;
    		}
   	}
    }else{
    	if(!current_state){
    		bemfcounter++;
    	}else{
    		bad_count++;
    	    if(bad_count > bad_count_threshold){
    	    bemfcounter = 0;
    	  }
    	}
    }
}


void commutate(){
	inner_step=0;
	commutation_intervals[step-1] = commutation_interval;
	e_com_time = (commutation_intervals[0] + commutation_intervals[1] + commutation_intervals[2] + commutation_intervals[3] + commutation_intervals[4] +commutation_intervals[5]) >> 1;  // COMMUTATION INTERVAL IS 0.5US INCREMENTS

	if (forward == 1){
		step++;
		if (step > 6) {
			step = 1;
			desync_check = 1;
		}
		rising = step % 2;
	}else{
		step--;
		if (step < 1) {
			step = 6;
			desync_check = 1;
		}
		rising = !(step % 2);
	}
/***************************************************/
	UTILITY_TIMER->cval = 0;
	if(!prop_brake_active){
	comStep(step);
	}
  comp_change_time = UTILITY_TIMER->cval;
/****************************************************/
	changeCompInput();
  if(average_interval > 2000 && (stall_protection || RC_CAR_REVERSE)){
	old_routine = 1;
}
	bemfcounter = 0;
	zcfound = 0;
	  if(use_speed_control_loop && running){
	  input_override += doPidCalculations(&speedPid, e_com_time, target_e_com_time)/10000;
	  if(input_override > 2000){
		  input_override = 2000;
	  }
	  if(input_override < 0){
		  input_override = 0;
	  }
	  if(zero_crosses < 100){
		  speedPid.integral = 0;
	  }
}		
}

void PeriodElapsedCallback(){
	    COM_TIMER->iden &= ~TMR_OVF_INT; // disable interrupt         

			commutate();

			if(!old_routine){
			enableCompInterrupts();     // enable comp interrupt
			}
			if(zero_crosses<10000){
			zero_crosses++;
			
			}

}
#if defined(USE_DEBUG)
int max_bad_count=0;
int debug_passed_time=0;
int min_commutation_interval=999999;
int debug_desync_max_diff_ratio=0;
#endif

void interruptRoutine(){
	if (average_interval > 125){
		if ((INTERVAL_TIMER->cval < 125) && (duty_cycle < 600) && (zero_crosses < 500)){    //should be impossible, desync?exit anyway
			return;
		}
		if(commutation_interval>1000){
			if ((INTERVAL_TIMER->cval < (commutation_interval / 2 ))){
				return;
			}
		}else{
			if ((INTERVAL_TIMER->cval < (commutation_interval / 4))){
				return;
			}
		}
		stuckcounter++;             // stuck at 100 interrupts before the main loop happens again.
		if (stuckcounter > 100){
			maskPhaseInterrupts();
			zero_crosses = 0;
			return;
		}
	}
	if(INTERVAL_TIMER->cval < 10){//about 285710 rpm for 14-poles motor
		return;
	}

		/*int prevtime=INTERVAL_TIMER->cval;
		for(int i=0;i<2;++i){
			while(prevtime==INTERVAL_TIMER->cval){
				if((rising && CMP_VALUE) || (!rising && !CMP_VALUE)){
					return;
				}
			}
			prevtime=INTERVAL_TIMER->cval;
		}*/
#if defined(FAST_INTERRUPT)
		thiszctime = INTERVAL_TIMER->cval;
		if(rising){
			for(int i = 0; i < filter_level; i++){
					if(CMP_VALUE){
						return;
					}
			}
		}else{
			for(int i = 0; i < filter_level; i++){
					if(!CMP_VALUE){
						return;
					}
			}
		}
#else
		for(int n=0;n<2;++n){
			thiszctime = INTERVAL_TIMER->cval;
			int badcount=0;
			if(rising){
				for(int i = 0; i < filter_level*4; i++){
						if(CMP_VALUE){
							++badcount;
						}
				}
			}else{
				for(int i = 0; i < filter_level*4; i++){
						if(!CMP_VALUE){
							++badcount;
						}
				}
			}
#if defined(USE_DEBUG)
			if(badcount>max_bad_count){
				max_bad_count=badcount;
			}
#endif

			if(badcount>filter_level){//if badcount > 1/4*total test
				if(n==1){
					return;
				}
			}else{
				break;
			}
		}
#endif
		
			int passed_time=INTERVAL_TIMER->cval - thiszctime;
			INTERVAL_TIMER->cval = passed_time;
#if defined(USE_DEBUG)
			debug_passed_time=passed_time;
#endif

			int c0=commutation_interval;
		
		  commutation_interval = thiszctime;
		  //commutation_interval = (( 3*commutation_interval) + thiszctime)>>2;
		
			int c1=commutation_interval;

			
			//commutation_interval = (commutation_interval+thiszctime)>>1;
			//commutation_interval = (( 3*commutation_interval) + thiszctime)>>2;			
			int next_commutation_interval=commutation_interval;
		
#if defined(USE_DEBUG)
			if(min_commutation_interval>commutation_interval){
				min_commutation_interval=commutation_interval;
			}
#endif
			
			if(fast_accel){
					int c2;
					if(average_interval>800){
						c2=c1;
					}else if(average_interval>300){
						c2=c1/2;
					}else{
						c2=c1/3;
					}
					next_commutation_interval=c2;
			}
			advance = (next_commutation_interval>>3) * advance_level;   // 60 divde 8 7.5 degree increments
			waitTime = (next_commutation_interval >>1)  - advance;
			
			inner_step=-1;
			maskPhaseInterrupts();						
			//waitTime = waitTime >> fast_accel;
			COM_TIMER->cval = INTERVAL_TIMER->cval;//already passed INTERVAL_TIMER->cval
			if(waitTime<INTERVAL_TIMER->cval+2){
				waitTime=INTERVAL_TIMER->cval+2;
			}
			COM_TIMER->pr = waitTime;
			COM_TIMER->ists = 0x00;
			COM_TIMER->iden |= TMR_OVF_INT;
}

void startMotor() {
	if (running == 0){
	commutate();
	commutation_interval = 10000;
		INTERVAL_TIMER->cval = 5000;
	running = 1;
	}
	enableCompInterrupts();
}

void stopStuckMotor(){
		allOff();
		maskPhaseInterrupts();
		input = 0;
		bemf_timeout_happened = 102;
#ifdef USE_RGB_LED
			GPIOB->BRR = LL_GPIO_PIN_8; // on red
			GPIOB->BSRR = LL_GPIO_PIN_5;  //
			GPIOB->BSRR = LL_GPIO_PIN_3;
#endif
}

void zcfoundroutine();
void commutateProcess();
void IntervalTimerOverflowProcess(int commutateRightNow){
	bemf_timeout_happened++;
	//set pwm as bemf_timeout_happened changed
	setPwmRatio();
	const int bemf_timeout=100;
	if(bemf_timeout_happened > bemf_timeout * ( 1 + (crawler_mode*100)) && stuck_rotor_protection){
		stopStuckMotor();
		return;
	}

	maskPhaseInterrupts();
	old_routine = 1;
	if(input < 48){
	 running = 0;
	}
	zero_crosses = 0;
	if(commutateRightNow){
		commutateProcess();
	}else{
		zcfoundroutine();
	}
	
 // if(stall_protection){
	 // min_startup_duty = 130;
	 // minimum_duty_cycle = minimum_duty_cycle + 10;
	 // if(minimum_duty_cycle > 80){
		 // minimum_duty_cycle = 80;
	 // }
 // }

}

//interval timer overflow process, in case main loop is stucked and the zc timeout escaped
int intervaloverflowcount=0;
void IntervalTimerOverflow(){
	if(running){
		IntervalTimerOverflowProcess(1);
		#if defined(USE_DEBUG)
			++intervaloverflowcount;
		#endif
	}
}


void tenKhzRoutine(){
	tenkhzcounter++;
	
		  if(boot_up_tune_played == 0){
			if(tenkhzcounter > 1000){ 
			playStartupTune();
			boot_up_tune_played = 1;
			}
	  }
	
	if(tenkhzcounter > 10000){      // 1s sample interval
		consumed_current = (float)actual_current/360 + consumed_current;
							switch (dshot_extended_telemetry){

					case 1:
					    send_extended_dshot = 0b0010 << 8 | degrees_celsius;
					    dshot_extended_telemetry = 2;
					break;
					case 2:
					    send_extended_dshot = 0b0110 << 8 | (uint8_t)actual_current/50;
					    dshot_extended_telemetry = 3;
					break;
					case 3:
					    send_extended_dshot = 0b0100 << 8 | (uint8_t)(battery_voltage/25);
					    dshot_extended_telemetry = 1;
					break;

					}
		tenkhzcounter = 0;
	}
if(!armed){
	if(inputSet){
		if(adjusted_input == 0){
			armed_timeout_count++;
			if(armed_timeout_count > 10000){    // one second
				if(zero_input_count > 30){
					armed = 1;
		//			receiveDshotDma();
					#ifdef USE_RGB_LED
									GPIOB->BRR = LL_GPIO_PIN_3;    // turn on green
									GPIOB->BSRR = LL_GPIO_PIN_8;   // turn on green
									GPIOB->BSRR = LL_GPIO_PIN_5;
					#endif
									if(cell_count == 0 && LOW_VOLTAGE_CUTOFF){
										cell_count = battery_voltage / 370;
										for (int i = 0 ; i < cell_count; i++){
										playInputTune();
										delayMillis(100);
		//			  			 IWDG_ReloadCounter(IWDG);
										}
										}else{
										playInputTune();
										}
									if(!servoPwm){
										RC_CAR_REVERSE = 0;
									}
						int batcellcount=battery_voltage / 370;
						if(batcellcount<=0){
							batcellcount=4;
						}
						targetmax_rpm=(int)motor_kv*batcellcount*42/10;
						int targetrpmlow=2000*6*42/10;
						int targetrpmhigh=3500*6*42/10;
						target_rampratio_mul10=map(targetmax_rpm,targetrpmlow,targetrpmhigh,10,6);
				}else{
					inputSet = 0;
					armed_timeout_count =0;
				}
			}
		}else{
			armed_timeout_count =0;
		}
	}
}

	if(TLM_ON_INTERVAL){
		telem_ms_count++;
		if(telem_ms_count>telemetry_interval_ms*10){
			send_telemetry = 1;
			telem_ms_count = 0;
		}
	}
#ifndef BRUSHED_MODE
	if(!stepper_sine){
	  if (input >= 47 +(80*use_sin_start) && armed){
			stop_counter_for_alloff=0;
		  if (running == 0){
			  allOff();
			  if(!old_routine){
			 startMotor();
			  }
			  running = 1;
			  last_duty_cycle = min_startup_duty;

		  }
	  if(use_sin_start){
		duty_cycle = map(input, 137, 2047, minimum_duty_cycle, TIMER1_MAX_ARR);
  	  }else{
	 	 duty_cycle = map(input, 47, 2047, minimum_duty_cycle, TIMER1_MAX_ARR);
	  }
	  if(tenkhzcounter%10 == 0){     // 1khz PID loop
		  if(use_current_limit && running){
			use_current_limit_adjust -= (int16_t)(doPidCalculations(&currentPid, actual_current, CURRENT_LIMIT*100)/10000);
			if(use_current_limit_adjust < minimum_duty_cycle){
				use_current_limit_adjust = minimum_duty_cycle;
			}
			if(use_current_limit_adjust > duty_cycle){
				use_current_limit_adjust = duty_cycle;
			}

	  }

		  	 if(stall_protection && running ){  // this boosts throttle as the rpm gets lower, for crawlers and rc cars only, do not use for multirotors.
		  		 stall_protection_adjust += (doPidCalculations(&stallPid, commutation_interval, stall_protect_target_interval))/10000;
		  					 if(stall_protection_adjust > 150){
		  						stall_protection_adjust = 150;
		  					 }
		  					 if(stall_protection_adjust <= 0){
		  						stall_protection_adjust = 0;
		  					 }
		  	 }
	  }
	  if(!RC_CAR_REVERSE){
		  prop_brake_active = 0;
	  }
	  }
	  if (input < 47 + (80*use_sin_start)){
		if(play_tone_flag != 0){
			if(play_tone_flag == 1){
				playDefaultTone();

			}if(play_tone_flag == 2){
				playChangedTone();
			}
			play_tone_flag = 0;
		}

		  if(!comp_pwm){
			duty_cycle = 0;
			if(!running){
				old_routine = 1;
				zero_crosses = 0;
				  if(brake_on_stop){
					  fullBrake();
				  }else{
					  if(!prop_brake_active){
					  allOff();
					  }
				  }
			}
			if (RC_CAR_REVERSE && prop_brake_active) {
#ifndef PWM_ENABLE_BRIDGE
					duty_cycle = getAbsDif(1000, newinput) + 1000;
					if(duty_cycle == 2000){
						fullBrake();
					}else{
						proportionalBrake();
					}
#endif
					}
		  }else{
		  if (!running){
			  duty_cycle = 0;
			  old_routine = 1;
			  zero_crosses = 0;
			  bad_count = 0;
			  	  if(brake_on_stop){
			  		  if(!use_sin_start){
#ifndef PWM_ENABLE_BRIDGE				
			  			  duty_cycle = (TIMER1_MAX_ARR-19) + drag_brake_strength*2;
			  			  proportionalBrake();
			  			  prop_brake_active = 1;
#else
	//todo add proportional braking for pwm/enable style bridge.
#endif
			  		  }
			  	  }else{							
							if(stop_counter_for_alloff>1000){
								allOff();
							}else{
								++stop_counter_for_alloff;
							}
			  		  duty_cycle = 0;
			  	  }
		  }

		  	  phase_A_position = ((step-1) * 60) + enter_sine_angle;
		  	  if(phase_A_position > 359){
		  		  phase_A_position -= 360;
		  	  }
		  	  phase_B_position = phase_A_position +  119;
		  	  if(phase_B_position > 359){
		  		  phase_B_position -= 360;
		  	  }
		  	  phase_C_position = phase_A_position + 239;
		  	 if(phase_C_position > 359){
		  	 phase_C_position -= 360;
		  	 }

		 	  if(use_sin_start == 1){
		    	 stepper_sine = 1;
		 	  }
		  }
		  }
if(!prop_brake_active){
 if (zero_crosses < (20 >> stall_protection)){
	   if (duty_cycle < min_startup_duty){
	   duty_cycle = min_startup_duty;

	   }
	   if (duty_cycle > startup_max_duty_cycle){
		   duty_cycle = startup_max_duty_cycle;
	   }
 }

	 if (duty_cycle > duty_cycle_maximum){
		 duty_cycle = duty_cycle_maximum;
	 }
	 if(use_current_limit){
		 if (duty_cycle > use_current_limit_adjust){
			 duty_cycle = use_current_limit_adjust;
		 }
	 }

	 if(stall_protection_adjust > 0){

		 duty_cycle = duty_cycle + (uint16_t)stall_protection_adjust;
	 }

	 if(maximum_throttle_change_ramp){
		 	int ramp_ratio;
		  if(duty_cycle<TIMER1_MAX_ARR/10*1){
				ramp_ratio=1;
			}else {
				if(average_interval>500){
					ramp_ratio=2;
				}else{
					ramp_ratio=4;
				}
			}
				
		  int max_duty_cycle_change = 2;
	//	max_duty_cycle_change = map(k_erpm, low_rpm_level, high_rpm_level, 1, 40);
			if(average_interval > 500){
				max_duty_cycle_change = 15*ramp_ratio;
			}else{
				max_duty_cycle_change = 45*ramp_ratio;
			}
#if defined(USE_KV_VBAT_RAMP)
			if(target_rampratio_mul10>0){
				max_duty_cycle_change=max_duty_cycle_change*target_rampratio_mul10/10;
				if(max_duty_cycle_change<2)max_duty_cycle_change=2;
			}
#endif
			
			int max_duty_cycle_change_dec;
			if(average_interval > 500){
				max_duty_cycle_change_dec = 20*ramp_ratio;
			}else{
				max_duty_cycle_change_dec = 60*ramp_ratio;
			}
			
			 if ((duty_cycle - last_duty_cycle) > max_duty_cycle_change){
				duty_cycle = last_duty_cycle + max_duty_cycle_change;
				fast_accel = 1;
				/*if(commutation_interval > 500){
					fast_accel = 1;
				}else{
					fast_accel = 0;
				}*/
			}else if ((last_duty_cycle - duty_cycle) > max_duty_cycle_change_dec){
				duty_cycle = last_duty_cycle - max_duty_cycle_change_dec;
				fast_accel = 0;
			}else{
				fast_accel = 0;
			}
			if(average_interval>1000){
				if(duty_cycle>TIMER1_MAX_ARR/2){
					duty_cycle=TIMER1_MAX_ARR/2;
				}
			}
			/*if(e_com_time > 10 * 1000000/(targetmax_rpm/60*motor_poles/2)){// 1/10 max speed
				if(duty_cycle>TIMER1_MAX_ARR/2){//limit to half max duty
					duty_cycle=TIMER1_MAX_ARR/2;
				}
			}*/
		}
	}
		if ((armed && running) && input > 47){
			if(VARIABLE_PWM){
				//tim1_arr = map(commutation_interval, 96, 200, TIMER1_MAX_ARR/2, TIMER1_MAX_ARR);				
				tim1_arr = map(commutation_interval, 150, 750, VARIABLE_PWM_MIN_ARR, TIMER1_MAX_ARR);
			}
			adjusted_duty_cycle = ((duty_cycle * tim1_arr)/TIMER1_MAX_ARR)+1;
		}else{
				if(prop_brake_active){
					adjusted_duty_cycle = TIMER1_MAX_ARR - ((duty_cycle * tim1_arr)/TIMER1_MAX_ARR)+1;
				}else{
				adjusted_duty_cycle = DEAD_TIME * running;
				}
	    }
		last_duty_cycle = duty_cycle;
		TMR1->pr = tim1_arr;

	last_adjusted_duty_cycle=adjusted_duty_cycle;
	setPwmRatio();
	}
average_interval = e_com_time / 3;
if(desync_check && zero_crosses > 10){
//	if(average_interval < last_average_interval){
//
//	}
		int diff=getAbsDif(last_average_interval,average_interval);
#if defined(USE_DEBUG)
		int diffratio=diff*10000/average_interval;
		if( average_interval < 2000 && debug_desync_max_diff_ratio<diffratio){
			debug_desync_max_diff_ratio=diffratio;
		}
#endif
		if((diff > average_interval>>1) && (average_interval < 2000)){ //throttle resitricted before zc 20.
		zero_crosses = 0;
		desync_happened ++;
		running = 0;
		old_routine = 1;
			if(zero_crosses > 100){
				average_interval = 5000;
			}
		last_duty_cycle = min_startup_duty/2;
		}
		desync_check = 0;
//	}
	last_average_interval = average_interval;
	}
//#ifndef MCU_F031
//if(commutation_interval > 400){
//	   NVIC_SetPriority(IC_DMA_IRQ_NAME, 0);
//	   NVIC_SetPriority(ADC_CMP_IRQn, 1);
//}else{
//	NVIC_SetPriority(IC_DMA_IRQ_NAME, 1);
//	NVIC_SetPriority(ADC_CMP_IRQn, 0);
//}
//#endif   //mcu f031

#endif // ndef brushed_mode

if(send_telemetry){
#ifdef	USE_SERIAL_TELEMETRY
	  makeTelemPackage(degrees_celsius,
			           battery_voltage,
					   actual_current,
	  				   (uint16_t)consumed_current,
	  					e_rpm);
	  send_telem_DMA();
	  send_telemetry = 0;
#endif
	}
#if defined(FIXED_DUTY_MODE) || defined(FIXED_SPEED_MODE)

//		if(INPUT_PIN_PORT->IDR & INPUT_PIN){
//			signaltimeout ++;
//			if(signaltimeout > 10000){
//				NVIC_SystemReset();
//			}
//		}else{
//			signaltimeout = 0;
//		}
#else
	  
		signaltimeout++;
	
		if(signaltimeout > 5000) { // quarter second timeout when armed half second for servo;
			if(armed){
				allOff();
				armed = 0;
				input = 0;
				inputSet = 0;
				zero_input_count = 0;
				
				TMR1->c1dt = 0;
			  TMR1->c2dt = 0;
				TMR1->c3dt = 0;

				IC_TIMER_REGISTER->div = 0;
				IC_TIMER_REGISTER->cval = 0;

				for(int i = 0; i < 64; i++){
					dma_buffer[i] = 0;
				}
				NVIC_SystemReset();
			}
#if !defined USE_DEBUG
		if ( signaltimeout > 50000){     // 5 second
			allOff();
			armed = 0;
			input = 0;
			inputSet = 0;
			zero_input_count = 0;
				
				TMR1->c1dt = 0;
			  TMR1->c2dt = 0;
				TMR1->c3dt = 0;
				IC_TIMER_REGISTER->div = 0;
				IC_TIMER_REGISTER->cval = 0;
			for(int i = 0; i < 64; i++){
				dma_buffer[i] = 0;
			}
			NVIC_SystemReset();
		}
#endif

			}
#endif
}


void advanceincrement(){
if (!forward){
	phase_A_position ++;
    if (phase_A_position > 359){
	   phase_A_position = 0 ;
    }
	    phase_B_position ++;
	     if (phase_B_position > 359){
		phase_B_position = 0 ;
	}
	    phase_C_position ++;
	     if (phase_C_position > 359){
		phase_C_position = 0 ;
	}
}else{
	       phase_A_position --;
	    if (phase_A_position < 0){
		   phase_A_position = 359 ;
	    }
		    phase_B_position --;
		     if (phase_B_position < 0){
			phase_B_position = 359;
		}
		    phase_C_position --;
		     if (phase_C_position < 0){
			phase_C_position = 359 ;
		}
}
#ifdef GIMBAL_MODE
    TMR1->c1dt = ((2*pwmSin[phase_A_position])+gate_drive_offset)*TIMER1_MAX_ARR/2000;
    TMR1->c2dt = ((2*pwmSin[phase_B_position])+gate_drive_offset)*TIMER1_MAX_ARR/2000;
    TMR1->c3dt = ((2*pwmSin[phase_C_position])+gate_drive_offset)*TIMER1_MAX_ARR/2000;
#else
    TMR1->c1dt = (((2*pwmSin[phase_A_position]/SINE_DIVIDER)+gate_drive_offset)*TIMER1_MAX_ARR/2000)*sine_mode_power / 10;
    TMR1->c2dt = (((2*pwmSin[phase_B_position]/SINE_DIVIDER)+gate_drive_offset)*TIMER1_MAX_ARR/2000)*sine_mode_power / 10;
    TMR1->c3dt = (((2*pwmSin[phase_C_position]/SINE_DIVIDER)+gate_drive_offset)*TIMER1_MAX_ARR/2000)*sine_mode_power / 10;
#endif
}

void commutateProcess(){
		commutate();
    bemfcounter = 0;
    bad_count = 0;

    zero_crosses++;
    if(stall_protection || RC_CAR_REVERSE){
   	 if (zero_crosses >= 20 && commutation_interval <= 2000) {
   	    	old_routine = 0;
   	    	enableCompInterrupts();          // enable interrupt

    	 }
    }else{
    	if(zero_crosses > 30){
    	old_routine = 0;
    	enableCompInterrupts();          // enable interrupt

    }
   }
}
void zcfoundroutine(){   // only used in polling mode, blocking routine.
	thiszctime = INTERVAL_TIMER->cval;
	INTERVAL_TIMER->cval = 0;
	commutation_interval = (thiszctime + (3*commutation_interval)) / 4;
	advance = commutation_interval / advancedivisor;
	waitTime = commutation_interval /2  - advance;
	inner_step=-1;
	while (INTERVAL_TIMER->cval < waitTime){
	}
	commutateProcess();
}

inline int inrange(int target,int val,int percent){
	return val < target*(100+percent)/100 && val > target*(100-percent)/100;
}
const uint8_t detectorNotes[]={0xa0,0x19,0xe0,0,0xa0,0x19,0xe0,0,0xa0,0x19,0xe0,0,0xa0,0x19,0xe0,0,0xa0,0x19,0xe0,0,0xa0,0x19,0xe0,0};
uint8_t bjNotesTemp[4*8];
uint16_t alldiagnose_data[4*3];
char phaseError[3];
char compErrorTemp[12];
char compError[4];	
void SystemDiagnose(){
	__disable_irq();	
	ADC_Init_Detector();
	delayMillis(100);
	playBlueJayTune(detectorNotes,0,sizeof(detectorNotes),1);

	int voltateAverage=0;
	int effectiveVolateCount=0;
	for(int i=0;i<3;++i){
		allOff();
		TMR1->c1dt=0;
		TMR1->c2dt=0;
		TMR1->c3dt=0;
		switch(i){//charging
			case 0:
				phaseAPWM();
				delayMillis(1);
			break;
			case 1:
				phaseBPWM();
				delayMillis(1);
			break;
			case 2:
				phaseCPWM();
				delayMillis(1);
			break;
		}
		TMR1->c1dt=TMR1->pr+1;
		TMR1->c2dt=TMR1->pr+1;
		TMR1->c3dt=TMR1->pr+1;
		delayMicros(500);
		adc_ordinary_software_trigger_enable(ADC1, TRUE);
		while(dma_flag_get(DMA1_FDT1_FLAG) == RESET);
		dma_flag_clear(DMA1_FDT1_FLAG);
		for(int j=0;j<4;++j){
			alldiagnose_data[i*4+j]=adc_diagnose_data[j];
			if(adc_diagnose_data[j]>100){				
				voltateAverage+=adc_diagnose_data[j];
				++effectiveVolateCount;
			}
		}
	}
	if(effectiveVolateCount>0){
		voltateAverage=voltateAverage/effectiveVolateCount;
	}else{
		voltateAverage=1000;
	}
	TMR1->c1dt=0;
	TMR1->c2dt=0;
	TMR1->c3dt=0;
	allOff();
	delayMillis(100);
	

	for(int i=0;i<3;++i){
		for(int j=0;j<4;++j){
			compErrorTemp[i*4+j]=!inrange(voltateAverage,alldiagnose_data[i*4+j],5);
		}
		phaseError[i]=compErrorTemp[i*4+0] & compErrorTemp[i*4+1] & compErrorTemp[i*4+2] & compErrorTemp[i*4+3];
	}
	for(int j=0;j<4;++j){
		compError[j]=compErrorTemp[0*4+j]&compErrorTemp[1*4+j]&compErrorTemp[2*4+j];
	}
	

	for(int i=0;i<8;++i){
		bjNotesTemp[i*4]=0xa0;
		if(i<1){
			bjNotesTemp[i*4+1]=0x21;
		}else if(i<4){
			bjNotesTemp[i*4+1]=phaseError[i-1]?0:0x19;
		}else{
			bjNotesTemp[i*4+1]=compError[i-4]?0:0x19;
		}
		bjNotesTemp[i*4+2]=0xe0;
		bjNotesTemp[i*4+3]=0;		
	}
	playBlueJayTune(bjNotesTemp,0,sizeof(bjNotesTemp),0);			
	__enable_irq();
}

int main(void)
{
	__enable_irq();

 adc_counter = 2;
 initCorePeripherals();
	
	tmr_channel_enable(TMR1, TMR_SELECT_CHANNEL_1, TRUE);
	tmr_channel_enable(TMR1, TMR_SELECT_CHANNEL_2, TRUE);
	tmr_channel_enable(TMR1, TMR_SELECT_CHANNEL_3, TRUE);
	tmr_channel_enable(TMR1, TMR_SELECT_CHANNEL_1C, TRUE);
	tmr_channel_enable(TMR1, TMR_SELECT_CHANNEL_2C, TRUE);
	tmr_channel_enable(TMR1, TMR_SELECT_CHANNEL_3C, TRUE);


  /* Enable tim1 */
 TMR1->ctrl1_bit.tmren = TRUE;
 TMR1->brk_bit.oen = TRUE;
 
 /* Force update generation */
TMR1->swevt |= TMR_OVERFLOW_SWTRIG;


adc_counter = 4;
#ifdef USE_RGB_LED
  LED_GPIO_init();
  GPIOB->BRR = LL_GPIO_PIN_8; // turn on red
  GPIOB->BSRR = LL_GPIO_PIN_5;
  GPIOB->BSRR = LL_GPIO_PIN_3; //
#endif

#ifndef BRUSHED_MODE             // commutation_timer priority 0
	  COM_TIMER->ctrl1_bit.tmren = TRUE;
    COM_TIMER->swevt |= TMR_OVERFLOW_SWTRIG;
    COM_TIMER->iden &= ~TMR_OVF_INT;
   #endif

 UTILITY_TIMER->ctrl1_bit.tmren = TRUE;
 //delayMillis(2000);

INTERVAL_TIMER->ctrl1_bit.tmren = TRUE;
INTERVAL_TIMER->swevt |= TMR_OVERFLOW_SWTRIG;
//TMR6->ists = (uint16_t)~TMR_OVF_FLAG;
INTERVAL_TIMER->iden |= TMR_OVF_INT;


adc_counter = 5;

 TEN_KHZ_TIMER->ctrl1_bit.tmren = TRUE;
 TEN_KHZ_TIMER->swevt |= TMR_OVERFLOW_SWTRIG;
 TEN_KHZ_TIMER->iden |= TMR_OVF_INT;


adc_counter = 6;
loadEEpromSettings();
  if(VERSION_MAJOR != eepromBuffer[3] || VERSION_MINOR != eepromBuffer[4]){
	  eepromBuffer[3] = VERSION_MAJOR;
	  eepromBuffer[4] = VERSION_MINOR;
	  for(int i = 0; i < 12 ; i ++){
		  eepromBuffer[5+i] = (uint8_t)FIRMWARE_NAME[i];
	  }
	  saveEEpromSettings();
  }

  if(use_sin_start){
  min_startup_duty = sin_mode_min_s_d;
  }
	if (dir_reversed == 1){
			forward = 0;
		}else{
			forward = 1;
		}
	tim1_arr = TIMER1_MAX_ARR;
	startup_max_duty_cycle = startup_max_duty_cycle * TIMER1_MAX_ARR / 2000  + dead_time_override;  // adjust for pwm frequency
	throttle_max_at_low_rpm  = throttle_max_at_low_rpm * TIMER1_MAX_ARR / 2000;  // adjust to new pwm frequency
    throttle_max_at_high_rpm = TIMER1_MAX_ARR;  // adjust to new pwm frequency
	if(!comp_pwm){
		use_sin_start = 0;  // sine start requires complementary pwm.
	}
adc_counter = 7;
	if (RC_CAR_REVERSE) {         // overrides a whole lot of things!
		throttle_max_at_low_rpm = 1000;
		bi_direction = 1;
		use_sin_start = 0;
		low_rpm_throttle_limit = 1;
		VARIABLE_PWM = 0;
		//stall_protection = 1;
		comp_pwm = 0;
    stuck_rotor_protection = 0;
		minimum_duty_cycle = minimum_duty_cycle + 50;
		stall_protect_minimum_duty = stall_protect_minimum_duty + 50;
		min_startup_duty = min_startup_duty + 50;
	}
if(eepromBuffer[39]==1){//hull senser
	SystemDiagnose();
}
#ifdef USE_ADC
   ADC_Init();
#endif

#ifdef USE_ADC_INPUT

#else
tmr_channel_enable(IC_TIMER_REGISTER, IC_TIMER_CHANNEL, TRUE);
IC_TIMER_REGISTER->ctrl1_bit.tmren = TRUE;

#endif

#ifdef MCU_F031
	  GPIOF->BSRR = LL_GPIO_PIN_6;            // uncomment to take bridge out of standby mode and set oc level
	  GPIOF->BRR = LL_GPIO_PIN_7;				// out of standby mode
	  GPIOA->BRR = LL_GPIO_PIN_11;
#endif

#if defined(FIXED_DUTY_MODE) || defined(FIXED_SPEED_MODE)
 MX_IWDG_Init();
 WDT->cmd = WDT_CMD_RELOAD;
 inputSet = 1;
 armed = 1;
 adjusted_input = 48;
 newinput = 48;
#ifdef FIXED_SPEED_MODE
 use_speed_control_loop = 1;
 use_sin_start = 0;
 target_e_com_time = 60000000 / FIXED_SPEED_MODE_RPM / (motor_poles/2) ;
 input = 48;
#endif

#else
#ifdef BRUSHED_MODE
		maskPhaseInterrupts();
	 	commutation_interval = 5000;
	 	use_sin_start = 0;
		playBrushedStartupTune();
#else
	 //  playStartupTune();
#endif
	   zero_input_count = 0;
	   MX_IWDG_Init();
     WDT->cmd = WDT_CMD_RELOAD;

#ifdef GIMBAL_MODE
	bi_direction = 1;
	use_sin_start = 1;
#endif

#ifdef USE_ADC_INPUT
   armed_count_threshold = 5000;
   inputSet = 1;

#else
adc_counter = 8;

UN_TIM_Init();
receiveDshotDma();
 
if(drive_by_rpm){
	 use_speed_control_loop = 1;
 }
#endif

#endif      // end fixed duty mode ifdef

adc_counter = 9;


 while (1)
  {
WDT->cmd = WDT_CMD_RELOAD;

	  adc_counter++;
	  if(adc_counter>200){   // for testing adc and telemetry

					ADC_raw_temp = ADC_raw_temp - (temperature_offset);
					converted_degrees =(12600 - (int32_t)ADC_raw_temp * 33000 / 4096) / -42 + 25;
					degrees_celsius =(7 * degrees_celsius + converted_degrees) >> 3;
          battery_voltage = ((7 * battery_voltage) + ((ADC_raw_volts * 3300 / 4095 * VOLTAGE_DIVIDER)/100)) >> 3;
          smoothed_raw_current = ((7*smoothed_raw_current + (ADC_raw_current) )>> 3);
          actual_current = (smoothed_raw_current * 3300/41) / (MILLIVOLT_PER_AMP)  + CURRENT_OFFSET;

			adc_ordinary_software_trigger_enable(ADC1, TRUE);
			

		  if(LOW_VOLTAGE_CUTOFF){
			  if(battery_voltage < (cell_count * low_cell_volt_cutoff)){
				  low_voltage_count++;
				  if(low_voltage_count > (20000 - (stepper_sine * 900))){
				  input = 0;
				  allOff();
				  maskPhaseInterrupts();
				  running = 0;
				  zero_input_count = 0;
				  armed = 0;

				  }
			  }else{
				  low_voltage_count = 0;
			  }
		  }
		  adc_counter = 0;
#ifdef USE_ADC_INPUT
		  if(ADC_raw_input < 10){
			  zero_input_count++;
		  }else{
			  zero_input_count=0;
		  }
#endif
	  }
#ifdef USE_ADC_INPUT

signaltimeout = 0;
ADC_smoothed_input = (((10*ADC_smoothed_input) + ADC_raw_input)/11);
newinput = ADC_smoothed_input / 2;
if(newinput > 2000){
	newinput = 2000;
}
#endif
	  stuckcounter = 0;

  		  if (bi_direction == 1 && dshot == 0){
  			  if(RC_CAR_REVERSE){
  				  if (newinput > (1000 + (servo_dead_band<<1))) {
  					  if (forward == dir_reversed) {
  						  adjusted_input = 0;
  						  if(running){
  							  prop_brake_active = 1;
  						  }else{
  							  forward = 1 - dir_reversed;
  						  }
  					  }
  					  if (prop_brake_active == 0) {
  						  adjusted_input = map(newinput, 1000 + (servo_dead_band<<1), 2000, 47, 2047);
  					  }
  				  }
  				  if (newinput < (1000 -(servo_dead_band<<1))) {
  					  if (forward == (1 - dir_reversed)) {
  						  if(running){
  							  prop_brake_active = 1;
  						  }else{
  							  forward = dir_reversed;
  						  }
  						  adjusted_input = 0;

  					  }
  					  if (prop_brake_active == 0) {
  						  adjusted_input = map(newinput, 0, 1000-(servo_dead_band<<1), 2047, 47);
  					  }
  				  }


  				  if (newinput >= (1000 - (servo_dead_band << 1)) && newinput <= (1000 + (servo_dead_band <<1))) {
  					  adjusted_input = 0;
  					  prop_brake_active = 0;
  				  }
  			  }else{
  				  if (newinput > (1000 + (servo_dead_band<<1))) {
  					  if (forward == dir_reversed) {
  						  if((commutation_interval > reverse_speed_threshold )|| stepper_sine){
  							  forward = 1 - dir_reversed;
  							  zero_crosses = 0;
  							  old_routine = 1;
  							  maskPhaseInterrupts();
  							brushed_direction_set = 0;
  						  }else{
  							  newinput = 1000;
  						  }
  					  }
  					  adjusted_input = map(newinput, 1000 + (servo_dead_band<<1), 2000, 47, 2047);
  				  }
  				  if (newinput < (1000 -(servo_dead_band<<1))) {
  					  if (forward == (1 - dir_reversed)) {
  						  if((commutation_interval > reverse_speed_threshold) || stepper_sine){
  							  zero_crosses = 0;
  							  old_routine = 1;
  							  forward = dir_reversed;
  							  maskPhaseInterrupts();
  							brushed_direction_set = 0;
  						  }else{
  							  newinput = 1000;

  						  }
  					  }
  					  adjusted_input = map(newinput, 0, 1000-(servo_dead_band<<1), 2047, 47);
  				  }

 				  if (newinput >= (1000 - (servo_dead_band << 1)) && newinput <= (1000 + (servo_dead_band <<1))) {
  					  adjusted_input = 0;
  					brushed_direction_set = 0;
  				  }
  			  }
 		  }else if (dshot && bi_direction) {
  			  if (newinput > 1047) {

  				  if (forward == dir_reversed) {
  					  if(commutation_interval > reverse_speed_threshold || stepper_sine){
  						  forward = 1 - dir_reversed;
  						  zero_crosses = 0;
  						  old_routine = 1;
  						  maskPhaseInterrupts();
  						brushed_direction_set = 0;
  					  }else{
  						  newinput = 0;

  					  }
  				  }
  				  adjusted_input = ((newinput - 1048) * 2 + 47) - reversing_dead_band;

  			  }
  			  if (newinput <= 1047  && newinput > 47) {
  				  //	startcount++;

  				  if (forward == (1 - dir_reversed)) {
  					  if(commutation_interval > reverse_speed_threshold || stepper_sine){
  						  zero_crosses = 0;
  						  old_routine = 1;
  						  forward = dir_reversed;
  						  maskPhaseInterrupts();
  						brushed_direction_set = 0;
  					  }else{
  						  newinput = 0;

  					  }
  				  }
  				  adjusted_input = ((newinput - 48) * 2 + 47) - reversing_dead_band;
  			  }
  			  if ( newinput < 48) {
  				  adjusted_input = 0;
  				brushed_direction_set = 0;
  			  }


  		  }else{
  			  adjusted_input = newinput;
  		  }
#ifndef BRUSHED_MODE

	 	 if ((zero_crosses > 1000) || (adjusted_input == 0)){
 	 		bemf_timeout_happened = 0;
#ifdef USE_RGB_LED
 	 		if(adjusted_input == 0 && armed){
			  GPIOB->BSRR = LL_GPIO_PIN_8; // off red
			  GPIOB->BRR = LL_GPIO_PIN_5;  // on green
			  GPIOB->BSRR = LL_GPIO_PIN_3;  //off blue
 	 		}
#endif
 	 	 }
	 	 if(zero_crosses > 100 && adjusted_input < 200){
	 		bemf_timeout_happened = 0;
	 	 }
	 	 if(use_sin_start && adjusted_input < 160){
	 		bemf_timeout_happened = 0;
	 	 }

 	 	 if(crawler_mode){
 	 		if (adjusted_input < 400){
 	 			bemf_timeout_happened = 0;
 	 		}
 	 	 }else{
 	 		if (adjusted_input < 150){              // startup duty cycle should be low enough to not burn motor
 	 			bemf_timeout = 100;
 	 	 	 }else{
 	 	 		bemf_timeout = 10;
 	 	 	 }
 	 	 }
	  if(bemf_timeout_happened > bemf_timeout * ( 1 + (crawler_mode*100))&& stuck_rotor_protection){
	 		stopStuckMotor();
	 	 }else{
#ifdef FIXED_DUTY_MODE
  			input = FIXED_DUTY_MODE_POWER * 20;
#else
	  	  	if(use_sin_start){
  				if(adjusted_input < 30){           // dead band ?
  					input= 0;
  					}

  					if(adjusted_input > 30 && adjusted_input < (sine_mode_changeover_thottle_level * 20)){
  					input= map(adjusted_input, 30 , (sine_mode_changeover_thottle_level * 20) , 47 ,160);
  					}
  					if(adjusted_input >= (sine_mode_changeover_thottle_level * 20)){
  					input = map(adjusted_input , (sine_mode_changeover_thottle_level * 20) ,2000 , 160, 2000);
  					}
  				}else{
  					if(use_speed_control_loop){
  					  if (drive_by_rpm){
 						target_e_com_time = 60000000 / map(adjusted_input , 47 ,2047 , MINIMUM_RPM_SPEED_CONTROL, MAXIMUM_RPM_SPEED_CONTROL) / (motor_poles/2);
  		  				if(adjusted_input < 47){           // dead band ?
  		  					input= 0;
  		  					speedPid.error = 0;
  		  				    input_override = 0;
  		  				}else{
  	  						input = (uint16_t)input_override;  // speed control pid override
  	  						if(input_override > 2047){
  	  							input = 2047;
  	  						}
  	  						if(input_override < 48){
  	  							input = 48;
  	  						}
  		  				}
					    }else{

  						input = (uint16_t)input_override;  // speed control pid override
  						if(input_override > 1999){
  							input = 1999;
  						}
  						if(input_override < 48){
  							input = 48;
  						}
					    }
  					}else{

  						input = adjusted_input;

  					}
  				}
#endif
	 	  }
		  if ( stepper_sine == 0){

  e_rpm = running * (100000/ e_com_time) * 6;       // in tens of rpm
  k_erpm =  e_rpm / 10; // ecom time is time for one electrical revolution in microseconds

  if(low_rpm_throttle_limit){     // some hardware doesn't need this, its on by default to keep hardware / motors protected but can slow down the response in the very low end a little.

  duty_cycle_maximum = map(k_erpm, low_rpm_level, high_rpm_level, throttle_max_at_low_rpm, throttle_max_at_high_rpm);   // for more performance lower the high_rpm_level, set to a consvervative number in source.
   }

   if(degrees_celsius > TEMPERATURE_LIMIT){
	   duty_cycle_maximum = map(degrees_celsius, TEMPERATURE_LIMIT, TEMPERATURE_LIMIT+20, throttle_max_at_high_rpm/2, 1);
   }



#if defined(FAST_INTERRUPT)
	if (zero_crosses < 100 || commutation_interval > 500) {
		filter_level = 12;
	} else {
		filter_level = map(average_interval, 100 , 500, 4 , 12);
	}
#else
	if (zero_crosses < 100 || commutation_interval > 500) {
		filter_level = 8;
	} else {
		filter_level = map(average_interval, 100 , 500, 2 , 8);
	}
#endif
	
	if (commutation_interval < 100){
		filter_level = 2;
	}

if(motor_kv < 900){

	filter_level = filter_level * 2;
}

/**************** old routine*********************/
if (old_routine && running){
	maskPhaseInterrupts();
	 		 getBemfState();
	 	  if (!zcfound){
	 		  if (rising){
	 		 if (bemfcounter > min_bemf_counts_up){
	 			 zcfound = 1;
	 			 zcfoundroutine();
	 		}
	 		  }else{
	 			  if (bemfcounter > min_bemf_counts_down){
 			  			 zcfound = 1;
	 		  			 zcfoundroutine();
	 			  		}
	 		  }
	 	  }
}
#if defined(COMM_TIMEOUT_PROCESS)
			COMM_TIMEOUT_PROCESS;			
#endif

	 	  if (INTERVAL_TIMER->cval > comm_timeout  && running == 1){//zc timeout
				IntervalTimerOverflowProcess(0);
	 	  }
	 	  }else{            // stepper sine

#ifdef GIMBAL_MODE
	 				step_delay = 300;
	 				maskPhaseInterrupts();
	 				allpwm();
	 				if(newinput>1000){
	 					desired_angle = map(newinput, 1000, 2000, 180, 360);
	 				}else{
	 					desired_angle = map(newinput, 0, 1000, 0, 180);
	 				}
	 				if(current_angle > desired_angle){
	 					forward = 1;
	 					advanceincrement();
	 					delayMicros(step_delay);
	 					current_angle--;
	 				}
	 				if(current_angle < desired_angle){
	 					forward = 0;
	 					advanceincrement();
	 					delayMicros(step_delay);
	 					current_angle++;
	 				}
#else



if(input > 48 && armed){

	 		  if (input > 48 && input < 137){// sine wave stepper
         running = 1;
	 			 maskPhaseInterrupts();
	 			 allpwm();
	 		 advanceincrement();
             step_delay = map (input, 48, 120, 7000/motor_poles, 1000/motor_poles);
	 		 delayMicros(step_delay);
			 e_rpm =   600/ step_delay ;         // in hundreds so 33 e_rpm is 3300 actual erpm
				if(step_delay < 181){
					e_com_time = step_delay * 360;
				}else{ 
					e_com_time = 65535;
				}
	 		  }else{
	 			 advanceincrement();
	 			  if(input > 200){
	 				 phase_A_position = 0;
	 				 step_delay = 80;
	 			  }

	 			 delayMicros(step_delay);
	 			  if (phase_A_position == 0){
	 			  stepper_sine = 0;
	 			  running = 1;
				  old_routine = 1;
		 		  commutation_interval = 9000;
		 		  average_interval = 9000;
				  last_average_interval = average_interval;
						INTERVAL_TIMER->cval = 9000;
				  zero_crosses = 10;
				  prop_brake_active = 0;
	 			  step = changeover_step;                    // rising bemf on a same as position 0.
		 		// comStep(step);// rising bemf on a same as position 0.
				if(stall_protection){
				minimum_duty_cycle = stall_protect_minimum_duty;
				}
	 			commutate();
				TMR1->swevt |= TMR_OVERFLOW_SWTRIG;
	 			  }
	 		  }

}else{
	if(brake_on_stop){
#ifndef PWM_ENABLE_BRIDGE
	duty_cycle = (TIMER1_MAX_ARR-19) + drag_brake_strength*2;
	adjusted_duty_cycle = TIMER1_MAX_ARR - ((duty_cycle * tim1_arr)/TIMER1_MAX_ARR)+1;

TMR1->c1dt = adjusted_duty_cycle;
TMR1->c2dt = adjusted_duty_cycle;
TMR1->c3dt = adjusted_duty_cycle;
	proportionalBrake();
	prop_brake_active = 1;
#else
		// todo add braking for PWM /enable style bridges.
#endif
	}else{
		TMR1->c1dt = 0;
		TMR1->c2dt = 0;
		TMR1->c3dt = 0;
		allOff();
	}
	running = 0;
}

#endif      // gimbal mode
	 }  // stepper/sine mode end
#endif    // end of brushless mode

#ifdef BRUSHED_MODE
          TMR14->c1dt = adjusted_input;
	  			if(brushed_direction_set == 0 && adjusted_input > 48){
	  				if(forward){
	  					allOff();
	  					delayMicros(10);
	  					twoChannelForward();
	  				}else{
	  					allOff();
	  					delayMicros(10);
	  					twoChannelReverse();
	  				}
	  				brushed_direction_set = 1;
	  			}
	  			if(adjusted_input > 1900){
	  				adjusted_input = 1900;
	  			}
	  			input = map(adjusted_input, 48, 2047, 0, TIMER1_MAX_ARR);

	  			if(input > 0 && armed){
	  				TMR1->c1dt = input;												// set duty cycle to 50 out of 768 to start.
	  				TMR1->c2dt = input;
	  				TMR1->c3dt = input;
	  			}else{
	  				TMR1->c1dt = 0;												// set duty cycle to 50 out of 768 to start.
	  				TMR1->c2dt = 0;
	  				TMR1->c3dt = 0;
	  			}
#endif
  		}
}

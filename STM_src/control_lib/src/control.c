#include "control.h"
#include "motorControl.h"
#include "retarget.h"
#include "sensorBar.h"

#define ARM_MATH_CM3
#include "arm_math.h"

#define M_PI 3.14159265358979323846
#define max_heading_delta  27.1213034042
#define max_heading_delta_rad  max_heading_delta* M_PI/180

// gain param, influence lateral error.
float gain = 1;

// last valid measurements
float prevSensorReadings[5];


// setup routine for prev sensor data
void getInitialReadings()
{
	uint8_t i;
	for(i = 0; i<NUM_SENSORS; i++)
			prevSensorReadings[i] = sensorReadings[i];
}

float computeNewHeading()
{

	// constants
	float sensor_dist = 0.3;
	float gain = 2;
	float treshold = 0.55;
	
	// error terms
	float left_orientation_error, right_orientation_error;
	float avg_orientation_error;
	float lat_err;

	// regulator output
	float delta;

	// compute orientation error
	float radius_diff; // represents the difference between two sensors on the same side.
	float beta;
	
	
	// store last valid measurements 
//	for(i = 0; i<NUM_SENSORS; i++)
//		if(sensorReadings[i] < treshold)
//			prevSensorReadings[i] = sensorReadings[i];
//			
	if(sensorReadings[0] > treshold)
			sensorReadings[0] = sensorReadings[3];
	
	if(sensorReadings[3] > treshold)
			sensorReadings[3] = sensorReadings[0];
		
	// on first entry save sensor data

	// restore valid measurements 
//	if(fabs(sensorReadings[0] - prevSensorReadings[0]) < treshold)
//		prevSensorReadings[0] = sensorReadings[0];
//	else 
//		sensorReadings[0] = prevSensorReadings[0];
//	
//	if(fabs(sensorReadings[3] - prevSensorReadings[3]) < treshold)
//		prevSensorReadings[3] = sensorReadings[3];
//	else 
//		sensorReadings[3] = prevSensorReadings[3];


		
	// comptute orientation error for each side
	radius_diff  = sensorReadings[0] - sensorReadings[1];
	if( radius_diff > sensor_dist) // cap
		beta = M_PI/2;
	else if(radius_diff <- sensor_dist)
		beta = -M_PI/2;
	else 
		beta = asin( radius_diff/sensor_dist);
	// store left orientation err.
	left_orientation_error = beta;
	
	radius_diff  = sensorReadings[3] - sensorReadings[2];
		if( radius_diff > sensor_dist) // cap
		beta = M_PI/2;
	else if(radius_diff <- sensor_dist)
		beta = -M_PI/2;
	else 
		beta = asin( radius_diff/sensor_dist);
	// store left orientation err.
	right_orientation_error = -beta;
	
	// compute average oriention error
	avg_orientation_error =  0.5*(right_orientation_error+left_orientation_error);
	
	// compute lateral error
	lat_err = sensorReadings[2] - sensorReadings[1];
	
	// evalute regulator
	if(velocity > 0.1)
		delta = avg_orientation_error + atan(gain * (lat_err) /(velocity/1000.));
	else 
		delta = avg_orientation_error + atan(gain * (lat_err) /2.0);
	
		/*put stauration constraints on heading.*/
	if(delta > max_heading_delta_rad)
		delta = max_heading_delta_rad;
	else if(delta < - max_heading_delta_rad)
		delta = - max_heading_delta_rad;
		
	// compute output in degrees.
	return delta*180/M_PI;
}

uint16_t getPWM_from_angle(float angle)
{
	uint16_t pwm;
	
	// normalize angle 
	angle = angle/max_heading_delta;
	
	pwm = 1500 + angle*500;
	
	if(pwm < 1000)
		pwm = 1000;
	else if(pwm > 2000)
		pwm = 2000;
	
	return pwm;
}


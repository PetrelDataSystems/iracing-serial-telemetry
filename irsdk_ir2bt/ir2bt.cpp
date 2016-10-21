//------
#define MIN_WIN_VER 0x0501

#ifndef WINVER
#	define WINVER			MIN_WIN_VER
#endif

#ifndef _WIN32_WINNT
#	define _WIN32_WINNT		MIN_WIN_VER 
#endif

#pragma warning(disable:4996) //_CRT_SECURE_NO_WARNINGS


#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <signal.h>
#include <time.h>

#include "irsdk_defines.h"
#include "irsdk_client.h"
#include "serial.h"

// for timeBeginPeriod
#pragma comment(lib, "Winmm")

#define SERIAL_BUFFER_SIZE 500

// bool, driver is in the car and physics are running
// shut off motion if this is not true
irsdkCVar g_playerInCar("IsOnTrack");

irsdkCVar g_EnterExitReset("EnterExitReset");

// Session time in seconds (double)
irsdkCVar g_SessionTime("SessionTime");

// Lap number. Starts at 0 before timing started. First times lap is 1
irsdkCVar g_lap("Lap");
// % lap complete
irsdkCVar g_lapDistancePercentage("LapDistPct"); // Float from 0 to 1 (start to finish). Can exceed 1 when there is distance between start and finish such as bridge to gantry.

// double, cars position in lat/lon decimal degrees
irsdkCVar g_carLat("Lat");
irsdkCVar g_carLon("Lon");
// float, cars altitude in meters relative to sea levels
irsdkCVar g_carAlt("Alt");

// Vehicle data
irsdkCVar g_carSpeed("Speed"); // Speed in m/s (float)
irsdkCVar g_carThrottleRaw("ThrottleRaw"); // Throttle % (float) scaled from 0 to 1
irsdkCVar g_carBrakeRaw("BrakeRaw"); // Brake % (float) scaled from 0 to 1
irsdkCVar g_carSteeringWheelAngle("SteeringWheelAngle"); // Steering angle radians (float)
irsdkCVar g_carRPM("RPM"); // Engine RPM (float)

// float individial speed vectors
irsdkCVar g_carVelocityX("VelocityX");
irsdkCVar g_carVelocityY("VelocityY");
irsdkCVar g_carVelocityZ("VelocityZ");

// float, cars acceleration in m/s^2
irsdkCVar g_carLatAccel("LatAccel");
irsdkCVar g_carLongAccel("LongAccel");
irsdkCVar g_carVertAccel("VertAccel");

// float, cars change in orientation in rad/s
irsdkCVar g_carYawRate("YawRate");
irsdkCVar g_carPitchRate("PitchRate");
irsdkCVar g_carRollRate("RollRate");

// Absolute orientation
irsdkCVar g_carPitch("Pitch");
irsdkCVar g_carRoll("Roll");

Serial serial;

char serialBuffer[SERIAL_BUFFER_SIZE];

void ex_program(int sig) 
{
	(void)sig;

	printf("recieved ctrl-c, exiting\n\n");

	serial.closeSerial();
	timeEndPeriod(1);

	signal(SIGINT, SIG_DFL);
	exit(0);
}

bool init()
{
	// trap ctrl-c
	signal(SIGINT, ex_program);
	printf("press enter to exit:\n\n");

	// bump priority up so we get time from the sim
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	// ask for 1ms timer so sleeps are more precise
	timeBeginPeriod(1);
	
	// enumerate serial ports and pick the highest port
	//****FixMe, in a real program you would want to handle the port going away and coming back again in a graceful manner.
	int portList[32];
	int portCount = 32;

	// TODO remove hard-coded port number
	int port = 3; // serial.enumeratePorts(portList, &portCount);

	// open serial, hopefully this is the arduino
	if (serial.openSerial(port, CBR_9600)) {
		printf("Opened port %d\n", port);
		return true;
	}
	else
		printf("failed to open port %d\n", port);

	return false;
}

void monitorConnectionStatus()
{
	// keep track of connection status
	bool isConnected = irsdkClient::instance().isConnected();
	static bool wasConnected = !isConnected;
	if(wasConnected != isConnected)
	{
		if(isConnected)
			printf("Connected to iRacing\n");
		else
			printf("Lost connection to iRacing\n");
		wasConnected = isConnected;
	}
}

void run()
{
	// wait up to 16 ms for start of session or new data
	if(irsdkClient::instance().waitForData(16))
	{
		// and grab the data
		if(g_playerInCar.getBool())
		{
			int printedSize = snprintf(
				serialBuffer, 
				SERIAL_BUFFER_SIZE, 
				"$IRTEL,%0.3f,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", 
				g_SessionTime.getDouble(),
				g_EnterExitReset.getInt(),
				g_lap.getInt(),
				g_lapDistancePercentage.getFloat(),
				0.0f, //g_carLon.getDouble(),
				0.0f, //g_carLat.getDouble(),
				0.0f, //g_carAlt.getFloat(),
				g_carSpeed.getFloat(),
				g_carThrottleRaw.getFloat(),
				g_carBrakeRaw.getFloat(),
				g_carSteeringWheelAngle.getFloat(),
				g_carRPM.getFloat(), 
				g_carVelocityX.getFloat(),
				g_carVelocityY.getFloat(),
				g_carVelocityZ.getFloat(),
				g_carLatAccel.getFloat(),
				g_carLongAccel.getFloat(),
				g_carVertAccel.getFloat(),
				g_carYawRate.getFloat(),
				g_carPitchRate.getFloat(),
				g_carRollRate.getFloat(),
				g_carPitch.getFloat(),
				g_carRoll.getFloat()
			);

			printf("%s\n", serialBuffer);
			serial.writeSerialPrintf(serialBuffer);

			if (printedSize > SERIAL_BUFFER_SIZE) {
				// TODO Dynamically allocate buffer and attempt to increase its size
				printf("Buffer too small for output.");
			}
		}
	}

	// your normal process loop would go here
	monitorConnectionStatus();
}

int main(int argc, char *argv[])
{
	printf("ir2ad 1.0\n");
	printf(" send iracing data to arduino\n\n");

	if(init())
	{
		while(!_kbhit())
		{
			run();
		}

		printf("Shutting down.\n\n");
		serial.closeSerial();
		timeEndPeriod(1);
	}
	else
		printf("init failed\n");

	return 0;
}

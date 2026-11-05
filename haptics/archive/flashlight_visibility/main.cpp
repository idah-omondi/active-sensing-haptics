/////////////////////////////////////////////////////////////////////////////
// ARCHIVED VARIANT - Experiment 1 with "flashlight" visibility
// The target is only visible when the subject box is close to it.
// Visibility falls linearly from 1 (on top of the target) to 0 at visibilityRadius.
// Tried as an alternative to the velocity-driven fade. Not used for the
// collected Experiment 1 or Experiment 2 datasets.
//////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

#include <QHHeadersWin32.h>
#include <HD/hd.h>
#include <HL/hl.h>
#include <HDU/hduMath.h>
#include <HDU/hduRecord.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <fstream>

class DataTransportClass
{
public:
	Box* Model;
	Box* cursorSphere;
	Cursor* deviceCursor;
	Text* descriptionText;
};

hduMatrix WorldToDevice;

// Global variables
HDdouble g_targetVel = 0.0;
HDdouble g_humanVel = 0.0;
HDdouble g_relVel = 0.0;
HDdouble g_visualTargetX = 0.0;
HDdouble g_humanX = 0.0;
HDdouble g_relPos = 0.0;

HDdouble g_equation10_visibility = 0;
HDdouble g_distanceToTarget = 0.0;

// --- FLASHLIGHT VISIBILITY PARAMETERS ---
HDdouble visibilityRadius = 20.0; // 10 = hard, 15 = medium, 20 = easy, 25 = very easy

// Equation 10 parameters (unused by the flashlight logic)
HDdouble a = 1;
HDdouble b = 1;

HDdouble scalingFactor = 225.0;
HDdouble frequency = 36;  // 36 = 0.1 Hz
HDdouble A = 40.0;
HDdouble part = 2;
HDdouble B = 15.0;
HDdouble ph[5] = { 4.1185, 1.0756, 0.7979, 4.4362, 0.20 };

// --- DATA LOGGING ---
std::ofstream csvLogFile;

void GraphicsCallback(void);
void HLCALLBACK computeForceCB(HDdouble force[3], HLcache* cache, void* userdata);
void HLCALLBACK startEffectCB(HLcache* cache, void* userdata);
void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata);

void closeLogFile() {
	if (csvLogFile.is_open()) {
		csvLogFile.close();
		std::cout << "\nData successfully saved to TrackingData.csv\n";
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	QHWin32* DisplayObject = new QHWin32;
	DataTransportClass dataObject;

	csvLogFile.open("TrackingData.csv");
	if (csvLogFile.is_open()) {
		csvLogFile << "Time(s),"
			<< "TargetPosition,"
			<< "TargetVelocity,"
			<< "HumanPosition,"
			<< "HumanVelocity,"
			<< "RelativePosition,"
			<< "RelativeVelocity,"
			<< "DistanceToTarget,"
			<< "Visibility\n";
	}
	atexit(closeLogFile);

	DeviceSpace* OmniSpace = new DeviceSpace;
	DisplayObject->setName((char*)"Active Sensing Demonstration");
	DisplayObject->tell(OmniSpace);
	DisplayObject->setBackgroundColor(0, 0, 0);   // dark background, brighter means more visible

	dataObject.descriptionText = new Text(34.0, (char*)"Active Sensing Demo", 0.30, 0.80);
	dataObject.descriptionText->setShapeColor(1.0, 1.0, 1.0);
	DisplayObject->tell(dataObject.descriptionText);

	dataObject.deviceCursor = new Cursor();
	dataObject.deviceCursor->setName((char*)"devCursor");
	dataObject.deviceCursor->setCursorGraphicallyVisible(false);
	DisplayObject->tell(dataObject.deviceCursor);

	dataObject.cursorSphere = new Box(15.0, 15.0, 0.0);
	dataObject.cursorSphere->setName((char*)"cursorSphere");
	dataObject.cursorSphere->setShapeColor(0.85, 0.35, 0.0);
	dataObject.cursorSphere->setHapticVisibility(false);
	DisplayObject->tell(dataObject.cursorSphere);

	dataObject.Model = new Box(15.0, 15.0, 0.0);
	dataObject.Model->setName((char*)"Skull");
	dataObject.Model->setHapticVisibility(false);
	dataObject.Model->setShapeColor(0.25, 0.25, 0.25);
	DisplayObject->tell(dataObject.Model);

	DisplayObject->preDrawCallback(GraphicsCallback);
	OmniSpace->startServoLoopCallback(startEffectCB, computeForceCB, stopEffectCB, &dataObject);

	DisplayObject->setDefaultCamera();
	float fov, nearplane, farplane;
	hduVector3Dd eyepoint, lookat, up;
	DisplayObject->getCamera(&fov, &nearplane, &farplane, &eyepoint, &lookat, &up);
	eyepoint[2] += 100.;
	nearplane = 1.0;
	farplane = 500.0;
	DisplayObject->setCamera(fov + 15., nearplane, farplane, eyepoint, lookat, up);

	std::cout << "  ACTIVE SENSING DEMONSTRATION: FLASHLIGHT VISIBILITY\n";
	std::cout << "  Goal: follow the target box with the orange box.\n";

	qhStart();
	return 0;
}

void GraphicsCallback(void)
{
	QHWin32* localDisplayObject = QHWin32::searchWindow((char*)"Active Sensing Demonstration");
	Box* localCursorSphere = Box::searchBox((char*)"cursorSphere");
	Box* localModel = Box::searchBox((char*)"Skull");

	if (localDisplayObject == NULL || localCursorSphere == NULL || localModel == NULL)
		return;

	printf("\rDistance: %7.2f | Visible: %1.2f", g_distanceToTarget, g_equation10_visibility);

	hduVector3Dd localCursorPosition;
	hdGetDoublev(HD_CURRENT_POSITION, localCursorPosition);
	g_humanX = localCursorPosition[0];

	// 1. Subject (orange box), always solid
	localCursorSphere->setTranslation(-localCursorSphere->getTranslation());
	localCursorSphere->setTranslation(localCursorPosition[0], 0, 2.0);
	localCursorSphere->setShapeColor(0.85, 0.35, 0.0);

	// 2. Target brightness follows visibility (0 invisible, 1 fully visible)
	HDdouble tColor = g_equation10_visibility;
	localModel->setShapeColor(tColor, tColor, tColor);
	localModel->setTranslation(-localModel->getTranslation());

	// Color alone can leave a faint glow, so at zero visibility the target is moved off screen.
	if (g_equation10_visibility <= 0.0)
	{
		localModel->setTranslation(10000.0, 10000.0, 0.0);
	}
	else
	{
		localModel->setTranslation(g_visualTargetX, 0, 0.0);
	}
}

void HLCALLBACK computeForceCB(HDdouble force[3], HLcache* cache, void* userdata)
{
	HDdouble instRate = 0.0;
	HDdouble deltaT = 0.0;
	static float counter = 0.0;
	float degInRad = 0.0;
	static HDdouble prevActualTargetX = 0.0;
	static int logCounter = 0;

	hdGetDoublev(HD_INSTANTANEOUS_UPDATE_RATE, &instRate);
	deltaT = 1.0 / instRate;
	counter += deltaT;
	degInRad = counter * frequency * 3.14159 / 180;

	HDdouble actualTargetX = 0.0;
	if (part == 1) {
		actualTargetX = cos(degInRad) * A;
	}
	else {
		actualTargetX = (cos(degInRad + ph[0]) + cos(degInRad * 2.5 + ph[1]) + cos(degInRad * 5.5 + ph[2]) + cos(degInRad * 8.5 + ph[3]) + cos(degInRad * 10 + ph[4])) * B;
	}

	g_targetVel = (actualTargetX - prevActualTargetX) / deltaT;
	prevActualTargetX = actualTargetX;

	hduVector3Dd currentPos;
	hdGetDoublev(HD_CURRENT_POSITION, currentPos);
	g_humanX = currentPos[0];

	hduVector3Dd currentVeloc;
	hdGetDoublev(HD_CURRENT_VELOCITY, currentVeloc);
	g_humanVel = currentVeloc[0];
	g_relVel = g_targetVel - g_humanVel;
	g_relPos = actualTargetX - g_humanX;

	// Flashlight visibility
	HDdouble distanceToTarget = fabs(actualTargetX - g_humanX);
	g_distanceToTarget = distanceToTarget;

	if (distanceToTarget <= visibilityRadius)
	{
		g_equation10_visibility = 1.0 - (distanceToTarget / visibilityRadius);
	}
	else
	{
		g_equation10_visibility = 0.0;
	}

	// Data logging at about 100 Hz
	logCounter++;
	if (logCounter >= 10) {
		if (csvLogFile.is_open()) {
			csvLogFile << counter << ","
				<< actualTargetX << ","
				<< g_targetVel << ","
				<< g_humanX << ","
				<< g_humanVel << ","
				<< g_relPos << ","
				<< g_relVel << ","
				<< g_distanceToTarget << ","
				<< g_equation10_visibility
				<< "\n";
		}
		logCounter = 0;
	}

	force[0] = 0.0;
	force[1] = 0.0;
	force[2] = 0.0;
	g_visualTargetX = actualTargetX;
}

void HLCALLBACK startEffectCB(HLcache* cache, void* userdata) {}
void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata) {}

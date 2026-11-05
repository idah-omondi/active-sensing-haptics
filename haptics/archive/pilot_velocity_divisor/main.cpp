/////////////////////////////////////////////////////////////////////////////
// Active Sensing in Human Visual Tracking
// Experiment 1 - Pursuit scene (target and subject both drawn)
// Touch X + OpenHaptics 3.5.0 (QuickHaptics), Visual Studio 2017
//
// The subject moves the stylus left and right to follow a gray target box
// with an orange box. The target fades toward the white background, and is
// removed from the scene, when the relative velocity between target and
// stylus (v_q) is close to zero. This mimics the loss of observability in the
// weakly electric fish electrosensory model (Karagoz et al. 2025, Eq. 9/10).
//
// No force is rendered. Data are logged to CSV at about 100 Hz.
//
// CONDITIONS USED
//   Part 1  part = 1 (single sine, A = 40 mm), VISIBILITY_DIVISOR = 10
//   Part 2  part = 2 (sum of five sines, B = 20 mm), VISIBILITY_DIVISOR = 100
//   Base frequency setting 36 corresponds to 0.1 Hz.
//
// Change LOG_FILENAME before every trial so that files are not overwritten.
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
	Box* Model;           // target (gray)
	Box* cursorSphere;    // subject (orange)
	Cursor* deviceCursor;
	Text* descriptionText;
};

// ---------------- CONDITION SETTINGS (edit per condition) ----------------
HDdouble frequency = 36;   // 36 = 0.1 Hz, 90 = 0.25 Hz, 198 = 0.55 Hz, 306 = 0.85 Hz, 360 = 1 Hz
HDdouble part = 2;         // 1 = single sine, 2 = sum of five sines
HDdouble A = 40.0;         // single-sine amplitude (mm)
HDdouble B = 20.0;         // sum-of-sines amplitude per component (mm)
HDdouble ph[5] = { 4.1185, 1.0756, 0.7979, 4.4362, 0.20 };  // component phases (rad)

const char* LOG_FILENAME = "TrackingData1.csv";

// ---------------- VISIBILITY RULE PARAMETERS ----------------
const HDdouble VISIBILITY_DIVISOR = 100.0;  // |v_q| / divisor gives raw visibility in [0,1]. Part 1 used 10.
const HDdouble REL_VEL_DEADBAND = 5.0;      // mm/s, |v_q| below this is treated as exactly 0
const HDdouble SMOOTHING_ALPHA = 0.02;      // exponential moving average weight on the newest sample
const HDdouble HIDE_THRESHOLD = 0.5;        // below this smoothed visibility the target is moved off screen

// ---------------- SHARED STATE (servo thread writes, graphics thread reads) ----------------
HDdouble g_targetVel = 0.0;
HDdouble g_humanVel = 0.0;
HDdouble g_relVel = 0.0;
HDdouble g_visualTargetX = 0.0;
HDdouble g_humanX = 0.0;
HDdouble g_relPos = 0.0;
HDdouble g_visibility = 0.0;

std::ofstream csvLogFile;

void GraphicsCallback(void);
void HLCALLBACK computeForceCB(HDdouble force[3], HLcache* cache, void* userdata);
void HLCALLBACK startEffectCB(HLcache* cache, void* userdata);
void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata);

void closeLogFile() {
	if (csvLogFile.is_open()) {
		csvLogFile.close();
		std::cout << "\nData saved to " << LOG_FILENAME << "\n";
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	QHWin32* DisplayObject = new QHWin32;
	DataTransportClass dataObject;

	csvLogFile.open(LOG_FILENAME);
	if (csvLogFile.is_open()) {
		csvLogFile << "Time(s),"
			<< "TargetPosition,"
			<< "TargetVelocity,"
			<< "HumanPosition,"
			<< "HumanVelocity,"
			<< "RelativePosition,"
			<< "RelativeVelocity,"
			<< "Visibility\n";
	}
	atexit(closeLogFile);

	DeviceSpace* OmniSpace = new DeviceSpace;
	DisplayObject->setName((char*)"Active Sensing Demonstration");
	DisplayObject->tell(OmniSpace);
	DisplayObject->setBackgroundColor(1, 1, 1);

	dataObject.descriptionText = new Text(34.0, (char*)"Active Sensing Demo", 0.30, 0.80);
	dataObject.descriptionText->setShapeColor(0.0, 0.0, 0.0);
	DisplayObject->tell(dataObject.descriptionText);

	dataObject.deviceCursor = new Cursor();
	dataObject.deviceCursor->setName((char*)"devCursor");
	dataObject.deviceCursor->setCursorGraphicallyVisible(false);
	DisplayObject->tell(dataObject.deviceCursor);

	// Subject (orange box)
	dataObject.cursorSphere = new Box(20.0, 20.0, 0.0);
	dataObject.cursorSphere->setName((char*)"cursorSphere");
	dataObject.cursorSphere->setShapeColor(0.85, 0.35, 0.0);
	dataObject.cursorSphere->setHapticVisibility(false);
	DisplayObject->tell(dataObject.cursorSphere);

	// Target (gray box)
	dataObject.Model = new Box(20.0, 20.0, 0.0);
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

	std::cout << "  ACTIVE SENSING DEMONSTRATION: VELOCITY-DRIVEN VISIBILITY\n";
	std::cout << "  Goal: follow the gray box with the orange box.\n";
	std::cout << "  Logging data to " << LOG_FILENAME << "\n";

	qhStart();
	return 0;
}

// Runs once per rendered frame (graphics thread)
void GraphicsCallback(void)
{
	QHWin32* localDisplayObject = QHWin32::searchWindow((char*)"Active Sensing Demonstration");
	Box* localCursorSphere = Box::searchBox((char*)"cursorSphere");
	Box* localModel = Box::searchBox((char*)"Skull");

	if (localDisplayObject == NULL || localCursorSphere == NULL || localModel == NULL)
		return;

	printf("\rRelVel: %7.2f | Visibility: %4.2f", g_relVel, g_visibility);

	hduVector3Dd localCursorPosition;
	hdGetDoublev(HD_CURRENT_POSITION, localCursorPosition);
	g_humanX = localCursorPosition[0];

	// 1. Subject (orange) is always solid
	localCursorSphere->setTranslation(-localCursorSphere->getTranslation());
	localCursorSphere->setTranslation(localCursorPosition[0], 0, 0.0);
	localCursorSphere->setShapeColor(0.85, 0.35, 0.0);

	// 2. Target color fades from dark gray (0.25) at visibility 1 to white at visibility 0
	HDdouble tColor = 1.0 - (0.75 * g_visibility);
	localModel->setShapeColor(tColor, tColor, tColor);
	localModel->setTranslation(-localModel->getTranslation());

	// Recoloring alone leaves a visible silhouette where the target overlaps the subject,
	// so below the threshold the target is moved out of view instead.
	// The target is drawn in front of the subject (z = 2.0) when visible.
	if (g_visibility < HIDE_THRESHOLD)
	{
		localModel->setTranslation(10000.0, 10000.0, 0.0);
	}
	else
	{
		localModel->setTranslation(g_visualTargetX, 0, 2.0);
	}
}

// Runs at about 1 kHz (servo thread)
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

	// 1. True target position
	HDdouble actualTargetX = 0.0;
	if (part == 1) {
		actualTargetX = cos(degInRad) * A;
	}
	else {
		actualTargetX = (cos(degInRad + ph[0]) + cos(degInRad * 2.5 + ph[1]) + cos(degInRad * 5.5 + ph[2]) + cos(degInRad * 8.5 + ph[3]) + cos(degInRad * 10 + ph[4])) * B;
	}

	// 2. Target velocity (finite difference)
	g_targetVel = (actualTargetX - prevActualTargetX) / deltaT;
	prevActualTargetX = actualTargetX;

	// 3. Subject position and velocity, then relative position and velocity
	hduVector3Dd currentPos;
	hdGetDoublev(HD_CURRENT_POSITION, currentPos);
	g_humanX = currentPos[0];

	hduVector3Dd currentVeloc;
	hdGetDoublev(HD_CURRENT_VELOCITY, currentVeloc);
	g_humanVel = currentVeloc[0];
	g_relVel = g_targetVel - g_humanVel;   // v_q
	g_relPos = actualTargetX - g_humanX;   // p_q

	// 4. Visibility rule
	HDdouble relVelForVisibility = g_relVel;
	if (fabs(relVelForVisibility) < REL_VEL_DEADBAND) {
		relVelForVisibility = 0.0;
	}

	HDdouble visibility = fabs(relVelForVisibility) / VISIBILITY_DIVISOR;
	if (visibility > 1.0)
		visibility = 1.0;

	static HDdouble smoothedVisibility = 0.0;
	smoothedVisibility = SMOOTHING_ALPHA * visibility + (1.0 - SMOOTHING_ALPHA) * smoothedVisibility;
	g_visibility = smoothedVisibility;

	// 5. Data logging at about 100 Hz (every 10th servo tick)
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
				<< g_visibility
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

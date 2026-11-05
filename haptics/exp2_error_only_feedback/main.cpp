/////////////////////////////////////////////////////////////////////////////
// Experiment 2 - Error-only (compensatory) feedback
// Touch X, OpenHaptics 3.5.0 (QuickHaptics), Visual Studio
//
// The target is never drawn. The subject sees only a teal bar whose length is
// the tracking error (target minus stylus) and tries to keep the bar short.
// The bar is shown or hidden by the same power-law visibility rule used in
// Experiment 1, Score = a * v^n, but driven by the subject's own smoothed
// hand speed, with a single threshold.
//
//   Score      = g_a * (EMA of |hand velocity|)^g_exponentN
//   Score >= g_threshold  -> bar visible (VisibilityState = 1)
//   Score <  g_threshold  -> bar hidden  (VisibilityState = 0)
//
// Baseline (bar always visible): g_exponentN = 0 or g_threshold = -1.
// Per condition set g_exponentN to 2, 4 or 6.
//
// CSV columns (about 100 Hz, every 10th servo tick)
//   Time(s), ErrorMagnitude, RawHumanVelocity, SmoothedHumanVelocity,
//   VisibilityScore, VisibilityState, TargetPos, HumanPos
/////////////////////////////////////////////////////////////////////////////
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
	Box* plusH;
	Cylinder* forceArrow;
	Cursor* deviceCursor;
	Text* descriptionText;
};

double dataPoints = 120000;
hduMatrix WorldToDevice;
hduVector3Dd forceVec;
hduVector3Dd posit;

// --- GLOBALS FOR SUPERVISOR'S VISIBILITY LOGIC ---
HDdouble g_rawHumanVelocity = 0.0;
HDdouble g_smoothedHumanVelocity = 0.0;
HDdouble g_visibilityScore = 1.0;
HDdouble g_visibility = 1.0;

// Tuning Parameters from Experiment 1
HDdouble g_alphaEMA = 0.02;       // EMA filter smoothing factor for velocity
HDdouble g_a = 0.007;             // tuned 'a' multiplier (Experiment 1 value)
HDdouble g_threshold = 0.5;       // single visibility threshold
HDdouble g_exponentN = 2.0;       // Set this per trial (Baseline=0, Tracking=2, 4, 6)


// --- Set these per trial ---
HDdouble frequency = 36;  // 36=0.1Hz  90=0.25Hz  198=0.55Hz  306=0.85Hz  360=1Hz
HDdouble A = 10.0;        // single-sine amplitude, mm
HDdouble B = 5.0;         // sum-of-sines per-component amplitude, mm
HDdouble part = 2;        // 1 = single sine   2 = sum of sines
HDdouble secsum = 0;      // 0 = first trial   1 = second trial (sum-of-sines only)

HDdouble ph[5] = { 4.1185, 1.0756, 0.7979, 4.4362, 0.20 };

// --- DATA LOGGING ---
std::ofstream csvLogFile;

void GraphicsCallback(void);
void HLCALLBACK computeForceCB(HDdouble force[3], HLcache* cache, void* userdata);
void HLCALLBACK startEffectCB(HLcache* cache, void* userdata);
void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata);
char* recordCallback(void* userdata);

void closeLogFile() {
	if (csvLogFile.is_open()) {
		csvLogFile.close();
		std::cout << "\nData successfully saved to TrackingData_Exp2.csv!\n";
	}
}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	QHWin32* DisplayObject = new QHWin32;
	DataTransportClass dataObject;

	// OPEN THE CSV FILE AND WRITE THE HEADERS
	csvLogFile.open("TrackingData_Exp2.csv");
	if (csvLogFile.is_open()) {
		csvLogFile << "Time(s),"
			<< "ErrorMagnitude,"
			<< "RawHumanVelocity,"
			<< "SmoothedHumanVelocity,"
			<< "VisibilityScore,"
			<< "VisibilityState,"
			<< "TargetPos,"
			<< "HumanPos\n";
	}

	atexit(closeLogFile);

	DeviceSpace* OmniSpace = new DeviceSpace;
	DisplayObject->setName((char*)"Coulomb Field Demo");
	DisplayObject->tell(OmniSpace);
	DisplayObject->setBackgroundColor(1, 1, 1);

	dataObject.descriptionText = new Text(34.0, (char*)"Visual-Motor Integration Study", 0.2, 0.9);
	dataObject.descriptionText->setShapeColor(0.0, 0.0, 0.0);  // black
	DisplayObject->tell(dataObject.descriptionText);


	dataObject.deviceCursor = new Cursor();
	dataObject.deviceCursor->setName((char*)"devCursor");
	dataObject.deviceCursor->setCursorGraphicallyVisible(false);
	DisplayObject->tell(dataObject.deviceCursor);

	// Target - stays invisible, error-only feedback is the whole point of this scene
	dataObject.Model = new Box(9, 9, 0.01);
	dataObject.Model->setName((char*)"Skull");
	dataObject.Model->setHapticVisibility(false);
	dataObject.Model->setShapeColor(0.35, 0.35, 0.35);
	// intentionally NOT told to DisplayObject

	// Error bar - teal, enlarged
	dataObject.forceArrow = new Cylinder(3, 1, 50);
	dataObject.forceArrow->setShapeColor(0.45, 0.75, 0.79);  // deep teal
	dataObject.forceArrow->setHapticVisibility(false);
	dataObject.forceArrow->setName((char*)"forceArrow");
	DisplayObject->tell(dataObject.forceArrow);

	// Zero-error reference marker
	dataObject.plusH = new Box(20, 7, 10);
	dataObject.plusH->setName((char*)"plusH");
	dataObject.plusH->setShapeColor(1, 1.0, 1.0);  // dark gray, visible on white bg
	dataObject.plusH->setTranslation(10.0, 0.0, -10.0);
	dataObject.plusH->setHapticVisibility(false);
	DisplayObject->tell(dataObject.plusH);


	DisplayObject->preDrawCallback(GraphicsCallback);
	OmniSpace->startServoLoopCallback(startEffectCB, computeForceCB, stopEffectCB, &dataObject);

	DisplayObject->setDefaultCamera();

	float fov, nearplane, farplane;
	hduVector3Dd eyepoint, lookat, up;
	DisplayObject->getCamera(&fov, &nearplane, &farplane, &eyepoint, &lookat, &up);

	eyepoint[2] += 100.;
	nearplane += 80.;
	farplane += 80.;
	DisplayObject->setCamera(fov + 15., nearplane, farplane, eyepoint, lookat, up);

	FILE* pFile = NULL;
	if (part == 1 && frequency == 36)       pFile = fopen("exp3_0.10Hz.txt", "w");
	else if (part == 1 && frequency == 90)  pFile = fopen("exp3_0.25Hz.txt", "w");
	else if (part == 1 && frequency == 198) pFile = fopen("exp3_0.55Hz.txt", "w");
	else if (part == 1 && frequency == 306) pFile = fopen("exp3_0.85Hz.txt", "w");
	else if (part == 1 && frequency == 360) pFile = fopen("exp3_1.00Hz.txt", "w");
	else if (part == 2 && secsum == 0)      pFile = fopen("exp4_sumofsines_trial1.txt", "w");
	else if (part == 2 && secsum == 1)      pFile = fopen("exp4_sumofsines_trial2.txt", "w");

	if (pFile != NULL) {
		fprintf(pFile, "HumanPos, Error(RelPos), TargetPos, HumanVel, TargetVel, RelVel\n");
		hduStartRecord(pFile, recordCallback, NULL, dataPoints);
	}
	else {
		std::cout << "Warning: Could not open file for writing." << std::endl;
	}

	std::cout << "  COMPENSATORY SCENE: VELOCITY-DEPENDENT VISIBILITY\n";
	std::cout << "  Goal: Keep the teal cylinder as small as possible.\n";

	qhStart();
	return 0;
}

void GraphicsCallback(void)
{
	QHWin32* localDisplayObject = QHWin32::searchWindow((char*)"Coulomb Field Demo");
	Cursor* localDeviceCursor = Cursor::searchCursor((char*)"devCursor");
	Cylinder* localForceArrow = Cylinder::searchCylinder((char*)"forceArrow");
	Box* localPlusH = Box::searchBox((char*)"plusH");

	if (localDisplayObject == NULL || localDeviceCursor == NULL || localForceArrow == NULL)
		return;

	// LIVE CONSOLE READOUT
	printf(
		"\rSmoothVel: %8.2f | Score: %5.2f | Visible: %1.0f",
		g_smoothedHumanVelocity,
		g_visibilityScore,
		g_visibility
	);

	// Hide the grey box by moving it infinitely far away AFTER the camera has already used it for setup
	if (localPlusH != NULL) {
		localPlusH->setTranslation(10000.0, 10000.0, 10000.0);
	}

	hduMatrix CylinderTransform;
	WorldToDevice = localDisplayObject->getWorldToDeviceTransform();
	HDdouble ForceMagnitude = forceVec.magnitude();

	if (forceVec[0] > 0.0) {
		CylinderTransform[0][0] = 0.0;  CylinderTransform[0][1] = 1.0; CylinderTransform[0][2] = 0.0; CylinderTransform[0][3] = 0.0;
		CylinderTransform[1][0] = -1.0; CylinderTransform[1][1] = 0.0; CylinderTransform[1][2] = 0.0; CylinderTransform[1][3] = 0.0;
		CylinderTransform[2][0] = 0.0;  CylinderTransform[2][1] = 0.0; CylinderTransform[2][2] = 1.0; CylinderTransform[2][3] = 0.0;
		CylinderTransform[3][0] = 0.0;  CylinderTransform[3][1] = 0.0; CylinderTransform[3][2] = 0.0; CylinderTransform[3][3] = 1.0;
	}
	else {
		CylinderTransform[0][0] = 0.0;  CylinderTransform[0][1] = -1.0; CylinderTransform[0][2] = 0.0; CylinderTransform[0][3] = 0.0;
		CylinderTransform[1][0] = 1.0;  CylinderTransform[1][1] = 0.0;  CylinderTransform[1][2] = 0.0; CylinderTransform[1][3] = 0.0;
		CylinderTransform[2][0] = 0.0;  CylinderTransform[2][1] = 0.0;  CylinderTransform[2][2] = 1.0; CylinderTransform[2][3] = 0.0;
		CylinderTransform[3][0] = 0.0;  CylinderTransform[3][1] = 0.0;  CylinderTransform[3][2] = 0.0; CylinderTransform[3][3] = 1.0;
	}

	// Visibility gates whether the bar is shown at all; the bar's
	// length always represents the true error magnitude.
	if (g_visibility == 0.0)
	{
		localForceArrow->update(0.0, 0.0, 55);
	}
	else
	{
		localForceArrow->update(4, ForceMagnitude, 55);
	}

	localForceArrow->setTranslation(0, 0, 0);
	localForceArrow->setTransform(CylinderTransform);
}

void HLCALLBACK computeForceCB(HDdouble force[3], HLcache* cache, void* userdata)
{
	DataTransportClass* localdataObject =
		(DataTransportClass*)userdata;

	if (localdataObject == NULL)
	{
		int x = 1;
	}

	if (localdataObject->Model == NULL)
	{
		int y = 1;
	}

	hduVector3Dd skullPositionDS;
	hduVector3Dd skullPos;
	hduVector3Dd proxyPosition;
	HDdouble instRate = 0.0;
	HDdouble deltaT = 0.0;
	static int logCounter = 0;
	static float counter = 0.0;
	float degInRad = 0.0;

	hdGetDoublev(HD_INSTANTANEOUS_UPDATE_RATE, &instRate);
	deltaT = 1.0 / instRate;
	counter += deltaT;
	degInRad = counter * frequency * 3.14159 / 180;

	hduVector3Dd ModelPos = localdataObject->Model->getTranslation();
	localdataObject->Model->setTranslation(-ModelPos);

	if (part == 1) {
		localdataObject->Model->setTranslation(cos(degInRad) * A, 0, 0);
		skullPos = { cos(degInRad) * A, 0.0, 0.0 };
	}
	else {
		localdataObject->Model->setTranslation(
			(cos(degInRad + ph[0]) + cos(degInRad * 2.5 + ph[1]) + cos(degInRad * 5.5 + ph[2])
				+ cos(degInRad * 8.5 + ph[3]) + cos(degInRad * 10 + ph[4])) * B, 0.0, 0.0);
		skullPos = { (cos(degInRad + ph[0]) + cos(degInRad * 2.5 + ph[1]) + cos(degInRad * 5.5 + ph[2])
			+ cos(degInRad * 8.5 + ph[3]) + cos(degInRad * 10 + ph[4])) * B, 0.0, 0.0 };
	}

	WorldToDevice.multVecMatrix(localdataObject->Model->getTranslation(), skullPositionDS);
	hlCacheGetDoublev(cache, HL_PROXY_POSITION, proxyPosition);

	// forceVec holds the error (Relative Position)
	forceVec = { (skullPos[0] - proxyPosition[0]), 0, 0 };
	HDdouble errorMagnitude = fabs(forceVec[0]);

	// --- 1. Calculate Pure Human Velocity ---
	static HDdouble prevHumanPos = 0.0;
	HDdouble currentHumanPos = proxyPosition[0];

	g_rawHumanVelocity = (currentHumanPos - prevHumanPos) / deltaT;
	prevHumanPos = currentHumanPos;

	// --- 2. EMA Low-Pass Filter ---
	// Smooth the absolute human velocity to remove biomechanical hand jitter
	g_smoothedHumanVelocity = (g_alphaEMA * fabs(g_rawHumanVelocity)) + ((1.0 - g_alphaEMA) * g_smoothedHumanVelocity);

	// --- 3. Supervisor's Power-Law Equation ---
	// Equation: Score = a * (v^n)
	g_visibilityScore = g_a * pow(g_smoothedHumanVelocity, g_exponentN);

	// --- 4. Single Threshold Logic ---
	if (g_visibilityScore >= g_threshold)
	{
		g_visibility = 1.0; // bar visible
	}
	else
	{
		g_visibility = 0.0; // bar hidden
	}

	logCounter++;

	if (logCounter >= 10)
	{
		if (csvLogFile.is_open())
		{
			csvLogFile
				<< counter << ","
				<< errorMagnitude << ","
				<< g_rawHumanVelocity << ","
				<< g_smoothedHumanVelocity << ","
				<< g_visibilityScore << ","
				<< g_visibility << ","
				<< skullPos[0] << ","        // Target Position
				<< proxyPosition[0]          // Human Position
				<< "\n";
		}

		logCounter = 0;
	}
}

void HLCALLBACK startEffectCB(HLcache* cache, void* userdata)
{
	printf("Custom effect started\n");
}

void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata)
{
	printf("Custom effect stopped\n");
}

char* recordCallback(void* UserData)
{
	hduVector3Dd skullPos;
	HDdouble instRate = 0.0;
	HDdouble deltaT = 0.0;
	static float counter = 0.0;
	float degInRad = 0.0;

	hdGetDoublev(HD_INSTANTANEOUS_UPDATE_RATE, &instRate);
	deltaT = 1.0 / instRate;
	counter += deltaT;
	degInRad = counter * frequency * 3.14159 / 180;

	if (part == 1) {
		skullPos = { cos(degInRad) * A, 0.0, 0.0 };
	}
	else {
		skullPos = { (cos(degInRad + ph[0]) + cos(degInRad * 2.5 + ph[1]) + cos(degInRad * 5.5 + ph[2])
			+ cos(degInRad * 8.5 + ph[3]) + cos(degInRad * 10 + ph[4])) * B, 0.0, 0.0 };
	}

	hduVector3Dd gimbalAngles;
	hdGetDoublev(HD_CURRENT_GIMBAL_ANGLES, gimbalAngles);
	hdGetDoublev(HD_CURRENT_POSITION, posit);
	hduVector3Dd veloc;
	hdGetDoublev(HD_CURRENT_VELOCITY, veloc);

	char* c = new char[200];
	sprintf(c, "%lf, %lf, %lf, %lf", posit[0], forceVec[0], skullPos[0], veloc[0]);
	return c;
}

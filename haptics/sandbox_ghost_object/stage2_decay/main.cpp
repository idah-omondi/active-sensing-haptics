/////////////////////////////////////////////////////////////////////////////
// Sandbox - Ghost Object, Stage 2
// Static sphere whose push-back force decays the longer the stylus is held
// still inside it (a haptic analogue of Troxler fading). Moving or tapping
// resets the decay timer.
//
// The collision force is computed by hand here (simple spring on penetration
// depth, no god-object proxy), which feels softer than the Stage 1 automatic
// rendering. STIFFNESS was not calibrated on the device. Raise it in small
// steps until the wall feels solid, and back off if the stylus buzzes.
//
// Windows subsystem note. This project links as a Windows application, so a
// console is opened with AllocConsole() to make printf visible.
/////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

#include <QHHeadersWin32.h>
#include <HD/hd.h>
#include <HL/hl.h>
#include <HDU/hduMath.h>
#include <iostream>
#include <cmath>
#include <windows.h>
#include <cstdio>

class DataTransportClass
{
public:
	Sphere* touchSphere;
	Cursor* deviceCursor;
	Text* descriptionText;
};

// --- Sphere geometry (must match between visual object and force computation) ---
const HDdouble SPHERE_RADIUS = 15.0;
const hduVector3Dd SPHERE_CENTER(0.0, 0.0, 0.0);

// --- Force and decay tuning ---
const HDdouble STIFFNESS = 0.3;               // spring constant, force per mm of penetration (needs tuning)
const HDdouble STILL_SPEED_THRESHOLD = 5.0;   // mm/s, below this counts as holding still
const HDdouble DECAY_TAU = 1.5;               // seconds, smaller means faster fade

// --- Globals for live readout ---
HDdouble g_stillTimer = 0.0;
HDdouble g_decayFactor = 1.0;
bool g_inContact = false;

void GraphicsCallback(void);
void HLCALLBACK computeForceCB(HDdouble force[3], HLcache* cache, void* userdata);
void HLCALLBACK startEffectCB(HLcache* cache, void* userdata);
void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	AllocConsole();
	FILE* dummy;
	freopen_s(&dummy, "CONOUT$", "w", stdout);
	freopen_s(&dummy, "CONOUT$", "w", stderr);

	QHWin32* DisplayObject = new QHWin32;
	DataTransportClass dataObject;

	DeviceSpace* OmniSpace = new DeviceSpace;
	DisplayObject->setName((char*)"Ghost Object Demo - Stage 2");
	DisplayObject->tell(OmniSpace);
	DisplayObject->setBackgroundColor(1, 1, 1);

	dataObject.descriptionText = new Text(30.0, (char*)"Ghost Object", 0.40, 0.9);
	dataObject.descriptionText->setShapeColor(0.0, 0.0, 0.0);
	DisplayObject->tell(dataObject.descriptionText);

	dataObject.deviceCursor = new Cursor();
	dataObject.deviceCursor->setName((char*)"devCursor");
	dataObject.deviceCursor->setCursorGraphicallyVisible(true);  // set to false for blind trials
	DisplayObject->tell(dataObject.deviceCursor);

	dataObject.touchSphere = new Sphere(SPHERE_RADIUS, 30.0);
	dataObject.touchSphere->setName((char*)"touchSphere");
	dataObject.touchSphere->setTranslation(SPHERE_CENTER[0], SPHERE_CENTER[1], SPHERE_CENTER[2]);
	dataObject.touchSphere->setShapeColor(0.85, 0.35, 0.0);
	dataObject.touchSphere->setHapticVisibility(false);  // force is computed in computeForceCB instead
	DisplayObject->tell(dataObject.touchSphere);

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

	std::cout << "\n  GHOST OBJECT DEMO - STAGE 2\n";
	std::cout << "  Push into the sphere and hold still, the force should fade.\n";
	std::cout << "  Tap or wiggle to bring the feeling back.\n\n";

	qhStart();
	return 0;
}

void GraphicsCallback(void)
{
	printf("\r[LIVE] Contact: %s | Still timer: %.2fs | Decay factor: %.2f    ",
		g_inContact ? "YES" : "no ", g_stillTimer, g_decayFactor);
	fflush(stdout);   // console output is buffered after freopen_s, flush so it updates live
}

void HLCALLBACK computeForceCB(HDdouble force[3], HLcache* cache, void* userdata)
{
	HDdouble instRate = 0.0;
	hdGetDoublev(HD_INSTANTANEOUS_UPDATE_RATE, &instRate);
	HDdouble deltaT = (instRate > 0.0) ? (1.0 / instRate) : 0.001;

	hduVector3Dd devicePos, deviceVel;
	hdGetDoublev(HD_CURRENT_POSITION, devicePos);
	hdGetDoublev(HD_CURRENT_VELOCITY, deviceVel);
	HDdouble speed = deviceVel.magnitude();

	hduVector3Dd toDevice = devicePos - SPHERE_CENTER;
	HDdouble dist = toDevice.magnitude();

	force[0] = 0.0;
	force[1] = 0.0;
	force[2] = 0.0;

	if (dist < SPHERE_RADIUS && dist > 0.0001)
	{
		g_inContact = true;

		// Stillness timer, movement resets it
		if (speed < STILL_SPEED_THRESHOLD) {
			g_stillTimer += deltaT;
		}
		else {
			g_stillTimer = 0.0;
		}

		// Exponential decay, 1.0 at timer = 0
		g_decayFactor = exp(-g_stillTimer / DECAY_TAU);

		// Spring force along the outward normal, scaled by the decay factor
		HDdouble penetration = SPHERE_RADIUS - dist;
		hduVector3Dd direction = toDevice / dist;
		HDdouble forceMagnitude = g_decayFactor * STIFFNESS * penetration;
		force[0] = direction[0] * forceMagnitude;
		force[1] = direction[1] * forceMagnitude;
		force[2] = direction[2] * forceMagnitude;
	}
	else
	{
		// Decay only builds while in contact
		g_inContact = false;
		g_stillTimer = 0.0;
		g_decayFactor = 1.0;
	}
}

void HLCALLBACK startEffectCB(HLcache* cache, void* userdata)
{
	printf("Servo loop started\n");
}

void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata)
{
	printf("Servo loop stopped\n");
}

/////////////////////////////////////////////////////////////////////////////
// Sandbox - Ghost Object, Stage 1
// Render a static sphere that can be seen and felt. No decay, no logging.
// Purpose is to confirm that QuickHaptics collision force rendering works
// before adding the adaptation (Troxler-style fading) mechanic in Stage 2.
//
// Note on the Sphere constructor. Sphere(radius) does not exist in the
// QuickHaptics 3.5.0 header. The constructors are
//   Sphere(radius, resolution)
//   Sphere(radius, resolution, x, y, z)
/////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

#include <QHHeadersWin32.h>
#include <HD/hd.h>
#include <HL/hl.h>
#include <iostream>

class DataTransportClass
{
public:
	Sphere* touchSphere;
	Cursor* deviceCursor;
	Text* descriptionText;
};

void HLCALLBACK startEffectCB(HLcache* cache, void* userdata);
void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	QHWin32* DisplayObject = new QHWin32;
	DataTransportClass dataObject;

	DeviceSpace* OmniSpace = new DeviceSpace;
	DisplayObject->setName((char*)"Ghost Object Demo - Stage 1");
	DisplayObject->tell(OmniSpace);
	DisplayObject->setBackgroundColor(1, 1, 1);

	dataObject.descriptionText = new Text(34.0, (char*)"Feel the sphere. Nothing else happens yet.", 0.1, 0.9);
	dataObject.descriptionText->setShapeColor(0.0, 0.0, 0.0);
	DisplayObject->tell(dataObject.descriptionText);

	dataObject.deviceCursor = new Cursor();
	dataObject.deviceCursor->setName((char*)"devCursor");
	dataObject.deviceCursor->setCursorGraphicallyVisible(true);  // visible for the first test
	DisplayObject->tell(dataObject.deviceCursor);

	// Static sphere at the origin, visually and haptically present
	dataObject.touchSphere = new Sphere(15.0, 20.0);
	dataObject.touchSphere->setName((char*)"touchSphere");
	dataObject.touchSphere->setTranslation(0.0, 0.0, 0.0);
	dataObject.touchSphere->setShapeColor(0.85, 0.35, 0.0);
	dataObject.touchSphere->setHapticVisibility(true);  // QuickHaptics handles the collision force
	DisplayObject->tell(dataObject.touchSphere);

	OmniSpace->startServoLoopCallback(startEffectCB, NULL, stopEffectCB, &dataObject);

	DisplayObject->setDefaultCamera();

	float fov, nearplane, farplane;
	hduVector3Dd eyepoint, lookat, up;
	DisplayObject->getCamera(&fov, &nearplane, &farplane, &eyepoint, &lookat, &up);
	eyepoint[2] += 100.;
	nearplane = 1.0;
	farplane = 500.0;
	DisplayObject->setCamera(fov + 15., nearplane, farplane, eyepoint, lookat, up);

	std::cout << "\n  GHOST OBJECT DEMO - STAGE 1\n";
	std::cout << "  Find the sphere and feel it push back.\n\n";

	qhStart();
	return 0;
}

void HLCALLBACK startEffectCB(HLcache* cache, void* userdata)
{
	printf("Servo loop started\n");
}

void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata)
{
	printf("Servo loop stopped\n");
}

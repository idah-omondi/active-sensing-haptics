/////////////////////////////////////////////////////////////////////////////
// Active Sensing in Human Visual-Motor Tracking
// Experiment 1 - Target tracking with velocity-dependent target visibility
// Touch X, OpenHaptics 3.5.0 (QuickHaptics), Visual Studio
//
// Scene structure follows the lab's visual tracking demo (A. Kilic, METU, 2023).
// The visibility logic is new.
//
// The subject moves the stylus left and right to follow a gray target box with
// an orange box. The target path is a sum of five sines (0.1, 0.25, 0.55, 0.85
// and 1.0 Hz) with a random phase for each component on every trial. No force
// is rendered.
//
// Relative velocity   v_q = target velocity - stylus velocity
// Visibility          V   = (a * |v_q|)^n
//
// CONDITIONS
//   BASELINE    V = 1, the target is always visible.
//   THRESHOLD   |v_q| is low-pass filtered, V is computed from the filtered
//               value and passed through a two-level hysteresis gate, so the
//               target is either fully visible or gone. When it is gone it is
//               moved off screen.
//   CONTINUOUS  V is used directly as a gray level, 1 - V, so the target fades
//               smoothly into the white background.
//
// One run is one trial of 60 s. Set PARTICIPANT_ID, CONDITION and
// CURRENT_TRIAL (0..9) before each build. Output files
//   <PARTICIPANT_ID><condition number>_<trial>.csv   e.g. P11_0.csv
//   Phase_Log.csv                                    one row of phases per trial
//
// CSV columns, about 100 Hz (every 10th servo tick), first 60 s
//   Time(s), TargetPosition, TargetVelocity, HumanPosition, HumanVelocity,
//   RelativePosition, RelativeVelocity, Visibility
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
#include <cstdio>
#include <fstream>
#include <ctime>

// ======================= TRIAL SETTINGS (edit per run) =====================
enum Condition { BASELINE = 1, THRESHOLD = 2, CONTINUOUS = 3 };

const Condition CONDITION      = BASELINE;
const char*     PARTICIPANT_ID = "P1";
const int       CURRENT_TRIAL  = 0;       // 0..9
const double    TRIAL_SECONDS  = 60.0;    // logging stops after this time

// ======================= STIMULUS SETTINGS =================================
HDdouble frequency = 36;   // 36 = 0.1 Hz base. 90 = 0.25, 198 = 0.55, 306 = 0.85, 360 = 1.0 (single sine only)
HDdouble A = 40.0;         // single-sine amplitude, mm (part 1)
HDdouble B = 20.0;         // amplitude of each sine in the sum, mm (part 2)
HDdouble part = 2;         // 1 = single sine, 2 = sum of sines
HDdouble ph[5];            // random phases, filled in WinMain

// ======================= VISIBILITY SETTINGS ===============================
const int      N_EXPONENT          = 2;      // n in V = (a|v_q|)^n
const HDdouble A_THRESHOLD         = 0.005;  // a, THRESHOLD condition
const HDdouble A_CONTINUOUS        = 0.006;  // a, CONTINUOUS condition
const HDdouble T_HIGH              = 0.50;   // hysteresis, target turns on above this V
const HDdouble T_LOW               = 0.20;   // hysteresis, target turns off below this V
const HDdouble SMOOTHING_ALPHA     = 0.05;   // EMA weight on the newest |v_q| sample (THRESHOLD)

// ======================= SHARED STATE (servo thread writes, graphics thread reads) ===
class DataTransportClass
{
public:
    Box* Model;            // target
    Box* cursorSphere;     // subject
    Cursor* deviceCursor;
    Text* descriptionText;
};

hduMatrix WorldToDevice;

HDdouble targetVel = 0.0;
HDdouble humanVel = 0.0;
HDdouble relVel = 0.0;
HDdouble visualTargetX = 0.0;
HDdouble humanX = 0.0;
HDdouble relPos = 0.0;
HDdouble visibility = 1.0;

std::ofstream csvLogFile;

void GraphicsCallback(void);
void HLCALLBACK computeForceCB(HDdouble force[3], HLcache* cache, void* userdata);
void HLCALLBACK startEffectCB(HLcache* cache, void* userdata);
void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata);

static const char* conditionName()
{
    if (CONDITION == BASELINE)   return "Baseline";
    if (CONDITION == THRESHOLD)  return "Threshold";
    return "Continuous";
}

void closeLogFile()
{
    if (csvLogFile.is_open()) {
        csvLogFile.close();
        std::cout << "\nData saved.\n";
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 1. Random phase for each of the five sines, uniform on [0, 2*pi)
    srand((unsigned)time(0));
    for (int j = 0; j < 5; j++) {
        ph[j] = ((HDdouble)rand() / RAND_MAX) * 2.0 * 3.14159265;
    }

    // 2. Append the phases of this trial to Phase_Log.csv
    {
        std::ofstream phaseLog("Phase_Log.csv", std::ios_base::app);
        if (phaseLog.is_open()) {
            phaseLog << PARTICIPANT_ID << "," << conditionName() << "," << CURRENT_TRIAL;
            for (int j = 0; j < 5; j++) phaseLog << "," << ph[j];
            phaseLog << "\n";
        }
    }

    QHWin32* DisplayObject = new QHWin32;
    DataTransportClass dataObject;

    // 3. Open the trial CSV
    char filename[128];
    sprintf(filename, "%s%d_%d.csv", PARTICIPANT_ID, (int)CONDITION, CURRENT_TRIAL);
    csvLogFile.open(filename);
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
    dataObject.cursorSphere = new Box(15.0, 15.0, 0.0);
    dataObject.cursorSphere->setName((char*)"cursorSphere");
    dataObject.cursorSphere->setShapeColor(0.85, 0.35, 0.0);
    dataObject.cursorSphere->setHapticVisibility(false);
    DisplayObject->tell(dataObject.cursorSphere);

    // Target (gray box)
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

    std::cout << "  ACTIVE SENSING DEMONSTRATION: " << conditionName() << "\n";
    std::cout << "  Goal: follow the gray box with the orange box.\n";
    std::cout << "  Logging to " << filename << "\n";

    qhStart();
    return 0;
}

// Graphics thread, once per rendered frame
void GraphicsCallback(void)
{
    QHWin32* localDisplayObject = QHWin32::searchWindow((char*)"Active Sensing Demonstration");
    Box* localCursorSphere = Box::searchBox((char*)"cursorSphere");
    Box* localModel = Box::searchBox((char*)"Skull");

    if (localDisplayObject == NULL || localCursorSphere == NULL || localModel == NULL)
        return;

    printf("\rRelVel (v_q): %7.2f | Visibility: %5.2f", relVel, visibility);

    hduVector3Dd localCursorPosition;
    hdGetDoublev(HD_CURRENT_POSITION, localCursorPosition);
    humanX = localCursorPosition[0];

    // Subject is always solid
    localCursorSphere->setTranslation(-localCursorSphere->getTranslation());
    localCursorSphere->setShapeColor(0.85, 0.35, 0.0);

    if (CONDITION == CONTINUOUS)
    {
        // Subject in front, target fades from gray into the white background
        localCursorSphere->setTranslation(localCursorPosition[0], 0, 2.0);

        HDdouble level = 1.0 - visibility;
        if (level < 0.0) level = 0.0;     // V is not capped, the renderer clamps anyway
        if (level > 1.0) level = 1.0;
        localModel->setShapeColor(level, level, level);
        localModel->setTranslation(-localModel->getTranslation());
        localModel->setTranslation(visualTargetX, 0, 0.0);
    }
    else
    {
        // BASELINE and THRESHOLD. Target in front when visible, otherwise
        // moved off screen. Recoloring alone would leave a silhouette where
        // the target overlaps the subject.
        localCursorSphere->setTranslation(localCursorPosition[0], 0, 0.0);

        if (visibility >= 1.0) {
            localModel->setShapeColor(0.25, 0.25, 0.25);
            localModel->setTranslation(-localModel->getTranslation());
            localModel->setTranslation(visualTargetX, 0, 2.0);
        }
        else {
            localModel->setTranslation(10000.0, 10000.0, 0.0);
        }
    }
}

// Servo thread, about 1 kHz
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
        actualTargetX = (cos(degInRad + ph[0]) +
                         cos(degInRad * 2.5 + ph[1]) +
                         cos(degInRad * 5.5 + ph[2]) +
                         cos(degInRad * 8.5 + ph[3]) +
                         cos(degInRad * 10 + ph[4])) * B;
    }

    // 2. Target velocity (finite difference)
    targetVel = (actualTargetX - prevActualTargetX) / deltaT;
    prevActualTargetX = actualTargetX;

    // 3. Stylus position and velocity, relative velocity v_q and position p_q
    hduVector3Dd currentPos;
    hdGetDoublev(HD_CURRENT_POSITION, currentPos);
    humanX = currentPos[0];

    hduVector3Dd currentVeloc;
    hdGetDoublev(HD_CURRENT_VELOCITY, currentVeloc);
    humanVel = currentVeloc[0];

    relVel = targetVel - humanVel;
    relPos = actualTargetX - humanX;

    // 4. Visibility
    static HDdouble smoothed_vel = 0.0;
    static bool isTargetVisible = true;

    if (CONDITION == BASELINE)
    {
        visibility = 1.0;
    }
    else if (CONDITION == THRESHOLD)
    {
        // Low-pass filter on |v_q|, then V = (a * filtered)^n, then hysteresis
        smoothed_vel = (SMOOTHING_ALPHA * fabs(relVel)) + ((1.0 - SMOOTHING_ALPHA) * smoothed_vel);
        HDdouble V = pow(A_THRESHOLD * smoothed_vel, N_EXPONENT);

        if (isTargetVisible) {
            if (V < T_LOW) isTargetVisible = false;
        }
        else {
            if (V > T_HIGH) isTargetVisible = true;
        }
        visibility = isTargetVisible ? 1.0 : 0.0;
    }
    else
    {
        // CONTINUOUS, V straight from the raw relative velocity
        visibility = pow(A_CONTINUOUS * fabs(relVel), N_EXPONENT);
    }

    // 5. Log every 10th servo tick, first TRIAL_SECONDS only
    logCounter++;
    if (logCounter >= 10) {
        if (counter <= TRIAL_SECONDS) {
            if (csvLogFile.is_open()) {
                csvLogFile << counter << ","
                    << actualTargetX << ","
                    << targetVel << ","
                    << humanX << ","
                    << humanVel << ","
                    << relPos << ","
                    << relVel << ","
                    << visibility
                    << "\n";
            }
        }
        else {
            static bool finishedPrinted = false;
            if (!finishedPrinted) {
                printf("\n--- TRIAL COMPLETE ---\n");
                finishedPrinted = true;
            }
        }
        logCounter = 0;
    }

    force[0] = 0.0;
    force[1] = 0.0;
    force[2] = 0.0;
    visualTargetX = actualTargetX;
}

void HLCALLBACK startEffectCB(HLcache* cache, void* userdata) {}
void HLCALLBACK stopEffectCB(HLcache* cache, void* userdata) {}

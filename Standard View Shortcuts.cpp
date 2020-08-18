#include <Core/CoreAll.h>
#include <Fusion/FusionAll.h>
#include <CAM/CAMAll.h>

#include <thread> 
#include <sstream>
#include <chrono>
#include <random>
#include <windows.h>

using namespace adsk::core;
using namespace adsk::fusion;

const std::string myCustomEvent = "MyCustomEventId1";

Ptr<Application> app;
Ptr<UserInterface> ui;
Ptr<CustomEvent> customEvent;
bool stopFlag;
std::default_random_engine generator;
std::uniform_int_distribution<int> distribution(1, 20);

//Ptr<Vector3D> upVector;
//Ptr<Point3D> homeEye;
time_t lastChangeViewTimer;

class ThreadEventHandler : public CustomEventHandler
{
public:
	void notify(const Ptr<CustomEventArgs>& eventArgs) override
	{
		// ALT key hold
		if (!(GetKeyState(VK_MENU) & 0x8000))
			return;

		// Trigger event once
		time_t timer;
		time(&timer);
		if (difftime(timer, lastChangeViewTimer) <= 1) // 1.5
			return;

		Ptr<Camera> camera = app->activeViewport()->camera();
		if (!camera)
			return;

		std::vector<double> asArray = camera->upVector()->asArray();

		// Sets the default upVector
		Ptr<Vector3D> upVector = Vector3D::create(0, 1, 0);

		ViewOrientations viewOrientationOld = camera->viewOrientation();
		ViewOrientations viewOrientationNew = viewOrientationOld;

		// Front/back
		if (GetKeyState('1') & 0x8000)
			viewOrientationNew = FrontViewOrientation;
		else if (GetKeyState(0x51 /*Q*/) & 0x8000)
			viewOrientationNew = BackViewOrientation;

		// Left/right
		else if (GetKeyState('2') & 0x8000)
			viewOrientationNew = LeftViewOrientation;
		else if (GetKeyState(0x57 /*W*/) & 0x8000)
			viewOrientationNew = RightViewOrientation;

		// Top/bottom
		else if (GetKeyState('3') & 0x8000)
		{
			viewOrientationNew = TopViewOrientation;
			//upVector = Vector3D::create(0, 0, -1);
		}
		else if (GetKeyState(0x45 /*E*/) & 0x8000)
		{
			viewOrientationNew = BottomViewOrientation;
			//upVector = Vector3D::create(0, 0, -1);
		}

		// Iso top left/bottom left
		else if (GetKeyState('4') & 0x8000)
			viewOrientationNew = IsoTopLeftViewOrientation;
		else if (GetKeyState(0x52 /*R*/) & 0x8000)
			viewOrientationNew = IsoBottomLeftViewOrientation;

		// Iso top right/bottom right
		else if (GetKeyState('5') & 0x8000)
			viewOrientationNew = IsoTopRightViewOrientation;
		else if (GetKeyState(0x54 /*T*/) & 0x8000)
			viewOrientationNew = IsoBottomRightViewOrientation;

		if (viewOrientationOld == viewOrientationNew)
			return;

		camera->viewOrientation(viewOrientationNew);
		camera->isFitView(true);
		camera->upVector(upVector);

		app->activeViewport()->camera(camera);
		lastChangeViewTimer = timer;
	}
} onCustomEvent_;

void myThreadRun()
{
	while (!stopFlag) {
		int randint = distribution(generator);

		std::string additionalInfo = std::to_string(randint);
		app->fireCustomEvent(myCustomEvent, additionalInfo);

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

extern "C" XI_EXPORT bool run(const char* context)
{
	app = Application::get();
	if (!app)
		return false;

	ui = app->userInterface();
	if (!ui)
		return false;

	customEvent = app->registerCustomEvent(myCustomEvent);
	if (!customEvent)
	{
		ui->messageBox("!customEvent");
		return false;
	}

	if (!customEvent->add(&onCustomEvent_))
	{
		ui->messageBox("!customEvent->add(&onCustomEvent_)");
		return false;
	}

	stopFlag = false;
	std::thread myThread(myThreadRun);
	myThread.detach();

	return true;
}

extern "C" XI_EXPORT bool stop(const char* context)
{
	if (ui)
	{
		customEvent->remove(&onCustomEvent_);
		stopFlag = true;
		app->unregisterCustomEvent(myCustomEvent);
		ui = nullptr;
	}

	return true;
}


#ifdef XI_WIN

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif // XI_WIN
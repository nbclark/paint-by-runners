#include "Main.h"
#include "Iw2d.h"
#include "s3eLocation.h"

class CIwUIController2 : CIwUIController
{
	struct EventStruct
	{
		CIwUIElement* pReceiver;
		CIwUIEventClick* pEvent;
	};
public:
	~CIwUIController2()
	{
		for (uint32 i = 0; i < g_events.size(); ++i)
		{
			CIwUIEventClick* pEvent = g_events[i].pEvent;
			delete pEvent;
		}

		g_events.clear();
	}
	void Update()
	{
		CIwUIController::Update();

		if (g_events.size())
		{
			for (uint32 i = 0; i < g_events.size(); ++i)
			{
				CIwUIElement* pReceiver = g_events[i].pReceiver;
				CIwUIEventClick* pEvent = g_events[i].pEvent;
				CIwUIController::Notify(g_events[i].pReceiver, g_events[i].pEvent);
			}

			g_events.clear();
			CIwUIController::Update();
		}
	}
protected:
	bool Notify(CIwUIElement* pReceiver, CIwEvent* pEvent)
	{
		int id = pEvent->GetID();

		if (id == IWUI_EVENT_CLICK)
		{
			CIwUIEventClick* a = (CIwUIEventClick*)pEvent;
			
			if (!a->GetPressed())
			{
				EventStruct x;
				x.pReceiver = pReceiver;
				x.pEvent = IW_GX_ALLOC(CIwUIEventClick, 1);

				memcpy(x.pEvent, a, sizeof(CIwUIEventClick));

				g_events.append(x);

				return true;
			}
		}

		return CIwUIController::Notify(pReceiver, pEvent);
		if (pEvent->GetSender())
		{
			s3eDeviceYield(0);
			return true;
		}
		return CIwUIController::FilterEvent(pEvent);
	}
	CIwArray<EventStruct> g_events;
};

int main()
{
	
	//s3eInetAddress addr;
	//memset(&addr, 0, sizeof(addr));
	//s3eInetLookup("localhost", &addr, NULL, NULL);
	//addr.m_Port = s3eInetHtons(81);

	//s3eSocket* pSock = s3eSocketCreate(S3E_SOCKET_UDP, 0);
	//char szData[20000];
	//for (int i=0; i <20000;++i)
	//{
	//	szData[i] = i;
	//}
	//while (true)
	//{
	//	int sent = s3eSocketSendTo(pSock, szData, 20000, 0, &addr);
	//	s3eDeviceYield(0);
	//}

	s3eLocationStart();

	//IwGx can be initialised in a number of different configurations to help the linker eliminate unused code.
	//For the examples we link in all of IwGx's possible functionality.
	//You can see the code size reduce if you remove any of the IwGxInit calls following IwGxInit_Base.
	//Note that, for demonstration purposes, some examples call IwGxInit() themselves - this will link in the 
	//standard renderer and the GL renderer, so on those examples these features cannot be excluded from the build.
	IwGxInit_Base();
	IwGxInit_GLRender();

	Iw2DInit();
	IwUIInit();

	//Instantiate the view and controller singletons.
	//IwUI will not instantiate these itself, since they can be subclassed to add functionality.
	new CIwUIView;
	CIwUIController2* pController = new CIwUIController2;

	IwDebugTraceLinePrintf("Creating Game Object");
	IwDebugTraceLinePrintf("Entering Main");
	int result = GameMain();

	Iw2DTerminate();
	
	delete IwGetUIView();
	delete pController;
	IwUITerminate();
	IwGxTerminate();
	s3eLocationStop();


	return result;
}

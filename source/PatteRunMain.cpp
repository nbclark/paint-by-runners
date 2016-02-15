#include "Main.h"
#include "PatterunGameEngine.h"

int main()
{
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

	PatteRunGameEngine* pGameEngine = new PatteRunGameEngine;
	int result = GameMain(pGameEngine);
	delete pGameEngine;

	Iw2DTerminate();
	IwUITerminate();
	IwGxTerminate();
	s3eLocationStop();

	return result;
}

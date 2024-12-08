#include <iostream>
#include "App.h"


int main(int argc, char* argv[])
{
	App app;

	if (argc == 1)
	{
		app.Init();
	}
	else if (argc == 2)
	{
		std::string title = argv[1];
		app.InitWithStartCapture(title);
	}

	app.Run();
	app.Close();

	return 0;
}
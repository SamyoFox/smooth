/*
 * smooth tutorial 1
 */

#include <smooth.h>
#include <smooth/main.h>

#include "t01.h"

Int smooth::Main()
{
	return Hello().Loop();
}

Hello::Hello()
{
	mainWnd	= new Window("Hello world!", Point(100, 100), Size(200, 100));
	title	= new Titlebar(TB_CLOSEBUTTON);
	hello	= new Button("Hello world!", Point(20, 15), Size(160, 40));

	mainWnd->Add(hello);
	mainWnd->Add(title);

	Add(mainWnd);
}

Hello::~Hello()
{
	DeleteObject(mainWnd);
	DeleteObject(title);
	DeleteObject(hello);
}

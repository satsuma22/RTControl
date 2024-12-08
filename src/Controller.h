#pragma once

#include <string>
#include <Windows.h>

struct KeyboardEvent
{
	enum TYPE
	{
		KEYDOWN, KEYUP
	};

	bool altKey = false;
	bool ctrlKey = false;
	bool shiftKey = false;
	bool repeat = false;
	int code;
	std::string key;
	TYPE type;
};

struct MouseEvent
{
	enum TYPE
	{
		MOUSEDOWN, MOUSEUP, MOUSEMOVE
	};

	bool altKey = false;
	bool ctrlKey = false;
	bool shiftKey = false;
	unsigned int button = 0;		// 0: left-click, 1: scroll-click, 2: right-click, 3: fourth-button (browser-back), 4: fifth-button (browser forward)
	unsigned int offsetX = 0;
	unsigned int offsetY = 0;
	TYPE type;
};


struct WheelEvent
{
	bool altKey = false;
	bool ctrlKey = false;
	bool shiftKey = false;
	unsigned int button = 0;
	unsigned int offsetX = 0;
	unsigned int offsetY = 0;
	int deltaX = 0;
	int deltaY = 0;
};

class WindowController
{
public:
	WindowController();
	WindowController(LPCSTR title);
	WindowController(HWND handle);

	void AttachController(HWND hwnd);
	
	void GenerateInput(KeyboardEvent event, bool scancode);
	void GenerateInput(WheelEvent event);
	void GenerateInput(MouseEvent event, bool isMonitorCapture = false);

	HWND GetHandle() { return m_Handle; }
private:
	HWND m_Handle;
};

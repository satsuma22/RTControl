#include "Controller.h"
#include <iostream>

WindowController::WindowController()
	:m_Handle(nullptr)
{

}

WindowController::WindowController(LPCWSTR title)
{
	m_Handle = FindWindow(NULL, title);
	if (m_Handle)
	{
		std::cout << "SUCCESS: Attached Controller to ";
		std::wcout << title << "\n";

		SetForegroundWindow(m_Handle);
	}
}

WindowController::WindowController(HWND handle)
	: m_Handle(handle)
{
}

void WindowController::AttachController(HWND hwnd)
{
	std::cout << "Attaching controller to application...\n";
	m_Handle = hwnd;
}

void WindowController::GenerateInput(KeyboardEvent event, bool scancode)
{
	if (!SetForegroundWindow(m_Handle)) return;

	INPUT input = {};
	input.type = INPUT_KEYBOARD;
	input.ki.wScan = event.code;
	input.ki.dwFlags = KEYEVENTF_SCANCODE;

	if (event.type == KeyboardEvent::KEYUP) input.ki.dwFlags |= KEYEVENTF_KEYUP;

	SendInput(1, &input, sizeof(INPUT));
}

void WindowController::GenerateInput(WheelEvent event)
{
	if (!SetForegroundWindow(m_Handle)) return;


	RECT rect;
	GetWindowRect(m_Handle, &rect);

	INPUT input = {};
	input.type = INPUT_MOUSE;
	input.mi.dx = (event.offsetX + rect.left + 0) * (65535 / GetSystemMetrics(SM_CXSCREEN)); // Calculate the x-coordinate relative to the screen resolution.
	input.mi.dy = (event.offsetY + rect.top + 0) * (65535 / GetSystemMetrics(SM_CYSCREEN)); // Calculate the y-coordinate relative to the screen resolution.

	if (event.deltaX != 0)
	{
		input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
		input.mi.mouseData = event.deltaX;
	}
	else if (event.deltaY != 0)
	{
		input.mi.dwFlags = MOUSEEVENTF_WHEEL;
		input.mi.mouseData = event.deltaY;
	}

	SendInput(1, &input, sizeof(INPUT));
}

void WindowController::GenerateInput(MouseEvent event, bool isMonitorCapture)
{
	if (!SetForegroundWindow(m_Handle)) return;

	INPUT input = { 0 };

	if (isMonitorCapture)
	{
		RECT rect;

		GetWindowRect(m_Handle, &rect);

		// Windows adds a border around the application. Need to crop that border to set accurate mouse position
		rect.left += 8;
		rect.top += 8;

		input.type = INPUT_MOUSE;
		input.mi.dx = 1.0035 * (event.offsetX + rect.left) * (65535 / GetSystemMetrics(SM_CXSCREEN)); // Calculate the x-coordinate relative to the screen resolution.
		input.mi.dy = 1.01 * (event.offsetY + rect.top) * (65535 / GetSystemMetrics(SM_CYSCREEN)); // Calculate the y-coordinate relative to the screen resolution.
		input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;

	}
	else
	{

		RECT rect;

		GetWindowRect(m_Handle, &rect);

		// Windows adds a border around the application. Need to crop that border to set accurate mouse position
		rect.left += 6;
		rect.top += 0;

		input.type = INPUT_MOUSE;
		input.mi.dx = 1.0035 * (event.offsetX + rect.left) * (65535 / GetSystemMetrics(SM_CXSCREEN)); // Calculate the x-coordinate relative to the screen resolution.
		input.mi.dy = 1.01 * (event.offsetY + rect.top) * (65535 / GetSystemMetrics(SM_CYSCREEN)); // Calculate the y-coordinate relative to the screen resolution.
		input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
	}

	if (event.type == MouseEvent::MOUSEMOVE) input.mi.dwFlags |= MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK;
	else if (event.type == MouseEvent::MOUSEDOWN)
	{
		switch (event.button)
		{
		case 0:	input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
			break;
		case 1: input.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
			break;
		case 2: input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
			break;
		}
	}
	else if (event.type == MouseEvent::MOUSEUP)
	{
		switch (event.button)
		{
		case 0:	input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
			break;
		case 1: input.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
			break;
		case 2: input.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
			break;
		}
	}

	SendInput(1, &input, sizeof(INPUT));
}
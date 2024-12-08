#pragma once
#include "pch.h"
#include "Capturer.h"
#include "../PacketQueue.h"

#include <DispatcherQueue.h>

class CaptureApp
{
public:
	CaptureApp(PacketQueue* packetQueue, HWND hwnd);
	~CaptureApp() { StopCapture();  d3dDevice->Release(); }

	winrt::Windows::Graphics::Capture::GraphicsCaptureItem TryStartCaptureFromWindowHandle(HWND hwnd);
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem TryStartCaptureFromMonitorHandle(HMONITOR hmon);
	winrt::Windows::Graphics::DirectX::DirectXPixelFormat PixelFormat() { return m_pixelFormat; }

	void StopCapture();

private:
	void StartCaptureFromItem(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item);

private:
	HWND m_hwnd;
	winrt::Windows::System::DispatcherQueue m_mainThread{ nullptr };
	winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
	std::unique_ptr<Capturer> m_capture{ nullptr };
	winrt::Windows::Graphics::DirectX::DirectXPixelFormat m_pixelFormat = winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized;

	winrt::com_ptr<ID3D11Device> d3dDevice;

	PacketQueue* m_packetQueue;
};

#include "CaptureApp.h"

#include "util.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "CoreMessaging.lib")

extern "C"
{
	HRESULT __stdcall CreateDirect3D11DeviceFromDXGIDevice(::IDXGIDevice* dxgiDevice,
		::IInspectable** graphicsDevice);

	HRESULT __stdcall CreateDirect3D11SurfaceFromDXGISurface(::IDXGISurface* dgxiSurface,
		::IInspectable** graphicsSurface);
}

CaptureApp::CaptureApp(PacketQueue* packetQueue, HWND hwnd)
{
	m_hwnd = hwnd;

	m_mainThread = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
	//WINRT_VERIFY(m_mainThread != nullptr);

	HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, d3dDevice.put(),
		nullptr, nullptr);
	winrt::check_hresult(hr);

	auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
	// Not sure but this might make things faster
	dxgiDevice->SetGPUThreadPriority(7);

	winrt::com_ptr<::IInspectable> d3d_device;
	winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), d3d_device.put()));
	m_device = d3d_device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

	m_packetQueue = packetQueue;
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem CaptureApp::TryStartCaptureFromWindowHandle(HWND hwnd)
{
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{ nullptr };
	try
	{
		item = util::CreateCaptureItemForWindow(hwnd);
		StartCaptureFromItem(item);
	}
	catch (winrt::hresult_error const& error)
	{
		MessageBoxW(nullptr,
			error.message().c_str(),
			L"Win32CaptureSample",
			MB_OK | MB_ICONERROR);
	}
	return item;
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem CaptureApp::TryStartCaptureFromMonitorHandle(HMONITOR hmon)
{
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{ nullptr };
	try
	{
		item = util::CreateCaptureItemForMonitor(hmon);
		StartCaptureFromItem(item);
	}
	catch (winrt::hresult_error const& error)
	{
		MessageBoxW(nullptr,
			error.message().c_str(),
			L"Win32CaptureSample",
			MB_OK | MB_ICONERROR);
	}
	return item;
}

void CaptureApp::StopCapture()
{
	if (m_capture)
	{
		m_capture->Close();
		m_capture = nullptr;
	}
}

void CaptureApp::StartCaptureFromItem(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item)
{
	m_capture = std::make_unique<Capturer>(m_device, item, m_pixelFormat, m_packetQueue, m_hwnd);
	m_capture->StartCapture();
}
#pragma once

#include <fstream>

#include "pch.h"
#include "NvEncoder/NvEncoderD3D11.h"
#include "../PacketQueue.h"

struct EncoderSetting
{
    unsigned int bitrateAvg = 0;
    unsigned int bitrateMax = 0;
    unsigned int fps = 0;
    unsigned int gopLength = 0;
    unsigned int idrPeriod = 0;
    unsigned int iFramesOnly = 0;
    unsigned int outputDelay = 3;
    unsigned int preset = 0;
    unsigned int QP = 0;
    unsigned int targetQP = 0;
    std::string rateControlMode = "";

    bool useMonitorCapture = true;
};

struct CapturerSettings
{
    unsigned int captureCursor = 1;
};

class Capturer
{
public:
    Capturer(
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item,
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixelFormat,
        PacketQueue* packetQueue,
        HWND hwnd);
    ~Capturer() { Close(); }

    void StartCapture();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem CaptureItem() { return m_item; }

    void Close();

private:
    void OnFrameArrivedForMonitorCapture(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);

    void OnFrameArrivedForWindowCapture(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);

    bool TryResizeSwapChainForMonitorCapture(winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame const& frame);
    bool TryResizeSwapChainForWindowCapture(winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame const& frame);
    
    void ResizeSwapChain();
    void InitializeEncoder(ID3D11Device* pD3D11Device, int width, int height, NV_ENC_BUFFER_FORMAT fmt = NV_ENC_BUFFER_FORMAT_ARGB);

    void ReadConfigfile();
   
    void WritePacketToQueue();

private:
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
    winrt::Windows::Graphics::SizeInt32 m_lastSize;

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    winrt::com_ptr<IDXGISwapChain1> m_swapChain{ nullptr };
    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
    winrt::Windows::Graphics::DirectX::DirectXPixelFormat m_pixelFormat;

    ID3D11Device* pD3D11Device = nullptr;

    // Encoder
    NvEncoderD3D11* pEnc = nullptr;
    ID3D11Texture2D* pEncBuf = nullptr;
    /// NVENCODEAPI session intialization parameters
    NV_ENC_INITIALIZE_PARAMS encInitParams = { 0 };
    /// NVENCODEAPI video encoding configuration parameters
    NV_ENC_CONFIG encConfig = { 0 };

    /// Encoded video bitstream packet in CPU memory
    std::vector<std::vector<uint8_t>> vPacket;

    PacketQueue* m_packetQueue;
    unsigned int currentFrame = 0;

    EncoderSetting m_settings;
    CapturerSettings m_capSettings;
    HWND m_hwnd;
    RECT m_rect;
};


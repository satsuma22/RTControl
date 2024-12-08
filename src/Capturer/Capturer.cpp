#include "pch.h"
#include "Capturer.h"
#include <iostream>
#include <nvEncodeAPI.h>
#include <nlohmann/json.hpp>


#define returnIfError(x)\
    if (FAILED(x))\
    {\
        printf(__FUNCTION__": Line %d, File %s Returning error 0x%08x\n", __LINE__, __FILE__, x);\
        \
    }

struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
    IDirect3DDxgiInterfaceAccess : ::IUnknown
{
    virtual HRESULT __stdcall GetInterface(GUID const& id, void** object) = 0;
};

template <typename T>
auto GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object)
{
    auto access = object.as<IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

inline auto CreateDXGISwapChain(winrt::com_ptr<ID3D11Device> const& device, const DXGI_SWAP_CHAIN_DESC1* desc)
{
    auto dxgiDevice = device.as<IDXGIDevice2>();
    winrt::com_ptr<IDXGIAdapter> adapter;
    winrt::check_hresult(dxgiDevice->GetParent(winrt::guid_of<IDXGIAdapter>(), adapter.put_void()));
    winrt::com_ptr<IDXGIFactory2> factory;
    winrt::check_hresult(adapter->GetParent(winrt::guid_of<IDXGIFactory2>(), factory.put_void()));

    winrt::com_ptr<IDXGISwapChain1> swapchain;
    winrt::check_hresult(factory->CreateSwapChainForComposition(device.get(), desc, nullptr, swapchain.put()));
    return swapchain;
}

inline auto CreateDXGISwapChain(winrt::com_ptr<ID3D11Device> const& device,
    uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t bufferCount)
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.Format = format;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferCount = bufferCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    return CreateDXGISwapChain(device, &desc);
}


Capturer::Capturer(winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device, winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item, winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixelFormat, PacketQueue* packetQueue, HWND hwnd)
{
    ReadConfigfile();

    m_item = item;
    m_device = device;
    m_pixelFormat = pixelFormat;
    m_hwnd = hwnd;
    m_packetQueue = packetQueue;

    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    d3dDevice->GetImmediateContext(m_d3dContext.put());

    pD3D11Device = d3dDevice.get();
    
    m_swapChain = CreateDXGISwapChain(d3dDevice, static_cast<uint32_t>(m_item.Size().Width), static_cast<uint32_t>(m_item.Size().Height),
        static_cast<DXGI_FORMAT>(m_pixelFormat), 2);

    m_framePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(m_device, m_pixelFormat, 1, m_item.Size());
    m_session = m_framePool.CreateCaptureSession(m_item);
    m_lastSize = m_item.Size();

    m_session.IsCursorCaptureEnabled(m_capSettings.captureCursor);

    if (m_settings.useMonitorCapture)
    {
        m_framePool.FrameArrived({ this, &Capturer::OnFrameArrivedForMonitorCapture });
        
        GetWindowRect(m_hwnd, &m_rect);
        // Windows adds an 8 px border around the application window
        m_rect.left += 8;
        m_rect.right -= 8;
        m_rect.top += 8;
        m_rect.bottom -= 8;

        // Initialize the encoder
        InitializeEncoder(pD3D11Device, m_rect.right - m_rect.left , m_rect.bottom - m_rect.top );

    }
    else
    {
        m_framePool.FrameArrived({ this, &Capturer::OnFrameArrivedForWindowCapture });

        InitializeEncoder(pD3D11Device, m_lastSize.Width, m_lastSize.Height);
    }

}

void Capturer::StartCapture()
{
    m_session.StartCapture();
}

void Capturer::Close()
{
    pEnc->EndEncode(vPacket);
    WritePacketToQueue();
    pEnc->DestroyEncoder();

    m_session.Close();
    m_framePool.Close();

    m_swapChain = nullptr;
    m_framePool = nullptr;
    m_session = nullptr;
    m_item = nullptr;

    m_device = nullptr;
}

void Capturer::ResizeSwapChain()
{
    winrt::check_hresult(m_swapChain->ResizeBuffers(2, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height),
        static_cast<DXGI_FORMAT>(m_pixelFormat), 0));
}

bool Capturer::TryResizeSwapChainForMonitorCapture(winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame const& frame)
{
    // Force the window to stay un-minimized
    ShowWindow(m_hwnd, SW_NORMAL);
    
    RECT rect;
    GetWindowRect(m_hwnd, &rect);
    rect.left += 8;
    rect.right -= 8;
    rect.top += 8;
    rect.bottom -= 8;

    if (rect != m_rect)
    {
        // The thing we have been capturing has changed size

        if (rect.right - rect.left <= 0 || rect.bottom - rect.top <= 0)
        {
            std::cout << "Application was either minimized or closed.\n";
        }

        m_rect = rect;
        return true;
    }

    return false;
}

bool Capturer::TryResizeSwapChainForWindowCapture(winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame const& frame)
{
    auto const contentSize = frame.ContentSize();
    
    if ((contentSize.Width != m_lastSize.Width) ||
        (contentSize.Height != m_lastSize.Height))
    {
        // The thing we have been capturing has changed size, resize the swap chain to match.
     
        m_lastSize = contentSize;
        ResizeSwapChain();
        return true;
    }

    return false;
}

void Capturer::InitializeEncoder(ID3D11Device* pD3D11Device, int width, int height, NV_ENC_BUFFER_FORMAT fmt)
{
    if (pEnc) 
        delete pEnc;

    pEnc = new NvEncoderD3D11(pD3D11Device, width, height, fmt, m_settings.outputDelay, false, false);

    std::cout << "Application Window Size: " << width << ", " << height << std::endl;

    if (!pEnc)
    {
        std::cout << "Encoder could not be created!\n";
    }

    memset(&encInitParams, 0, sizeof(encInitParams));
    memset(&encConfig, 0, sizeof(encConfig));

    encInitParams.encodeConfig = &encConfig;


    try
    {
        GUID Preset;

        switch (m_settings.preset)
        {
            case 1: Preset = NV_ENC_PRESET_P1_GUID;
                    break;
            case 2: Preset = NV_ENC_PRESET_P2_GUID;
                    break;
            case 3: Preset = NV_ENC_PRESET_P3_GUID;
                    break;
            case 4: Preset = NV_ENC_PRESET_P4_GUID;
                    break;
            case 5: Preset = NV_ENC_PRESET_P5_GUID;
                    break;
            case 6: Preset = NV_ENC_PRESET_P6_GUID;
                    break;
            case 7: Preset = NV_ENC_PRESET_P7_GUID;
                    break;
            default: Preset = NV_ENC_PRESET_P4_GUID;
        }

        pEnc->CreateDefaultEncoderParams(&encInitParams, NV_ENC_CODEC_H264_GUID, Preset, NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY);


        auto config = encInitParams.encodeConfig;

        // Set the frame rate
        encInitParams.frameRateDen = 1;
        encInitParams.frameRateNum = m_settings.fps;
        
        // Set the GOP length. 0 implies infinite GOP length
        if (m_settings.gopLength) config->gopLength = m_settings.gopLength;
        else config->gopLength = NVENC_INFINITE_GOPLENGTH;

        // Set whether or not to use only I frames
        if (m_settings.iFramesOnly) config->frameIntervalP = 0;
        else config->frameIntervalP = 1;

        config->profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;

        // Set the IDR period
        if (m_settings.idrPeriod) config->encodeCodecConfig.h264Config.idrPeriod = m_settings.idrPeriod;
        config->encodeCodecConfig.h264Config.repeatSPSPPS = 1;
      
        // Set the Rate Control Mode 
        if (m_settings.rateControlMode == "CBR")
        {
            config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
            config->rcParams.averageBitRate = m_settings.bitrateAvg;
        }
        else if (m_settings.rateControlMode == "VBR")
        {
            config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;
            config->rcParams.averageBitRate = m_settings.bitrateAvg;
            config->rcParams.maxBitRate = m_settings.bitrateMax;
        }
        else if (m_settings.rateControlMode == "CQP")
        {
            config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
            uint32_t x = m_settings.QP;
            config->rcParams.constQP = { x, x, x }; // QP for P, B, and I frames respectively
        }
        else if (m_settings.rateControlMode == "Target Quality")
        {
            config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;
            config->rcParams.targetQuality = m_settings.targetQP;
            config->rcParams.maxBitRate = m_settings.bitrateMax;
        }

        pEnc->CreateEncoder(&encInitParams);
    }
    catch (NVENCException e)
    {
        std::cout << "[Capturer] An error occured while creating the encoder. Hint: the application being captured was either closed or minimized.\n";
        std::cout << e.getErrorCode() << ": " << e.getErrorString();
        returnIfError(E_FAIL);
        Close();
    }
}

void Capturer::ReadConfigfile()
{
    std::fstream f("config.json");
    nlohmann::json settings = nlohmann::json::parse(f);

    std::cout << settings << std::endl;

    m_settings.bitrateAvg = settings["Encoder"]["Bitrate Average"];
    std::cout << m_settings.bitrateAvg << std::endl; 
    m_settings.bitrateMax = settings["Encoder"]["Bitrate Max"];
    std::cout << m_settings.bitrateMax << std::endl;
    m_settings.fps = settings["Encoder"]["fps"];
    std::cout << m_settings.fps << std::endl;
    m_settings.gopLength = settings["Encoder"]["GOP Length"];
    std::cout << m_settings.gopLength << std::endl;
    m_settings.idrPeriod = settings["Encoder"]["IDR Period"];
    std::cout << m_settings.idrPeriod << std::endl;
    m_settings.iFramesOnly = settings["Encoder"]["I-Frames Only"];
    std::cout << m_settings.iFramesOnly << std::endl;
    m_settings.outputDelay = settings["Encoder"]["OutputDelay"];
    std::cout << m_settings.outputDelay << std::endl;
    m_settings.preset = settings["Encoder"]["Preset"];
    std::cout << m_settings.preset << std::endl;
    m_settings.rateControlMode = settings["Encoder"]["Rate Control Mode"];
    std::cout << m_settings.rateControlMode << std::endl;
    m_settings.QP = settings["Encoder"]["QP"];
    std::cout << m_settings.QP << std::endl;
    m_settings.targetQP = settings["Encoder"]["Target Quality"];
    std::cout << m_settings.targetQP << std::endl;

    m_settings.useMonitorCapture = settings["Global"]["CaptureMode"] == "Monitor";

    m_capSettings.captureCursor = settings["Global"]["CaptureCursor"];
}

void Capturer::WritePacketToQueue()
{
    int nFrame = 0;
    nFrame = (int)vPacket.size();

    for (std::vector<uint8_t>& packet : vPacket)
    {
        currentFrame++;
        m_packetQueue->Insert(packet);
    }

}

void Capturer::OnFrameArrivedForMonitorCapture(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args)
{
    bool swapChainResizedToFrame = false;

    auto frame = sender.TryGetNextFrame();
    {
       // auto time = frame.SystemRelativeTime().count();
        //auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        //std::cout << "Frame: " << time << std::endl;
        //std::cout << "Now:   " <<  now / 100 << std::endl;
    }
    swapChainResizedToFrame = TryResizeSwapChainForMonitorCapture(frame);

    if (!m_packetQueue->IsWriteReady())
        return;
    
    StatPacket packet;
    packet.Start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    const NvEncInputFrame* pEncInput = pEnc->GetNextInputFrame();
    pEncBuf = (ID3D11Texture2D*)pEncInput->inputPtr;

    auto surfaceTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    //m_d3dContext->CopyResource(pEncBuf, surfaceTexture.get());
    
    {
        D3D11_BOX box;
        box.left = m_rect.left;
        box.top = m_rect.top;
        box.front = 0;
        box.right = m_rect.right;
        box.bottom = m_rect.bottom;
        box.back = 1;

        m_d3dContext->CopySubresourceRegion(pEncBuf, 0, 0, 0, 0, surfaceTexture.get(), 0, &box);
    }

    DXGI_PRESENT_PARAMETERS presentParameters{};
    m_swapChain->Present1(1, 0, &presentParameters);

    packet.FrameCopy = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    {
        pEnc->EncodeFrame(vPacket);
        packet.Encode = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    {
        packet.id = currentFrame;
        m_packetQueue->SetBackPacket(packet);
        
        WritePacketToQueue();
    }

    if (swapChainResizedToFrame)
    {
        InitializeEncoder(pD3D11Device, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top);
    }
    
}

void Capturer::OnFrameArrivedForWindowCapture(winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& args)
{
    bool swapChainResizedToFrame = false;

    auto frame = sender.TryGetNextFrame();
    swapChainResizedToFrame = TryResizeSwapChainForWindowCapture(frame);

    if (!m_packetQueue->IsWriteReady())
        return;

    StatPacket packet;
    packet.Start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    const NvEncInputFrame* pEncInput = pEnc->GetNextInputFrame();
    pEncBuf = (ID3D11Texture2D*)pEncInput->inputPtr;

    auto surfaceTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    m_d3dContext->CopyResource(pEncBuf, surfaceTexture.get());

    DXGI_PRESENT_PARAMETERS presentParameters{};
    m_swapChain->Present1(1, 0, &presentParameters);

    packet.FrameCopy = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    {
        pEnc->EncodeFrame(vPacket);
        packet.Encode = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    {
        packet.id = currentFrame;
        m_packetQueue->SetBackPacket(packet);
        WritePacketToQueue();
    }

    if (swapChainResizedToFrame)
    {
        m_framePool.Recreate(m_device, m_pixelFormat, 1, m_lastSize);
        InitializeEncoder(pD3D11Device, m_lastSize.Width, m_lastSize.Height);
    }

}

#include "pch.h"
#include <DispatcherQueue.h>

#include <winrt/Windows.Graphics.Capture.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.capture.h>

namespace util
{
    namespace impl
    {
        inline void ShutdownAndThenPostQuitMessage(winrt::Windows::System::DispatcherQueueController const& controller, int exitCode)
        {
            auto action = controller.ShutdownQueueAsync();
            action.Completed([exitCode](auto&&, auto&&)
                {
                    PostQuitMessage(exitCode);
                });
        }
    }

    inline auto CreateDispatcherQueueControllerForCurrentThread()
    {
        namespace abi = ABI::Windows::System;

        DispatcherQueueOptions options
        {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_NONE
        };

        winrt::Windows::System::DispatcherQueueController controller{ nullptr };
        winrt::check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<abi::IDispatcherQueueController**>(winrt::put_abi(controller))));
        return controller;
    }


    inline int ShutdownDispatcherQueueControllerAndWait(winrt::Windows::System::DispatcherQueueController const& controller, int exitCode)
    {
        impl::ShutdownAndThenPostQuitMessage(controller, exitCode);
        MSG msg = {};
        while (GetMessageW(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return static_cast<int>(msg.wParam);
    }

    inline auto CreateCaptureItemForWindow(HWND hwnd)
    {
        auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
        winrt::check_hresult(interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
        return item;
    }

    inline auto CreateCaptureItemForMonitor(HMONITOR hmon)
    {
        auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
        winrt::check_hresult(interop_factory->CreateForMonitor(hmon, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
        return item;
    }
}
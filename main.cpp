#include <iostream>

#include <Windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <DispatcherQueue.h>

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::System;
using namespace winrt::Windows::Storage::Streams;

using namespace std::chrono_literals;

#define INTERVAL 1s

IAsyncAction observe_async();
IAsyncAction read_sessions_async(GlobalSystemMediaTransportControlsSessionManager&);
std::string represent(winrt::hstring const&);

int main()
{
    auto action = observe_async();
    action.get();
}

IAsyncAction observe_async()
{
    auto manager = co_await GlobalSystemMediaTransportControlsSessionManager::RequestAsync();

    while (true) {
        co_await read_sessions_async(manager);
        co_await INTERVAL;
    }
}

IAsyncAction read_sessions_async(GlobalSystemMediaTransportControlsSessionManager& manager)
{
    for (auto session : manager.GetSessions()) {
        GlobalSystemMediaTransportControlsSessionMediaProperties properties{nullptr};
        try {
            properties = co_await session.TryGetMediaPropertiesAsync();
        }
        catch (winrt::hresult_error const& err) {
            std::cout << "failed to get media properties: "
                << winrt::to_string(err.message()) << std::endl;
            continue;
        }
        std::cout << "player: " << represent(session.SourceAppUserModelId()) << std::endl;
        std::cout << "title:  " << represent(properties.Title()) << std::endl;
        std::cout << "artist: " << represent(properties.Artist()) << std::endl;
        std::cout << "album:  " << represent(properties.AlbumTitle()) << std::endl;
        std::cout << std::endl;
    }
    co_return;
}

std::string represent(winrt::hstring const& value)
{
    std::string str{ winrt::to_string(value) };
    if (str.empty()) {
        return "<empty>";
    }
    return str;
}

#include <iostream>
#include <chrono>

#include <Windows.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Media;
using namespace std::chrono_literals;

#define INTERVAL 1s

IAsyncAction observe_async();
IAsyncAction read_sessions_async(GlobalSystemMediaTransportControlsSessionManager&);
std::string represent(winrt::hstring const&);
std::string represent_playback_type(MediaPlaybackType);
std::string represent_playback_status(GlobalSystemMediaTransportControlsSessionPlaybackStatus);
bool represent_playing(GlobalSystemMediaTransportControlsSessionPlaybackStatus);

int main()
{
    SetConsoleOutputCP(CP_UTF8);
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

template<typename R, typename P>
inline std::chrono::milliseconds to_millis(std::chrono::duration<R, P> value)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(value);
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
        auto playback_info = session.GetPlaybackInfo();
        auto playback_status = playback_info.PlaybackStatus();
        auto timeline_properties = session.GetTimelineProperties();
        auto position = to_millis(timeline_properties.Position());
        auto position_timestamp = to_millis(winrt::clock::to_sys(timeline_properties.LastUpdatedTime()).time_since_epoch());
        auto now = to_millis(std::chrono::system_clock::now().time_since_epoch());
        auto position_live = represent_playing(playback_status) ? position + (now - position_timestamp) : position;
        auto duration = to_millis(timeline_properties.EndTime() - timeline_properties.StartTime());
        std::cout << "AppUserModelId: " << represent(session.SourceAppUserModelId()) << std::endl;
        std::cout << "Title: " << represent(properties.Title()) << std::endl;
        std::cout << "Subtitle: " << represent(properties.Subtitle()) << std::endl;
        std::cout << "Artist: " << represent(properties.Artist()) << std::endl;
        std::cout << "AlbumTitle: " << represent(properties.AlbumTitle()) << std::endl;
        std::cout << "AlbumArtist: " << represent(properties.AlbumArtist()) << std::endl;
        std::cout << "PlaybackStatus: " << represent_playback_status(playback_status) << std::endl;
        std::cout << "Position (live): " << position_live << std::endl;
        std::cout << "Position (fixed): " << position << std::endl;
        std::cout << "LastUpdatedTime: " << position_timestamp << std::endl;
        std::cout << "EndTime - StartTime (duration): " << duration << std::endl;
        std::cout << "MinSeekTime: " << to_millis(timeline_properties.MinSeekTime()) << std::endl;
        std::cout << "MaxSeekTime: " << to_millis(timeline_properties.MaxSeekTime()) << std::endl;
        std::cout << "MaxSeekTime - MinSeekTime (duration): " << to_millis(timeline_properties.MaxSeekTime() - timeline_properties.MinSeekTime()) << std::endl;
        std::cout << "PlaybackType: " << represent_playback_type(properties.PlaybackType().Value()) << std::endl;
        std::cout << "TrackNumber: " << properties.TrackNumber() << std::endl;
        std::cout << "AlbumTrackCount: " << properties.AlbumTrackCount() << std::endl;
        std::cout << "Genres: "; for (auto const& genre : properties.Genres()) std::cout << represent(genre) << ", "; std::cout << std::endl;
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

std::string represent_playback_type(MediaPlaybackType value)
{
    switch (value) {
    case MediaPlaybackType::Unknown: return "Unknown";
    case MediaPlaybackType::Music: return "Music";
    case MediaPlaybackType::Video: return "Video";
    case MediaPlaybackType::Image: return "Image";
    default: return "<invalid>";
    }
}

std::string represent_playback_status(GlobalSystemMediaTransportControlsSessionPlaybackStatus status)
{
    switch (status) {
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing: return "Playing";
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused: return "Paused";
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Opened: return "Opened";
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped: return "Stopped";
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Changing: return "Changing";
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Closed: return "Closed";
    default: return "<invalid>";
    }
}

bool represent_playing(GlobalSystemMediaTransportControlsSessionPlaybackStatus status)
{
    switch (status) {
    case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
        return true;
    default:
        return false;
    }
}

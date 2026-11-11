#include <iostream>
#include <chrono>
#include <memory>
#include <sstream>
#include <cstdint>
#include <iomanip>
#include <ctime>
#include <vector>
#include <cassert>
#include <string>

#include <Windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Storage::Streams;
using namespace std::chrono_literals;

#define INTERVAL 1s
#define THUMBNAIL_MAX_SIZE_BYTES (512*1024*1024)

struct Image
{
    std::vector<uint8_t> data;
    std::string content_type;

    std::string str() const
    {
        std::ostringstream oss;
        oss << "<" << this->content_type << " " << this->data.size() << " bytes>";
        return oss.str();
    }
};

IAsyncAction observe_async();
IAsyncAction read_sessions_async(
    std::size_t, GlobalSystemMediaTransportControlsSessionManager&);
std::string current_timestamp();
void log_playback_controls(
    GlobalSystemMediaTransportControlsSessionPlaybackControls const& controls);
std::string represent(winrt::hstring const&);
std::string represent_playback_type(MediaPlaybackType);
std::string represent_playback_status(GlobalSystemMediaTransportControlsSessionPlaybackStatus);
bool represent_playing(GlobalSystemMediaTransportControlsSessionPlaybackStatus);
IAsyncAction load_media_thumbnail(
    IRandomAccessStreamReference thumbnail, std::shared_ptr<Image>& out);

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    auto action = observe_async();
    action.get();
}

IAsyncAction observe_async()
{
    auto manager = co_await GlobalSystemMediaTransportControlsSessionManager::RequestAsync();

    for (std::size_t i = 0;; i++) {
        co_await read_sessions_async(i, manager);
        co_await winrt::resume_after(INTERVAL);
    }
}

template<typename R, typename P>
inline std::chrono::milliseconds to_millis(std::chrono::duration<R, P> value)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(value);
}

IAsyncAction read_sessions_async(
    std::size_t index, GlobalSystemMediaTransportControlsSessionManager& manager)
{
    std::ostringstream out;

    if (index > 0) {
        out << "--------------------------------";
        out << std::endl;
        out << std::endl;
    }

    auto sessions = manager.GetSessions();
    for (std::uint32_t i = 0; i < sessions.Size(); i++) {
        auto session = sessions.GetAt(i);
        out << "" << (i + 1) << "/" << sessions.Size() << " " << current_timestamp() << std::endl;

        GlobalSystemMediaTransportControlsSessionMediaProperties properties{ nullptr };
        try {
            properties = co_await session.TryGetMediaPropertiesAsync();
        } catch (winrt::hresult_error const& err) {
            std::cerr << "Error: Failed to get media properties: "
                << winrt::to_string(err.message()) << std::endl;
            continue;
        }
        std::shared_ptr<Image> image;
        co_await load_media_thumbnail(properties.Thumbnail(), image);
        auto playback_info = session.GetPlaybackInfo();
        auto playback_status = playback_info.PlaybackStatus();
        auto controls = playback_info.Controls();
        auto timeline_properties = session.GetTimelineProperties();
        auto playback_rate = playback_info.PlaybackRate();
        auto playback_type = properties.PlaybackType();
        auto position = to_millis(timeline_properties.Position());
        auto position_timestamp = to_millis(winrt::clock::to_sys(timeline_properties.LastUpdatedTime()).time_since_epoch());
        auto now = to_millis(std::chrono::system_clock::now().time_since_epoch());
        auto position_live = represent_playing(playback_status) ? position + (now - position_timestamp) : position;
        auto duration = to_millis(timeline_properties.EndTime() - timeline_properties.StartTime());
        out << "AppUserModelId: " << represent(session.SourceAppUserModelId()) << std::endl;
        out << "Title: " << represent(properties.Title()) << std::endl;
        out << "Subtitle: " << represent(properties.Subtitle()) << std::endl;
        out << "Artist: " << represent(properties.Artist()) << std::endl;
        out << "AlbumTitle: " << represent(properties.AlbumTitle()) << std::endl;
        out << "AlbumArtist: " << represent(properties.AlbumArtist()) << std::endl;
        out << "PlaybackStatus: " << represent_playback_status(playback_status) << std::endl;
        out << "PlaybackRate: " << (playback_rate ? std::to_string(playback_rate.Value()) : "<null>") << std::endl;
        out << "Position (live): " << position_live << std::endl;
        out << "Position (fixed): " << position << std::endl;
        out << "LastUpdatedTime: " << position_timestamp << std::endl;
        out << "StartTime: " << to_millis(timeline_properties.StartTime()) << std::endl;
        out << "EndTime: " << to_millis(timeline_properties.EndTime()) << std::endl;
        out << "EndTime - StartTime (duration): " << duration << std::endl;
        out << "MinSeekTime: " << to_millis(timeline_properties.MinSeekTime()) << std::endl;
        out << "MaxSeekTime: " << to_millis(timeline_properties.MaxSeekTime()) << std::endl;
        out << "MaxSeekTime - MinSeekTime (duration): " << to_millis(timeline_properties.MaxSeekTime() - timeline_properties.MinSeekTime()) << std::endl;

        std::vector<std::string> enabled_playback_controls;
        if (controls.IsChannelDownEnabled()) enabled_playback_controls.push_back("ChannelDown");
        if (controls.IsChannelUpEnabled()) enabled_playback_controls.push_back("ChannelUp");
        if (controls.IsFastForwardEnabled()) enabled_playback_controls.push_back("FastForward");
        if (controls.IsNextEnabled()) enabled_playback_controls.push_back("Next");
        if (controls.IsPauseEnabled()) enabled_playback_controls.push_back("Pause");
        if (controls.IsPlaybackPositionEnabled()) enabled_playback_controls.push_back("PlaybackPosition");
        if (controls.IsPlaybackRateEnabled()) enabled_playback_controls.push_back("PlaybackRate");
        if (controls.IsPlayEnabled()) enabled_playback_controls.push_back("Play");
        if (controls.IsPlayPauseToggleEnabled()) enabled_playback_controls.push_back("PlayPauseToggle");
        if (controls.IsPreviousEnabled()) enabled_playback_controls.push_back("Previous");
        if (controls.IsRecordEnabled()) enabled_playback_controls.push_back("Record");
        if (controls.IsRepeatEnabled()) enabled_playback_controls.push_back("Repeat");
        if (controls.IsRewindEnabled()) enabled_playback_controls.push_back("Rewind");
        if (controls.IsShuffleEnabled()) enabled_playback_controls.push_back("Shuffle");
        if (controls.IsStopEnabled()) enabled_playback_controls.push_back("Stop");
        out << "PlaybackControls: ";
        if (enabled_playback_controls.empty()) {
            out << "<null>";
        } else {
            for (size_t i = 0; i < enabled_playback_controls.size(); ++i) {
                if (i > 0) {
                    out << ", ";
                }
                out << enabled_playback_controls[i];
            }
        }
        out << std::endl;

        out << "PlaybackType: " << (playback_type ? represent_playback_type(playback_type.Value()) : "<null>") << std::endl;
        out << "Thumbnail: " << (image ? image->str() : "<null>") << std::endl;
        out << "TrackNumber: " << properties.TrackNumber() << std::endl;
        out << "AlbumTrackCount: " << properties.AlbumTrackCount() << std::endl;

        out << "Genres: ";
        if (properties.Genres().Size() == 0) {
            out << "<null>";
        } else {
            for (uint32_t i = 0; i < properties.Genres().Size(); ++i) {
                if (i > 0) {
                    out << ", ";
                }
                out << represent(properties.Genres().GetAt(i));
            }
        }
        out << std::endl;
    }

    std::cout << std::move(out).str();

    co_return;
}

std::string current_timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm utc_time{};
    gmtime_s(&utc_time, &time);

    std::ostringstream oss;
    oss << std::put_time(&utc_time, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3)
        << to_millis(now.time_since_epoch() % 1s).count()
        << 'Z';

    return oss.str();
}

std::string represent(winrt::hstring const& value)
{
    std::string str{ winrt::to_string(value) };
    if (str.empty()) {
        return "<null>";
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
    default: return std::to_string(static_cast<std::int32_t>(value));
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
    default: return std::to_string(static_cast<std::int32_t>(status));
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

IAsyncAction load_media_thumbnail(
    IRandomAccessStreamReference thumbnail, std::shared_ptr<Image>& out)
{
    if (!thumbnail) {
        co_return;
    }
    IRandomAccessStreamWithContentType stream;
    try {
        stream = co_await thumbnail.OpenReadAsync();
    }
    catch (winrt::hresult_error const&) {
        co_return;
    }
    auto thumbnail_size{ static_cast<uint32_t>(stream.Size()) };
    if (thumbnail_size > THUMBNAIL_MAX_SIZE_BYTES) {
        co_return;
    }
    // The content type can contain multiple content types...
    // This is not documented directly for the ContentType() method.
    // The exact return values can be found here though:
    // https://learn.microsoft.com/de-de/windows/win32/wic/jpeg-format-overview
    // Splitting at the first comma is necessary, to get a single content type.
    std::istringstream iss(winrt::to_string(stream.ContentType()));
    std::string content_type;
    std::getline(iss, content_type, ',');

    IInputStream input_stream{ stream.GetInputStreamAt(0) };
    DataReader data_reader{ input_stream };

    unsigned int bytes_loaded;
    try {
        bytes_loaded = co_await data_reader.LoadAsync(thumbnail_size);
    }
    catch (winrt::hresult_error const&) {
        co_return;
    }
    if (bytes_loaded != thumbnail_size) {
        assert(false);
        co_return;
    }

    IBuffer buffer{ data_reader.ReadBuffer(bytes_loaded) };
    auto result = std::make_shared<Image>();
    result->data = std::vector<uint8_t>(buffer.data(), buffer.data() + buffer.Length());
    result->content_type = content_type;
    out = result;
}

/*
  ==============================================================================

    MediaVideo.h
    Created: 26 Sep 2020 1:51:42pm
    Author:  Mediaupe

  ==============================================================================
*/

#pragma once

//#include "../../Common/CommonIncludes.h"
#include "Definitions/Media/Media.h"

class FixturePatch;
struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_list_player_t;
struct libvlc_media_list_t;
struct libvlc_media_t;

class MediaVideo :
    public Media,
    public Thread
{
public:
    MediaVideo(var params = var());
    ~MediaVideo();

    FileParameter* filePath;

    Trigger* startBtn;
    Trigger* stopBtn;
    Trigger* restartBtn;
    Trigger* pauseBtn;
    FloatParameter* mediaVolume;
    String currentVolumeController = "";
    String nextVolumeController = "";
    FloatParameter* speedRate;
    FloatParameter* seek;

    void clearItem() override;
    void onContainerParameterChanged(Parameter* p) override;
    void triggerTriggered(Trigger* t);
    
    String getTypeString() const override { return "Video File"; }
    static MediaVideo* create(var params) { return new MediaVideo(); }

    libvlc_instance_t* VLCInstance = nullptr;
    libvlc_media_player_t* VLCMediaPlayer = nullptr;
    libvlc_media_list_player_t* VLCMediaListPlayer = nullptr;
    libvlc_media_list_t* VLCMediaList = nullptr;
    libvlc_media_t* VLCMedia = nullptr;

    int imageWidth = 0;
    int imageHeight = 0;
    int imagePitches = 0;
    int imageLines = 0;
    //uint32_t* vlcData;

    std::shared_ptr<Image::BitmapData> vlcBitmapData = nullptr;
    bool vlcDataIsValid = false;

    void play(); 
    void stop();
    void pause();
    void restart();

    void run() override;
    void threadLoop();

    void* lock(void** pixels);
    static void* lock(void* self, void** pixels) {  return static_cast<MediaVideo*>(self)->lock(pixels);   };

    void unlock(void* oldBuffer, void* const* pixels);
    static void unlock(void* self, void* oldBuffer, void* const* pixels) { static_cast<MediaVideo*>(self)->unlock(oldBuffer, pixels); };

    void display(void* nextBuffer);
    static void display(void* self, void* nextBuffer) { static_cast<MediaVideo*>(self)->display(nextBuffer); };

    unsigned setup_video(char* chroma, unsigned* width, unsigned* height, unsigned* pitches, unsigned* lines);
    static unsigned setup_video(void** self, char* chroma, unsigned* width, unsigned* height, unsigned* pitches, unsigned* lines) {
        return static_cast<MediaVideo*>(*self)->setup_video(chroma, width, height, pitches, lines);
    }

    void cleanup_video();
    static void cleanup_video(void* self) {
        static_cast<MediaVideo*>(self)->cleanup_video();
    }

    bool vlcSeekedLast = false;
    void vlcSeek();
    static void vlcSeek(const struct libvlc_event_t* p_event, void* p_data) {
        static_cast<MediaVideo*>(p_data)->vlcSeek();
    }
    //virtual MediaUI* createUI() {return new MediaVideo(); };

    double lastTapTempo;
    Trigger* tapTempoBtn;
    IntParameter* beatPerCycle;
    void tapTempo();


};
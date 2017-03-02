
#include "Session.h"

#include "VideoReceiver.h"
#include "AudioReceiver.h"

#include <capsulerun.h>

using namespace std;

static int receive_video_format (Session *s, video_format_t *vfmt) {
  return s->video->receive_format(vfmt);
}

static int receive_video_frame (Session *s, uint8_t *buffer, size_t buffer_size, int64_t *timestamp) {
  return s->video->receive_frame(buffer, buffer_size, timestamp);
}

static int receive_audio_format (Session *s, audio_format_t *afmt) {
  return s->audio->receive_format(afmt);
}

static void *receive_audio_frames (Session *s, int *frames_received) {
  return s->audio->receive_frames(frames_received);
}

void Session::start () {
  memset(&encoder_params, 0, sizeof(encoder_params));
  encoder_params.private_data = this;
  encoder_params.receive_video_format = (receive_video_format_t) receive_video_format;
  encoder_params.receive_video_frame  = (receive_video_frame_t) receive_video_frame;

  if (audio) {
    encoder_params.has_audio = 1;
    encoder_params.receive_audio_format = (receive_audio_format_t) receive_audio_format;
    encoder_params.receive_audio_frames = (receive_audio_frames_t) receive_audio_frames;
  } else {
    encoder_params.has_audio = 0;  
  }

  encoder_params.use_yuv444 = args->yuv444;

  encoder_thread = new thread(encoder_run, &encoder_params);
}

void Session::stop () {
  if (video) {
    video->stop();
  }

  if (audio) {
    audio->stop();
  }
}

void Session::join () {
  capsule_log("Waiting for encoder thread...");
  encoder_thread->join();
}

Session::~Session () {
  if (video) {
    delete video;
  }
  if (audio) {
    delete audio;
  }
  delete encoder_thread;
}

#include <capsule/messages.h>

#include "VideoReceiver.h"

using namespace Capsule::Messages;
using namespace std;

#include <capsulerun_macros.h>

#ifdef CAPSULERUN_PROFILE
#include <chrono>
#endif // CAPSULERUN_PROFILE

int VideoReceiver::receive_format(video_format_t *vfmt_out) {
  memcpy(vfmt_out, &vfmt, sizeof(video_format_t));
  return 0;
}

int VideoReceiver::receive_frame(uint8_t *buffer, size_t buffer_size, int64_t *timestamp) {
  FrameInfo info {};

  while (true) {
    if (queue.tryWaitAndPop(info, 200)) {
      // got a frame!
      break;
    }

    // didn't get a frame, are we stopped?
    {
      lock_guard<mutex> lock(stopped_mutex);
      if (stopped) {
        return 0;
      }
    }
  }
  
  void *source = shm->mapped + (buffer_size * info.index);
#ifdef CAPSULERUN_PROFILE
  auto t1 = chrono::steady_clock::now();
#endif // CAPSULERUN_PROFILE
  memcpy(buffer, source, buffer_size);
#ifdef CAPSULERUN_PROFILE
  auto t2 = chrono::steady_clock::now();
  eprintf("memcpy took %.3f ms", (double) (chrono::duration_cast<chrono::microseconds>(t2 - t1).count()) / 1000.0);
#endif // CAPSULERUN_PROFILE
  *timestamp = info.timestamp;

  flatbuffers::FlatBufferBuilder builder(1024);
  auto vfp = CreateVideoFrameProcessed(builder, info.index); 
  auto opkt = CreatePacket(builder, Message_VideoFrameProcessed, vfp.Union());
  builder.Finish(opkt);
  conn->write(builder);

  return buffer_size;
}

void VideoReceiver::frame_committed(int index, int64_t timestamp) {
  {
    lock_guard<mutex> lock(stopped_mutex);
    if (stopped) {
      // just ignore
      return;
    }
  }

  FrameInfo info {index, timestamp};
  queue.push(info);
}

void VideoReceiver::stop() {
  lock_guard<mutex> lock(stopped_mutex);
  stopped = true;
}

VideoReceiver::~VideoReceiver () {
  delete shm;
}

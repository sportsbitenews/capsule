
#include "main_loop.h"

#include <capsule/messages.h>

#include <microprofile.h>

MICROPROFILE_DEFINE(MainLoopMain, "MainLoop", "Main", 0xff0000);
MICROPROFILE_DEFINE(MainLoopCycle, "MainLoop", "Cycle", 0xff00ff38);
MICROPROFILE_DEFINE(MainLoopRead, "MainLoop", "Read", 0xff00ff00);
MICROPROFILE_DEFINE(MainLoopProcess, "MainLoop", "Process", 0xff773744);

namespace capsule {

void MainLoop::Run () {
  MICROPROFILE_SCOPE(MainLoopCycle);
  CapsuleLog("In MainLoop::Run, exec is %s", args_->exec);

  while (true) {
    MICROPROFILE_SCOPE(MainLoopCycle);
    MicroProfileFlip(0);

    char *buf;

    {
      MICROPROFILE_SCOPE(MainLoopRead);
      buf = conn_->read();
      if (!buf) {
        CapsuleLog("MainLoop::Run: pipe closed");
        break;
      }
    }

    {
      MICROPROFILE_SCOPE(MainLoopProcess);
      auto pkt = messages::GetPacket(buf);
      switch (pkt->message_type()) {
        case messages::Message_VideoSetup: {
          auto vs = static_cast<const messages::VideoSetup*>(pkt->message());
          StartSession(vs);
          break;
        }
        case messages::Message_VideoFrameCommitted: {
          auto vfc = static_cast<const messages::VideoFrameCommitted*>(pkt->message());
          if (session_ && session_->video_) {
            session_->video_->FrameCommitted(vfc->index(), vfc->timestamp());
          } else {
            CapsuleLog("no session, ignoring VideoFrameCommitted")
          }
          break;
        }
        default: {
          CapsuleLog("MainLoop::Run: received %s - not sure what to do", messages::EnumNameMessage(pkt->message_type()));
          break;
        }
      }
    }

    delete[] buf;
  }

  EndSession();  
  JoinSessions();
}

void MainLoop::CaptureFlip () {
  CapsuleLog("MainLoop::CaptureFlip")
  if (session_) {
    CaptureStop();
  } else {
    // TODO: ignore subsequent CaptureStart until the capture actually started
    CaptureStart();
  }
}

void MainLoop::CaptureStart () {
  flatbuffers::FlatBufferBuilder builder(1024);
  auto cps = messages::CreateCaptureStart(builder, args_->fps, args_->size_divider, args_->gpu_color_conv);
  auto opkt = messages::CreatePacket(builder, messages::Message_CaptureStart, cps.Union());
  builder.Finish(opkt);
  conn_->write(builder);
}

void MainLoop::EndSession () {
  if (!session_) {
    CapsuleLog("MainLoop::end_session: no session to end")
    return;
  }

  CapsuleLog("MainLoop::end_session: ending %p", session_)
  auto old_session = session_;
  session_ = nullptr;
  old_session->Stop();
  old_sessions_.push_back(old_session);
}

void MainLoop::JoinSessions () {
  CapsuleLog("MainLoop::join_sessions: joining %d sessions", old_sessions_.size())

  for (Session *session: old_sessions_) {
    CapsuleLog("MainLoop::join_sessions: joining session_ %p", session)
    session->Join();
  }

  CapsuleLog("MainLoop::join_sessions: joined all sessions!")
}

void MainLoop::CaptureStop () {
  EndSession();

  flatbuffers::FlatBufferBuilder builder(1024);
  auto cps = messages::CreateCaptureStop(builder);
  auto opkt = CreatePacket(builder, messages::Message_CaptureStop, cps.Union());
  builder.Finish(opkt);
  conn_->write(builder);
}

void MainLoop::StartSession (const messages::VideoSetup *vs) {
  CapsuleLog("Setting up encoder");

  video_format_t vfmt;
  vfmt.width = vs->width();
  vfmt.height = vs->height();
  vfmt.format = (capsule_pix_fmt_t) vs->pix_fmt();
  vfmt.vflip = vs->vflip();
  
  // TODO: support offset (for planar formats)

  // TODO: support multiple linesizes (for planar formats)
  auto linesize_vec = vs->linesize();
  vfmt.pitch = linesize_vec->Get(0);

  auto shm_path = vs->shmem()->path()->str();  
  auto shm = new shoom::Shm(shm_path, vs->shmem()->size());
  int ret = shm->Open();
  if (ret != shoom::kOK) {
    CapsuleLog("Could not open shared memory area: code %d", ret);
    return;
  }

  int num_buffered_frames = 60;
  if (args_->buffered_frames) {
    num_buffered_frames = args_->buffered_frames;
  }

  auto video = new video::VideoReceiver(conn_, vfmt, shm, num_buffered_frames);
  audio::AudioReceiver *audio = nullptr;
  if (audio_receiver_factory_ && !args_->no_audio) {
    audio = audio_receiver_factory_();
  }

  session_ = new Session(args_, video, audio);
  session_->Start();
}

} // namespace capsule
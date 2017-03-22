
#include <stdio.h>
#include <NktHookLib.h>

#include <thread>

#include <lab/platform.h>
#include <lab/paths.h>
#include <lab/strings.h>
#include <lab/env.h>

#include "quote.h"
#include "wasapi_receiver.h"

#include "../hotkey.h"
#include "../main_loop.h"

namespace capsule {

static bool connected = false;
static DWORD exit_code = 0;

audio::AudioReceiver *AudioReceiverFactory() {
  return new audio::WasapiReceiver();
}

static void WaitForChild (HANDLE hProcess) {
  LONG platform = NktHookLibHelpers::GetProcessPlatform(hProcess);
  if (platform == NKTHOOKLIB_ProcessPlatformX86) {
    cdprintf("Child is 32-bit!");
  } else if (platform == NKTHOOKLIB_ProcessPlatformX64) {
    cdprintf("Child is 64-bit!");
  }

  cdprintf("Waiting on child...");
  WaitForSingleObject(hProcess, INFINITE);
  cdprintf("Done waiting on child");

  GetExitCodeProcess(hProcess, &exit_code);
  CapsuleLog("Exit code: %d (%x)", exit_code, exit_code);

  if (!connected) {
    exit(exit_code);
  }
}

int Main (MainArgs *args) {
  // From Deviare-InProc's doc: 
  //   If "szDllNameW" string ends with 'x86.dll', 'x64.dll', '32.dll', '64.dll', the dll name will be adjusted
  //   in order to match the process platform. I.e.: "mydll_x86.dll" will become "mydll_x64.dll" on 64-bit processes.
  // hence the blanket 'capsule32.dll' here
  // N.B: even if the .exe we launch appears to be PE32, it might actually end up being a 64-bit process.
  // Don't ask me, I don't know either.
  std::string libcapsule_path = lab::paths::Join(std::string(args->libpath), "capsule32.dll");

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  DWORD err;

  ZeroMemory(&si, sizeof(si));  
  si.cb = sizeof(si);

  ZeroMemory(&pi, sizeof(pi));  

  wchar_t *executable_path_w;
  lab::strings::ToWideChar(args->exec, &executable_path_w);

  wchar_t *libcapsule_path_w;
  lab::strings::ToWideChar(libcapsule_path.c_str(), &libcapsule_path_w);

  std::string pipe_r_path = "\\\\.\\pipe\\capsule.runr";
  std::string pipe_w_path = "\\\\.\\pipe\\capsule.runw";

  Connection *conn = new Connection(pipe_r_path, pipe_w_path);

  bool env_success = true;
  env_success &= lab::env::Set("CAPSULE_R_PATH", pipe_w_path); // swapped on purpose
  env_success &= lab::env::Set("CAPSULE_W_PATH", pipe_r_path); // swapped on purose
  env_success &= lab::env::Set("CAPSULE_LIBRARY_PATH", libcapsule_path);

  if (!env_success) {
    CapsuleLog("Could not set environment variables for the child");
    exit(1);
  }

  bool first_arg = true;

  std::wstring command_line_w;
  for (int i = 0; i < args->exec_argc; i++) {
    wchar_t *arg;
    // this "leaks" mem, but it's one time, so don't care
    lab::strings::ToWideChar(args->exec_argv[i], &arg);

    if (first_arg) {
      first_arg = false;
    } else {
      command_line_w.append(L" ");
    }

    std::wstring arg_w = arg;
    ArgvQuote(arg_w, command_line_w, false);
  }

  CapsuleLog("Launching '%S' with args '%S'", executable_path_w, command_line_w.c_str());
  CapsuleLog("Injecting '%S'", libcapsule_path_w);
  const char* libcapsule_init_function_name = "CapsuleWindowsInit";

  err = NktHookLibHelpers::CreateProcessWithDllW(
    executable_path_w, // applicationName
    (LPWSTR) command_line_w.c_str(), // commandLine
    NULL, // processAttributes
    NULL, // threadAttributes
    FALSE, // inheritHandles
    0, // creationFlags
    NULL, // environment
    NULL, // currentDirectory
    &si, // startupInfo
    &pi, // processInfo
    libcapsule_path_w, // dllName
    NULL, // signalCompletedEvent
    libcapsule_init_function_name // initFunctionName
  );

  if (err == ERROR_SUCCESS) {
    CapsuleLog("Process #%lu successfully launched with dll injected!", pi.dwProcessId);

    std::thread child_thread(WaitForChild, pi.hProcess);
    child_thread.detach();

    conn->Connect();
    connected = true;

    MainLoop ml {args, conn};
    ml.audio_receiver_factory_ = AudioReceiverFactory;

    hotkey::Init(&ml);

    ml.Run();
  } else {
    CapsuleLog("Error %lu: Cannot launch process and inject dll.", err);
    return 127;
  }

  return exit_code;
}

} // namespace capsule

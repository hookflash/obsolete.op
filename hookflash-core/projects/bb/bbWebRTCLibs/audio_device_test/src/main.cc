/*
* Copyright (c) 2011-2012 Research In Motion Limited.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <bps/bps.h>
#include <bps/event.h>
#include <bps/navigator.h>
#include <bps/screen.h>
#include <fcntl.h>
#include <screen/screen.h>
#include <slog2.h>

#include "trace.h"
#include "thread_wrapper.h"
#include "audio_device.h"
#include "audio_device_impl.h"

using namespace std;
using namespace webrtc;

extern char *__progname;

static bool shutdown;
const char* trace_file_name;

bool test_thread(ThreadObj obj);

int hookflash_test(const char* file_name)
{
	trace_file_name = file_name;

    ThreadWrapper* _testThread = ThreadWrapper::CreateThread(test_thread, NULL,
    		kLowPriority, "TestThread");
    unsigned int threadId;
    _testThread->Start(threadId);

    return 0;
}

bool test_thread(ThreadObj obj)
{
	slog2_buffer_set_config_t buffer_config;
	slog2_buffer_t buffer_handle;
	buffer_config.buffer_set_name = __progname;
	buffer_config.num_buffers = 1;
	buffer_config.verbosity_level = SLOG2_INFO;
	buffer_config.buffer_config[0].buffer_name = "test_logger";
	buffer_config.buffer_config[0].num_pages = 7;
	if (-1 == slog2_register(&buffer_config, &buffer_handle, 0)) {
	    fprintf(stderr, "Error registering slogger2 buffer!\n");
	    return false;
	}

	slog2f(buffer_handle, 0, SLOG2_INFO,
	        "Writing a formatted string into the buffer: %s", "test" );

	Trace::CreateTrace();
    Trace::SetTraceFile(trace_file_name);
    Trace::SetLevelFilter(kTraceAll);

    AudioDeviceModule* audioDevice(NULL);

    audioDevice = AudioDeviceModuleImpl::Create(1, AudioDeviceModule::kPlatformDefaultAudio);

    audioDevice->Init();

    audioDevice->SetRecordingDevice(0);
    audioDevice->SetPlayoutDevice(0);

    audioDevice->InitRecording();
    audioDevice->InitPlayout();

    audioDevice->StartRecording();
    audioDevice->StartPlayout();

    usleep(10000000);

    audioDevice->StopRecording();
    audioDevice->StopPlayout();

    audioDevice->Terminate();

    Trace::ReturnTrace();

    int i =0; i++;

    return 0;
}

static void
handle_screen_event(bps_event_t *event)
{
    int screen_val;

    screen_event_t screen_event = screen_event_get_event(event);
    screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &screen_val);

    switch (screen_val) {
    case SCREEN_EVENT_MTOUCH_TOUCH:
        fprintf(stderr,"Touch event");
        break;
    case SCREEN_EVENT_MTOUCH_MOVE:
        fprintf(stderr,"Move event");
        break;
    case SCREEN_EVENT_MTOUCH_RELEASE:
        fprintf(stderr,"Release event");
        break;
    default:
        break;
    }
    fprintf(stderr,"\n");
}

static void
handle_navigator_event(bps_event_t *event) {
    switch (bps_event_get_code(event)) {
    case NAVIGATOR_SWIPE_DOWN:
        fprintf(stderr,"Swipe down event");
        break;
    case NAVIGATOR_EXIT:
        fprintf(stderr,"Exit event");
        shutdown = true;
        break;
    default:
        break;
    }
    fprintf(stderr,"\n");
}

static void
handle_event()
{
    int domain;

    bps_event_t *event = NULL;
    if (BPS_SUCCESS != bps_get_event(&event, -1)) {
        fprintf(stderr, "bps_get_event() failed\n");
        return;
    }
    if (event) {
        domain = bps_event_get_domain(event);
        if (domain == navigator_get_domain()) {
            handle_navigator_event(event);
        } else if (domain == screen_get_domain()) {
            handle_screen_event(event);
        }
    }
}

int
main(int argc, char **argv)
{
    const int usage = SCREEN_USAGE_NATIVE;

    screen_context_t screen_ctx;
    screen_window_t screen_win;
    screen_buffer_t screen_buf = NULL;
    int rect[4] = { 0, 0, 0, 0 };

    /* Setup the window */
    screen_create_context(&screen_ctx, 0);
    screen_create_window(&screen_win, screen_ctx);
    screen_set_window_property_iv(screen_win, SCREEN_PROPERTY_USAGE, &usage);
    screen_create_window_buffers(screen_win, 1);

    screen_get_window_property_pv(screen_win, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)&screen_buf);
    screen_get_window_property_iv(screen_win, SCREEN_PROPERTY_BUFFER_SIZE, rect+2);

    /* Fill the screen buffer with blue */
    int attribs[] = { SCREEN_BLIT_COLOR, 0xff0000ff, SCREEN_BLIT_END };
    screen_fill(screen_ctx, screen_buf, attribs);
    screen_post_window(screen_win, screen_buf, 1, rect, 0);

    /* Signal bps library that navigator and screen events will be requested */
    bps_initialize();
    screen_request_events(screen_ctx);
    navigator_request_events(0);

    hookflash_test(NULL);

    while (!shutdown) {
        /* Handle user input */
        handle_event();
    }

    /* Clean up */
    screen_stop_events(screen_ctx);
    bps_shutdown();
    screen_destroy_window(screen_win);
    screen_destroy_context(screen_ctx);
    return 0;
}


#include <stdio.h>

#if defined(ENABLE_OPENGL) || defined(__APPLE__)

#ifdef __MINGW32__
#define FOR_WINDOWS 1
#else
#define FOR_WINDOWS 0
#endif

#include "Context.h"
#include "config/ConsoleVariable.h"

#if FOR_WINDOWS
#include <GL/glew.h>
#include "SDL.h"
#define GL_GLEXT_PROTOTYPES 1
#include "SDL_opengl.h"
#elif __APPLE__
#include <SDL.h>
#include "gfx_metal.h"
#include "utils/macUtils.h"
#else
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
#endif

#include "window/gui/Gui.h"
#include "public/bridge/consolevariablebridge.h"

#include "gfx_window_manager_api.h"
#include "gfx_screen_config.h"
#ifdef _WIN32
#include <WTypesbase.h>
#include <Windows.h>
#include <SDL_syswm.h>
#endif

#define GFX_BACKEND_NAME "SDL"
#define _100NANOSECONDS_IN_SECOND 10000000

static SDL_Window* wnd;
static SDL_GLContext ctx;
static SDL_Renderer* renderer;
static int sdl_to_lus_table[512];
static bool vsync_enabled = true;
static float mouse_wheel_x = 0.0f;
static float mouse_wheel_y = 0.0f;
// OTRTODO: These are redundant. Info can be queried from SDL.
static int window_width = DESIRED_SCREEN_WIDTH;
static int window_height = DESIRED_SCREEN_HEIGHT;
static bool fullscreen_state;
static bool is_running = true;
static void (*on_fullscreen_changed_callback)(bool is_now_fullscreen);
static bool (*on_key_down_callback)(int scancode);
static bool (*on_key_up_callback)(int scancode);
static void (*on_all_keys_up_callback)();
static bool (*on_mouse_button_down_callback)(int btn);
static bool (*on_mouse_button_up_callback)(int btn);

#ifdef _WIN32
LONG_PTR SDL_WndProc;
#endif

const SDL_Scancode lus_to_sdl_table[] = {
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_ESCAPE,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_5,
    SDL_SCANCODE_6, /* 0 */
    SDL_SCANCODE_7,
    SDL_SCANCODE_8,
    SDL_SCANCODE_9,
    SDL_SCANCODE_0,
    SDL_SCANCODE_MINUS,
    SDL_SCANCODE_EQUALS,
    SDL_SCANCODE_BACKSPACE,
    SDL_SCANCODE_TAB, /* 0 */

    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_R,
    SDL_SCANCODE_T,
    SDL_SCANCODE_Y,
    SDL_SCANCODE_U,
    SDL_SCANCODE_I, /* 1 */
    SDL_SCANCODE_O,
    SDL_SCANCODE_P,
    SDL_SCANCODE_LEFTBRACKET,
    SDL_SCANCODE_RIGHTBRACKET,
    SDL_SCANCODE_RETURN,
    SDL_SCANCODE_LCTRL,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S, /* 1 */

    SDL_SCANCODE_D,
    SDL_SCANCODE_F,
    SDL_SCANCODE_G,
    SDL_SCANCODE_H,
    SDL_SCANCODE_J,
    SDL_SCANCODE_K,
    SDL_SCANCODE_L,
    SDL_SCANCODE_SEMICOLON, /* 2 */
    SDL_SCANCODE_APOSTROPHE,
    SDL_SCANCODE_GRAVE,
    SDL_SCANCODE_LSHIFT,
    SDL_SCANCODE_BACKSLASH,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_X,
    SDL_SCANCODE_C,
    SDL_SCANCODE_V, /* 2 */

    SDL_SCANCODE_B,
    SDL_SCANCODE_N,
    SDL_SCANCODE_M,
    SDL_SCANCODE_COMMA,
    SDL_SCANCODE_PERIOD,
    SDL_SCANCODE_SLASH,
    SDL_SCANCODE_RSHIFT,
    SDL_SCANCODE_PRINTSCREEN, /* 3 */
    SDL_SCANCODE_LALT,
    SDL_SCANCODE_SPACE,
    SDL_SCANCODE_CAPSLOCK,
    SDL_SCANCODE_F1,
    SDL_SCANCODE_F2,
    SDL_SCANCODE_F3,
    SDL_SCANCODE_F4,
    SDL_SCANCODE_F5, /* 3 */

    SDL_SCANCODE_F6,
    SDL_SCANCODE_F7,
    SDL_SCANCODE_F8,
    SDL_SCANCODE_F9,
    SDL_SCANCODE_F10,
    SDL_SCANCODE_NUMLOCKCLEAR,
    SDL_SCANCODE_SCROLLLOCK,
    SDL_SCANCODE_HOME, /* 4 */
    SDL_SCANCODE_UP,
    SDL_SCANCODE_PAGEUP,
    SDL_SCANCODE_KP_MINUS,
    SDL_SCANCODE_LEFT,
    SDL_SCANCODE_KP_5,
    SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_KP_PLUS,
    SDL_SCANCODE_END, /* 4 */

    SDL_SCANCODE_DOWN,
    SDL_SCANCODE_PAGEDOWN,
    SDL_SCANCODE_INSERT,
    SDL_SCANCODE_DELETE,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_NONUSBACKSLASH,
    SDL_SCANCODE_F11, /* 5 */
    SDL_SCANCODE_F12,
    SDL_SCANCODE_PAUSE,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_LGUI,
    SDL_SCANCODE_RGUI,
    SDL_SCANCODE_APPLICATION,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN, /* 5 */

    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_F13,
    SDL_SCANCODE_F14,
    SDL_SCANCODE_F15,
    SDL_SCANCODE_F16, /* 6 */
    SDL_SCANCODE_F17,
    SDL_SCANCODE_F18,
    SDL_SCANCODE_F19,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN, /* 6 */

    SDL_SCANCODE_INTERNATIONAL2,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_INTERNATIONAL1,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN, /* 7 */
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_INTERNATIONAL4,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_INTERNATIONAL5,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_INTERNATIONAL3,
    SDL_SCANCODE_UNKNOWN,
    SDL_SCANCODE_UNKNOWN /* 7 */
};

const SDL_Scancode scancode_rmapping_extended[][2] = {
    { SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_RETURN },
    { SDL_SCANCODE_RALT, SDL_SCANCODE_LALT },
    { SDL_SCANCODE_RCTRL, SDL_SCANCODE_LCTRL },
    { SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_SLASH },
    //{SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_CAPSLOCK}
};

const SDL_Scancode scancode_rmapping_nonextended[][2] = { { SDL_SCANCODE_KP_7, SDL_SCANCODE_HOME },
                                                          { SDL_SCANCODE_KP_8, SDL_SCANCODE_UP },
                                                          { SDL_SCANCODE_KP_9, SDL_SCANCODE_PAGEUP },
                                                          { SDL_SCANCODE_KP_4, SDL_SCANCODE_LEFT },
                                                          { SDL_SCANCODE_KP_6, SDL_SCANCODE_RIGHT },
                                                          { SDL_SCANCODE_KP_1, SDL_SCANCODE_END },
                                                          { SDL_SCANCODE_KP_2, SDL_SCANCODE_DOWN },
                                                          { SDL_SCANCODE_KP_3, SDL_SCANCODE_PAGEDOWN },
                                                          { SDL_SCANCODE_KP_0, SDL_SCANCODE_INSERT },
                                                          { SDL_SCANCODE_KP_PERIOD, SDL_SCANCODE_DELETE },
                                                          { SDL_SCANCODE_KP_MULTIPLY, SDL_SCANCODE_PRINTSCREEN } };

static void set_fullscreen(bool on, bool call_callback) {
    if (fullscreen_state == on) {
        return;
    }
    int display_in_use = SDL_GetWindowDisplayIndex(wnd);
    if (display_in_use < 0) {
        SPDLOG_WARN("Can't detect on which monitor we are. Probably out of display area?");
        SPDLOG_WARN(SDL_GetError());
    }

    if (on) {
        // OTRTODO: Get mode from config.
        SDL_DisplayMode mode;
        if (SDL_GetDesktopDisplayMode(display_in_use, &mode) >= 0) {
            SDL_SetWindowDisplayMode(wnd, &mode);
        } else {
            SPDLOG_ERROR(SDL_GetError());
        }
    }

#if defined(__APPLE__)
    // Implement fullscreening with native macOS APIs
    if (on != isNativeMacOSFullscreenActive(wnd)) {
        toggleNativeMacOSFullscreen(wnd);
    }
    fullscreen_state = on;
#else
    if (SDL_SetWindowFullscreen(wnd,
                                on ? (CVarGetInteger(CVAR_SDL_WINDOWED_FULLSCREEN, 0) ? SDL_WINDOW_FULLSCREEN_DESKTOP
                                                                                      : SDL_WINDOW_FULLSCREEN)
                                   : 0) >= 0) {
        fullscreen_state = on;
    } else {
        SPDLOG_ERROR("Failed to switch from or to fullscreen mode.");
        SPDLOG_ERROR(SDL_GetError());
    }
#endif

    if (!on) {
        auto conf = Ship::Context::GetInstance()->GetConfig();
        window_width = conf->GetInt("Window.Width", 640);
        window_height = conf->GetInt("Window.Height", 480);
        int32_t posX = conf->GetInt("Window.PositionX", 100);
        int32_t posY = conf->GetInt("Window.PositionY", 100);
        if (display_in_use < 0) { // Fallback to default if out of bounds
            posX = 100;
            posY = 100;
        }
        SDL_SetWindowPosition(wnd, posX, posY);
        SDL_SetWindowSize(wnd, window_width, window_height);
    }

    if (on_fullscreen_changed_callback != NULL && call_callback) {
        on_fullscreen_changed_callback(on);
    }
}

static void gfx_sdl_get_active_window_refresh_rate(uint32_t* refresh_rate) {
    int display_in_use = SDL_GetWindowDisplayIndex(wnd);

    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(display_in_use, &mode);
    *refresh_rate = mode.refresh_rate != 0 ? mode.refresh_rate : 60;
}

static uint64_t previous_time;
#ifdef _WIN32
static HANDLE timer;
#endif

static int target_fps = 60;

#define FRAME_INTERVAL_US_NUMERATOR 1000000
#define FRAME_INTERVAL_US_DENOMINATOR (target_fps)

static void gfx_sdl_close(void) {
    is_running = false;
}

#ifdef _WIN32
static LRESULT CALLBACK gfx_sdl_wnd_proc(HWND h_wnd, UINT message, WPARAM w_param, LPARAM l_param) {
    switch (message) {
        case WM_GETDPISCALEDSIZE:
            // Something is wrong with SDLs original implementation of WM_GETDPISCALEDSIZE, so pass it to the default
            // system window procedure instead.
            return DefWindowProc(h_wnd, message, w_param, l_param);
        case WM_ENDSESSION:
            // Apparently SDL2 does not handle this
            if (w_param == TRUE) {
                gfx_sdl_close();
            }
            break;
        default:
            // Pass anything else to SDLs original window procedure.
            return CallWindowProc((WNDPROC)SDL_WndProc, h_wnd, message, w_param, l_param);
    }
    return 0;
};
#endif

static void gfx_sdl_init(const char* game_name, const char* gfx_api_name, bool start_in_fullscreen, uint32_t width,
                         uint32_t height, int32_t posX, int32_t posY) {
    window_width = width;
    window_height = height;

#if SDL_VERSION_ATLEAST(2, 24, 0)
    /* fix DPI scaling issues on Windows */
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

    SDL_Init(SDL_INIT_VIDEO);

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

#if defined(__APPLE__)
    bool use_opengl = strcmp(gfx_api_name, "OpenGL") == 0;
#else
    bool use_opengl = true;
#endif

    if (use_opengl) {
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    } else {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    }

#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

#ifdef _WIN32
    // Use high-resolution timer by default on Windows 10 (so that NtSetTimerResolution (...) hacks are not needed)
    timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    // Fallback to low resolution timer if unsupported by the OS
    if (timer == nullptr) {
        timer = CreateWaitableTimer(nullptr, false, nullptr);
    }
#endif

    char title[512];
    int len = sprintf(title, "%s (%s)", game_name, gfx_api_name);

#ifdef __IOS__
    Uint32 flags = SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN;
#else
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#endif

    if (use_opengl) {
        flags = flags | SDL_WINDOW_OPENGL;
    } else {
        flags = flags | SDL_WINDOW_METAL;
    }

    wnd = SDL_CreateWindow(title, posX, posY, window_width, window_height, flags);
#ifdef _WIN32
    // Get Windows window handle and use it to subclass the window procedure.
    // Needed to circumvent SDLs DPI scaling problems under windows (original does only scale *sometimes*).
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(wnd, &wmInfo);
    HWND hwnd = wmInfo.info.win.window;
    SDL_WndProc = SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)gfx_sdl_wnd_proc);
#endif
    Ship::GuiWindowInitData window_impl;

    int display_in_use = SDL_GetWindowDisplayIndex(wnd);
    if (display_in_use < 0) { // Fallback to default if out of bounds
        posX = 100;
        posY = 100;
    }

    if (use_opengl) {
        SDL_GL_GetDrawableSize(wnd, &window_width, &window_height);

        if (start_in_fullscreen) {
            set_fullscreen(true, false);
        }

        ctx = SDL_GL_CreateContext(wnd);

        SDL_GL_MakeCurrent(wnd, ctx);
        SDL_GL_SetSwapInterval(vsync_enabled ? 1 : 0);

        window_impl.Opengl = { wnd, ctx };
    } else {
        uint32_t flags = SDL_RENDERER_ACCELERATED;
        if (vsync_enabled) {
            flags |= SDL_RENDERER_PRESENTVSYNC;
        }
        renderer = SDL_CreateRenderer(wnd, -1, flags);
        if (renderer == NULL) {
            SPDLOG_ERROR("Error creating renderer: {}", SDL_GetError());
            return;
        }

        if (start_in_fullscreen) {
            set_fullscreen(true, false);
        }

        SDL_GetRendererOutputSize(renderer, &window_width, &window_height);
        window_impl.Metal = { wnd, renderer };
    }

    Ship::Context::GetInstance()->GetWindow()->GetGui()->Init(window_impl);

    for (size_t i = 0; i < sizeof(lus_to_sdl_table) / sizeof(SDL_Scancode); i++) {
        sdl_to_lus_table[lus_to_sdl_table[i]] = i;
    }

    for (size_t i = 0; i < sizeof(scancode_rmapping_extended) / sizeof(scancode_rmapping_extended[0]); i++) {
        sdl_to_lus_table[scancode_rmapping_extended[i][0]] = sdl_to_lus_table[scancode_rmapping_extended[i][1]] + 0x100;
    }

    for (size_t i = 0; i < sizeof(scancode_rmapping_nonextended) / sizeof(scancode_rmapping_nonextended[0]); i++) {
        sdl_to_lus_table[scancode_rmapping_nonextended[i][0]] = sdl_to_lus_table[scancode_rmapping_nonextended[i][1]];
        sdl_to_lus_table[scancode_rmapping_nonextended[i][1]] += 0x100;
    }
}

static void gfx_sdl_set_fullscreen_changed_callback(void (*on_fullscreen_changed)(bool is_now_fullscreen)) {
    on_fullscreen_changed_callback = on_fullscreen_changed;
}

static void gfx_sdl_set_fullscreen(bool enable) {
    set_fullscreen(enable, true);
}

static void gfx_sdl_set_cursor_visibility(bool visible) {
    if (visible) {
        SDL_ShowCursor(SDL_ENABLE);
    } else {
        SDL_ShowCursor(SDL_DISABLE);
    }
}

static void gfx_sdl_set_mouse_pos(int32_t x, int32_t y) {
    SDL_WarpMouseInWindow(wnd, x, y);
}

static void gfx_sdl_get_mouse_pos(int32_t* x, int32_t* y) {
    SDL_GetMouseState(x, y);
}

static void gfx_sdl_get_mouse_delta(int32_t* x, int32_t* y) {
    SDL_GetRelativeMouseState(x, y);
}

static void gfx_sdl_get_mouse_wheel(float* x, float* y) {
    *x = mouse_wheel_x;
    *y = mouse_wheel_y;
    mouse_wheel_x = 0.0f;
    mouse_wheel_y = 0.0f;
}

static bool gfx_sdl_get_mouse_state(uint32_t btn) {
    return SDL_GetMouseState(NULL, NULL) & (1 << btn);
}

static void gfx_sdl_set_mouse_capture(bool capture) {
    SDL_SetRelativeMouseMode(static_cast<SDL_bool>(capture));
}

static bool gfx_sdl_is_mouse_captured() {
    return (SDL_GetRelativeMouseMode() == SDL_TRUE);
}

static void gfx_sdl_set_keyboard_callbacks(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode),
                                           void (*on_all_keys_up)()) {
    on_key_down_callback = on_key_down;
    on_key_up_callback = on_key_up;
    on_all_keys_up_callback = on_all_keys_up;
}

static void gfx_sdl_set_mouse_callbacks(bool (*on_btn_down)(int btn), bool (*on_btn_up)(int btn)) {
    on_mouse_button_down_callback = on_btn_down;
    on_mouse_button_up_callback = on_btn_up;
}

static void gfx_sdl_get_dimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY) {
#ifdef __APPLE__
    SDL_GetWindowSize(wnd, static_cast<int*>((void*)width), static_cast<int*>((void*)height));
#else
    SDL_GL_GetDrawableSize(wnd, static_cast<int*>((void*)width), static_cast<int*>((void*)height));
#endif
    SDL_GetWindowPosition(wnd, static_cast<int*>(posX), static_cast<int*>(posY));
}

static int translate_scancode(int scancode) {
    if (scancode < 512) {
        return sdl_to_lus_table[scancode];
    } else {
        return 0;
    }
}

static int untranslate_scancode(int translatedScancode) {
    for (int i = 0; i < 512; i++) {
        if (sdl_to_lus_table[i] == translatedScancode) {
            return i;
        }
    }
    return 0;
}

static void gfx_sdl_onkeydown(int scancode) {
    int key = translate_scancode(scancode);
    if (on_key_down_callback != NULL) {
        on_key_down_callback(key);
    }
}

static void gfx_sdl_onkeyup(int scancode) {
    int key = translate_scancode(scancode);
    if (on_key_up_callback != NULL) {
        on_key_up_callback(key);
    }
}

static void gfx_sdl_on_mouse_button_down(int btn) {
    if (!(btn >= 0 && btn < 5)) {
        return;
    }
    if (on_mouse_button_down_callback != NULL) {
        on_mouse_button_down_callback(btn);
    }
}

static void gfx_sdl_on_mouse_button_up(int btn) {
    if (on_mouse_button_up_callback != NULL) {
        on_mouse_button_up_callback(btn);
    }
}

static void gfx_sdl_handle_single_event(SDL_Event& event) {
    Ship::WindowEvent event_impl;
    event_impl.Sdl = { &event };
    Ship::Context::GetInstance()->GetWindow()->GetGui()->HandleWindowEvents(event_impl);
    switch (event.type) {
#ifndef TARGET_WEB
        // Scancodes are broken in Emscripten SDL2: https://bugzilla.libsdl.org/show_bug.cgi?id=3259
        case SDL_KEYDOWN:
            gfx_sdl_onkeydown(event.key.keysym.scancode);
            break;
        case SDL_KEYUP:
            gfx_sdl_onkeyup(event.key.keysym.scancode);
            break;
        case SDL_MOUSEBUTTONDOWN:
            gfx_sdl_on_mouse_button_down(event.button.button - 1);
            break;
        case SDL_MOUSEBUTTONUP:
            gfx_sdl_on_mouse_button_up(event.button.button - 1);
            break;
        case SDL_MOUSEWHEEL:
            mouse_wheel_x = event.wheel.x;
            mouse_wheel_y = event.wheel.y;
            break;
#endif
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
#ifdef __APPLE__
                    SDL_GetWindowSize(wnd, &window_width, &window_height);
#else
                    SDL_GL_GetDrawableSize(wnd, &window_width, &window_height);
#endif
                    break;
                case SDL_WINDOWEVENT_CLOSE:
                    if (event.window.windowID == SDL_GetWindowID(wnd)) {
                        // We listen specifically for main window close because closing main window
                        // on macOS does not trigger SDL_Quit.
                        gfx_sdl_close();
                    }
                    break;
            }
            break;
        case SDL_DROPFILE:
            Ship::Context::GetInstance()->GetConsoleVariables()->SetString(CVAR_DROPPED_FILE, event.drop.file);
            Ship::Context::GetInstance()->GetConsoleVariables()->SetInteger(CVAR_NEW_FILE_DROPPED, 1);
            Ship::Context::GetInstance()->GetConsoleVariables()->Save();
            break;
        case SDL_QUIT:
            gfx_sdl_close();
            break;
    }
}

static void gfx_sdl_handle_events() {
    SDL_Event event;
    SDL_PumpEvents();
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_CONTROLLERDEVICEADDED - 1) > 0) {
        gfx_sdl_handle_single_event(event);
    }
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_CONTROLLERDEVICEREMOVED + 1, SDL_LASTEVENT) > 0) {
        gfx_sdl_handle_single_event(event);
    }

    // resync fullscreen state
#ifdef __APPLE__
    auto nextFullscreenState = isNativeMacOSFullscreenActive(wnd);
    if (fullscreen_state != nextFullscreenState) {
        fullscreen_state = nextFullscreenState;
        if (on_fullscreen_changed_callback != nullptr) {
            on_fullscreen_changed_callback(fullscreen_state);
        }
    }
#endif
}

static bool gfx_sdl_is_frame_ready() {
    return true;
}

static uint64_t qpc_to_100ns(uint64_t qpc) {
    const uint64_t qpc_freq = SDL_GetPerformanceFrequency();
    return qpc / qpc_freq * _100NANOSECONDS_IN_SECOND + qpc % qpc_freq * _100NANOSECONDS_IN_SECOND / qpc_freq;
}

static inline void sync_framerate_with_timer() {
    uint64_t t;
    t = qpc_to_100ns(SDL_GetPerformanceCounter());

    const int64_t next = previous_time + 10 * FRAME_INTERVAL_US_NUMERATOR / FRAME_INTERVAL_US_DENOMINATOR;
    int64_t left = next - t;
#if defined(_WIN32) || defined(__APPLE__)
    // We want to exit a bit early, so we can busy-wait the rest to never miss the deadline
    left -= 15000UL;
#endif
    if (left > 0) {
#ifndef _WIN32
        const timespec spec = { 0, left * 100 };
        nanosleep(&spec, nullptr);
#else
        // The accuracy of this timer seems to usually be within +- 1.0 ms
        LARGE_INTEGER li;
        li.QuadPart = -left;
        SetWaitableTimer(timer, &li, 0, nullptr, nullptr, false);
        WaitForSingleObject(timer, INFINITE);
#endif
    }

#if defined(_WIN32) || defined(__APPLE__)
    t = qpc_to_100ns(SDL_GetPerformanceCounter());
    while (t < next) {
#ifdef _WIN32
        YieldProcessor();
#elif defined(__APPLE__)
        sched_yield();
#endif
        t = qpc_to_100ns(SDL_GetPerformanceCounter());
    }
#endif
    t = qpc_to_100ns(SDL_GetPerformanceCounter());
    if (left > 0 && t - next < 10000) {
        // In case it takes some time for the application to wake up after sleep,
        // or inaccurate timer,
        // don't let that slow down the framerate.
        t = next;
    }
    previous_time = t;
}

static void gfx_sdl_swap_buffers_begin() {
    bool nextVsyncEnabled = Ship::Context::GetInstance()->GetConsoleVariables()->GetInteger(CVAR_VSYNC_ENABLED, 1);

    if (vsync_enabled != nextVsyncEnabled) {
        vsync_enabled = nextVsyncEnabled;
        SDL_GL_SetSwapInterval(vsync_enabled ? 1 : 0);
        SDL_RenderSetVSync(renderer, vsync_enabled ? 1 : 0);
    }

    sync_framerate_with_timer();
    SDL_GL_SwapWindow(wnd);
}

static void gfx_sdl_swap_buffers_end() {
}

static double gfx_sdl_get_time() {
    return 0.0;
}

static void gfx_sdl_set_target_fps(int fps) {
    target_fps = fps;
}

static void gfx_sdl_set_maximum_frame_latency(int latency) {
    // Not supported by SDL :(
}

static const char* gfx_sdl_get_key_name(int scancode) {
    return SDL_GetScancodeName((SDL_Scancode)untranslate_scancode(scancode));
}

bool gfx_sdl_can_disable_vsync() {
    return true;
}

bool gfx_sdl_is_running() {
    return is_running;
}

void gfx_sdl_destroy() {
    // TODO: destroy _any_ resources used by SDL

    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

bool gfx_sdl_is_fullscreen() {
    return fullscreen_state;
}

struct GfxWindowManagerAPI gfx_sdl = { gfx_sdl_init,
                                       gfx_sdl_close,
                                       gfx_sdl_set_keyboard_callbacks,
                                       gfx_sdl_set_mouse_callbacks,
                                       gfx_sdl_set_fullscreen_changed_callback,
                                       gfx_sdl_set_fullscreen,
                                       gfx_sdl_get_active_window_refresh_rate,
                                       gfx_sdl_set_cursor_visibility,
                                       gfx_sdl_set_mouse_pos,
                                       gfx_sdl_get_mouse_pos,
                                       gfx_sdl_get_mouse_delta,
                                       gfx_sdl_get_mouse_wheel,
                                       gfx_sdl_get_mouse_state,
                                       gfx_sdl_set_mouse_capture,
                                       gfx_sdl_is_mouse_captured,
                                       gfx_sdl_get_dimensions,
                                       gfx_sdl_handle_events,
                                       gfx_sdl_is_frame_ready,
                                       gfx_sdl_swap_buffers_begin,
                                       gfx_sdl_swap_buffers_end,
                                       gfx_sdl_get_time,
                                       gfx_sdl_set_target_fps,
                                       gfx_sdl_set_maximum_frame_latency,
                                       gfx_sdl_get_key_name,
                                       gfx_sdl_can_disable_vsync,
                                       gfx_sdl_is_running,
                                       gfx_sdl_destroy,
                                       gfx_sdl_is_fullscreen };

#endif

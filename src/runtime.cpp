#include "runtime.h"

#include "bink_playback.h"
#include "config.h"
#include "crash_dump.h"
#include "d3d_proc_redirect.h"
#include "direct_input_policy.h"
#include "engine_fov.h"
#include "engine_vars_overrides.h"
#include "execution_policy.h"
#include "frame_rate_override.h"
#include "gameplay_delta.h"
#include "intro_skip.h"
#include "legacy_runtime_policy.h"
#include "lost_device_policy.h"
#include "memory_utils.h"
#include "present_params.h"
#include "precise_sleep.h"
#include "projection_fix.h"
#include "qpc_timer.h"
#include "raw_mouse.h"
#include "render_hook_targets.h"
#include "scheduler_timing.h"
#include "scheduler_targets.h"
#include "timing_diagnostics.h"
#include "thread_diagnostics.h"
#include "viewport_fix.h"
#include "window_mode.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <d3d9.h>
#include <DbgHelp.h>
#include <avrt.h>
#include <intrin.h>
#include <mmsystem.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace shh
{
namespace
{
using Direct3DCreate9Fn = IDirect3D9*(WINAPI*)(UINT);
using Direct3DCreate9ExFn = HRESULT(WINAPI*)(UINT, IDirect3D9Ex**);
using CreateDeviceFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD,
                                D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
using CreateDeviceExFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirect3D9Ex*, UINT, D3DDEVTYPE, HWND, DWORD,
                                D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*,
                                IDirect3DDevice9Ex**);
using ResetFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
using ResetExFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9Ex*, D3DPRESENT_PARAMETERS*,
                                D3DDISPLAYMODEEX*);
using TestCooperativeLevelFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*);
using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND,
                                             const RGNDATA*);
using PresentExFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9Ex*, const RECT*, const RECT*,
                                                HWND, const RGNDATA*, DWORD);
using SwapChainPresentFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirect3DSwapChain9*, const RECT*, const RECT*, HWND,
                                const RGNDATA*, DWORD);
using EndSceneFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*);
using SetTransformFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DTRANSFORMSTATETYPE, const D3DMATRIX*);
using SetViewportFn = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*, CONST D3DVIEWPORT9*);
using EngineSettingsInitFn = void(__fastcall*)(void*);
using EngineSleepExFn = void(__cdecl*)(float, BOOL);
using SchedulerLoopUpdateFn = void(__fastcall*)(void*, void*, float);
using GameplayUpdateFn = std::uint32_t(__fastcall*)(void*, void*, float*, char);
using ShvMainLoopFn = void(__fastcall*)(void*, void*, std::uintptr_t, std::uintptr_t);
using GetTickCountFn = DWORD(WINAPI*)(void);
using DirectInput8CreateFn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
using DirectInputCreateDeviceAFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirectInput8A*, REFGUID, LPDIRECTINPUTDEVICE8A*, LPUNKNOWN);
using DirectInputCreateDeviceWFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirectInput8W*, REFGUID, LPDIRECTINPUTDEVICE8W*, LPUNKNOWN);
using DirectInputGetDeviceStateAFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8A*, DWORD, LPVOID);
using DirectInputGetDeviceStateWFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8W*, DWORD, LPVOID);
using DirectInputSetCooperativeLevelAFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8A*, HWND, DWORD);
using DirectInputSetCooperativeLevelWFn =
    HRESULT(STDMETHODCALLTYPE*)(IDirectInputDevice8W*, HWND, DWORD);
using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
using SetProcessDPIAwareFn = BOOL(WINAPI*)();
using CreateThreadFn = HANDLE(WINAPI*)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE,
                                       LPVOID, DWORD, LPDWORD);
using ResumeThreadFn = DWORD(WINAPI*)(HANDLE);
using SuspendThreadFn = DWORD(WINAPI*)(HANDLE);
using TerminateThreadFn = BOOL(WINAPI*)(HANDLE, DWORD);
using SleepFn = VOID(WINAPI*)(DWORD);
using ExitProcessFn = VOID(WINAPI*)(UINT);
using TerminateProcessFn = BOOL(WINAPI*)(HANDLE, UINT);
using RaiseExceptionFn = VOID(WINAPI*)(DWORD, DWORD, DWORD, const ULONG_PTR*);
using SetProcessInformationFn =
    BOOL(WINAPI*)(HANDLE, PROCESS_INFORMATION_CLASS, LPVOID, DWORD);
using SetThreadInformationFn = BOOL(WINAPI*)(HANDLE, THREAD_INFORMATION_CLASS, LPVOID, DWORD);
using SetTimerFn = UINT_PTR(WINAPI*)(HWND, UINT_PTR, UINT, TIMERPROC);
using PostQuitMessageFn = VOID(WINAPI*)(int);
using GetProcAddressFn = FARPROC(WINAPI*)(HMODULE, LPCSTR);
using TimePeriodFn = UINT(WINAPI*)(UINT);
using TimeSetEventFn = MMRESULT(WINAPI*)(UINT, UINT, LPTIMECALLBACK, DWORD_PTR, UINT);
using TimeKillEventFn = MMRESULT(WINAPI*)(MMRESULT);
using SetWaitableTimerFn =
    BOOL(WINAPI*)(HANDLE, const LARGE_INTEGER*, LONG, PTIMERAPCROUTINE, LPVOID, BOOL);
using AvSetMmThreadCharacteristicsWFn = HANDLE(WINAPI*)(LPCWSTR, LPDWORD);
using AvSetMmThreadPriorityFn = BOOL(WINAPI*)(HANDLE, AVRT_PRIORITY);
using AvRevertMmThreadCharacteristicsFn = BOOL(WINAPI*)(HANDLE);
using BinkOpenFn = void*(__stdcall*)(const char*, std::uint32_t);
using BinkDoFrameFn = std::uint32_t(__stdcall*)(void*);
using BinkNextFrameFn = std::uint32_t(__stdcall*)(void*);
using BinkGotoFn = int(__stdcall*)(void*, int, std::uint32_t);
using BinkPauseFn = int(__stdcall*)(void*, int);
using BinkWaitFn = int(__stdcall*)(void*);
using BinkCloseFn = void(__stdcall*)(void*);

constexpr std::size_t kCreateDeviceVtableIndex = 16;
constexpr std::size_t kCreateDeviceExVtableIndex = 20;
constexpr std::size_t kTestCooperativeLevelVtableIndex = 3;
constexpr std::size_t kResetVtableIndex = 16;
constexpr std::size_t kPresentVtableIndex = 17;
constexpr std::size_t kPresentExVtableIndex = 121;
constexpr std::size_t kEndSceneVtableIndex = 42;
constexpr std::size_t kResetExVtableIndex = 132;
constexpr std::size_t kSetTransformVtableIndex = 44;
constexpr std::size_t kSetViewportVtableIndex = 47;
constexpr std::size_t kSwapChainPresentVtableIndex = 3;
constexpr std::size_t kPresentDetourLength = 8;
constexpr std::size_t kSwapChainPresentDetourLength = 8;
constexpr std::size_t kEndSceneDetourLength = 7;
constexpr std::size_t kDirectInputCreateDeviceVtableIndex = 3;
constexpr std::size_t kDirectInputGetDeviceStateVtableIndex = 9;
constexpr std::size_t kDirectInputSetCooperativeLevelVtableIndex = 13;
constexpr std::size_t kRelativeJumpLength = 5;
constexpr std::uintptr_t kEngineSettingsInitRva = 0x00A27970;
constexpr std::uintptr_t kLegacyFramePacingTimerCallerRva = 0x00A436F6;
constexpr std::uintptr_t kEngineSleepExRva = 0x0080BAC0;
constexpr std::uintptr_t kHashStringRva = 0x0088B6A0;
constexpr std::uintptr_t kEngineFramePacingSleepCallerRva = 0x00A4CCEB;
constexpr std::uintptr_t kLegacyWaitableTimerWorkerSleepReturnRva = 0x00822B78;
constexpr std::uintptr_t kLegacyWaitableTimerSetupReturnRva = 0x00822CFB;
constexpr std::uintptr_t kLegacyThreadWrapperCreateReturnRva = 0x0080BCFC;
constexpr std::uintptr_t kLegacyThreadWrapperTerminateReturnRva = 0x0080B7C2;
constexpr std::uintptr_t kLegacyThreadWrapperResumeReturnRva = 0x0080B7DD;
constexpr std::uintptr_t kLegacyThreadWrapperSuspendReturnRva = 0x0080B7FD;
constexpr std::uintptr_t kSchedulerLoopUpdateRva = 0x04ED490;
constexpr std::uintptr_t kSchedulerLoopSleepCallerRva = 0x04EB528;
constexpr std::uintptr_t kGameplayUpdateRva = 0x01264F0;
constexpr std::uintptr_t kShvMainLoopRva = 0x000CAFA0;
constexpr std::uintptr_t kLegacyPeriodicTimerCallbackRva = 0x000159DD;
constexpr std::uintptr_t kLegacyGraphicsRetrySleepReturnRva = 0x00B8CF2E;
constexpr std::uintptr_t kEngineFrameDurationMsRva = 0x0172E7E0;
constexpr std::uintptr_t kEngineTargetFrameRateRva = 0x0172E7E8;
constexpr std::uintptr_t kSchedulerUpdateDeltaSecondsRva = 0x00D90A14;
constexpr std::uintptr_t kSchedulerSleepMillisecondsRva = 0x00D90A18;
constexpr std::uint32_t kSchedulerTimingLogSampleLimit = 8;
constexpr std::uint32_t kSchedulerTimingSummaryInterval = 120;
constexpr std::uint32_t kGameplayDeltaLogSampleLimit = 12;
constexpr std::uint32_t kRenderCadenceWarmupLogLimit = 12;
constexpr std::uint32_t kRenderCadenceOutlierLogLimit = 12;
constexpr std::uint32_t kRenderDurationWarmupLogLimit = 8;
constexpr std::uint32_t kRenderDurationOutlierLogLimit = 12;
constexpr std::uint32_t kRenderTimingSummaryInterval = 120;
constexpr std::size_t kDefaultFovOffset = 0x114;
constexpr std::size_t kCurrentFovOffset = 0x118;
constexpr DWORD kInitRetryCount = 200;
constexpr DWORD kInitRetryDelayMs = 50;
constexpr std::uint32_t kBinkOpenLogLimit = 16;
constexpr std::uint32_t kBinkPlaybackLogLimit = 24;
constexpr DWORD kLostDeviceDeactivateWindowMs = 8000;
constexpr DWORD kLostDeviceRestoreWindowMs = 4000;
constexpr std::uint32_t kSyntheticLegacyTimerIdBase = 0x51510000u;

bool ShouldUseImmediatePresentationInBorderless(const Config& config)
{
    return config.forceBorderless && config.reduceBorderlessPresentStutter;
}

INIT_ONCE g_runtimeInitOnce = INIT_ONCE_STATIC_INIT;
HMODULE g_module = nullptr;
Config g_config{};
std::filesystem::path g_moduleDirectory;
std::mutex g_hookMutex;

struct CadenceTracker
{
    std::uint64_t lastCounter = 0;
    std::uint32_t warmupLogCount = 0;
    std::uint32_t outlierLogCount = 0;
    std::uint32_t durationWarmupLogCount = 0;
    std::uint32_t durationOutlierLogCount = 0;
    std::uint32_t summaryLogCount = 0;
    bool firstHitLogged = false;
    DWORD firstThreadId = 0;
    CadenceSummaryWindow cadenceSummary = {};
    CadenceSummaryWindow durationSummary = {};
};

struct TrackedBinkMovieState
{
    std::string path;
    std::uint32_t openFlags = 0;
    std::uint32_t openOrdinal = 0;
    DWORD openedAtTick = 0;
    CadenceTracker waitTracker = {};
    CadenceTracker doFrameTracker = {};
    CadenceTracker nextFrameTracker = {};
    CadenceTracker pauseTracker = {};
};

Direct3DCreate9Fn g_originalDirect3DCreate9 = nullptr;
Direct3DCreate9ExFn g_originalDirect3DCreate9Ex = nullptr;
CreateDeviceFn g_originalCreateDevice = nullptr;
CreateDeviceExFn g_originalCreateDeviceEx = nullptr;
TestCooperativeLevelFn g_originalTestCooperativeLevel = nullptr;
ResetFn g_originalReset = nullptr;
ResetExFn g_originalResetEx = nullptr;
PresentFn g_originalPresent = nullptr;
PresentFn g_originalPresentCodeGateway = nullptr;
PresentExFn g_originalPresentEx = nullptr;
SwapChainPresentFn g_originalSwapChainPresent = nullptr;
SwapChainPresentFn g_originalSwapChainPresentCodeGateway = nullptr;
EndSceneFn g_originalEndScene = nullptr;
EndSceneFn g_originalEndSceneCodeGateway = nullptr;
SetTransformFn g_originalSetTransform = nullptr;
SetViewportFn g_originalSetViewport = nullptr;
EngineSettingsInitFn g_originalEngineSettingsInit = nullptr;
EngineSleepExFn g_originalEngineSleepEx = nullptr;
SchedulerLoopUpdateFn g_originalSchedulerLoopUpdate = nullptr;
GameplayUpdateFn g_originalGameplayUpdate = nullptr;
ShvMainLoopFn g_originalShvMainLoop = nullptr;
GetTickCountFn g_originalGetTickCount = nullptr;
DirectInput8CreateFn g_originalDirectInput8Create = nullptr;
DirectInputCreateDeviceAFn g_originalDirectInputCreateDeviceA = nullptr;
DirectInputCreateDeviceWFn g_originalDirectInputCreateDeviceW = nullptr;
DirectInputGetDeviceStateAFn g_originalDirectInputGetDeviceStateA = nullptr;
DirectInputGetDeviceStateWFn g_originalDirectInputGetDeviceStateW = nullptr;
DirectInputSetCooperativeLevelAFn g_originalDirectInputSetCooperativeLevelA = nullptr;
DirectInputSetCooperativeLevelWFn g_originalDirectInputSetCooperativeLevelW = nullptr;
CreateThreadFn g_originalCreateThread = nullptr;
ResumeThreadFn g_originalResumeThread = nullptr;
SuspendThreadFn g_originalSuspendThread = nullptr;
TerminateThreadFn g_originalTerminateThread = nullptr;
SleepFn g_originalShvSleep = nullptr;
SleepFn g_originalEngineSleep = nullptr;
ExitProcessFn g_originalExitProcess = nullptr;
TerminateProcessFn g_originalTerminateProcess = nullptr;
RaiseExceptionFn g_originalRaiseException = nullptr;
SetProcessInformationFn g_setProcessInformation = nullptr;
SetThreadInformationFn g_setThreadInformation = nullptr;
SetTimerFn g_originalShvSetTimer = nullptr;
GetProcAddressFn g_originalShvGetProcAddress = nullptr;
PostQuitMessageFn g_originalPostQuitMessage = nullptr;
AvSetMmThreadCharacteristicsWFn g_avSetMmThreadCharacteristicsW = nullptr;
AvSetMmThreadPriorityFn g_avSetMmThreadPriority = nullptr;
AvRevertMmThreadCharacteristicsFn g_avRevertMmThreadCharacteristics = nullptr;
HANDLE g_renderThreadMmcssHandle = nullptr;
DWORD g_renderThreadMmcssTaskIndex = 0;
BinkOpenFn g_originalBinkOpen = nullptr;
BinkDoFrameFn g_originalBinkDoFrame = nullptr;
BinkNextFrameFn g_originalBinkNextFrame = nullptr;
BinkGotoFn g_originalBinkGoto = nullptr;
BinkPauseFn g_originalBinkPause = nullptr;
BinkWaitFn g_originalBinkWait = nullptr;
BinkCloseFn g_originalBinkClose = nullptr;
std::once_flag g_projectionLogOnce;
std::once_flag g_executionPolicyApiOnce;
std::once_flag g_renderThreadExecutionPolicyOnce;
std::atomic<std::uint32_t> g_backBufferWidth(0);
std::atomic<std::uint32_t> g_backBufferHeight(0);
std::once_flag g_viewportClampLogOnce;
HighPrecisionTimerState g_highPrecisionTimerState{};
TimePeriodFn g_timeBeginPeriod = nullptr;
TimePeriodFn g_timeEndPeriod = nullptr;
TimeSetEventFn g_originalTimeSetEvent = nullptr;
TimeKillEventFn g_originalTimeKillEvent = nullptr;
SetWaitableTimerFn g_originalSetWaitableTimer = nullptr;
bool g_requestedHighResolutionTimer = false;
void* g_mouseDeviceA = nullptr;
void* g_mouseDeviceW = nullptr;
HWND g_rawInputWindow = nullptr;
WNDPROC g_originalRawInputWndProc = nullptr;
HWND g_gameWindow = nullptr;
LostDeviceRecoveryState g_lostDeviceRecoveryState{};
std::atomic<long> g_rawMouseDeltaX(0);
std::atomic<long> g_rawMouseDeltaY(0);
std::once_flag g_rawMouseLogOnce;
std::once_flag g_directInputMouseRecoveryLogOnce;
std::once_flag g_directInputMouseCoopLogOnce;
std::once_flag g_preciseSleepLogOnce;
std::once_flag g_legacyPeriodicTimerLogOnce;
std::once_flag g_legacyPeriodicTimerSuppressionLogOnce;
std::once_flag g_legacyPeriodicTimerKillLogOnce;
std::once_flag g_legacyWaitableTimerWorkerCaptureLogOnce;
std::once_flag g_legacyWaitableTimerWorkerSleepLogOnce;
std::once_flag g_legacyWaitableTimerWorkerYieldLogOnce;
std::once_flag g_legacyWaitableTimerWorkerWaitFailureLogOnce;
std::once_flag g_stdAudioWorkerTrackingLogOnce;
std::once_flag g_engineFrameBudgetLogOnce;
std::once_flag g_schedulerLoopTargetLogOnce;
std::atomic<std::uint32_t> g_schedulerUpdateLogCount(0);
std::atomic<std::uint32_t> g_schedulerSleepLogCount(0);
std::atomic<std::uint32_t> g_gameplayDeltaLogCount(0);
std::mutex g_timingDiagnosticsMutex;
std::mutex g_schedulerTimingMutex;
std::mutex g_gameplayTimingMutex;
std::mutex g_lostDeviceMutex;
std::unordered_set<std::uintptr_t> g_loggedSleepCallers;
std::unordered_set<std::uintptr_t> g_loggedTimeSetEventCallers;
std::unordered_set<std::uintptr_t> g_loggedSetWaitableTimerCallers;
std::unordered_set<std::uintptr_t> g_loggedLegacyGraphicsRetrySleepCallers;
std::unordered_set<std::uintptr_t> g_loggedCreateThreadCallers;
std::unordered_set<std::uintptr_t> g_loggedLegacyThreadWrapperCreateCallers;
std::unordered_set<std::uint64_t> g_loggedLegacyThreadWrapperPayloads;
std::unordered_set<std::uintptr_t> g_loggedLegacyThreadResumeSuppressCallers;
std::unordered_set<std::uintptr_t> g_loggedLegacyThreadSuspendSuppressCallers;
std::unordered_set<std::uintptr_t> g_loggedLegacyThreadTerminateSoftenCallers;
std::unordered_set<std::uintptr_t> g_loggedLegacyThreadTerminateFallbackCallers;
std::unordered_set<std::uintptr_t> g_loggedLegacyThreadTerminateFailureCallers;
std::unordered_set<std::uintptr_t> g_loggedExitProcessCallers;
std::unordered_set<std::uintptr_t> g_loggedTerminateProcessCallers;
std::unordered_set<std::uintptr_t> g_loggedRaiseExceptionCallers;
std::unordered_set<std::uintptr_t> g_loggedPostQuitMessageCallers;
std::atomic<std::uintptr_t> g_chainedUnhandledExceptionFilter(0);
volatile long g_crashDumpWriteState = 0;
HMODULE g_engineModule = nullptr;
HMODULE g_shvModule = nullptr;
float* g_engineFrameDurationMs = nullptr;
std::uint32_t* g_engineTargetFrameRate = nullptr;
float* g_schedulerUpdateDeltaSeconds = nullptr;
float* g_schedulerSleepMilliseconds = nullptr;
std::uint64_t g_schedulerLastWakeCounter = 0;
std::unordered_map<std::uintptr_t, std::uint64_t> g_schedulerLastCounterByOwner;
std::unordered_map<std::uintptr_t, std::uint64_t> g_gameplayLastCounterByOwner;
CadenceTracker g_devicePresentCadence;
CadenceTracker g_devicePresentExCadence;
CadenceTracker g_swapChainPresentCadence;
CadenceTracker g_endSceneCadence;
CadenceTracker g_shvMainLoopCadence;
CadenceSummaryWindow g_schedulerTimingSummary;
std::mutex g_introMovieMutex;
std::unordered_map<void*, std::string> g_zeroWaitMenuMovies;
std::unordered_map<void*, std::shared_ptr<TrackedBinkMovieState>> g_trackedBinkMovies;
std::unordered_map<std::string, std::uint32_t> g_trackedBinkMovieOpenOrdinals;
std::once_flag g_gameplayThreadLogOnce;
std::atomic<std::uint32_t> g_schedulerTimingSummaryLogCount(0);
std::atomic<std::uint32_t> g_devicePresentHitCount(0);
std::atomic<std::uint32_t> g_devicePresentExHitCount(0);
std::atomic<std::uint32_t> g_swapChainPresentHitCount(0);
std::atomic<std::uint32_t> g_endSceneHitCount(0);
std::atomic<std::uint32_t> g_shvSleepHitCount(0);
std::atomic<std::uint32_t> g_shvSetTimerHitCount(0);
std::atomic<std::uint32_t> g_shvMainLoopHitCount(0);
std::atomic<std::uint32_t> g_rawInputPacketCount(0);
std::atomic<std::uint32_t> g_rawMouseOverrideCount(0);
std::atomic<std::uint32_t> g_directInputRecoveryCount(0);
std::atomic<std::uint32_t> g_introInterventionCount(0);
std::atomic<std::uint32_t> g_lostDeviceSignalCount(0);
std::atomic<std::uint32_t> g_legacyPeriodicTimerSuppressionCount(0);
std::atomic<std::uint32_t> g_legacyPeriodicTimerKillSwallowCount(0);
std::atomic<std::uint32_t> g_legacyWaitableTimerAlignmentCount(0);
std::atomic<std::uint32_t> g_legacyWaitableTimerYieldCount(0);
std::atomic<std::uint32_t> g_legacyThreadWrapperCreateObservedCount(0);
std::atomic<std::uint32_t> g_shvSleepLogCount(0);
std::atomic<std::uint32_t> g_shvSetTimerLogCount(0);
std::atomic<std::uint32_t> g_nextSyntheticLegacyTimerId(kSyntheticLegacyTimerIdBase);
std::atomic<std::uint32_t> g_activeSyntheticLegacyTimerId(0);
std::atomic<HANDLE> g_legacyWaitableTimerWorkerHandle(nullptr);
std::atomic<DWORD> g_stdAudioThreadId(0);
std::atomic<std::uintptr_t> g_stdAudioRootObject(0);
std::atomic<std::uintptr_t> g_stdAudioRouterVtable(0);
CadenceTracker g_stdAudioWorkerSleepCadence;
std::atomic<std::uint32_t> g_invalidRenderDeviceLogCount(0);
std::atomic<std::uint32_t> g_binkOpenLogCount(0);
std::atomic<std::uint32_t> g_binkPlaybackLogCount(0);
std::atomic<bool> g_shvHooksInstalled(false);
std::once_flag g_shvCreate9RedirectLogOnce;
std::once_flag g_shvCreate9ExRedirectLogOnce;
std::uintptr_t g_lastDeviceAddress = 0;
std::uintptr_t g_lastSwapChainAddress = 0;
std::uintptr_t g_lastOriginalPresentAddress = 0;
std::uintptr_t g_lastOriginalPresentExAddress = 0;
std::uintptr_t g_lastOriginalSwapChainPresentAddress = 0;
std::uintptr_t g_lastOriginalEndSceneAddress = 0;
std::uintptr_t g_lastPresentCodeGatewayAddress = 0;
std::uintptr_t g_lastSwapChainPresentCodeGatewayAddress = 0;
std::uintptr_t g_lastEndSceneCodeGatewayAddress = 0;
thread_local std::uint32_t g_d3dProbeCreateDepth = 0;

void Log(std::string_view message);
std::string FormatAddress(std::uintptr_t address);
bool PatchPointer(void** target, void* replacement);
bool PatchVtableEntry(void* object, std::size_t index, void* replacement, void** original);
bool InstallRelativeJumpDetour(void* target, std::size_t overwriteLength, void* replacement,
                               void** original);
HRESULT TryGetSwapChainNoexcept(IDirect3DDevice9* device, UINT swapChainIndex,
                                IDirect3DSwapChain9** swapChain);
void ReleaseNoexcept(IUnknown* object);
HRESULT STDMETHODCALLTYPE HookedTestCooperativeLevel(IDirect3DDevice9* device);
HRESULT STDMETHODCALLTYPE HookedDirectInputGetDeviceStateA(IDirectInputDevice8A* device,
                                                           DWORD stateSize, LPVOID state);
HRESULT STDMETHODCALLTYPE HookedDirectInputGetDeviceStateW(IDirectInputDevice8W* device,
                                                           DWORD stateSize, LPVOID state);
HRESULT STDMETHODCALLTYPE HookedDirectInputSetCooperativeLevelA(IDirectInputDevice8A* device,
                                                                HWND window,
                                                                DWORD flags);
HRESULT STDMETHODCALLTYPE HookedDirectInputSetCooperativeLevelW(IDirectInputDevice8W* device,
                                                                HWND window,
                                                                DWORD flags);
void __cdecl HookedEngineSleepEx(float milliseconds, BOOL alertable);
void __fastcall HookedSchedulerLoopUpdate(void* objectPointer, void* reserved,
                                          float deltaSeconds);
std::uint32_t __fastcall HookedGameplayUpdate(void* objectPointer, void* reserved,
                                              float* deltaSeconds, char allowAdvance);
MMRESULT WINAPI HookedTimeSetEvent(UINT delay, UINT resolution, LPTIMECALLBACK callback,
                                   DWORD_PTR user, UINT eventType);
MMRESULT WINAPI HookedTimeKillEvent(MMRESULT timerId);
BOOL WINAPI HookedSetWaitableTimer(HANDLE timer, const LARGE_INTEGER* dueTime, LONG period,
                                   PTIMERAPCROUTINE completionRoutine, LPVOID argument,
                                   BOOL resumeFromSuspend);
HANDLE WINAPI HookedCreateThread(LPSECURITY_ATTRIBUTES attributes, SIZE_T stackSize,
                                 LPTHREAD_START_ROUTINE startAddress, LPVOID parameter,
                                 DWORD creationFlags, LPDWORD threadId);
DWORD WINAPI HookedResumeThread(HANDLE threadHandle);
DWORD WINAPI HookedSuspendThread(HANDLE threadHandle);
BOOL WINAPI HookedTerminateThread(HANDLE threadHandle, DWORD exitCode);
VOID WINAPI HookedEngineSleep(DWORD milliseconds);
VOID WINAPI HookedShvSleep(DWORD milliseconds);
UINT_PTR WINAPI HookedShvSetTimer(HWND window, UINT_PTR timerId, UINT elapse,
                                  TIMERPROC timerProc);
FARPROC WINAPI HookedShvGetProcAddress(HMODULE module, LPCSTR procName);
void __fastcall HookedShvMainLoop(void* self, void* /*edx*/, std::uintptr_t arg1,
                                  std::uintptr_t arg2);
void* __stdcall HookedBinkOpen(const char* path, std::uint32_t flags);
std::uint32_t __stdcall HookedBinkDoFrame(void* movieHandle);
std::uint32_t __stdcall HookedBinkNextFrame(void* movieHandle);
int __stdcall HookedBinkGoto(void* movieHandle, int frameIndex, std::uint32_t flags);
int __stdcall HookedBinkPause(void* movieHandle, int paused);
int __stdcall HookedBinkWait(void* movieHandle);
void __stdcall HookedBinkClose(void* movieHandle);
VOID WINAPI HookedExitProcess(UINT exitCode);
BOOL WINAPI HookedTerminateProcess(HANDLE processHandle, UINT exitCode);
VOID WINAPI HookedRaiseException(DWORD exceptionCode, DWORD exceptionFlags,
                                 DWORD numberOfArguments,
                                 const ULONG_PTR* arguments);
VOID WINAPI HookedPostQuitMessage(int exitCode);
LONG WINAPI HookedUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers);
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI HookedSetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER filter);
void InitializeExecutionPolicyApis();
void ApplyProcessExecutionPolicy();
void ApplyCurrentRenderThreadExecutionPolicy();
DWORD GetLostDeviceNowTick();
void ArmLostDeviceRecoveryWindow(const char* reason, DWORD durationMs, bool markDeviceLoss);
void NoteUserInitiatedWindowClose();
void ClearLostDeviceRecoveryWindow(const char* reason);
bool ShouldSuppressLostDeviceShutdownNow();
bool EnsureGameWindowHook(HWND window);
bool ResolveEngineFrameBudgetPointers(HMODULE engineModule);
void ApplyLegacyFramePacingBudget(std::uint32_t fallbackRequestedFrameRate);
bool IsCommittedMemoryRange(void* address, std::size_t size);
bool IsWritableMemoryRange(void* address, std::size_t size);
bool ResolveSchedulerTimingPointers(HMODULE engineModule);
std::uintptr_t ReadVtableEntryAddress(void* object, std::size_t index);
bool TryPatchVtableEntrySafe(void* object, std::size_t index, void* replacement, void** original);
bool HasExecutableVtableEntries(void* object,
                                std::initializer_list<std::size_t> requiredIndices);
bool StartsWithRelativeJumpDetour(std::uintptr_t address);
bool InstallGlobalD3DCodeHooks();
bool InstallBinkMovieHooks(HMODULE module, const char* moduleLabel);
bool InstallDeviceExHooks(IDirect3DDevice9* device);
void EnsureRenderHooksStillInstalled();
bool TryInstallShvHooks();
void InstallSwapChainPresentHook(IDirect3DDevice9* device);
HRESULT STDMETHODCALLTYPE HookedPresent(IDirect3DDevice9* device, const RECT* sourceRect,
                                        const RECT* destRect, HWND destWindowOverride,
                                        const RGNDATA* dirtyRegion);
HRESULT STDMETHODCALLTYPE HookedPresentEx(IDirect3DDevice9Ex* device, const RECT* sourceRect,
                                          const RECT* destRect, HWND destWindowOverride,
                                          const RGNDATA* dirtyRegion, DWORD flags);
HRESULT STDMETHODCALLTYPE HookedEndScene(IDirect3DDevice9* device);
HRESULT STDMETHODCALLTYPE HookedSetTransform(IDirect3DDevice9* device,
                                             D3DTRANSFORMSTATETYPE state,
                                             const D3DMATRIX* matrix);
HRESULT STDMETHODCALLTYPE HookedSetViewport(IDirect3DDevice9* device,
                                            CONST D3DVIEWPORT9* viewport);
HRESULT STDMETHODCALLTYPE HookedSwapChainPresent(IDirect3DSwapChain9* swapChain,
                                                 const RECT* sourceRect,
                                                 const RECT* destRect,
                                                 HWND destWindowOverride,
                                                 const RGNDATA* dirtyRegion,
                                                 DWORD flags);
HRESULT STDMETHODCALLTYPE HookedResetEx(IDirect3DDevice9Ex* device,
                                        D3DPRESENT_PARAMETERS* presentationParameters,
                                        D3DDISPLAYMODEEX* fullscreenDisplayMode);
HRESULT STDMETHODCALLTYPE HookedCreateDeviceEx(IDirect3D9Ex* direct3d, UINT adapter,
                                               D3DDEVTYPE deviceType, HWND focusWindow,
                                               DWORD behaviorFlags,
                                               D3DPRESENT_PARAMETERS* presentationParameters,
                                               D3DDISPLAYMODEEX* fullscreenDisplayMode,
                                               IDirect3DDevice9Ex** returnedDevice);

double ResolveDiagnosticTargetFrameDurationMs()
{
    if (g_config.targetFrameRate > 0)
    {
        return 1000.0 / static_cast<double>(g_config.targetFrameRate);
    }

    return 1000.0 / 60.0;
}

void LogSchedulerLoopTargets(void* objectPointer)
{
    std::uintptr_t vtableAddress = 0;
    if (!TryReadPointerValue(objectPointer, vtableAddress))
    {
        return;
    }

    const SchedulerLoopTargets targets = ResolveSchedulerLoopTargets(objectPointer);

    std::ostringstream message;
    message << "Scheduler loop update active on thread " << ::GetCurrentThreadId()
            << ", object=" << FormatAddress(reinterpret_cast<std::uintptr_t>(objectPointer))
            << ", vtable=" << FormatAddress(vtableAddress)
            << ", update=" << FormatAddress(targets.updateTarget)
            << ", post=" << FormatAddress(targets.postUpdateTarget);
    if (g_schedulerUpdateDeltaSeconds != nullptr && g_schedulerSleepMilliseconds != nullptr)
    {
        message << ", observedDelta=" << *g_schedulerUpdateDeltaSeconds
                << "s, observedSleep=" << *g_schedulerSleepMilliseconds << "ms";
    }
    Log(message.str());
}

void LogSchedulerTimingSummary(double targetFrameMs)
{
    const auto summary = ConsumeCadenceSummary(&g_schedulerTimingSummary,
                                               kSchedulerTimingSummaryInterval);
    if (!summary.has_value())
    {
        return;
    }

    if (g_devicePresentHitCount.load(std::memory_order_relaxed) == 0 &&
        g_devicePresentExHitCount.load(std::memory_order_relaxed) == 0 &&
        g_swapChainPresentHitCount.load(std::memory_order_relaxed) == 0 &&
        g_endSceneHitCount.load(std::memory_order_relaxed) == 0)
    {
        EnsureRenderHooksStillInstalled();
    }

    const std::uint32_t summaryIndex =
        g_schedulerTimingSummaryLogCount.fetch_add(1, std::memory_order_relaxed) + 1;
    const std::uint32_t deviceHits =
        g_devicePresentHitCount.load(std::memory_order_relaxed);
    const std::uint32_t deviceExHits =
        g_devicePresentExHitCount.load(std::memory_order_relaxed);
    const std::uint32_t swapHits =
        g_swapChainPresentHitCount.load(std::memory_order_relaxed);
    const std::uint32_t endSceneHits =
        g_endSceneHitCount.load(std::memory_order_relaxed);
    const std::uint32_t shvSleepHits =
        g_shvSleepHitCount.load(std::memory_order_relaxed);
    const std::uint32_t shvSetTimerHits =
        g_shvSetTimerHitCount.load(std::memory_order_relaxed);
    const std::uint32_t shvMainLoopHits =
        g_shvMainLoopHitCount.load(std::memory_order_relaxed);

    std::ostringstream message;
    message << "Scheduler loop summary " << summaryIndex << ": samples="
            << summary->sampleCount << ", avgWork=" << summary->averageMilliseconds
            << "ms, minWork=" << summary->minMilliseconds << "ms, maxWork="
            << summary->maxMilliseconds << "ms, stutters=" << summary->stutterCount
            << ", target=" << targetFrameMs << "ms, renderHits(device=" << deviceHits
            << ", deviceEx=" << deviceExHits << ", swap=" << swapHits
            << ", endScene=" << endSceneHits
            << "), shvHits(mainLoop=" << shvMainLoopHits << ", sleep=" << shvSleepHits
            << ", setTimer=" << shvSetTimerHits << ')';

    if (deviceHits == 0 && deviceExHits == 0 && swapHits == 0 && endSceneHits == 0)
    {
        message << ", hookState(device=" << FormatAddress(g_lastDeviceAddress)
                << ", swapChain=" << FormatAddress(g_lastSwapChainAddress)
                << ", presentOriginal=" << FormatAddress(g_lastOriginalPresentAddress)
                << ", presentExOriginal=" << FormatAddress(g_lastOriginalPresentExAddress)
                << ", swapOriginal=" << FormatAddress(g_lastOriginalSwapChainPresentAddress)
                << ", endSceneOriginal=" << FormatAddress(g_lastOriginalEndSceneAddress)
                << ", presentGateway=" << FormatAddress(g_lastPresentCodeGatewayAddress)
                << ", swapGateway="
                << FormatAddress(g_lastSwapChainPresentCodeGatewayAddress)
                << ", endSceneGateway=" << FormatAddress(g_lastEndSceneCodeGatewayAddress)
                << ", currentPresent="
                << FormatAddress(ReadVtableEntryAddress(
                       reinterpret_cast<void*>(g_lastDeviceAddress), kPresentVtableIndex))
                << ", currentEndScene="
                << FormatAddress(ReadVtableEntryAddress(
                       reinterpret_cast<void*>(g_lastDeviceAddress), kEndSceneVtableIndex))
                << ", currentViewport="
                << FormatAddress(ReadVtableEntryAddress(
                       reinterpret_cast<void*>(g_lastDeviceAddress), kSetViewportVtableIndex))
                << ')';
    }

    Log(message.str());
}

void LogRenderCadenceSample(const char* label, CadenceTracker& tracker,
                            double callDurationMs = -1.0)
{
    if (g_highPrecisionTimerState.frequency == 0)
    {
        return;
    }

    LARGE_INTEGER counter = {};
    if (!::QueryPerformanceCounter(&counter))
    {
        return;
    }

    const DWORD threadId = ::GetCurrentThreadId();
    const double targetFrameMs = ResolveDiagnosticTargetFrameDurationMs();
    double cadenceDeltaMs = 0.0;
    bool hasSample = false;
    bool isOutlier = false;
    std::uint32_t sampleNumber = 0;
    bool hasDurationSample = false;
    bool isDurationOutlier = false;
    std::uint32_t durationSampleNumber = 0;
    bool logFirstHit = false;
    DWORD firstThreadId = 0;
    std::optional<CadenceSummary> cadenceSummary;
    std::optional<CadenceSummary> durationSummary;
    std::uint32_t summaryNumber = 0;

    {
        std::scoped_lock lock(g_hookMutex);
        if (!tracker.firstHitLogged)
        {
            tracker.firstHitLogged = true;
            tracker.firstThreadId = threadId;
            logFirstHit = true;
            firstThreadId = threadId;
        }

        if (tracker.lastCounter != 0)
        {
            cadenceDeltaMs = static_cast<double>(counter.QuadPart - tracker.lastCounter) * 1000.0 /
                             static_cast<double>(g_highPrecisionTimerState.frequency);

            if (tracker.warmupLogCount < kRenderCadenceWarmupLogLimit)
            {
                hasSample = true;
                sampleNumber = ++tracker.warmupLogCount;
            }
            else if (IsCadenceOutlier(cadenceDeltaMs, targetFrameMs) &&
                     tracker.outlierLogCount < kRenderCadenceOutlierLogLimit)
            {
                hasSample = true;
                isOutlier = true;
                sampleNumber = ++tracker.outlierLogCount;
            }

            AccumulateCadenceSummary(&tracker.cadenceSummary, cadenceDeltaMs, targetFrameMs);
            cadenceSummary =
                ConsumeCadenceSummary(&tracker.cadenceSummary, kRenderTimingSummaryInterval);
        }

        if (callDurationMs >= 0.0)
        {
            AccumulateCadenceSummary(&tracker.durationSummary, callDurationMs, targetFrameMs);
            if (tracker.durationWarmupLogCount < kRenderDurationWarmupLogLimit)
            {
                hasDurationSample = true;
                durationSampleNumber = ++tracker.durationWarmupLogCount;
            }
            else if (IsCadenceOutlier(callDurationMs, targetFrameMs) &&
                     tracker.durationOutlierLogCount < kRenderDurationOutlierLogLimit)
            {
                hasDurationSample = true;
                isDurationOutlier = true;
                durationSampleNumber = ++tracker.durationOutlierLogCount;
            }

            durationSummary =
                ConsumeCadenceSummary(&tracker.durationSummary, kRenderTimingSummaryInterval);
        }

        if (cadenceSummary.has_value() || durationSummary.has_value())
        {
            summaryNumber = ++tracker.summaryLogCount;
        }

        tracker.lastCounter = static_cast<std::uint64_t>(counter.QuadPart);
    }

    if (logFirstHit)
    {
        std::ostringstream message;
        message << label << " active on thread " << firstThreadId;
        Log(message.str());
    }

    if (hasSample)
    {
        std::ostringstream message;
        message << label << (isOutlier ? " outlier " : " sample ") << sampleNumber << ": "
                << cadenceDeltaMs << "ms, target=" << targetFrameMs << "ms";
        Log(message.str());
    }

    if (hasDurationSample)
    {
        std::ostringstream message;
        message << label << (isDurationOutlier ? " duration outlier " : " duration sample ")
                << durationSampleNumber << ": " << callDurationMs
                << "ms, target=" << targetFrameMs << "ms";
        Log(message.str());
    }

    if (cadenceSummary.has_value() || durationSummary.has_value())
    {
        std::ostringstream message;
        message << label << " summary " << summaryNumber << ':';
        if (cadenceSummary.has_value())
        {
            message << " cadenceAvg=" << cadenceSummary->averageMilliseconds
                    << "ms, cadenceMin=" << cadenceSummary->minMilliseconds
                    << "ms, cadenceMax=" << cadenceSummary->maxMilliseconds
                    << "ms, cadenceStutters=" << cadenceSummary->stutterCount;
        }

        if (durationSummary.has_value())
        {
            message << " durationAvg=" << durationSummary->averageMilliseconds
                    << "ms, durationMin=" << durationSummary->minMilliseconds
                    << "ms, durationMax=" << durationSummary->maxMilliseconds
                    << "ms, durationStutters=" << durationSummary->stutterCount;
        }

        Log(message.str());
    }
}

double MeasureHighPrecisionDurationMilliseconds(const LARGE_INTEGER& startCounter,
                                                const LARGE_INTEGER& endCounter)
{
    if (g_highPrecisionTimerState.frequency == 0)
    {
        return -1.0;
    }

    return static_cast<double>(endCounter.QuadPart - startCounter.QuadPart) * 1000.0 /
           static_cast<double>(g_highPrecisionTimerState.frequency);
}

std::uintptr_t ReadVtableEntryAddress(void* object, std::size_t index)
{
    std::uintptr_t vtableAddress = 0;
    if (!TryReadPointerValue(object, vtableAddress) || vtableAddress == 0)
    {
        return 0;
    }

    std::uintptr_t entryAddress = 0;
    auto* slot = reinterpret_cast<const std::uintptr_t*>(vtableAddress) + index;
    if (!TryReadPointerValue(slot, entryAddress))
    {
        return 0;
    }

    return entryAddress;
}

bool TryPatchVtableEntrySafe(void* object, std::size_t index, void* replacement, void** original)
{
    std::uintptr_t vtableAddress = 0;
    if (!TryReadPointerValue(object, vtableAddress) || vtableAddress == 0)
    {
        return false;
    }

    auto** vtable = reinterpret_cast<void**>(vtableAddress);
    std::uintptr_t currentEntry = 0;
    if (!TryReadPointerValue(vtable + index, currentEntry))
    {
        return false;
    }

    if (currentEntry == reinterpret_cast<std::uintptr_t>(replacement))
    {
        return true;
    }

    if (original != nullptr && *original == nullptr)
    {
        *original = reinterpret_cast<void*>(currentEntry);
    }

    return PatchPointer(&vtable[index], replacement);
}

bool HasExecutableVtableEntries(void* object, std::initializer_list<std::size_t> requiredIndices)
{
    if (object == nullptr || !IsReadableMemoryRange(object, sizeof(void*)))
    {
        return false;
    }

    for (const std::size_t index : requiredIndices)
    {
        const std::uintptr_t entry = ReadVtableEntryAddress(object, index);
        if (entry == 0 || !IsExecutableMemoryAddress(entry))
        {
            return false;
        }
    }

    return true;
}

HRESULT TryGetSwapChainNoexcept(IDirect3DDevice9* device, UINT swapChainIndex,
                                IDirect3DSwapChain9** swapChain)
{
    if (swapChain == nullptr)
    {
        return E_POINTER;
    }

    *swapChain = nullptr;
    if (device == nullptr)
    {
        return E_POINTER;
    }

    __try
    {
        return device->GetSwapChain(swapChainIndex, swapChain);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return E_FAIL;
    }
}

void ReleaseNoexcept(IUnknown* object)
{
    if (object == nullptr)
    {
        return;
    }

    __try
    {
        object->Release();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void EnsureRenderHooksStillInstalled()
{
    if (g_lastDeviceAddress == 0)
    {
        return;
    }

    auto* device = reinterpret_cast<IDirect3DDevice9*>(g_lastDeviceAddress);
    if (!HasExecutableVtableEntries(
            device, {kPresentVtableIndex, kEndSceneVtableIndex, kSetTransformVtableIndex,
                     kSetViewportVtableIndex}))
    {
        const std::uint32_t logIndex =
            g_invalidRenderDeviceLogCount.fetch_add(1, std::memory_order_relaxed);
        if (logIndex < 3)
        {
            Log("Render hook revalidation skipped for stale device pointer " +
                FormatAddress(g_lastDeviceAddress) + '.');
        }
        g_lastDeviceAddress = 0;
        g_lastSwapChainAddress = 0;
        return;
    }

    bool repairedAnyHook = false;
    {
        std::scoped_lock lock(g_hookMutex);

        if (ReadVtableEntryAddress(device, kPresentVtableIndex) !=
            reinterpret_cast<std::uintptr_t>(&HookedPresent))
        {
            repairedAnyHook |=
                TryPatchVtableEntrySafe(device, kPresentVtableIndex,
                                        reinterpret_cast<void*>(&HookedPresent),
                                        reinterpret_cast<void**>(&g_originalPresent));
        }

        if (ReadVtableEntryAddress(device, kEndSceneVtableIndex) !=
            reinterpret_cast<std::uintptr_t>(&HookedEndScene))
        {
            repairedAnyHook |=
                TryPatchVtableEntrySafe(device, kEndSceneVtableIndex,
                                        reinterpret_cast<void*>(&HookedEndScene),
                                        reinterpret_cast<void**>(&g_originalEndScene));
        }

        if (ReadVtableEntryAddress(device, kSetTransformVtableIndex) !=
            reinterpret_cast<std::uintptr_t>(&HookedSetTransform))
        {
            repairedAnyHook |=
                TryPatchVtableEntrySafe(device, kSetTransformVtableIndex,
                                        reinterpret_cast<void*>(&HookedSetTransform),
                                        reinterpret_cast<void**>(&g_originalSetTransform));
        }

        if (ReadVtableEntryAddress(device, kSetViewportVtableIndex) !=
            reinterpret_cast<std::uintptr_t>(&HookedSetViewport))
        {
            repairedAnyHook |=
                TryPatchVtableEntrySafe(device, kSetViewportVtableIndex,
                                        reinterpret_cast<void*>(&HookedSetViewport),
                                        reinterpret_cast<void**>(&g_originalSetViewport));
        }

    }

    InstallSwapChainPresentHook(device);
    InstallDeviceExHooks(device);
    InstallGlobalD3DCodeHooks();

    if (repairedAnyHook)
    {
        std::ostringstream message;
        message << "Reapplied D3D device hooks: devicePresent="
                << FormatAddress(ReadVtableEntryAddress(device, kPresentVtableIndex))
                << ", endScene="
                << FormatAddress(ReadVtableEntryAddress(device, kEndSceneVtableIndex))
                << ", setTransform="
                << FormatAddress(ReadVtableEntryAddress(device, kSetTransformVtableIndex))
                << ", setViewport="
                << FormatAddress(ReadVtableEntryAddress(device, kSetViewportVtableIndex));
        Log(message.str());
    }
}

void InstallSwapChainPresentHook(IDirect3DDevice9* device)
{
    if (!HasExecutableVtableEntries(
            device, {kPresentVtableIndex, kEndSceneVtableIndex, kSetTransformVtableIndex,
                     kSetViewportVtableIndex}))
    {
        return;
    }

    IDirect3DSwapChain9* swapChain = nullptr;
    const HRESULT hr = TryGetSwapChainNoexcept(device, 0, &swapChain);
    if (FAILED(hr) || swapChain == nullptr ||
        !HasExecutableVtableEntries(swapChain, {kSwapChainPresentVtableIndex}))
    {
        return;
    }

    const bool installed =
        TryPatchVtableEntrySafe(swapChain, kSwapChainPresentVtableIndex,
                                reinterpret_cast<void*>(&HookedSwapChainPresent),
                                reinterpret_cast<void**>(&g_originalSwapChainPresent));
    g_lastSwapChainAddress = reinterpret_cast<std::uintptr_t>(swapChain);
    g_lastOriginalSwapChainPresentAddress =
        reinterpret_cast<std::uintptr_t>(g_originalSwapChainPresent);
    ReleaseNoexcept(swapChain);

    if (installed)
    {
        std::ostringstream message;
        message << "IDirect3DSwapChain9::Present hook installed at "
                << FormatAddress(g_lastSwapChainAddress)
                << ", original=" << FormatAddress(g_lastOriginalSwapChainPresentAddress);
        Log(message.str());
    }
}

bool InstallGlobalD3DCodeHooks()
{
    bool installedAnyHook = false;

    const auto installHook = [&](const char* label, std::size_t overwriteLength,
                                 std::uintptr_t replacementAddress,
                                 RenderHookTargets targets, void** codeGateway,
                                 std::uintptr_t* gatewayAddress) {
        const bool targetStartsWithRelativeJump =
            StartsWithRelativeJumpDetour(targets.vtableOriginal);
        if (!ShouldInstallRenderCodeDetour(targets, replacementAddress,
                                           targetStartsWithRelativeJump))
        {
            if (targetStartsWithRelativeJump && targets.codeGateway == 0)
            {
                std::ostringstream message;
                message << label << " code detour skipped because the original target already "
                        << "starts with a relative jump at "
                        << FormatAddress(targets.vtableOriginal);
                Log(message.str());
            }
            return;
        }

        void* gateway = nullptr;
        if (!InstallRelativeJumpDetour(reinterpret_cast<void*>(targets.vtableOriginal),
                                       overwriteLength,
                                       reinterpret_cast<void*>(replacementAddress), &gateway))
        {
            std::ostringstream message;
            message << "Failed to install " << label << " code detour at "
                    << FormatAddress(targets.vtableOriginal);
            Log(message.str());
            return;
        }

        *codeGateway = gateway;
        *gatewayAddress = reinterpret_cast<std::uintptr_t>(gateway);
        installedAnyHook = true;

        std::ostringstream message;
        message << label << " code detour installed: target="
                << FormatAddress(targets.vtableOriginal)
                << ", gateway=" << FormatAddress(*gatewayAddress);
        Log(message.str());
    };

    installHook("IDirect3DDevice9::Present", kPresentDetourLength,
                reinterpret_cast<std::uintptr_t>(&HookedPresent),
                RenderHookTargets{
                    .vtableOriginal = reinterpret_cast<std::uintptr_t>(g_originalPresent),
                    .codeGateway =
                        reinterpret_cast<std::uintptr_t>(g_originalPresentCodeGateway),
                },
                reinterpret_cast<void**>(&g_originalPresentCodeGateway),
                &g_lastPresentCodeGatewayAddress);

    installHook("IDirect3DSwapChain9::Present", kSwapChainPresentDetourLength,
                reinterpret_cast<std::uintptr_t>(&HookedSwapChainPresent),
                RenderHookTargets{
                    .vtableOriginal =
                        reinterpret_cast<std::uintptr_t>(g_originalSwapChainPresent),
                    .codeGateway = reinterpret_cast<std::uintptr_t>(
                        g_originalSwapChainPresentCodeGateway),
                },
                reinterpret_cast<void**>(&g_originalSwapChainPresentCodeGateway),
                &g_lastSwapChainPresentCodeGatewayAddress);

    installHook("IDirect3DDevice9::EndScene", kEndSceneDetourLength,
                reinterpret_cast<std::uintptr_t>(&HookedEndScene),
                RenderHookTargets{
                    .vtableOriginal = reinterpret_cast<std::uintptr_t>(g_originalEndScene),
                    .codeGateway =
                        reinterpret_cast<std::uintptr_t>(g_originalEndSceneCodeGateway),
                },
                reinterpret_cast<void**>(&g_originalEndSceneCodeGateway),
                &g_lastEndSceneCodeGatewayAddress);

    return installedAnyHook;
}

bool InstallDeviceExHooks(IDirect3DDevice9* device)
{
    if (device == nullptr)
    {
        return false;
    }

    IDirect3DDevice9Ex* deviceEx = nullptr;
    if (FAILED(device->QueryInterface(IID_IDirect3DDevice9Ex,
                                      reinterpret_cast<void**>(&deviceEx))) ||
        deviceEx == nullptr ||
        !HasExecutableVtableEntries(deviceEx, {kPresentExVtableIndex, kResetExVtableIndex}))
    {
        ReleaseNoexcept(deviceEx);
        return false;
    }

    bool installedAnyHook = false;
    if (PatchVtableEntry(deviceEx, kPresentExVtableIndex,
                         reinterpret_cast<void*>(&HookedPresentEx),
                         reinterpret_cast<void**>(&g_originalPresentEx)))
    {
        g_lastOriginalPresentExAddress = reinterpret_cast<std::uintptr_t>(g_originalPresentEx);
        const std::uintptr_t deviceAddress =
            g_lastDeviceAddress != 0 ? g_lastDeviceAddress
                                     : reinterpret_cast<std::uintptr_t>(deviceEx);
        std::ostringstream message;
        message << "IDirect3DDevice9Ex::PresentEx hook installed at "
                << FormatAddress(deviceAddress)
                << ", original=" << FormatAddress(g_lastOriginalPresentExAddress);
        Log(message.str());
        installedAnyHook = true;
    }

    if (PatchVtableEntry(deviceEx, kResetExVtableIndex,
                         reinterpret_cast<void*>(&HookedResetEx),
                         reinterpret_cast<void**>(&g_originalResetEx)))
    {
        Log("IDirect3DDevice9Ex::ResetEx hook installed.");
        installedAnyHook = true;
    }

    ReleaseNoexcept(deviceEx);
    return installedAnyHook;
}

PresentParams ToPresentParams(const D3DPRESENT_PARAMETERS& params)
{
    return {
        params.BackBufferWidth,
        params.BackBufferHeight,
        params.Windowed != FALSE,
        params.FullScreen_RefreshRateInHz,
        static_cast<std::uint32_t>(params.SwapEffect),
        params.PresentationInterval,
    };
}

void ApplyPresentParams(const PresentParams& source, D3DPRESENT_PARAMETERS& destination)
{
    destination.BackBufferWidth = source.backBufferWidth;
    destination.BackBufferHeight = source.backBufferHeight;
    destination.Windowed = source.windowed ? TRUE : FALSE;
    destination.FullScreen_RefreshRateInHz = source.refreshRate;
    destination.SwapEffect = static_cast<D3DSWAPEFFECT>(source.swapEffect);
    destination.PresentationInterval = source.presentationInterval;
}

bool SamePresentParams(const PresentParams& lhs, const PresentParams& rhs)
{
    return lhs.backBufferWidth == rhs.backBufferWidth &&
           lhs.backBufferHeight == rhs.backBufferHeight && lhs.windowed == rhs.windowed &&
           lhs.refreshRate == rhs.refreshRate && lhs.swapEffect == rhs.swapEffect &&
           lhs.presentationInterval == rhs.presentationInterval;
}

std::string FormatSwapEffect(std::uint32_t swapEffect)
{
    switch (swapEffect)
    {
    case D3DSWAPEFFECT_DISCARD:
        return "discard";
    case D3DSWAPEFFECT_FLIP:
        return "flip";
    case D3DSWAPEFFECT_COPY:
        return "copy";
    case D3DSWAPEFFECT_OVERLAY:
        return "overlay";
    case D3DSWAPEFFECT_FLIPEX:
        return "flipex";
    default:
        return std::to_string(swapEffect);
    }
}

bool IsLostDeviceResult(HRESULT hr)
{
    return hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET ||
           hr == D3DERR_DRIVERINTERNALERROR || hr == D3DERR_DEVICEHUNG ||
           hr == D3DERR_DEVICEREMOVED;
}

std::string FormatPresentationInterval(std::uint32_t interval)
{
    switch (interval)
    {
    case D3DPRESENT_INTERVAL_DEFAULT:
        return "default";
    case D3DPRESENT_INTERVAL_ONE:
        return "one";
    case D3DPRESENT_INTERVAL_TWO:
        return "two";
    case D3DPRESENT_INTERVAL_THREE:
        return "three";
    case D3DPRESENT_INTERVAL_FOUR:
        return "four";
    case D3DPRESENT_INTERVAL_IMMEDIATE:
        return "immediate";
    default:
        return std::to_string(interval);
    }
}

void UpdateBackbufferDimensions(const PresentParams& params)
{
    g_backBufferWidth.store(params.backBufferWidth);
    g_backBufferHeight.store(params.backBufferHeight);
}

bool InitializeHighPrecisionTimingState()
{
    LARGE_INTEGER frequency = {};
    LARGE_INTEGER startCounter = {};
    if (QueryPerformanceFrequency(&frequency) == FALSE || frequency.QuadPart <= 0 ||
        QueryPerformanceCounter(&startCounter) == FALSE)
    {
        return false;
    }

    g_highPrecisionTimerState.frequency = static_cast<std::uint64_t>(frequency.QuadPart);
    g_highPrecisionTimerState.startCounter = static_cast<std::uint64_t>(startCounter.QuadPart);
    g_highPrecisionTimerState.baseTickCount = ::GetTickCount();
    return true;
}

void ApplyHighResolutionTimerRequest()
{
    if (!g_config.requestHighResolutionTimer)
    {
        return;
    }

    HMODULE winmmModule = LoadLibraryW(L"winmm.dll");
    if (winmmModule == nullptr)
    {
        Log("winmm.dll was unavailable for timer resolution request.");
        return;
    }

    g_timeBeginPeriod =
        reinterpret_cast<TimePeriodFn>(GetProcAddress(winmmModule, "timeBeginPeriod"));
    g_timeEndPeriod = reinterpret_cast<TimePeriodFn>(GetProcAddress(winmmModule, "timeEndPeriod"));
    if (g_timeBeginPeriod == nullptr || g_timeEndPeriod == nullptr)
    {
        Log("timeBeginPeriod/timeEndPeriod exports were unavailable.");
        return;
    }

    const UINT result = g_timeBeginPeriod(1);
    if (result == 0)
    {
        g_requestedHighResolutionTimer = true;
        Log("Requested 1ms timer resolution.");
        return;
    }

    std::ostringstream message;
    message << "timeBeginPeriod(1) failed with result " << result;
    Log(message.str());
}

void InitializeExecutionPolicyApis()
{
    std::call_once(g_executionPolicyApiOnce, []() {
        HMODULE kernel32Module = GetModuleHandleW(L"kernel32.dll");
        if (kernel32Module == nullptr)
        {
            kernel32Module = LoadLibraryW(L"kernel32.dll");
        }

        if (kernel32Module != nullptr)
        {
            g_setProcessInformation = reinterpret_cast<SetProcessInformationFn>(
                GetProcAddress(kernel32Module, "SetProcessInformation"));
            g_setThreadInformation = reinterpret_cast<SetThreadInformationFn>(
                GetProcAddress(kernel32Module, "SetThreadInformation"));
        }

        HMODULE avrtModule = GetModuleHandleW(L"avrt.dll");
        if (avrtModule == nullptr)
        {
            avrtModule = LoadLibraryW(L"avrt.dll");
        }

        if (avrtModule != nullptr)
        {
            g_avSetMmThreadCharacteristicsW =
                reinterpret_cast<AvSetMmThreadCharacteristicsWFn>(
                    GetProcAddress(avrtModule, "AvSetMmThreadCharacteristicsW"));
            g_avSetMmThreadPriority = reinterpret_cast<AvSetMmThreadPriorityFn>(
                GetProcAddress(avrtModule, "AvSetMmThreadPriority"));
            g_avRevertMmThreadCharacteristics =
                reinterpret_cast<AvRevertMmThreadCharacteristicsFn>(
                    GetProcAddress(avrtModule, "AvRevertMmThreadCharacteristics"));
        }
    });
}

void ApplyProcessExecutionPolicy()
{
    const ModernExecutionPlan plan = BuildModernExecutionPlan(
        g_config.disablePowerThrottling, g_config.enableGamesMmcssProfile);
    if (!plan.applyProcessPowerThrottling)
    {
        return;
    }

    InitializeExecutionPolicyApis();
    if (g_setProcessInformation == nullptr)
    {
        Log("Process power throttling control unavailable on this Windows build.");
        return;
    }

    PROCESS_POWER_THROTTLING_STATE state = {};
    state.Version = plan.processVersion;
    state.ControlMask = plan.processControlMask;
    state.StateMask = plan.processStateMask;

    if (g_setProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &state,
                                sizeof(state)) != FALSE)
    {
        Log("Process power throttling disabled.");
        return;
    }

    std::ostringstream message;
    message << "Failed to disable process power throttling, error=" << GetLastError();
    Log(message.str());
}

void ApplyCurrentRenderThreadExecutionPolicy()
{
    std::call_once(g_renderThreadExecutionPolicyOnce, []() {
        const ModernExecutionPlan plan = BuildModernExecutionPlan(
            g_config.disablePowerThrottling, g_config.enableGamesMmcssProfile);
        InitializeExecutionPolicyApis();

        if (plan.applyThreadPowerThrottling)
        {
            if (g_setThreadInformation == nullptr)
            {
                Log("Render thread power throttling control unavailable on this Windows build.");
            }
            else
            {
                THREAD_POWER_THROTTLING_STATE state = {};
                state.Version = plan.threadVersion;
                state.ControlMask = plan.threadControlMask;
                state.StateMask = plan.threadStateMask;

                if (g_setThreadInformation(GetCurrentThread(), ThreadPowerThrottling, &state,
                                           sizeof(state)) != FALSE)
                {
                    Log("Render thread power throttling disabled.");
                }
                else
                {
                    std::ostringstream message;
                    message << "Failed to disable render thread power throttling, error="
                            << GetLastError();
                    Log(message.str());
                }
            }
        }

        if (!plan.applyGamesMmcssProfile)
        {
            return;
        }

        if (g_avSetMmThreadCharacteristicsW == nullptr)
        {
            Log("MMCSS Games profile unavailable on this Windows build.");
            return;
        }

        DWORD taskIndex = 0;
        HANDLE taskHandle = g_avSetMmThreadCharacteristicsW(L"Games", &taskIndex);
        if (taskHandle == nullptr)
        {
            std::ostringstream message;
            message << "Failed to register render thread with MMCSS Games profile, error="
                    << GetLastError();
            Log(message.str());
            return;
        }

        g_renderThreadMmcssHandle = taskHandle;
        g_renderThreadMmcssTaskIndex = taskIndex;

        if (g_avSetMmThreadPriority != nullptr &&
            g_avSetMmThreadPriority(taskHandle, AVRT_PRIORITY_HIGH) == FALSE)
        {
            std::ostringstream message;
            message << "MMCSS Games profile applied, but priority raise failed with error="
                    << GetLastError();
            Log(message.str());
            return;
        }

        std::ostringstream message;
        message << "Render thread registered with MMCSS Games profile (taskIndex="
                << taskIndex << ").";
        Log(message.str());
    });
}

CrashDumpTimestamp ToCrashDumpTimestamp(const SYSTEMTIME& time)
{
    return CrashDumpTimestamp{
        .year = time.wYear,
        .month = time.wMonth,
        .day = time.wDay,
        .hour = time.wHour,
        .minute = time.wMinute,
        .second = time.wSecond,
        .milliseconds = time.wMilliseconds,
    };
}

bool WriteCrashDump(EXCEPTION_POINTERS* exceptionPointers)
{
    if (!g_config.enableCrashDumps || g_moduleDirectory.empty() ||
        InterlockedCompareExchange(&g_crashDumpWriteState, 1, 0) != 0)
    {
        return false;
    }

    SYSTEMTIME localTime = {};
    GetLocalTime(&localTime);
    const std::filesystem::path dumpPath = BuildCrashDumpPath(
        g_moduleDirectory, "Silent Hill Homecoming", ToCrashDumpTimestamp(localTime),
        GetCurrentProcessId());

    HANDLE dumpFile =
        CreateFileW(dumpPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dumpFile == INVALID_HANDLE_VALUE)
    {
        InterlockedExchange(&g_crashDumpWriteState, 0);
        return false;
    }

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = {};
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = exceptionPointers;
    exceptionInfo.ClientPointers = FALSE;

    const MINIDUMP_TYPE dumpType =
        static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo);
    const BOOL wroteDump =
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, dumpType,
                          exceptionPointers != nullptr ? &exceptionInfo : nullptr, nullptr,
                          nullptr);
    CloseHandle(dumpFile);

    if (wroteDump == FALSE)
    {
        DeleteFileW(dumpPath.c_str());
    }

    InterlockedExchange(&g_crashDumpWriteState, 0);
    return wroteDump != FALSE;
}

void SpinWaitForMilliseconds(float milliseconds)
{
    if (milliseconds <= 0.0f || g_highPrecisionTimerState.frequency == 0)
    {
        return;
    }

    LARGE_INTEGER startCounter = {};
    if (QueryPerformanceCounter(&startCounter) == FALSE)
    {
        return;
    }

    const std::uint64_t targetCounter =
        static_cast<std::uint64_t>(startCounter.QuadPart) +
        static_cast<std::uint64_t>(
            static_cast<double>(milliseconds) *
            static_cast<double>(g_highPrecisionTimerState.frequency) / 1000.0);

    LARGE_INTEGER currentCounter = {};
    while (QueryPerformanceCounter(&currentCounter) != FALSE &&
           static_cast<std::uint64_t>(currentCounter.QuadPart) < targetCounter)
    {
    }
}

DWORD GetLostDeviceNowTick()
{
    return g_originalGetTickCount != nullptr ? g_originalGetTickCount() : ::GetTickCount();
}

void ArmLostDeviceRecoveryWindow(const char* reason, DWORD durationMs, bool markDeviceLoss)
{
    const DWORD nowTick = GetLostDeviceNowTick();

    std::scoped_lock lock(g_lostDeviceMutex);
    NoteLostDeviceRecoveryWindow(&g_lostDeviceRecoveryState, nowTick, durationMs,
                                 markDeviceLoss);

    std::ostringstream message;
    message << "Lost-device recovery armed: reason=" << reason
            << ", markLoss=" << (markDeviceLoss ? 1 : 0)
            << ", windowMs=" << durationMs;
    Log(message.str());
}

void NoteUserInitiatedWindowClose()
{
    std::scoped_lock lock(g_lostDeviceMutex);
    NoteUserInitiatedClose(&g_lostDeviceRecoveryState);
    Log("Lost-device recovery disabled for user-initiated close.");
}

void ClearLostDeviceRecoveryWindow(const char* reason)
{
    std::scoped_lock lock(g_lostDeviceMutex);
    if (!g_lostDeviceRecoveryState.deviceLossObserved &&
        g_lostDeviceRecoveryState.recoveryDeadlineTick == 0)
    {
        return;
    }

    ClearLostDeviceRecovery(&g_lostDeviceRecoveryState);
    std::ostringstream message;
    message << "Lost-device recovery cleared: reason=" << reason;
    Log(message.str());
}

bool ShouldSuppressLostDeviceShutdownNow()
{
    const DWORD nowTick = GetLostDeviceNowTick();
    std::scoped_lock lock(g_lostDeviceMutex);
    return ShouldSuppressLostDeviceShutdown(g_lostDeviceRecoveryState, nowTick);
}

void NoteLostDeviceSignalFromApi(const char* apiName, HRESULT hr)
{
    if (!IsLostDeviceResult(hr))
    {
        return;
    }

    g_lostDeviceSignalCount.fetch_add(1, std::memory_order_relaxed);
    ArmLostDeviceRecoveryWindow(apiName, kLostDeviceDeactivateWindowMs, true);
    std::ostringstream message;
    message << "Lost-device signal: " << apiName << " -> 0x" << std::hex
            << static_cast<std::uint32_t>(hr) << std::dec;
    Log(message.str());
}

LRESULT CALLBACK HookedRawInputWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INPUT)
    {
        UINT size = 0;
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size,
                            sizeof(RAWINPUTHEADER)) == 0 &&
            size > 0 && size <= 256)
        {
            std::uint8_t buffer[256] = {};
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer, &size,
                                sizeof(RAWINPUTHEADER)) == size)
            {
                const auto* rawInput = reinterpret_cast<const RAWINPUT*>(buffer);
                if (rawInput->header.dwType == RIM_TYPEMOUSE &&
                    (rawInput->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
                {
                    g_rawInputPacketCount.fetch_add(1, std::memory_order_relaxed);
                    g_rawMouseDeltaX.fetch_add(rawInput->data.mouse.lLastX);
                    g_rawMouseDeltaY.fetch_add(rawInput->data.mouse.lLastY);
                }
            }
        }
    }
    else if (message == WM_ACTIVATEAPP)
    {
        if (wParam == FALSE)
        {
            ArmLostDeviceRecoveryWindow("window_deactivate", kLostDeviceDeactivateWindowMs,
                                        false);
        }
        else
        {
            ArmLostDeviceRecoveryWindow("window_activate", kLostDeviceRestoreWindowMs, false);
        }
    }
    else if (message == WM_SIZE)
    {
        if (wParam == SIZE_MINIMIZED)
        {
            ArmLostDeviceRecoveryWindow("window_minimize", kLostDeviceDeactivateWindowMs,
                                        false);
        }
        else if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
        {
            ArmLostDeviceRecoveryWindow("window_restore", kLostDeviceRestoreWindowMs, false);
        }
    }
    else if (message == WM_CLOSE || message == WM_DESTROY || message == WM_QUIT)
    {
        NoteUserInitiatedWindowClose();
    }

    return g_originalRawInputWndProc != nullptr
               ? CallWindowProcW(g_originalRawInputWndProc, window, message, wParam, lParam)
               : DefWindowProcW(window, message, wParam, lParam);
}

bool EnsureGameWindowHook(HWND window)
{
    if (window == nullptr)
    {
        return false;
    }

    if (g_gameWindow == window && g_originalRawInputWndProc != nullptr)
    {
        return true;
    }

    SetLastError(0);
    const auto previousWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(window, GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(&HookedRawInputWndProc)));
    if (previousWndProc == nullptr && GetLastError() != 0)
    {
        Log("Failed to install game window procedure.");
        return false;
    }

    g_gameWindow = window;
    g_originalRawInputWndProc = previousWndProc;
    Log("Game window hook installed.");
    return true;
}

bool EnsureRawInputWindow(HWND window)
{
    if (!g_config.enableRawMouseInput || window == nullptr)
    {
        return false;
    }

    if (!EnsureGameWindowHook(window))
    {
        return false;
    }

    if (g_rawInputWindow == window)
    {
        return true;
    }

    RAWINPUTDEVICE device = {};
    device.usUsagePage = 0x01;
    device.usUsage = 0x02;
    device.hwndTarget = window;
    if (RegisterRawInputDevices(&device, 1, sizeof(device)) == FALSE)
    {
        Log("RegisterRawInputDevices failed.");
        return false;
    }

    g_rawInputWindow = window;
    Log("Raw mouse input window hook installed.");
    return true;
}

bool IsMouseGuid(REFGUID guid)
{
    return InlineIsEqualGUID(guid, GUID_SysMouse);
}

bool IsMouseDevice(void* device)
{
    return device != nullptr && (device == g_mouseDeviceA || device == g_mouseDeviceW);
}

bool HasDirectInputMouseStateBuffer(DWORD stateSize, LPVOID state)
{
    return state != nullptr &&
           (stateSize == sizeof(DIMOUSESTATE) || stateSize == sizeof(DIMOUSESTATE2));
}

bool InstallDirectInputMouseHooksA(LPDIRECTINPUTDEVICE8A device)
{
    if (device == nullptr)
    {
        return false;
    }

    g_mouseDeviceA = device;
    bool installedAnyHook = false;
    if (PatchVtableEntry(device, kDirectInputGetDeviceStateVtableIndex,
                         reinterpret_cast<void*>(&HookedDirectInputGetDeviceStateA),
                         reinterpret_cast<void**>(&g_originalDirectInputGetDeviceStateA)))
    {
        installedAnyHook = true;
    }
    if (PatchVtableEntry(device, kDirectInputSetCooperativeLevelVtableIndex,
                         reinterpret_cast<void*>(&HookedDirectInputSetCooperativeLevelA),
                         reinterpret_cast<void**>(&g_originalDirectInputSetCooperativeLevelA)))
    {
        installedAnyHook = true;
    }

    if (installedAnyHook)
    {
        Log("DirectInput mouse hooks installed (ANSI).");
    }
    return installedAnyHook;
}

bool InstallDirectInputMouseHooksW(LPDIRECTINPUTDEVICE8W device)
{
    if (device == nullptr)
    {
        return false;
    }

    g_mouseDeviceW = device;
    bool installedAnyHook = false;
    if (PatchVtableEntry(device, kDirectInputGetDeviceStateVtableIndex,
                         reinterpret_cast<void*>(&HookedDirectInputGetDeviceStateW),
                         reinterpret_cast<void**>(&g_originalDirectInputGetDeviceStateW)))
    {
        installedAnyHook = true;
    }
    if (PatchVtableEntry(device, kDirectInputSetCooperativeLevelVtableIndex,
                         reinterpret_cast<void*>(&HookedDirectInputSetCooperativeLevelW),
                         reinterpret_cast<void**>(&g_originalDirectInputSetCooperativeLevelW)))
    {
        installedAnyHook = true;
    }

    if (installedAnyHook)
    {
        Log("DirectInput mouse hooks installed (Unicode).");
    }
    return installedAnyHook;
}

ProjectionMatrix ToProjectionMatrix(const D3DMATRIX& matrix)
{
    return {
        matrix._11, matrix._12, matrix._13, matrix._14, matrix._21, matrix._22, matrix._23,
        matrix._24, matrix._31, matrix._32, matrix._33, matrix._34, matrix._41, matrix._42,
        matrix._43, matrix._44,
    };
}

D3DMATRIX ToD3DMatrix(const ProjectionMatrix& matrix)
{
    D3DMATRIX result = {};
    result._11 = matrix.m11;
    result._12 = matrix.m12;
    result._13 = matrix.m13;
    result._14 = matrix.m14;
    result._21 = matrix.m21;
    result._22 = matrix.m22;
    result._23 = matrix.m23;
    result._24 = matrix.m24;
    result._31 = matrix.m31;
    result._32 = matrix.m32;
    result._33 = matrix.m33;
    result._34 = matrix.m34;
    result._41 = matrix.m41;
    result._42 = matrix.m42;
    result._43 = matrix.m43;
    result._44 = matrix.m44;
    return result;
}

std::filesystem::path ResolveModuleDirectory(HMODULE module)
{
    wchar_t buffer[MAX_PATH] = {};
    if (GetModuleFileNameW(module, buffer, static_cast<DWORD>(std::size(buffer))) == 0)
    {
        return {};
    }

    return std::filesystem::path(buffer).parent_path();
}

std::string FormatHr(HRESULT hr)
{
    char buffer[16] = {};
    std::snprintf(buffer, sizeof(buffer), "0x%08lX",
                  static_cast<unsigned long>(static_cast<std::uint32_t>(hr)));
    return buffer;
}

bool LogCallerOnce(std::unordered_set<std::uintptr_t>& callers, std::uintptr_t caller)
{
    std::scoped_lock lock(g_timingDiagnosticsMutex);
    return callers.insert(caller).second;
}

bool LogLegacyThreadWrapperPayloadOnce(std::uint64_t payloadKey)
{
    std::scoped_lock lock(g_timingDiagnosticsMutex);
    return g_loggedLegacyThreadWrapperPayloads.insert(payloadKey).second;
}

std::string FormatAddress(std::uintptr_t address)
{
    char buffer[24] = {};
    std::snprintf(buffer, sizeof(buffer), "0x%08llX",
                  static_cast<unsigned long long>(address));
    return buffer;
}

std::optional<LegacyThreadWrapperSnapshot> TryReadLegacyThreadWrapperSnapshot(void* parameter)
{
    if (parameter == nullptr)
    {
        return std::nullopt;
    }

    LegacyThreadWrapperSnapshot snapshot = {};
    if (!TryReadPointerValue(parameter, snapshot.threadHandleValue))
    {
        return std::nullopt;
    }

    const auto* bytes = static_cast<const std::uint8_t*>(parameter);
    std::uintptr_t value = 0;
    if (TryReadPointerValue(bytes + 0x8, value))
    {
        snapshot.entryPoint = value;
    }
    if (TryReadPointerValue(bytes + 0xC, value))
    {
        snapshot.userParameter = value;
    }
    if (TryReadPointerValue(bytes + 0x10, value))
    {
        snapshot.tag = value;
    }

    std::uint8_t stateByte = 0;
    if (TryReadByteValue(bytes + 0x14, stateByte))
    {
        snapshot.stateByte = stateByte;
    }

    return snapshot;
}

std::optional<LegacyThreadWrapperObjectGraphSnapshot> TryReadLegacyThreadWrapperObjectGraphSnapshot(
    std::uintptr_t rootObject)
{
    if (rootObject == 0)
    {
        return std::nullopt;
    }

    LegacyThreadWrapperObjectGraphSnapshot snapshot = {};
    snapshot.rootObject = rootObject;
    if (!TryReadPointerValue(reinterpret_cast<const void*>(rootObject), snapshot.rootVtable))
    {
        return std::nullopt;
    }

    std::uintptr_t object1C = 0;
    if (TryReadPointerValue(reinterpret_cast<const std::uint8_t*>(rootObject) + 0x1C, object1C))
    {
        snapshot.object1C = object1C;
        if (object1C != 0)
        {
            TryReadPointerValue(reinterpret_cast<const void*>(object1C), snapshot.object1CVtable);

            std::uintptr_t nestedB4 = 0;
            if (TryReadPointerValue(reinterpret_cast<const std::uint8_t*>(object1C) + 0xB4,
                                    nestedB4))
            {
                snapshot.nestedB4 = nestedB4;
                if (nestedB4 != 0)
                {
                    TryReadPointerValue(reinterpret_cast<const void*>(nestedB4),
                                        snapshot.nestedB4Vtable);
                }
            }
        }
    }

    std::uintptr_t object58 = 0;
    if (TryReadPointerValue(reinterpret_cast<const std::uint8_t*>(rootObject) + 0x58, object58))
    {
        snapshot.object58 = object58;
        if (object58 != 0)
        {
            TryReadPointerValue(reinterpret_cast<const void*>(object58), snapshot.object58Vtable);
        }
    }

    return snapshot;
}

std::optional<std::string> TryReadLegacyThreadTagText(std::uintptr_t tagAddress)
{
    if (tagAddress == 0)
    {
        return std::nullopt;
    }

    std::string text;
    text.reserve(48);
    for (std::size_t index = 0; index < 64; ++index)
    {
        std::uint8_t value = 0;
        if (!TryReadByteValue(reinterpret_cast<const std::uint8_t*>(tagAddress) + index, value))
        {
            break;
        }

        if (value == 0)
        {
            break;
        }

        if (value < 0x20 || value > 0x7E)
        {
            return std::nullopt;
        }

        text.push_back(static_cast<char>(value));
    }

    if (text.empty())
    {
        return std::nullopt;
    }

    return text;
}

void TrackLegacyWaitableTimerWorkerHandle(HANDLE timer,
                                          const std::optional<std::uint32_t>& dueTimeMs,
                                          LONG period, std::uintptr_t caller)
{
    if (!g_config.reduceLegacyWaitableTimerPolling || timer == nullptr)
    {
        return;
    }

    HANDLE duplicatedHandle = nullptr;
    if (DuplicateHandle(GetCurrentProcess(), timer, GetCurrentProcess(), &duplicatedHandle, 0,
                        FALSE, DUPLICATE_SAME_ACCESS) == FALSE ||
        duplicatedHandle == nullptr)
    {
        return;
    }

    HANDLE expectedHandle = nullptr;
    if (!g_legacyWaitableTimerWorkerHandle.compare_exchange_strong(expectedHandle,
                                                                   duplicatedHandle))
    {
        CloseHandle(duplicatedHandle);
        return;
    }

    std::call_once(g_legacyWaitableTimerWorkerCaptureLogOnce, [dueTimeMs, period, caller]() {
        std::ostringstream message;
        message << "Legacy waitable timer worker handle tracked: dueTimeMs=";
        if (dueTimeMs.has_value())
        {
            message << *dueTimeMs;
        }
        else
        {
            message << "absolute";
        }
        message << ", period=" << period << ", caller=" << FormatAddress(caller);
        Log(message.str());
    });
}

void LogStdAudioWorkerTracking(std::uint32_t requestedSleepMs, bool usingTrackedTimer)
{
    std::call_once(g_stdAudioWorkerTrackingLogOnce, [requestedSleepMs, usingTrackedTimer]() {
        std::ostringstream message;
        message << "StdAudio worker cadence tracking active: threadId="
                << g_stdAudioThreadId.load(std::memory_order_relaxed)
                << ", rootObject="
                << FormatAddress(g_stdAudioRootObject.load(std::memory_order_relaxed))
                << ", routerVtable="
                << FormatAddress(g_stdAudioRouterVtable.load(std::memory_order_relaxed))
                << ", requestedSleepMs=" << requestedSleepMs
                << ", trackedTimer=" << usingTrackedTimer;
        Log(message.str());
    });
}

void CloseLegacyWaitableTimerWorkerHandle()
{
    const HANDLE trackedHandle =
        g_legacyWaitableTimerWorkerHandle.exchange(nullptr, std::memory_order_acq_rel);
    if (trackedHandle != nullptr)
    {
        CloseHandle(trackedHandle);
    }
}

std::optional<bool> TryIsThreadHandleSignaled(HANDLE threadHandle)
{
    if (threadHandle == nullptr)
    {
        return std::nullopt;
    }

    const DWORD waitResult = WaitForSingleObject(threadHandle, 0);
    if (waitResult == WAIT_OBJECT_0)
    {
        return true;
    }

    if (waitResult == WAIT_TIMEOUT)
    {
        return false;
    }

    return std::nullopt;
}

bool TargetsCurrentThread(HANDLE threadHandle)
{
    if (threadHandle == nullptr)
    {
        return false;
    }

    const DWORD targetThreadId = GetThreadId(threadHandle);
    return targetThreadId != 0 && targetThreadId == GetCurrentThreadId();
}

void Log(std::string_view message)
{
    (void)message;
}

Config LoadConfigFromDisk()
{
    const std::filesystem::path configPath = g_moduleDirectory / "ShepherdPatch.ini";
    const std::filesystem::path legacyConfigPath = g_moduleDirectory / "SHHModernizer.ini";

    const std::filesystem::path pathToLoad =
        std::filesystem::exists(configPath) ? configPath : legacyConfigPath;
    if (!std::filesystem::exists(pathToLoad))
    {
        return {};
    }

    std::ifstream stream(pathToLoad, std::ios::binary);
    if (!stream.is_open())
    {
        return {};
    }

    std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return ParseConfig(text);
}

std::optional<std::string> ReadTextFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open())
    {
        return std::nullopt;
    }

    return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

bool WriteTextFile(const std::filesystem::path& path, std::string_view text)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open())
    {
        return false;
    }

    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    return stream.good();
}

void ApplyConfiguredEngineVarsOverrides()
{
    if (!g_config.synchronizeEngineVars)
    {
        return;
    }

    const std::filesystem::path varsConfig =
        g_moduleDirectory.parent_path() / "Engine" / "vars_pc.cfg";
    const std::optional<std::string> originalText = ReadTextFile(varsConfig);
    if (!originalText.has_value())
    {
        Log("Engine vars override skipped: vars_pc.cfg was unavailable.");
        return;
    }

    EngineVarsOverrides overrides = {};
    overrides.syncFullscreenToBorderless = g_config.forceBorderless;
    if (g_config.fallbackRefreshRate > 0)
    {
        overrides.screenRefresh = g_config.fallbackRefreshRate;
    }

    const std::string updatedText = ApplyEngineVarsOverrides(*originalText, overrides);
    if (updatedText == *originalText)
    {
        Log("Engine vars already matched ShepherdPatch overrides.");
        return;
    }

    if (!WriteTextFile(varsConfig, updatedText))
    {
        Log("Engine vars override failed: could not write vars_pc.cfg.");
        return;
    }

    std::ostringstream message;
    message << "Engine vars synchronized: FullScreen="
            << (g_config.forceBorderless ? "false" : "unchanged")
            ;
    if (g_config.fallbackRefreshRate > 0)
    {
        message << ", screenRefresh=" << g_config.fallbackRefreshRate;
    }
    Log(message.str());
}

std::optional<std::string> ReadEngineConfigValue(const std::filesystem::path& path,
                                                 std::string_view key)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open())
    {
        return std::nullopt;
    }

    std::string line;
    const std::string prefix(key);
    while (std::getline(stream, line))
    {
        if (line.starts_with(prefix))
        {
            const std::size_t separator = line.find('=');
            if (separator != std::string::npos)
            {
                return line.substr(separator + 1);
            }
        }
    }

    return std::nullopt;
}

void LogEngineConfigSnapshot()
{
    const std::filesystem::path gameRoot = g_moduleDirectory.parent_path();
    const std::filesystem::path defaultConfig = gameRoot / "Engine" / "default_pc.cfg";
    const std::filesystem::path varsConfig = gameRoot / "Engine" / "vars_pc.cfg";

    std::ostringstream message;
    message << "Engine cfg snapshot:";

    const struct
    {
        const std::filesystem::path* path;
        std::string_view key;
        std::string_view label;
    } entries[] = {
        {&defaultConfig, "enableFinalFPS", " enableFinalFPS"},
        {&defaultConfig, "vsync", " vsync"},
        {&defaultConfig, "vsyncInterval", " vsyncInterval"},
        {&varsConfig, "fpsLimit", " fpsLimit"},
        {&varsConfig, "maxFPSLimit", " maxFPSLimit"},
        {&varsConfig, "ScreenRefresh", " screenRefresh"},
        {&varsConfig, "FullScreen", " fullScreen"},
        {&varsConfig, "precisionmode", " precisionMode"},
        {&varsConfig, "timermaxcount", " timerMaxCount"},
        {&varsConfig, "bumppollrate", " bumpPollRate"},
    };

    for (const auto& entry : entries)
    {
        const std::optional<std::string> value = ReadEngineConfigValue(*entry.path, entry.key);
        if (value.has_value())
        {
            message << entry.label << '=' << *value;
        }
    }

    Log(message.str());
}

void LogShvConfigSnapshot()
{
    const std::filesystem::path shvConfig = g_moduleDirectory / "shv.cfg";
    std::ostringstream message;
    message << "shv cfg snapshot:";

    const struct
    {
        std::string_view key;
        std::string_view label;
    } entries[] = {
        {"GraphicsMode", " graphicsMode"},
        {"LoadingScreenSpeed", " loadingScreenSpeed"},
        {"WinXPCompat", " winXpCompat"},
        {"ErrorMode", " errorMode"},
        {"OverrideNumberOfControllers", " overrideControllers"},
        {"OverrideButtonPrompts", " overridePrompts"},
    };

    bool foundAnyValue = false;
    for (const auto& entry : entries)
    {
        const std::optional<std::string> value = ReadEngineConfigValue(shvConfig, entry.key);
        if (value.has_value())
        {
            message << entry.label << '=' << *value;
            foundAnyValue = true;
        }
    }

    if (foundAnyValue)
    {
        Log(message.str());
    }
}

void ApplyDpiAwareness()
{
    if (!g_config.enableDpiAwareness)
    {
        return;
    }

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32 == nullptr)
    {
        user32 = LoadLibraryW(L"user32.dll");
    }

    if (user32 != nullptr)
    {
        const auto setDpiAwarenessContext =
            reinterpret_cast<SetProcessDpiAwarenessContextFn>(
                GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setDpiAwarenessContext != nullptr &&
            setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE)
        {
            Log("Per-monitor DPI awareness enabled.");
            return;
        }

        const auto setProcessDPIAware =
            reinterpret_cast<SetProcessDPIAwareFn>(GetProcAddress(user32, "SetProcessDPIAware"));
        if (setProcessDPIAware != nullptr && setProcessDPIAware() != FALSE)
        {
            Log("System DPI awareness enabled.");
        }
    }
}

bool GetMonitorRectForWindow(HWND window, Rect* monitorRect)
{
    if (window == nullptr || monitorRect == nullptr)
    {
        return false;
    }

    MONITORINFO info = {};
    info.cbSize = sizeof(info);
    const HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    if (monitor == nullptr || GetMonitorInfoW(monitor, &info) == FALSE)
    {
        return false;
    }

    monitorRect->left = info.rcMonitor.left;
    monitorRect->top = info.rcMonitor.top;
    monitorRect->right = info.rcMonitor.right;
    monitorRect->bottom = info.rcMonitor.bottom;
    return true;
}

void FillMissingBorderlessDimensions(HWND window, D3DPRESENT_PARAMETERS& params)
{
    if (!g_config.forceBorderless)
    {
        return;
    }

    Rect monitorRect;
    if (!GetMonitorRectForWindow(window, &monitorRect))
    {
        return;
    }

    if (params.BackBufferWidth == 0)
    {
        params.BackBufferWidth = static_cast<UINT>(monitorRect.right - monitorRect.left);
    }

    if (params.BackBufferHeight == 0)
    {
        params.BackBufferHeight = static_cast<UINT>(monitorRect.bottom - monitorRect.top);
    }
}

void ApplyBorderlessWindow(HWND window)
{
    if (!g_config.forceBorderless || window == nullptr)
    {
        return;
    }

    Rect monitorRect;
    if (!GetMonitorRectForWindow(window, &monitorRect))
    {
        return;
    }

    const BorderlessWindowPlan plan = BuildBorderlessWindowPlan(true, monitorRect);
    if (!plan.apply)
    {
        return;
    }

    const LONG style = GetWindowLongW(window, GWL_STYLE);
    const LONG exStyle = GetWindowLongW(window, GWL_EXSTYLE);
    const LONG newStyle = (style & ~WS_OVERLAPPEDWINDOW) | static_cast<LONG>(plan.style) |
                          (style & WS_VISIBLE);
    const LONG newExStyle =
        (exStyle & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME |
                     WS_EX_STATICEDGE)) |
        static_cast<LONG>(plan.exStyle);

    SetWindowLongW(window, GWL_STYLE, newStyle);
    SetWindowLongW(window, GWL_EXSTYLE, newExStyle);
    SetWindowPos(window, HWND_TOP, plan.x, plan.y, plan.width, plan.height,
                 SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_NOACTIVATE);

    std::ostringstream message;
    message << "Borderless window applied: " << plan.width << 'x' << plan.height << " at "
            << plan.x << ',' << plan.y;
    Log(message.str());
}

class ScopedD3DProbeCreate
{
public:
    ScopedD3DProbeCreate()
    {
        ++g_d3dProbeCreateDepth;
    }

    ~ScopedD3DProbeCreate()
    {
        if (g_d3dProbeCreateDepth > 0)
        {
            --g_d3dProbeCreateDepth;
        }
    }

    ScopedD3DProbeCreate(const ScopedD3DProbeCreate&) = delete;
    ScopedD3DProbeCreate& operator=(const ScopedD3DProbeCreate&) = delete;
};

bool IsD3DProbeCreateActive()
{
    return g_d3dProbeCreateDepth != 0;
}

bool IsOrdinalProcName(LPCSTR procName)
{
    return reinterpret_cast<std::uintptr_t>(procName) <= 0xffffu;
}

std::string ResolveModulePathForHandle(HMODULE module)
{
    if (module == nullptr)
    {
        return {};
    }

    char buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(module, buffer, static_cast<DWORD>(std::size(buffer)));
    if (length == 0)
    {
        return {};
    }

    return std::string(buffer, buffer + length);
}

bool PatchPointer(void** target, void* replacement)
{
    DWORD oldProtection = 0;
    if (!VirtualProtect(target, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection))
    {
        return false;
    }

    *target = replacement;
    DWORD ignored = 0;
    VirtualProtect(target, sizeof(void*), oldProtection, &ignored);
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(void*));
    return true;
}

bool PatchVtableEntry(void* object, std::size_t index, void* replacement, void** original)
{
    if (object == nullptr)
    {
        return false;
    }

    auto** vtable = *reinterpret_cast<void***>(object);
    if (vtable[index] == replacement)
    {
        return true;
    }

    if (original != nullptr && *original == nullptr)
    {
        *original = vtable[index];
    }

    return PatchPointer(&vtable[index], replacement);
}

bool StartsWithRelativeJumpDetour(std::uintptr_t address)
{
    std::uint8_t opcode = 0;
    return TryReadByteValue(reinterpret_cast<const void*>(address), opcode) && opcode == 0xE9;
}

bool InstallRelativeJumpDetour(void* target, std::size_t overwriteLength, void* replacement,
                               void** original)
{
    if (target == nullptr || replacement == nullptr || overwriteLength < kRelativeJumpLength)
    {
        return false;
    }

    auto* targetBytes = static_cast<std::uint8_t*>(target);
    if (targetBytes[0] == 0xE9)
    {
        return true;
    }

    auto* gateway = static_cast<std::uint8_t*>(VirtualAlloc(
        nullptr, overwriteLength + kRelativeJumpLength, MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE));
    if (gateway == nullptr)
    {
        return false;
    }

    std::memcpy(gateway, targetBytes, overwriteLength);
    gateway[overwriteLength] = 0xE9;

    const auto gatewayReturn =
        reinterpret_cast<std::intptr_t>(targetBytes + overwriteLength) -
        reinterpret_cast<std::intptr_t>(gateway + overwriteLength + kRelativeJumpLength);
    const auto gatewayDisp = static_cast<std::int32_t>(gatewayReturn);
    std::memcpy(gateway + overwriteLength + 1, &gatewayDisp, sizeof(gatewayDisp));

    DWORD oldProtection = 0;
    if (!VirtualProtect(targetBytes, overwriteLength, PAGE_EXECUTE_READWRITE, &oldProtection))
    {
        VirtualFree(gateway, 0, MEM_RELEASE);
        return false;
    }

    targetBytes[0] = 0xE9;
    const auto replacementDispValue =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(targetBytes + kRelativeJumpLength);
    const auto replacementDisp = static_cast<std::int32_t>(replacementDispValue);
    std::memcpy(targetBytes + 1, &replacementDisp, sizeof(replacementDisp));
    for (std::size_t index = kRelativeJumpLength; index < overwriteLength; ++index)
    {
        targetBytes[index] = 0x90;
    }

    DWORD ignored = 0;
    VirtualProtect(targetBytes, overwriteLength, oldProtection, &ignored);
    FlushInstructionCache(GetCurrentProcess(), targetBytes, overwriteLength);
    FlushInstructionCache(GetCurrentProcess(), gateway, overwriteLength + kRelativeJumpLength);

    if (original != nullptr && *original == nullptr)
    {
        *original = gateway;
    }

    return true;
}

bool InstallIatHook(HMODULE module, const char* importedModuleName, const char* functionName,
                    void* replacement, void** original)
{
    if (module == nullptr)
    {
        return false;
    }

    auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return false;
    }

    auto* ntHeaders =
        reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module) +
                                            dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        return false;
    }

    const IMAGE_DATA_DIRECTORY& importDirectory =
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDirectory.VirtualAddress == 0)
    {
        return false;
    }

    auto* importDescriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        reinterpret_cast<std::uint8_t*>(module) + importDirectory.VirtualAddress);

    for (; importDescriptor->Name != 0; ++importDescriptor)
    {
        const char* currentModuleName = reinterpret_cast<const char*>(
            reinterpret_cast<std::uint8_t*>(module) + importDescriptor->Name);
        if (_stricmp(currentModuleName, importedModuleName) != 0)
        {
            continue;
        }

        auto* firstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
            reinterpret_cast<std::uint8_t*>(module) + importDescriptor->FirstThunk);
        auto* originalThunk = importDescriptor->OriginalFirstThunk != 0
                                  ? reinterpret_cast<PIMAGE_THUNK_DATA>(
                                        reinterpret_cast<std::uint8_t*>(module) +
                                        importDescriptor->OriginalFirstThunk)
                                  : firstThunk;

        for (; firstThunk->u1.Function != 0; ++firstThunk, ++originalThunk)
        {
            if (IMAGE_SNAP_BY_ORDINAL(originalThunk->u1.Ordinal))
            {
                continue;
            }

            auto* importByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                reinterpret_cast<std::uint8_t*>(module) + originalThunk->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char*>(importByName->Name), functionName) != 0)
            {
                continue;
            }

            if (original != nullptr && *original == nullptr)
            {
                *original = reinterpret_cast<void*>(firstThunk->u1.Function);
            }

            return PatchPointer(reinterpret_cast<void**>(&firstThunk->u1.Function), replacement);
        }
    }

    return false;
}

std::string CopyMoviePath(const char* path)
{
    return path != nullptr ? std::string(path) : std::string();
}

std::optional<std::string> FindZeroWaitMenuMoviePath(void* movieHandle)
{
    if (movieHandle == nullptr)
    {
        return std::nullopt;
    }

    std::scoped_lock lock(g_introMovieMutex);
    const auto match = g_zeroWaitMenuMovies.find(movieHandle);
    if (match == g_zeroWaitMenuMovies.end())
    {
        return std::nullopt;
    }

    return match->second;
}

CadenceTracker* SelectTrackedBinkCadenceTracker(TrackedBinkMovieState* state,
                                                BinkTrackedFunction function)
{
    if (state == nullptr)
    {
        return nullptr;
    }

    switch (function)
    {
    case BinkTrackedFunction::Wait:
        return &state->waitTracker;
    case BinkTrackedFunction::DoFrame:
        return &state->doFrameTracker;
    case BinkTrackedFunction::NextFrame:
        return &state->nextFrameTracker;
    case BinkTrackedFunction::Pause:
        return &state->pauseTracker;
    }

    return nullptr;
}

std::shared_ptr<TrackedBinkMovieState> FindTrackedBinkMovieState(void* movieHandle)
{
    if (movieHandle == nullptr)
    {
        return nullptr;
    }

    std::scoped_lock lock(g_hookMutex);
    const auto match = g_trackedBinkMovies.find(movieHandle);
    if (match == g_trackedBinkMovies.end())
    {
        return nullptr;
    }

    return match->second;
}

void RegisterTrackedBinkMovie(void* movieHandle, const std::string& moviePath,
                              std::uint32_t openFlags)
{
    if (movieHandle == nullptr || !ShouldTraceBinkPlayback(moviePath))
    {
        return;
    }

    auto trackedMovie = std::make_shared<TrackedBinkMovieState>();
    trackedMovie->path = moviePath;
    trackedMovie->openFlags = openFlags;
    trackedMovie->openedAtTick = ::GetTickCount();

    {
        std::scoped_lock lock(g_hookMutex);
        trackedMovie->openOrdinal = ++g_trackedBinkMovieOpenOrdinals[moviePath];
        g_trackedBinkMovies[movieHandle] = trackedMovie;
    }
}

std::string BuildTrackedBinkLabel(BinkTrackedFunction function, void* movieHandle,
                                  const TrackedBinkMovieState& state)
{
    std::ostringstream label;
    label << GetBinkTrackedFunctionName(function) << '[' << state.path << "#"
          << state.openOrdinal << ", flags=0x" << std::hex << state.openFlags
          << ", handle=" << FormatAddress(reinterpret_cast<std::uintptr_t>(movieHandle))
          << ']';
    return label.str();
}

void LogTrackedBinkCadenceSample(BinkTrackedFunction function, void* movieHandle,
                                 double callDurationMs = -1.0)
{
    auto trackedMovie = FindTrackedBinkMovieState(movieHandle);
    if (trackedMovie == nullptr)
    {
        return;
    }

    CadenceTracker* tracker =
        SelectTrackedBinkCadenceTracker(trackedMovie.get(), function);
    if (tracker == nullptr)
    {
        return;
    }

    const std::string label = BuildTrackedBinkLabel(function, movieHandle, *trackedMovie);
    LogRenderCadenceSample(label.c_str(), *tracker, callDurationMs);
}

void LogTrackedBinkClose(void* movieHandle)
{
    std::shared_ptr<TrackedBinkMovieState> trackedMovie;
    {
        std::scoped_lock lock(g_hookMutex);
        const auto match = g_trackedBinkMovies.find(movieHandle);
        if (match == g_trackedBinkMovies.end())
        {
            return;
        }

        trackedMovie = match->second;
        g_trackedBinkMovies.erase(match);
    }

    std::ostringstream message;
    message << "Tracked Bink movie closed: path=" << trackedMovie->path << '#'
            << trackedMovie->openOrdinal << ", flags=0x" << std::hex
            << trackedMovie->openFlags << std::dec
            << ", lifetimeMs=" << (::GetTickCount() - trackedMovie->openedAtTick)
            << ", handle=" << FormatAddress(reinterpret_cast<std::uintptr_t>(movieHandle));
    Log(message.str());
}

HRESULT STDMETHODCALLTYPE HookedReset(IDirect3DDevice9* device,
                                      D3DPRESENT_PARAMETERS* presentationParameters);
HRESULT STDMETHODCALLTYPE HookedPresent(IDirect3DDevice9* device, const RECT* sourceRect,
                                        const RECT* destRect, HWND destWindowOverride,
                                        const RGNDATA* dirtyRegion);
HRESULT STDMETHODCALLTYPE HookedSwapChainPresent(IDirect3DSwapChain9* swapChain,
                                                 const RECT* sourceRect,
                                                 const RECT* destRect,
                                                 HWND destWindowOverride,
                                                 const RGNDATA* dirtyRegion,
                                                 DWORD flags);
HRESULT STDMETHODCALLTYPE HookedEndScene(IDirect3DDevice9* device);
HRESULT STDMETHODCALLTYPE HookedSetTransform(IDirect3DDevice9* device,
                                             D3DTRANSFORMSTATETYPE state,
                                             const D3DMATRIX* matrix);
HRESULT STDMETHODCALLTYPE HookedSetViewport(IDirect3DDevice9* device,
                                            CONST D3DVIEWPORT9* viewport);
IDirect3D9* WINAPI HookedDirect3DCreate9(UINT sdkVersion);
HRESULT WINAPI HookedDirect3DCreate9Ex(UINT sdkVersion, IDirect3D9Ex** returnedInterface);
DWORD WINAPI HookedGetTickCount();
LONG WINAPI HookedUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers);
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI HookedSetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER filter);
HRESULT WINAPI HookedDirectInput8Create(HINSTANCE instance, DWORD version, REFIID riid,
                                        LPVOID* outInterface, LPUNKNOWN outerUnknown);
HRESULT STDMETHODCALLTYPE HookedDirectInputCreateDeviceA(IDirectInput8A* directInput,
                                                         REFGUID guid,
                                                         LPDIRECTINPUTDEVICE8A* device,
                                                         LPUNKNOWN outerUnknown);
HRESULT STDMETHODCALLTYPE HookedDirectInputCreateDeviceW(IDirectInput8W* directInput,
                                                         REFGUID guid,
                                                         LPDIRECTINPUTDEVICE8W* device,
                                                         LPUNKNOWN outerUnknown);
HRESULT STDMETHODCALLTYPE HookedDirectInputGetDeviceStateA(IDirectInputDevice8A* device,
                                                           DWORD stateSize, LPVOID state);
HRESULT STDMETHODCALLTYPE HookedDirectInputGetDeviceStateW(IDirectInputDevice8W* device,
                                                           DWORD stateSize, LPVOID state);
HRESULT STDMETHODCALLTYPE HookedDirectInputSetCooperativeLevelA(IDirectInputDevice8A* device,
                                                                HWND window,
                                                                DWORD flags);
HRESULT STDMETHODCALLTYPE HookedDirectInputSetCooperativeLevelW(IDirectInputDevice8W* device,
                                                                HWND window,
                                                                DWORD flags);
void __fastcall HookedEngineSettingsInit(void* settings);
void __cdecl HookedEngineSleepEx(float milliseconds, BOOL alertable);
void __fastcall HookedSchedulerLoopUpdate(void* objectPointer, void* reserved,
                                          float deltaSeconds);
void* __stdcall HookedBinkOpen(const char* path, std::uint32_t flags)
{
    if (g_originalBinkOpen == nullptr)
    {
        return nullptr;
    }

    void* movieHandle = g_originalBinkOpen(path, flags);
    const std::string moviePath = CopyMoviePath(path);
    const MenuMovieWaitPlan menuPlan =
        BuildMenuMovieWaitPlan(g_config.reduceMenuMovieStutter, moviePath);
    const std::uint32_t logIndex = g_binkOpenLogCount.fetch_add(1, std::memory_order_relaxed);
    if (logIndex < kBinkOpenLogLimit)
    {
        std::ostringstream message;
        message << "BinkOpen: path=" << (moviePath.empty() ? "<null>" : moviePath)
                << ", flags=0x" << std::hex << flags << ", handle="
                << FormatAddress(reinterpret_cast<std::uintptr_t>(movieHandle))
                << ", menuWaitMatch=" << menuPlan.trackHandle
                << ", zeroWait=" << menuPlan.forceZeroWait;
        Log(message.str());
    }

    if (movieHandle != nullptr)
    {
        RegisterTrackedBinkMovie(movieHandle, moviePath, flags);

        std::scoped_lock lock(g_introMovieMutex);
        if (menuPlan.trackHandle)
        {
            g_zeroWaitMenuMovies[movieHandle] = moviePath;
        }
    }

    return movieHandle;
}

std::uint32_t __stdcall HookedBinkDoFrame(void* movieHandle)
{
    LARGE_INTEGER startCounter = {};
    LARGE_INTEGER endCounter = {};
    const bool measuredStart = ::QueryPerformanceCounter(&startCounter) != FALSE;
    const std::uint32_t result =
        g_originalBinkDoFrame != nullptr ? g_originalBinkDoFrame(movieHandle) : 0u;

    double durationMs = -1.0;
    if (measuredStart && ::QueryPerformanceCounter(&endCounter) != FALSE)
    {
        durationMs = MeasureHighPrecisionDurationMilliseconds(startCounter, endCounter);
    }

    LogTrackedBinkCadenceSample(BinkTrackedFunction::DoFrame, movieHandle, durationMs);
    return result;
}

std::uint32_t __stdcall HookedBinkNextFrame(void* movieHandle)
{
    LARGE_INTEGER startCounter = {};
    LARGE_INTEGER endCounter = {};
    const bool measuredStart = ::QueryPerformanceCounter(&startCounter) != FALSE;
    const std::uint32_t result =
        g_originalBinkNextFrame != nullptr ? g_originalBinkNextFrame(movieHandle) : 0u;

    double durationMs = -1.0;
    if (measuredStart && ::QueryPerformanceCounter(&endCounter) != FALSE)
    {
        durationMs = MeasureHighPrecisionDurationMilliseconds(startCounter, endCounter);
    }

    LogTrackedBinkCadenceSample(BinkTrackedFunction::NextFrame, movieHandle, durationMs);
    return result;
}

int __stdcall HookedBinkGoto(void* movieHandle, int frameIndex, std::uint32_t flags)
{
    const int result =
        g_originalBinkGoto != nullptr ? g_originalBinkGoto(movieHandle, frameIndex, flags) : 0;

    return result;
}

int __stdcall HookedBinkPause(void* movieHandle, int paused)
{
    LARGE_INTEGER startCounter = {};
    LARGE_INTEGER endCounter = {};
    const bool measuredStart = ::QueryPerformanceCounter(&startCounter) != FALSE;
    const int result = g_originalBinkPause != nullptr ? g_originalBinkPause(movieHandle, paused)
                                                      : 0;

    double durationMs = -1.0;
    if (measuredStart && ::QueryPerformanceCounter(&endCounter) != FALSE)
    {
        durationMs = MeasureHighPrecisionDurationMilliseconds(startCounter, endCounter);
    }

    LogTrackedBinkCadenceSample(BinkTrackedFunction::Pause, movieHandle, durationMs);
    return result;
}

int __stdcall HookedBinkWait(void* movieHandle)
{
    LARGE_INTEGER startCounter = {};
    LARGE_INTEGER endCounter = {};
    const bool measuredStart = ::QueryPerformanceCounter(&startCounter) != FALSE;
    const std::optional<std::string> zeroWaitMoviePath = FindZeroWaitMenuMoviePath(movieHandle);
    if (zeroWaitMoviePath.has_value())
    {
        g_introInterventionCount.fetch_add(1, std::memory_order_relaxed);

        const std::uint32_t logIndex =
            g_binkPlaybackLogCount.fetch_add(1, std::memory_order_relaxed);
        if (logIndex < kBinkPlaybackLogLimit)
        {
            std::ostringstream message;
            message << "ReduceMenuMovieStutter active: forcing BinkWait=0 for "
                    << *zeroWaitMoviePath << ", handle="
                    << FormatAddress(reinterpret_cast<std::uintptr_t>(movieHandle));
            Log(message.str());
        }

        double durationMs = -1.0;
        if (measuredStart && ::QueryPerformanceCounter(&endCounter) != FALSE)
        {
            durationMs = MeasureHighPrecisionDurationMilliseconds(startCounter, endCounter);
        }

        LogTrackedBinkCadenceSample(BinkTrackedFunction::Wait, movieHandle, durationMs);
        return 0;
    }

    const int result = g_originalBinkWait != nullptr ? g_originalBinkWait(movieHandle) : 0;
    double durationMs = -1.0;
    if (measuredStart && ::QueryPerformanceCounter(&endCounter) != FALSE)
    {
        durationMs = MeasureHighPrecisionDurationMilliseconds(startCounter, endCounter);
    }

    LogTrackedBinkCadenceSample(BinkTrackedFunction::Wait, movieHandle, durationMs);
    return result;
}

void __stdcall HookedBinkClose(void* movieHandle)
{
    LogTrackedBinkClose(movieHandle);

    if (movieHandle != nullptr)
    {
        std::scoped_lock lock(g_introMovieMutex);
        g_zeroWaitMenuMovies.erase(movieHandle);
    }

    if (g_originalBinkClose != nullptr)
    {
        g_originalBinkClose(movieHandle);
    }
}

HRESULT STDMETHODCALLTYPE HookedCreateDevice(IDirect3D9* direct3d, UINT adapter,
                                             D3DDEVTYPE deviceType, HWND focusWindow,
                                             DWORD behaviorFlags,
                                             D3DPRESENT_PARAMETERS* presentationParameters,
                                             IDirect3DDevice9** returnedDevice)
{
    if (g_originalCreateDevice == nullptr)
    {
        return E_FAIL;
    }

    D3DPRESENT_PARAMETERS workingParams = {};
    D3DPRESENT_PARAMETERS* paramsForCall = presentationParameters;
    const bool isProbeCreate = IsD3DProbeCreateActive();
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (presentationParameters != nullptr && g_config.enableSafeResolutionChanges)
    {
        workingParams = *presentationParameters;
        const HWND targetWindow =
            workingParams.hDeviceWindow != nullptr ? workingParams.hDeviceWindow : focusWindow;
        FillMissingBorderlessDimensions(targetWindow, workingParams);
        const PresentParams original = ToPresentParams(workingParams);
        const PresentParams sanitized = SanitizeForReset(original, g_config);
        ApplyPresentParams(sanitized, workingParams);
        paramsForCall = &workingParams;

        std::ostringstream message;
        message << (isProbeCreate ? "Probe CreateDevice intercepted: "
                                  : "CreateDevice intercepted: ")
                << sanitized.backBufferWidth << 'x'
                << sanitized.backBufferHeight << ", windowed=" << sanitized.windowed
                << ", refresh=" << original.refreshRate << "->" << sanitized.refreshRate
                << ", swapEffect=" << FormatSwapEffect(original.swapEffect) << "->"
                << FormatSwapEffect(sanitized.swapEffect)
                << ", interval="
                << FormatPresentationInterval(original.presentationInterval) << "->"
                << FormatPresentationInterval(sanitized.presentationInterval)
                << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    HRESULT hr = g_originalCreateDevice(direct3d, adapter, deviceType, focusWindow,
                                        behaviorFlags, paramsForCall, returnedDevice);

    if (FAILED(hr) && presentationParameters != nullptr && g_config.enableSafeResolutionChanges &&
        g_config.retryResetInWindowedMode)
    {
        D3DPRESENT_PARAMETERS retryParams = *presentationParameters;
        const PresentParams retry =
            BuildRetryResetParams(ToPresentParams(retryParams), g_config);
        ApplyPresentParams(retry, retryParams);

        hr = g_originalCreateDevice(direct3d, adapter, deviceType, focusWindow, behaviorFlags,
                                    &retryParams, returnedDevice);
        Log("CreateDevice retry attempted, result=" + FormatHr(hr));
    }

    if (SUCCEEDED(hr) && returnedDevice != nullptr && *returnedDevice != nullptr)
    {
        if (!isProbeCreate)
        {
            UpdateBackbufferDimensions(ToPresentParams(workingParams));
            g_lastDeviceAddress = reinterpret_cast<std::uintptr_t>(*returnedDevice);
        }
        {
            std::ostringstream message;
            message << (isProbeCreate ? "Probe D3D swap chain count="
                                      : "D3D swap chain count=")
                    << (*returnedDevice)->GetNumberOfSwapChains();
            Log(message.str());
        }
        std::scoped_lock lock(g_hookMutex);
        if (PatchVtableEntry(*returnedDevice, kResetVtableIndex,
                             reinterpret_cast<void*>(&HookedReset),
                             reinterpret_cast<void**>(&g_originalReset)))
        {
            Log("IDirect3DDevice9::Reset hook installed.");
        }
        if (PatchVtableEntry(*returnedDevice, kTestCooperativeLevelVtableIndex,
                             reinterpret_cast<void*>(&HookedTestCooperativeLevel),
                             reinterpret_cast<void**>(&g_originalTestCooperativeLevel)))
        {
            Log("IDirect3DDevice9::TestCooperativeLevel hook installed.");
        }
        if (PatchVtableEntry(*returnedDevice, kPresentVtableIndex,
                             reinterpret_cast<void*>(&HookedPresent),
                             reinterpret_cast<void**>(&g_originalPresent)))
        {
            g_lastOriginalPresentAddress =
                reinterpret_cast<std::uintptr_t>(g_originalPresent);
            std::ostringstream message;
            message << "IDirect3DDevice9::Present hook installed at "
                    << FormatAddress(g_lastDeviceAddress)
                    << ", original=" << FormatAddress(g_lastOriginalPresentAddress);
            Log(message.str());
        }
        InstallSwapChainPresentHook(*returnedDevice);
        if (PatchVtableEntry(*returnedDevice, kEndSceneVtableIndex,
                             reinterpret_cast<void*>(&HookedEndScene),
                             reinterpret_cast<void**>(&g_originalEndScene)))
        {
            g_lastOriginalEndSceneAddress =
                reinterpret_cast<std::uintptr_t>(g_originalEndScene);
            std::ostringstream message;
            message << "IDirect3DDevice9::EndScene hook installed at "
                    << FormatAddress(g_lastDeviceAddress)
                    << ", original=" << FormatAddress(g_lastOriginalEndSceneAddress);
            Log(message.str());
        }
        if (PatchVtableEntry(*returnedDevice, kSetTransformVtableIndex,
                             reinterpret_cast<void*>(&HookedSetTransform),
                             reinterpret_cast<void**>(&g_originalSetTransform)))
        {
            Log("IDirect3DDevice9::SetTransform hook installed.");
        }
        if (PatchVtableEntry(*returnedDevice, kSetViewportVtableIndex,
                             reinterpret_cast<void*>(&HookedSetViewport),
                             reinterpret_cast<void**>(&g_originalSetViewport)))
        {
            Log("IDirect3DDevice9::SetViewport hook installed.");
        }
        InstallDeviceExHooks(*returnedDevice);
        InstallGlobalD3DCodeHooks();

        if (!isProbeCreate)
        {
            const HWND targetWindow =
                workingParams.hDeviceWindow != nullptr ? workingParams.hDeviceWindow : focusWindow;
            EnsureGameWindowHook(targetWindow);
            ApplyBorderlessWindow(targetWindow);
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE HookedCreateDeviceEx(IDirect3D9Ex* direct3d, UINT adapter,
                                               D3DDEVTYPE deviceType, HWND focusWindow,
                                               DWORD behaviorFlags,
                                               D3DPRESENT_PARAMETERS* presentationParameters,
                                               D3DDISPLAYMODEEX* fullscreenDisplayMode,
                                               IDirect3DDevice9Ex** returnedDevice)
{
    if (g_originalCreateDeviceEx == nullptr)
    {
        return E_FAIL;
    }

    D3DPRESENT_PARAMETERS workingParams = {};
    D3DPRESENT_PARAMETERS* paramsForCall = presentationParameters;
    const bool isProbeCreate = IsD3DProbeCreateActive();
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (presentationParameters != nullptr && g_config.enableSafeResolutionChanges)
    {
        workingParams = *presentationParameters;
        const HWND targetWindow =
            workingParams.hDeviceWindow != nullptr ? workingParams.hDeviceWindow : focusWindow;
        FillMissingBorderlessDimensions(targetWindow, workingParams);
        const PresentParams original = ToPresentParams(workingParams);
        const PresentParams sanitized = SanitizeForReset(original, g_config, true);
        ApplyPresentParams(sanitized, workingParams);
        paramsForCall = &workingParams;

        std::ostringstream message;
        message << (isProbeCreate ? "Probe CreateDeviceEx intercepted: "
                                  : "CreateDeviceEx intercepted: ")
                << sanitized.backBufferWidth << 'x'
                << sanitized.backBufferHeight << ", windowed=" << sanitized.windowed
                << ", refresh=" << original.refreshRate << "->" << sanitized.refreshRate
                << ", swapEffect=" << FormatSwapEffect(original.swapEffect) << "->"
                << FormatSwapEffect(sanitized.swapEffect)
                << ", interval="
                << FormatPresentationInterval(original.presentationInterval) << "->"
                << FormatPresentationInterval(sanitized.presentationInterval)
                << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    HRESULT hr = g_originalCreateDeviceEx(direct3d, adapter, deviceType, focusWindow,
                                          behaviorFlags, paramsForCall, fullscreenDisplayMode,
                                          returnedDevice);

    if (FAILED(hr) && presentationParameters != nullptr && g_config.enableSafeResolutionChanges &&
        g_config.retryResetInWindowedMode)
    {
        D3DPRESENT_PARAMETERS retryParams = *presentationParameters;
        const PresentParams retry =
            BuildRetryResetParams(ToPresentParams(retryParams), g_config, true);
        ApplyPresentParams(retry, retryParams);

        hr = g_originalCreateDeviceEx(direct3d, adapter, deviceType, focusWindow, behaviorFlags,
                                      &retryParams, nullptr, returnedDevice);
        Log("CreateDeviceEx retry attempted, result=" + FormatHr(hr));
    }

    if (SUCCEEDED(hr) && returnedDevice != nullptr && *returnedDevice != nullptr)
    {
        if (!isProbeCreate)
        {
            UpdateBackbufferDimensions(ToPresentParams(workingParams));
            g_lastDeviceAddress = reinterpret_cast<std::uintptr_t>(*returnedDevice);
        }
        {
            std::ostringstream message;
            message << (isProbeCreate ? "Probe D3DEx swap chain count="
                                      : "D3DEx swap chain count=")
                    << (*returnedDevice)->GetNumberOfSwapChains();
            Log(message.str());
        }
        std::scoped_lock lock(g_hookMutex);
        if (PatchVtableEntry(*returnedDevice, kResetVtableIndex,
                             reinterpret_cast<void*>(&HookedReset),
                             reinterpret_cast<void**>(&g_originalReset)))
        {
            Log("IDirect3DDevice9::Reset hook installed.");
        }
        if (PatchVtableEntry(*returnedDevice, kTestCooperativeLevelVtableIndex,
                             reinterpret_cast<void*>(&HookedTestCooperativeLevel),
                             reinterpret_cast<void**>(&g_originalTestCooperativeLevel)))
        {
            Log("IDirect3DDevice9::TestCooperativeLevel hook installed.");
        }
        if (PatchVtableEntry(*returnedDevice, kPresentVtableIndex,
                             reinterpret_cast<void*>(&HookedPresent),
                             reinterpret_cast<void**>(&g_originalPresent)))
        {
            g_lastOriginalPresentAddress =
                reinterpret_cast<std::uintptr_t>(g_originalPresent);
            std::ostringstream message;
            message << "IDirect3DDevice9::Present hook installed at "
                    << FormatAddress(g_lastDeviceAddress)
                    << ", original=" << FormatAddress(g_lastOriginalPresentAddress);
            Log(message.str());
        }
        InstallSwapChainPresentHook(*returnedDevice);
        if (PatchVtableEntry(*returnedDevice, kEndSceneVtableIndex,
                             reinterpret_cast<void*>(&HookedEndScene),
                             reinterpret_cast<void**>(&g_originalEndScene)))
        {
            g_lastOriginalEndSceneAddress =
                reinterpret_cast<std::uintptr_t>(g_originalEndScene);
            std::ostringstream message;
            message << "IDirect3DDevice9::EndScene hook installed at "
                    << FormatAddress(g_lastDeviceAddress)
                    << ", original=" << FormatAddress(g_lastOriginalEndSceneAddress);
            Log(message.str());
        }
        if (PatchVtableEntry(*returnedDevice, kSetTransformVtableIndex,
                             reinterpret_cast<void*>(&HookedSetTransform),
                             reinterpret_cast<void**>(&g_originalSetTransform)))
        {
            Log("IDirect3DDevice9::SetTransform hook installed.");
        }
        if (PatchVtableEntry(*returnedDevice, kSetViewportVtableIndex,
                             reinterpret_cast<void*>(&HookedSetViewport),
                             reinterpret_cast<void**>(&g_originalSetViewport)))
        {
            Log("IDirect3DDevice9::SetViewport hook installed.");
        }
        InstallDeviceExHooks(*returnedDevice);
        InstallGlobalD3DCodeHooks();

        if (!isProbeCreate)
        {
            const HWND targetWindow =
                workingParams.hDeviceWindow != nullptr ? workingParams.hDeviceWindow : focusWindow;
            EnsureGameWindowHook(targetWindow);
            ApplyBorderlessWindow(targetWindow);
        }
    }

    return hr;
}

IDirect3D9* WINAPI HookedDirect3DCreate9(UINT sdkVersion)
{
    if (g_originalDirect3DCreate9 == nullptr)
    {
        return nullptr;
    }

    IDirect3D9* direct3d = g_originalDirect3DCreate9(sdkVersion);
    if (direct3d == nullptr)
    {
        return nullptr;
    }

    std::scoped_lock lock(g_hookMutex);
    if (PatchVtableEntry(direct3d, kCreateDeviceVtableIndex,
                         reinterpret_cast<void*>(&HookedCreateDevice),
                         reinterpret_cast<void**>(&g_originalCreateDevice)))
    {
        Log("IDirect3D9::CreateDevice hook installed.");
    }

    return direct3d;
}

HRESULT WINAPI HookedDirect3DCreate9Ex(UINT sdkVersion, IDirect3D9Ex** returnedInterface)
{
    if (g_originalDirect3DCreate9Ex == nullptr)
    {
        return D3DERR_INVALIDCALL;
    }

    const HRESULT hr = g_originalDirect3DCreate9Ex(sdkVersion, returnedInterface);
    if (FAILED(hr) || returnedInterface == nullptr || *returnedInterface == nullptr)
    {
        return hr;
    }

    std::scoped_lock lock(g_hookMutex);
    if (PatchVtableEntry(*returnedInterface, kCreateDeviceVtableIndex,
                         reinterpret_cast<void*>(&HookedCreateDevice),
                         reinterpret_cast<void**>(&g_originalCreateDevice)))
    {
        Log("IDirect3D9Ex::CreateDevice hook installed.");
    }
    if (PatchVtableEntry(*returnedInterface, kCreateDeviceExVtableIndex,
                         reinterpret_cast<void*>(&HookedCreateDeviceEx),
                         reinterpret_cast<void**>(&g_originalCreateDeviceEx)))
    {
        Log("IDirect3D9Ex::CreateDeviceEx hook installed.");
    }

    return hr;
}

DWORD WINAPI HookedGetTickCount()
{
    if (!g_config.enableHighPrecisionTiming || g_highPrecisionTimerState.frequency == 0)
    {
        return g_originalGetTickCount != nullptr ? g_originalGetTickCount() : ::GetTickCount();
    }

    LARGE_INTEGER currentCounter = {};
    if (QueryPerformanceCounter(&currentCounter) == FALSE)
    {
        return g_originalGetTickCount != nullptr ? g_originalGetTickCount() : ::GetTickCount();
    }

    return ComputeHighPrecisionTickCount(
        g_highPrecisionTimerState, static_cast<std::uint64_t>(currentCounter.QuadPart));
}

LONG WINAPI HookedUnhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
{
    WriteCrashDump(exceptionPointers);

    const auto chainedFilter = reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(
        g_chainedUnhandledExceptionFilter.load());
    if (chainedFilter != nullptr &&
        chainedFilter != reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(
                             &HookedUnhandledExceptionFilter))
    {
        return chainedFilter(exceptionPointers);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI HookedSetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER filter)
{
    const auto previousFilter = reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(
        g_chainedUnhandledExceptionFilter.exchange(reinterpret_cast<std::uintptr_t>(filter)));
    ::SetUnhandledExceptionFilter(&HookedUnhandledExceptionFilter);
    return previousFilter;
}

void __cdecl HookedEngineSleepEx(float milliseconds, BOOL alertable)
{
    const std::uint32_t roundedMilliseconds =
        milliseconds > 0.0f ? static_cast<std::uint32_t>(milliseconds + 0.5f) : 0;
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (IsSuspiciousFramePacingIntervalMs(roundedMilliseconds) &&
        LogCallerOnce(g_loggedSleepCallers, caller))
    {
        std::ostringstream message;
        message << "Engine SleepEx candidate: " << milliseconds << "ms, alertable="
                << (alertable != FALSE) << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    const std::uintptr_t framePacingCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) + kEngineFramePacingSleepCallerRva
            : 0;
    if (caller == framePacingCaller && alertable == FALSE)
    {
        ApplyLegacyFramePacingBudget(30);
    }

    const std::uintptr_t schedulerLoopCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) + kSchedulerLoopSleepCallerRva
            : 0;
    if (caller == schedulerLoopCaller && alertable != FALSE &&
        g_config.enableFrameRateUnlock && g_originalEngineSleepEx != nullptr &&
        g_highPrecisionTimerState.frequency != 0)
    {
        const float originalSleepMilliseconds = milliseconds;
        LARGE_INTEGER workEndCounter = {};
        if (QueryPerformanceCounter(&workEndCounter) != FALSE)
        {
            float measuredWorkElapsedMilliseconds = 0.0f;
            {
                std::scoped_lock lock(g_schedulerTimingMutex);
                if (g_schedulerLastWakeCounter != 0 &&
                    static_cast<std::uint64_t>(workEndCounter.QuadPart) >
                        g_schedulerLastWakeCounter)
                {
                    const std::uint64_t elapsedCounter =
                        static_cast<std::uint64_t>(workEndCounter.QuadPart) -
                        g_schedulerLastWakeCounter;
                    measuredWorkElapsedMilliseconds =
                        static_cast<float>(static_cast<double>(elapsedCounter) * 1000.0 /
                                           static_cast<double>(g_highPrecisionTimerState.frequency));
                }
            }

            const SchedulerFramePlan schedulerPlan =
                BuildSchedulerFramePlan(g_config.enableFrameRateUnlock,
                                        g_config.targetFrameRate,
                                        (g_config.disableVsync ||
                                         ShouldUseImmediatePresentationInBorderless(g_config))
                                            ? 0.0f
                                            : g_config.presentWakeLeadMilliseconds,
                                        measuredWorkElapsedMilliseconds,
                                        0.0f,
                                        originalSleepMilliseconds);
            const std::uint32_t sampleIndex =
                g_schedulerSleepLogCount.fetch_add(1, std::memory_order_relaxed);
            const double targetFrameMs = ResolveDiagnosticTargetFrameDurationMs();
            const bool logOutlier =
                IsCadenceOutlier(measuredWorkElapsedMilliseconds, targetFrameMs) &&
                sampleIndex < (kSchedulerTimingLogSampleLimit + kRenderCadenceOutlierLogLimit);
            if (sampleIndex < kSchedulerTimingLogSampleLimit || logOutlier)
            {
                std::ostringstream message;
                message << "Scheduler sleep override "
                        << (logOutlier && sampleIndex >= kSchedulerTimingLogSampleLimit
                                ? "outlier "
                                : "sample ")
                        << (sampleIndex + 1)
                        << ": sleep=" << originalSleepMilliseconds << "->"
                        << schedulerPlan.sleepMilliseconds << "ms, measuredWork="
                        << measuredWorkElapsedMilliseconds << "ms, wakeLead="
                        << ((g_config.disableVsync ||
                             ShouldUseImmediatePresentationInBorderless(g_config))
                                ? 0.0f
                                : g_config.presentWakeLeadMilliseconds)
                        << "ms, target=";
                if (g_config.targetFrameRate == 0)
                {
                    message << "unlocked";
                }
                else
                {
                    message << g_config.targetFrameRate;
                }
                Log(message.str());
            }

            const PreciseSleepPlan precisePlan =
                BuildAlertablePreciseSleepPlan(g_config.enablePreciseSleepShim,
                                               schedulerPlan.sleepMilliseconds);
            if (precisePlan.useOriginalSleep)
            {
                g_originalEngineSleepEx(schedulerPlan.sleepMilliseconds, TRUE);
            }
            else
            {
                if (precisePlan.coarseSleepMs > 0)
                {
                    g_originalEngineSleepEx(static_cast<float>(precisePlan.coarseSleepMs), TRUE);
                }

                SpinWaitForMilliseconds(precisePlan.spinWaitMs);
                std::call_once(g_preciseSleepLogOnce, []() {
                    Log("Precise sleep shim active.");
                });
            }

            LARGE_INTEGER wakeCounter = {};
            if (QueryPerformanceCounter(&wakeCounter) != FALSE)
            {
                std::scoped_lock lock(g_schedulerTimingMutex);
                g_schedulerLastWakeCounter =
                    static_cast<std::uint64_t>(wakeCounter.QuadPart);
            }
            return;
        }
    }

    const PreciseSleepPlan plan =
        BuildPreciseSleepPlan(g_config.enablePreciseSleepShim, milliseconds,
                              alertable != FALSE);
    if (plan.useOriginalSleep || g_originalEngineSleepEx == nullptr ||
        g_highPrecisionTimerState.frequency == 0)
    {
        if (g_originalEngineSleepEx != nullptr)
        {
            g_originalEngineSleepEx(milliseconds, alertable);
        }
        return;
    }

    if (plan.coarseSleepMs > 0)
    {
        ::SleepEx(plan.coarseSleepMs, FALSE);
    }

    SpinWaitForMilliseconds(plan.spinWaitMs);
    std::call_once(g_preciseSleepLogOnce, []() { Log("Precise sleep shim active."); });
}

MMRESULT WINAPI HookedTimeSetEvent(UINT delay, UINT resolution, LPTIMECALLBACK callback,
                                   DWORD_PTR user, UINT eventType)
{
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (IsSuspiciousFramePacingIntervalMs(delay) &&
        LogCallerOnce(g_loggedTimeSetEventCallers, caller))
    {
        std::ostringstream message;
        message << "timeSetEvent candidate: delay=" << delay << "ms, resolution=" << resolution
                << ", flags=0x" << std::hex << eventType << std::dec
                << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    UINT adjustedDelay = delay;
    const std::uintptr_t legacyTimerCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) + kLegacyFramePacingTimerCallerRva
            : 0;
    if (caller == legacyTimerCaller)
    {
        ApplyLegacyFramePacingBudget(30);
        const std::uintptr_t legacyTimerCallback =
            g_engineModule != nullptr
                ? reinterpret_cast<std::uintptr_t>(g_engineModule) +
                      kLegacyPeriodicTimerCallbackRva
                : 0;
        const LegacyPeriodicTimerPlan timerPlan = BuildLegacyPeriodicTimerPlan(
            g_config.enableFrameRateUnlock, g_config.disableLegacyPeriodicIconicTimer,
            g_config.targetFrameRate, delay, true,
            reinterpret_cast<std::uintptr_t>(callback) == legacyTimerCallback);
        adjustedDelay = timerPlan.adjustedDelayMs;
        if (timerPlan.suppressTimer)
        {
            g_legacyPeriodicTimerSuppressionCount.fetch_add(1, std::memory_order_relaxed);
            const std::uint32_t syntheticTimerId =
                g_nextSyntheticLegacyTimerId.fetch_add(1, std::memory_order_relaxed) + 1u;
            g_activeSyntheticLegacyTimerId.store(syntheticTimerId, std::memory_order_relaxed);
            std::call_once(g_legacyPeriodicTimerSuppressionLogOnce,
                           [delay, syntheticTimerId]() {
                               std::ostringstream message;
                               message
                                   << "Legacy periodic iconic timer suppressed: delay="
                                   << delay << "ms, syntheticId=0x" << std::hex
                                   << syntheticTimerId << std::dec;
                               Log(message.str());
                           });
            return syntheticTimerId;
        }

        if (adjustedDelay != delay)
        {
            std::call_once(g_legacyPeriodicTimerLogOnce, [delay, adjustedDelay]() {
                std::ostringstream message;
                message << "Legacy periodic timer override active: " << delay << "ms -> "
                        << adjustedDelay << "ms";
                Log(message.str());
            });
        }
    }

    return g_originalTimeSetEvent != nullptr
               ? g_originalTimeSetEvent(adjustedDelay, resolution, callback, user, eventType)
               : 0;
}

MMRESULT WINAPI HookedTimeKillEvent(MMRESULT timerId)
{
    const std::uint32_t syntheticTimerId =
        g_activeSyntheticLegacyTimerId.load(std::memory_order_relaxed);
    if (ShouldSwallowSyntheticLegacyTimerKill(g_config.disableLegacyPeriodicIconicTimer,
                                              timerId, syntheticTimerId))
    {
        g_legacyPeriodicTimerKillSwallowCount.fetch_add(1, std::memory_order_relaxed);
        g_activeSyntheticLegacyTimerId.store(0, std::memory_order_relaxed);
        std::call_once(g_legacyPeriodicTimerKillLogOnce, [timerId]() {
            std::ostringstream message;
            message << "Legacy periodic iconic timer kill swallowed: timerId=0x" << std::hex
                    << timerId << std::dec;
            Log(message.str());
        });
        return TIMERR_NOERROR;
    }

    return g_originalTimeKillEvent != nullptr ? g_originalTimeKillEvent(timerId)
                                              : MMSYSERR_INVALPARAM;
}

BOOL WINAPI HookedSetWaitableTimer(HANDLE timer, const LARGE_INTEGER* dueTime, LONG period,
                                   PTIMERAPCROUTINE completionRoutine, LPVOID argument,
                                   BOOL resumeFromSuspend)
{
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const std::optional<std::uint32_t> dueTimeMs =
        dueTime != nullptr ? RelativeDueTimeToMilliseconds(dueTime->QuadPart) : std::nullopt;
    if (((period > 0 && IsSuspiciousFramePacingIntervalMs(static_cast<std::uint32_t>(period))) ||
         (dueTimeMs.has_value() && IsSuspiciousFramePacingIntervalMs(*dueTimeMs))) &&
        LogCallerOnce(g_loggedSetWaitableTimerCallers, caller))
    {
        std::ostringstream message;
        message << "SetWaitableTimer candidate: dueTimeMs=";
        if (dueTimeMs.has_value())
        {
            message << *dueTimeMs;
        }
        else
        {
            message << "absolute";
        }
        message << ", period=" << period << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    const std::uintptr_t legacyWaitableTimerSetupCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) + kLegacyWaitableTimerSetupReturnRva
            : 0;
    if (caller == legacyWaitableTimerSetupCaller)
    {
        TrackLegacyWaitableTimerWorkerHandle(timer, dueTimeMs, period, caller);
    }

    return g_originalSetWaitableTimer != nullptr
               ? g_originalSetWaitableTimer(timer, dueTime, period, completionRoutine, argument,
                                           resumeFromSuspend)
               : FALSE;
}

VOID WINAPI HookedEngineSleep(DWORD milliseconds)
{
    DWORD adjustedMilliseconds = milliseconds;
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const std::uintptr_t graphicsRetrySleepCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) +
                  kLegacyGraphicsRetrySleepReturnRva
            : 0;
    if (caller == graphicsRetrySleepCaller)
    {
        adjustedMilliseconds = ResolveLegacyGraphicsRetryDelayMs(
            g_config.hardenLegacyGraphicsRecovery,
            g_config.legacyGraphicsRetryDelayMilliseconds, milliseconds);
        if (adjustedMilliseconds != milliseconds &&
            LogCallerOnce(g_loggedLegacyGraphicsRetrySleepCallers, caller))
        {
            std::ostringstream message;
            message << "Legacy graphics retry delay override active: " << milliseconds
                    << "ms -> " << adjustedMilliseconds
                    << "ms, caller=" << FormatAddress(caller);
            Log(message.str());
        }
    }

    const std::uintptr_t legacyWaitableTimerWorkerSleepCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) +
                  kLegacyWaitableTimerWorkerSleepReturnRva
            : 0;
    const HANDLE trackedWaitableTimerHandle =
        g_legacyWaitableTimerWorkerHandle.load(std::memory_order_acquire);
    const LegacyWaitableTimerWorkerPlan waitableTimerWorkerPlan =
        BuildLegacyWaitableTimerWorkerPlan(g_config.reduceLegacyWaitableTimerPolling,
                                           milliseconds,
                                           caller == legacyWaitableTimerWorkerSleepCaller,
                                           trackedWaitableTimerHandle != nullptr);
    const DWORD currentThreadId = GetCurrentThreadId();
    const DWORD stdAudioThreadId = g_stdAudioThreadId.load(std::memory_order_relaxed);
    if (caller == legacyWaitableTimerWorkerSleepCaller && stdAudioThreadId != 0 &&
        currentThreadId == stdAudioThreadId)
    {
        LogStdAudioWorkerTracking(milliseconds, waitableTimerWorkerPlan.useTrackedTimer);
        LogRenderCadenceSample("StdAudio worker sleep", g_stdAudioWorkerSleepCadence);
    }

    if (waitableTimerWorkerPlan.useTrackedTimer && trackedWaitableTimerHandle != nullptr)
    {
        const DWORD waitResult =
            WaitForSingleObject(trackedWaitableTimerHandle,
                                waitableTimerWorkerPlan.adjustedSleepMs);
        if (waitResult != WAIT_FAILED)
        {
            g_legacyWaitableTimerAlignmentCount.fetch_add(1, std::memory_order_relaxed);
            std::call_once(g_legacyWaitableTimerWorkerSleepLogOnce, [caller, milliseconds]() {
                std::ostringstream message;
                message << "Legacy waitable timer worker sleep aligned to tracked timer: "
                        << milliseconds << "ms, caller=" << FormatAddress(caller);
                Log(message.str());
            });
            return;
        }

        const DWORD waitError = GetLastError();
        g_legacyWaitableTimerYieldCount.fetch_add(1, std::memory_order_relaxed);
        std::call_once(g_legacyWaitableTimerWorkerWaitFailureLogOnce, [caller, waitError]() {
            std::ostringstream message;
            message << "Legacy waitable timer worker timer wait failed; falling back to yield, "
                    << "caller=" << FormatAddress(caller)
                    << ", error=" << waitError;
            Log(message.str());
        });
        adjustedMilliseconds = 0;
    }
    else if (waitableTimerWorkerPlan.adjustedSleepMs != adjustedMilliseconds)
    {
        g_legacyWaitableTimerYieldCount.fetch_add(1, std::memory_order_relaxed);
        adjustedMilliseconds = waitableTimerWorkerPlan.adjustedSleepMs;
        std::call_once(g_legacyWaitableTimerWorkerYieldLogOnce, [caller, milliseconds]() {
            std::ostringstream message;
            message << "Legacy waitable timer worker polling override active: " << milliseconds
                    << "ms -> 0ms, caller=" << FormatAddress(caller);
            Log(message.str());
        });
    }

    if (g_originalEngineSleep != nullptr)
    {
        g_originalEngineSleep(adjustedMilliseconds);
        return;
    }

    ::Sleep(adjustedMilliseconds);
}

HANDLE WINAPI HookedCreateThread(LPSECURITY_ATTRIBUTES attributes, SIZE_T stackSize,
                                 LPTHREAD_START_ROUTINE startAddress, LPVOID parameter,
                                 DWORD creationFlags, LPDWORD threadId)
{
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (startAddress != nullptr && LogCallerOnce(g_loggedCreateThreadCallers, caller))
    {
        std::ostringstream message;
        message << "CreateThread candidate: start="
                << FormatAddress(reinterpret_cast<std::uintptr_t>(startAddress))
                << ", parameter=" << FormatAddress(reinterpret_cast<std::uintptr_t>(parameter))
                << ", flags=0x" << std::hex << creationFlags << std::dec
                << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    if (g_originalCreateThread == nullptr)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return nullptr;
    }

    HANDLE thread =
        g_originalCreateThread(attributes, stackSize, startAddress, parameter, creationFlags,
                               threadId);
    const std::uintptr_t legacyThreadWrapperCreateCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) + kLegacyThreadWrapperCreateReturnRva
            : 0;
    if (caller == legacyThreadWrapperCreateCaller)
    {
        g_legacyThreadWrapperCreateObservedCount.fetch_add(1, std::memory_order_relaxed);
        const auto snapshot = TryReadLegacyThreadWrapperSnapshot(parameter);
        std::optional<std::string> tagText;
        LegacyThreadRole role = LegacyThreadRole::Unknown;
        std::optional<LegacyThreadWrapperObjectGraphSnapshot> objectGraph;
        if (snapshot.has_value())
        {
            tagText = TryReadLegacyThreadTagText(snapshot->tag);
            if (tagText.has_value())
            {
                role = ClassifyLegacyThreadTag(*tagText);
            }

            objectGraph = TryReadLegacyThreadWrapperObjectGraphSnapshot(snapshot->userParameter);
            if (role == LegacyThreadRole::StdAudioEngine)
            {
                const DWORD observedThreadId = threadId != nullptr && *threadId != 0
                                                   ? *threadId
                                                   : (thread != nullptr ? GetThreadId(thread) : 0);
                if (observedThreadId != 0)
                {
                    g_stdAudioThreadId.store(observedThreadId, std::memory_order_relaxed);
                }

                g_stdAudioRootObject.store(snapshot->userParameter, std::memory_order_relaxed);
                if (objectGraph.has_value())
                {
                    g_stdAudioRootObject.store(objectGraph->rootObject,
                                               std::memory_order_relaxed);
                    g_stdAudioRouterVtable.store(objectGraph->object58Vtable,
                                                 std::memory_order_relaxed);
                }
            }
        }

        bool shouldLog = false;
        if (snapshot.has_value())
        {
            const std::uint64_t payloadKey =
                (static_cast<std::uint64_t>(snapshot->entryPoint) << 32) ^
                static_cast<std::uint64_t>(snapshot->tag) ^
                (static_cast<std::uint64_t>(snapshot->stateByte) << 24);
            shouldLog = LogLegacyThreadWrapperPayloadOnce(payloadKey);
        }
        else
        {
            shouldLog = LogCallerOnce(g_loggedLegacyThreadWrapperCreateCallers, caller);
        }

        if (!shouldLog)
        {
            return thread;
        }

        std::ostringstream message;
        message << "Legacy thread wrapper create observed: thread="
                << FormatAddress(reinterpret_cast<std::uintptr_t>(thread))
                << ", threadId=" << (threadId != nullptr ? *threadId : 0)
                << ", createSuspended="
                << ((creationFlags & CREATE_SUSPENDED) == CREATE_SUSPENDED)
                << ", stackSize=" << stackSize << ", start="
                << FormatAddress(reinterpret_cast<std::uintptr_t>(startAddress))
                << ", parameter=" << FormatAddress(reinterpret_cast<std::uintptr_t>(parameter));
        if (snapshot.has_value())
        {
            message << ", "
                    << DescribeLegacyThreadWrapperSnapshot(
                           *snapshot,
                           g_engineModule != nullptr
                               ? reinterpret_cast<std::uintptr_t>(g_engineModule)
                               : 0);

            if (tagText.has_value())
            {
                message << ", payload.tagText=\"" << *tagText << '"';
                message << ", payload.role=" << DescribeLegacyThreadRole(role);
            }

            if (objectGraph.has_value())
            {
                message << ", "
                        << DescribeLegacyThreadWrapperObjectGraphSnapshot(
                               *objectGraph,
                               g_engineModule != nullptr
                                   ? reinterpret_cast<std::uintptr_t>(g_engineModule)
                                   : 0);
            }

            std::uintptr_t payloadThreadId = 0;
            if (TryReadPointerValue(static_cast<const std::uint8_t*>(parameter) + 0x4,
                                    payloadThreadId))
            {
                message << ", payload.threadId=" << payloadThreadId;
            }
        }
        if (thread == nullptr)
        {
            message << ", error=" << GetLastError();
        }
        Log(message.str());
    }

    return thread;
}

DWORD WINAPI HookedResumeThread(HANDLE threadHandle)
{
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (g_originalResumeThread == nullptr)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return static_cast<DWORD>(-1);
    }

    const std::uintptr_t legacyResumeCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) + kLegacyThreadWrapperResumeReturnRva
            : 0;
    const std::optional<bool> threadExited = TryIsThreadHandleSignaled(threadHandle);
    const bool targetsCurrentThread = TargetsCurrentThread(threadHandle);
    const LegacyThreadControlPlan plan =
        BuildLegacyThreadControlPlan(g_config.hardenLegacyThreadWrapper,
                                     caller == legacyResumeCaller,
                                     threadExited.value_or(false), targetsCurrentThread);
    if (plan.suppressOperation)
    {
        if (LogCallerOnce(g_loggedLegacyThreadResumeSuppressCallers, caller))
        {
            std::ostringstream message;
            message << "Legacy thread wrapper resume swallowed: reason="
                    << (targetsCurrentThread ? "self-target" : "thread-exited")
                    << ", handle=" << FormatAddress(reinterpret_cast<std::uintptr_t>(threadHandle))
                    << ", caller=" << FormatAddress(caller);
            Log(message.str());
        }
        SetLastError(targetsCurrentThread ? ERROR_INVALID_PARAMETER : ERROR_ACCESS_DENIED);
        return static_cast<DWORD>(-1);
    }

    return g_originalResumeThread(threadHandle);
}

DWORD WINAPI HookedSuspendThread(HANDLE threadHandle)
{
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (g_originalSuspendThread == nullptr)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return static_cast<DWORD>(-1);
    }

    const std::uintptr_t legacySuspendCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) +
                  kLegacyThreadWrapperSuspendReturnRva
            : 0;
    const std::optional<bool> threadExited = TryIsThreadHandleSignaled(threadHandle);
    const bool targetsCurrentThread = TargetsCurrentThread(threadHandle);
    const LegacyThreadControlPlan plan =
        BuildLegacyThreadControlPlan(g_config.hardenLegacyThreadWrapper,
                                     caller == legacySuspendCaller,
                                     threadExited.value_or(false), targetsCurrentThread);
    if (plan.suppressOperation)
    {
        if (LogCallerOnce(g_loggedLegacyThreadSuspendSuppressCallers, caller))
        {
            std::ostringstream message;
            message << "Legacy thread wrapper suspend swallowed: reason="
                    << (targetsCurrentThread ? "self-target" : "thread-exited")
                    << ", handle=" << FormatAddress(reinterpret_cast<std::uintptr_t>(threadHandle))
                    << ", caller=" << FormatAddress(caller);
            Log(message.str());
        }
        SetLastError(targetsCurrentThread ? ERROR_INVALID_PARAMETER : ERROR_ACCESS_DENIED);
        return static_cast<DWORD>(-1);
    }

    return g_originalSuspendThread(threadHandle);
}

BOOL WINAPI HookedTerminateThread(HANDLE threadHandle, DWORD exitCode)
{
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (g_originalTerminateThread == nullptr)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return FALSE;
    }

    const std::uintptr_t legacyTerminateCaller =
        g_engineModule != nullptr
            ? reinterpret_cast<std::uintptr_t>(g_engineModule) +
                  kLegacyThreadWrapperTerminateReturnRva
            : 0;
    const std::optional<bool> threadExited = TryIsThreadHandleSignaled(threadHandle);
    const bool targetsCurrentThread = TargetsCurrentThread(threadHandle);
    const LegacyThreadTerminatePlan plan =
        BuildLegacyThreadTerminatePlan(g_config.hardenLegacyThreadWrapper,
                                       g_config.legacyThreadTerminateGraceMilliseconds,
                                       caller == legacyTerminateCaller,
                                       threadExited.value_or(false), targetsCurrentThread);
    if (plan.skipTerminateCall)
    {
        if (LogCallerOnce(g_loggedLegacyThreadTerminateSoftenCallers, caller))
        {
            std::ostringstream message;
            message << "Legacy thread wrapper terminate swallowed: reason="
                    << (targetsCurrentThread ? "self-target" : "thread-exited")
                    << ", handle=" << FormatAddress(reinterpret_cast<std::uintptr_t>(threadHandle))
                    << ", caller=" << FormatAddress(caller);
            Log(message.str());
        }
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    if (plan.graceWaitMs > 0 && threadHandle != nullptr)
    {
        const DWORD waitResult = WaitForSingleObject(threadHandle, plan.graceWaitMs);
        if (waitResult == WAIT_OBJECT_0)
        {
            if (LogCallerOnce(g_loggedLegacyThreadTerminateSoftenCallers, caller))
            {
                std::ostringstream message;
                message << "Legacy thread wrapper terminate softened: graceWaitMs="
                        << plan.graceWaitMs << ", handle="
                        << FormatAddress(reinterpret_cast<std::uintptr_t>(threadHandle))
                        << ", caller=" << FormatAddress(caller);
                Log(message.str());
            }
            SetLastError(ERROR_SUCCESS);
            return TRUE;
        }

        if (waitResult == WAIT_FAILED &&
            LogCallerOnce(g_loggedLegacyThreadTerminateFailureCallers, caller))
        {
            std::ostringstream message;
            message << "Legacy thread wrapper terminate grace wait failed; falling back: handle="
                    << FormatAddress(reinterpret_cast<std::uintptr_t>(threadHandle))
                    << ", caller=" << FormatAddress(caller)
                    << ", error=" << GetLastError();
            Log(message.str());
        }
        else if (waitResult == WAIT_TIMEOUT &&
                 LogCallerOnce(g_loggedLegacyThreadTerminateFallbackCallers, caller))
        {
            std::ostringstream message;
            message << "Legacy thread wrapper terminate fell back after grace wait: graceWaitMs="
                    << plan.graceWaitMs << ", handle="
                    << FormatAddress(reinterpret_cast<std::uintptr_t>(threadHandle))
                    << ", caller=" << FormatAddress(caller);
            Log(message.str());
        }
    }

    return g_originalTerminateThread(threadHandle, exitCode);
}

VOID WINAPI HookedShvSleep(DWORD milliseconds)
{
    g_shvSleepHitCount.fetch_add(1, std::memory_order_relaxed);
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const DWORD adjustedMilliseconds =
        ResolveThirdPartyFramePacingDelayMs(g_config.enableFrameRateUnlock,
                                            g_config.targetFrameRate, milliseconds);
    const std::uint32_t sampleIndex =
        g_shvSleepLogCount.fetch_add(1, std::memory_order_relaxed);
    if (sampleIndex < kSchedulerTimingLogSampleLimit ||
        (adjustedMilliseconds != milliseconds &&
         sampleIndex < (kSchedulerTimingLogSampleLimit + kRenderCadenceOutlierLogLimit)))
    {
        std::ostringstream message;
        message << "shv.dll Sleep "
                << ((adjustedMilliseconds != milliseconds && sampleIndex >= kSchedulerTimingLogSampleLimit)
                        ? "override "
                        : "sample ")
                << (sampleIndex + 1) << ": " << milliseconds << "->"
                << adjustedMilliseconds << "ms, caller=" << FormatAddress(caller);
        Log(message.str());
    }

    if (g_originalShvSleep != nullptr)
    {
        g_originalShvSleep(adjustedMilliseconds);
        return;
    }

    ::Sleep(adjustedMilliseconds);
}

UINT_PTR WINAPI HookedShvSetTimer(HWND window, UINT_PTR timerId, UINT elapse,
                                  TIMERPROC timerProc)
{
    g_shvSetTimerHitCount.fetch_add(1, std::memory_order_relaxed);
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    const UINT adjustedElapse =
        std::max(ResolveThirdPartyFramePacingDelayMs(g_config.enableFrameRateUnlock,
                                                     g_config.targetFrameRate, elapse),
                 1u);
    const std::uint32_t sampleIndex =
        g_shvSetTimerLogCount.fetch_add(1, std::memory_order_relaxed);
    if (sampleIndex < kSchedulerTimingLogSampleLimit ||
        (adjustedElapse != elapse &&
         sampleIndex < (kSchedulerTimingLogSampleLimit + kRenderCadenceOutlierLogLimit)))
    {
        std::ostringstream message;
        message << "shv.dll SetTimer "
                << ((adjustedElapse != elapse && sampleIndex >= kSchedulerTimingLogSampleLimit)
                        ? "override "
                        : "sample ")
                << (sampleIndex + 1) << ": elapse=" << elapse << "->" << adjustedElapse
                << "ms, caller=" << FormatAddress(caller);
        Log(message.str());
    }

    if (g_originalShvSetTimer != nullptr)
    {
        return g_originalShvSetTimer(window, timerId, adjustedElapse, timerProc);
    }

    return ::SetTimer(window, timerId, adjustedElapse, timerProc);
}

FARPROC WINAPI HookedShvGetProcAddress(HMODULE module, LPCSTR procName)
{
    if (g_originalShvGetProcAddress == nullptr)
    {
        return nullptr;
    }

    const FARPROC resolved = g_originalShvGetProcAddress(module, procName);
    if (procName == nullptr || IsOrdinalProcName(procName))
    {
        return resolved;
    }

    const D3DProcRedirectKind redirectKind =
        ClassifyD3DProcRedirect(ResolveModulePathForHandle(module), procName);
    switch (redirectKind)
    {
    case D3DProcRedirectKind::direct3d_create9:
        if (g_originalDirect3DCreate9 == nullptr && resolved != nullptr)
        {
            g_originalDirect3DCreate9 = reinterpret_cast<Direct3DCreate9Fn>(resolved);
        }
        std::call_once(g_shvCreate9RedirectLogOnce, [resolved]() {
            std::ostringstream message;
            message << "shv.dll GetProcAddress redirected Direct3DCreate9: original="
                    << FormatAddress(reinterpret_cast<std::uintptr_t>(resolved))
                    << ", replacement="
                    << FormatAddress(reinterpret_cast<std::uintptr_t>(&HookedDirect3DCreate9));
            Log(message.str());
        });
        return reinterpret_cast<FARPROC>(&HookedDirect3DCreate9);

    case D3DProcRedirectKind::direct3d_create9_ex:
        if (g_originalDirect3DCreate9Ex == nullptr && resolved != nullptr)
        {
            g_originalDirect3DCreate9Ex = reinterpret_cast<Direct3DCreate9ExFn>(resolved);
        }
        std::call_once(g_shvCreate9ExRedirectLogOnce, [resolved]() {
            std::ostringstream message;
            message << "shv.dll GetProcAddress redirected Direct3DCreate9Ex: original="
                    << FormatAddress(reinterpret_cast<std::uintptr_t>(resolved))
                    << ", replacement="
                    << FormatAddress(reinterpret_cast<std::uintptr_t>(&HookedDirect3DCreate9Ex));
            Log(message.str());
        });
        return reinterpret_cast<FARPROC>(&HookedDirect3DCreate9Ex);

    case D3DProcRedirectKind::none:
    default:
        return resolved;
    }
}

void __fastcall HookedShvMainLoop(void* self, void* /*edx*/, std::uintptr_t arg1,
                                  std::uintptr_t arg2)
{
    g_shvMainLoopHitCount.fetch_add(1, std::memory_order_relaxed);
    LogRenderCadenceSample("shv.dll MainLoop", g_shvMainLoopCadence);

    if (g_originalShvMainLoop != nullptr)
    {
        g_originalShvMainLoop(self, nullptr, arg1, arg2);
    }
}

VOID WINAPI HookedExitProcess(UINT exitCode)
{
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (LogCallerOnce(g_loggedExitProcessCallers, caller))
    {
        std::ostringstream message;
        message << "ExitProcess requested: exitCode=" << exitCode
                << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    if (ShouldSuppressLostDeviceShutdownNow())
    {
        Log("Suppressed ExitProcess during lost-device recovery window.");
        return;
    }

    if (g_originalExitProcess != nullptr)
    {
        g_originalExitProcess(exitCode);
        return;
    }

    ::ExitProcess(exitCode);
}

BOOL WINAPI HookedTerminateProcess(HANDLE processHandle, UINT exitCode)
{
    DWORD targetProcessId = 0;
    if (processHandle == GetCurrentProcess())
    {
        targetProcessId = GetCurrentProcessId();
    }
    else if (processHandle != nullptr && processHandle != INVALID_HANDLE_VALUE)
    {
        targetProcessId = GetProcessId(processHandle);
    }

    const bool terminatingCurrentProcess =
        targetProcessId != 0 && targetProcessId == GetCurrentProcessId();
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (terminatingCurrentProcess && LogCallerOnce(g_loggedTerminateProcessCallers, caller))
    {
        std::ostringstream message;
        message << "TerminateProcess requested for current process: exitCode=" << exitCode
                << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    if (terminatingCurrentProcess && ShouldSuppressLostDeviceShutdownNow())
    {
        Log("Suppressed TerminateProcess during lost-device recovery window.");
        return TRUE;
    }

    if (g_originalTerminateProcess != nullptr)
    {
        return g_originalTerminateProcess(processHandle, exitCode);
    }

    return ::TerminateProcess(processHandle, exitCode);
}

VOID WINAPI HookedRaiseException(DWORD exceptionCode, DWORD exceptionFlags,
                                 DWORD numberOfArguments, const ULONG_PTR* arguments)
{
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (LogCallerOnce(g_loggedRaiseExceptionCallers, caller))
    {
        std::ostringstream message;
        message << "RaiseException requested: code=0x" << std::hex << exceptionCode
                << std::dec << ", flags=0x" << std::hex << exceptionFlags << std::dec
                << ", argCount=" << numberOfArguments
                << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    if (g_originalRaiseException != nullptr)
    {
        g_originalRaiseException(exceptionCode, exceptionFlags, numberOfArguments, arguments);
        return;
    }

    ::RaiseException(exceptionCode, exceptionFlags, numberOfArguments, arguments);
}

VOID WINAPI HookedPostQuitMessage(int exitCode)
{
    const auto caller = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (LogCallerOnce(g_loggedPostQuitMessageCallers, caller))
    {
        std::ostringstream message;
        message << "PostQuitMessage requested: exitCode=" << exitCode
                << ", caller=" << FormatAddress(caller);
        Log(message.str());
    }

    if (ShouldSuppressLostDeviceShutdownNow())
    {
        Log("Suppressed PostQuitMessage during lost-device recovery window.");
        return;
    }

    if (g_originalPostQuitMessage != nullptr)
    {
        g_originalPostQuitMessage(exitCode);
        return;
    }

    ::PostQuitMessage(exitCode);
}

HRESULT WINAPI HookedDirectInput8Create(HINSTANCE instance, DWORD version, REFIID riid,
                                        LPVOID* outInterface, LPUNKNOWN outerUnknown)
{
    if (g_originalDirectInput8Create == nullptr)
    {
        return DIERR_GENERIC;
    }

    const HRESULT hr =
        g_originalDirectInput8Create(instance, version, riid, outInterface, outerUnknown);
    if (FAILED(hr) || outInterface == nullptr || *outInterface == nullptr)
    {
        return hr;
    }

    std::scoped_lock lock(g_hookMutex);
    if (InlineIsEqualGUID(riid, IID_IDirectInput8A))
    {
        if (PatchVtableEntry(*outInterface, kDirectInputCreateDeviceVtableIndex,
                             reinterpret_cast<void*>(&HookedDirectInputCreateDeviceA),
                             reinterpret_cast<void**>(&g_originalDirectInputCreateDeviceA)))
        {
            Log("DirectInput8Create hook installed for ANSI interface.");
        }
    }
    else if (InlineIsEqualGUID(riid, IID_IDirectInput8W))
    {
        if (PatchVtableEntry(*outInterface, kDirectInputCreateDeviceVtableIndex,
                             reinterpret_cast<void*>(&HookedDirectInputCreateDeviceW),
                             reinterpret_cast<void**>(&g_originalDirectInputCreateDeviceW)))
        {
            Log("DirectInput8Create hook installed for Unicode interface.");
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE HookedDirectInputCreateDeviceA(IDirectInput8A* directInput, REFGUID guid,
                                                         LPDIRECTINPUTDEVICE8A* device,
                                                         LPUNKNOWN outerUnknown)
{
    if (g_originalDirectInputCreateDeviceA == nullptr)
    {
        return DIERR_GENERIC;
    }

    const HRESULT hr =
        g_originalDirectInputCreateDeviceA(directInput, guid, device, outerUnknown);
    if (SUCCEEDED(hr) && device != nullptr && *device != nullptr && IsMouseGuid(guid))
    {
        std::scoped_lock lock(g_hookMutex);
        InstallDirectInputMouseHooksA(*device);
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE HookedDirectInputCreateDeviceW(IDirectInput8W* directInput, REFGUID guid,
                                                         LPDIRECTINPUTDEVICE8W* device,
                                                         LPUNKNOWN outerUnknown)
{
    if (g_originalDirectInputCreateDeviceW == nullptr)
    {
        return DIERR_GENERIC;
    }

    const HRESULT hr =
        g_originalDirectInputCreateDeviceW(directInput, guid, device, outerUnknown);
    if (SUCCEEDED(hr) && device != nullptr && *device != nullptr && IsMouseGuid(guid))
    {
        std::scoped_lock lock(g_hookMutex);
        InstallDirectInputMouseHooksW(*device);
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE HookedDirectInputGetDeviceStateA(IDirectInputDevice8A* device,
                                                           DWORD stateSize, LPVOID state)
{
    if (g_originalDirectInputGetDeviceStateA == nullptr)
    {
        return DIERR_GENERIC;
    }

    const HRESULT hr = g_originalDirectInputGetDeviceStateA(device, stateSize, state);
    const bool isMouseDevice = IsMouseDevice(device);
    const DirectInputMouseStateRecoveryPlan recoveryPlan =
        BuildMouseStateRecoveryPlan(g_config.hardenDirectInputMouseDevice &&
                                        g_config.enableRawMouseInput,
                                    isMouseDevice, hr,
                                    HasDirectInputMouseStateBuffer(stateSize, state));
    const HRESULT effectiveHr = recoveryPlan.overrideResult ? recoveryPlan.result : hr;
    if (recoveryPlan.zeroState)
    {
        g_directInputRecoveryCount.fetch_add(1, std::memory_order_relaxed);
        std::memset(state, 0, stateSize);
        std::call_once(g_directInputMouseRecoveryLogOnce, []() {
            Log("DirectInput mouse reacquire churn smoothed with zero-state recovery.");
        });
    }

    if (FAILED(effectiveHr) || !g_config.enableRawMouseInput || !isMouseDevice)
    {
        return effectiveHr;
    }

    const long deltaX = g_rawMouseDeltaX.exchange(0);
    const long deltaY = g_rawMouseDeltaY.exchange(0);
    long adjustedDeltaX = deltaX;
    long adjustedDeltaY = deltaY;
    TransformRawMouseDelta(g_config.rawMouseSensitivity, g_config.invertRawMouseY,
                           &adjustedDeltaX, &adjustedDeltaY);
    if ((adjustedDeltaX != 0 || adjustedDeltaY != 0) &&
        ApplyRawMouseDelta(state, stateSize, adjustedDeltaX, adjustedDeltaY))
    {
        g_rawMouseOverrideCount.fetch_add(1, std::memory_order_relaxed);
        std::call_once(g_rawMouseLogOnce, []() { Log("Raw mouse delta override active."); });
    }

    return effectiveHr;
}

HRESULT STDMETHODCALLTYPE HookedDirectInputGetDeviceStateW(IDirectInputDevice8W* device,
                                                           DWORD stateSize, LPVOID state)
{
    if (g_originalDirectInputGetDeviceStateW == nullptr)
    {
        return DIERR_GENERIC;
    }

    const HRESULT hr = g_originalDirectInputGetDeviceStateW(device, stateSize, state);
    const bool isMouseDevice = IsMouseDevice(device);
    const DirectInputMouseStateRecoveryPlan recoveryPlan =
        BuildMouseStateRecoveryPlan(g_config.hardenDirectInputMouseDevice &&
                                        g_config.enableRawMouseInput,
                                    isMouseDevice, hr,
                                    HasDirectInputMouseStateBuffer(stateSize, state));
    const HRESULT effectiveHr = recoveryPlan.overrideResult ? recoveryPlan.result : hr;
    if (recoveryPlan.zeroState)
    {
        g_directInputRecoveryCount.fetch_add(1, std::memory_order_relaxed);
        std::memset(state, 0, stateSize);
        std::call_once(g_directInputMouseRecoveryLogOnce, []() {
            Log("DirectInput mouse reacquire churn smoothed with zero-state recovery.");
        });
    }

    if (FAILED(effectiveHr) || !g_config.enableRawMouseInput || !isMouseDevice)
    {
        return effectiveHr;
    }

    const long deltaX = g_rawMouseDeltaX.exchange(0);
    const long deltaY = g_rawMouseDeltaY.exchange(0);
    long adjustedDeltaX = deltaX;
    long adjustedDeltaY = deltaY;
    TransformRawMouseDelta(g_config.rawMouseSensitivity, g_config.invertRawMouseY,
                           &adjustedDeltaX, &adjustedDeltaY);
    if ((adjustedDeltaX != 0 || adjustedDeltaY != 0) &&
        ApplyRawMouseDelta(state, stateSize, adjustedDeltaX, adjustedDeltaY))
    {
        g_rawMouseOverrideCount.fetch_add(1, std::memory_order_relaxed);
        std::call_once(g_rawMouseLogOnce, []() { Log("Raw mouse delta override active."); });
    }

    return effectiveHr;
}

HRESULT STDMETHODCALLTYPE HookedDirectInputSetCooperativeLevelA(IDirectInputDevice8A* device,
                                                                HWND window, DWORD flags)
{
    if (g_originalDirectInputSetCooperativeLevelA == nullptr)
    {
        return DIERR_GENERIC;
    }

    const bool isMouseDevice = IsMouseDevice(device);
    const DWORD effectiveFlags = SanitizeMouseCooperativeFlags(
        g_config.hardenDirectInputMouseDevice && g_config.enableRawMouseInput, isMouseDevice,
        flags);
    if (effectiveFlags != flags)
    {
        std::call_once(g_directInputMouseCoopLogOnce, [flags, effectiveFlags]() {
            std::ostringstream message;
            message << "DirectInput mouse cooperative level normalized: flags=0x" << std::hex
                    << flags << "->0x" << effectiveFlags << std::dec;
            Log(message.str());
        });
    }

    const HRESULT hr = g_originalDirectInputSetCooperativeLevelA(device, window, effectiveFlags);
    if (SUCCEEDED(hr) && g_config.enableRawMouseInput && isMouseDevice)
    {
        EnsureRawInputWindow(window);
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE HookedDirectInputSetCooperativeLevelW(IDirectInputDevice8W* device,
                                                                HWND window, DWORD flags)
{
    if (g_originalDirectInputSetCooperativeLevelW == nullptr)
    {
        return DIERR_GENERIC;
    }

    const bool isMouseDevice = IsMouseDevice(device);
    const DWORD effectiveFlags = SanitizeMouseCooperativeFlags(
        g_config.hardenDirectInputMouseDevice && g_config.enableRawMouseInput, isMouseDevice,
        flags);
    if (effectiveFlags != flags)
    {
        std::call_once(g_directInputMouseCoopLogOnce, [flags, effectiveFlags]() {
            std::ostringstream message;
            message << "DirectInput mouse cooperative level normalized: flags=0x" << std::hex
                    << flags << "->0x" << effectiveFlags << std::dec;
            Log(message.str());
        });
    }

    const HRESULT hr = g_originalDirectInputSetCooperativeLevelW(device, window, effectiveFlags);
    if (SUCCEEDED(hr) && g_config.enableRawMouseInput && isMouseDevice)
    {
        EnsureRawInputWindow(window);
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE HookedReset(IDirect3DDevice9* device,
                                      D3DPRESENT_PARAMETERS* presentationParameters)
{
    if (g_originalReset == nullptr || presentationParameters == nullptr)
    {
        return g_originalReset != nullptr
                   ? g_originalReset(device, presentationParameters)
                   : E_FAIL;
    }

    if (!g_config.enableSafeResolutionChanges)
    {
        return g_originalReset(device, presentationParameters);
    }

    D3DPRESENT_PARAMETERS workingParams = *presentationParameters;
    FillMissingBorderlessDimensions(workingParams.hDeviceWindow, workingParams);
    const PresentParams original = ToPresentParams(workingParams);
    const PresentParams sanitized = SanitizeForReset(original, g_config);
    ApplyPresentParams(sanitized, workingParams);

    {
        std::ostringstream message;
        message << "Reset intercepted: " << sanitized.backBufferWidth << 'x'
                << sanitized.backBufferHeight << ", windowed=" << sanitized.windowed
                << ", refresh=" << original.refreshRate << "->" << sanitized.refreshRate
                << ", swapEffect=" << FormatSwapEffect(original.swapEffect) << "->"
                << FormatSwapEffect(sanitized.swapEffect)
                << ", interval="
                << FormatPresentationInterval(original.presentationInterval) << "->"
                << FormatPresentationInterval(sanitized.presentationInterval);
        Log(message.str());
    }

    HRESULT hr = g_originalReset(device, &workingParams);
    if (SUCCEEDED(hr))
    {
        g_lastDeviceAddress = reinterpret_cast<std::uintptr_t>(device);
        UpdateBackbufferDimensions(ToPresentParams(workingParams));
        InstallSwapChainPresentHook(device);
        InstallDeviceExHooks(device);
        InstallGlobalD3DCodeHooks();
        EnsureGameWindowHook(workingParams.hDeviceWindow);
        ApplyBorderlessWindow(workingParams.hDeviceWindow);
        ClearLostDeviceRecoveryWindow("reset");
        return hr;
    }

    NoteLostDeviceSignalFromApi("Reset", hr);

    if (!g_config.retryResetInWindowedMode)
    {
        return hr;
    }

    const PresentParams retry = BuildRetryResetParams(original, g_config);
    if (SamePresentParams(retry, sanitized))
    {
        Log("Reset failed without alternative retry path, result=" + FormatHr(hr));
        return hr;
    }

    D3DPRESENT_PARAMETERS retryParams = *presentationParameters;
    FillMissingBorderlessDimensions(retryParams.hDeviceWindow, retryParams);
    ApplyPresentParams(retry, retryParams);

    const HRESULT retryHr = g_originalReset(device, &retryParams);
    if (SUCCEEDED(retryHr))
    {
        g_lastDeviceAddress = reinterpret_cast<std::uintptr_t>(device);
        UpdateBackbufferDimensions(ToPresentParams(retryParams));
        InstallSwapChainPresentHook(device);
        InstallDeviceExHooks(device);
        InstallGlobalD3DCodeHooks();
        EnsureGameWindowHook(retryParams.hDeviceWindow);
        ApplyBorderlessWindow(retryParams.hDeviceWindow);
        ClearLostDeviceRecoveryWindow("reset_retry");
    }
    else
    {
        NoteLostDeviceSignalFromApi("ResetRetry", retryHr);
    }
    Log("Reset retry attempted, initial=" + FormatHr(hr) +
        ", retry=" + FormatHr(retryHr));
    return retryHr;
}

HRESULT STDMETHODCALLTYPE HookedTestCooperativeLevel(IDirect3DDevice9* device)
{
    if (g_originalTestCooperativeLevel == nullptr)
    {
        return E_FAIL;
    }

    const HRESULT hr = g_originalTestCooperativeLevel(device);
    if (SUCCEEDED(hr))
    {
        ClearLostDeviceRecoveryWindow("cooperative_level_ok");
        return hr;
    }

    NoteLostDeviceSignalFromApi("TestCooperativeLevel", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE HookedResetEx(IDirect3DDevice9Ex* device,
                                        D3DPRESENT_PARAMETERS* presentationParameters,
                                        D3DDISPLAYMODEEX* fullscreenDisplayMode)
{
    if (g_originalResetEx == nullptr || presentationParameters == nullptr)
    {
        return g_originalResetEx != nullptr
                   ? g_originalResetEx(device, presentationParameters, fullscreenDisplayMode)
                   : E_FAIL;
    }

    if (!g_config.enableSafeResolutionChanges)
    {
        return g_originalResetEx(device, presentationParameters, fullscreenDisplayMode);
    }

    D3DPRESENT_PARAMETERS workingParams = *presentationParameters;
    FillMissingBorderlessDimensions(workingParams.hDeviceWindow, workingParams);
    const PresentParams original = ToPresentParams(workingParams);
    const PresentParams sanitized = SanitizeForReset(original, g_config, true);
    ApplyPresentParams(sanitized, workingParams);

    {
        std::ostringstream message;
        message << "ResetEx intercepted: " << sanitized.backBufferWidth << 'x'
                << sanitized.backBufferHeight << ", windowed=" << sanitized.windowed
                << ", refresh=" << original.refreshRate << "->" << sanitized.refreshRate
                << ", swapEffect=" << FormatSwapEffect(original.swapEffect) << "->"
                << FormatSwapEffect(sanitized.swapEffect)
                << ", interval="
                << FormatPresentationInterval(original.presentationInterval) << "->"
                << FormatPresentationInterval(sanitized.presentationInterval);
        Log(message.str());
    }

    HRESULT hr = g_originalResetEx(device, &workingParams, fullscreenDisplayMode);
    if (SUCCEEDED(hr))
    {
        g_lastDeviceAddress = reinterpret_cast<std::uintptr_t>(device);
        UpdateBackbufferDimensions(ToPresentParams(workingParams));
        InstallSwapChainPresentHook(device);
        InstallDeviceExHooks(device);
        InstallGlobalD3DCodeHooks();
        EnsureGameWindowHook(workingParams.hDeviceWindow);
        ApplyBorderlessWindow(workingParams.hDeviceWindow);
        ClearLostDeviceRecoveryWindow("reset_ex");
        return hr;
    }

    NoteLostDeviceSignalFromApi("ResetEx", hr);

    if (!g_config.retryResetInWindowedMode)
    {
        return hr;
    }

    const PresentParams retry = BuildRetryResetParams(original, g_config, true);
    if (SamePresentParams(retry, sanitized))
    {
        Log("ResetEx failed without alternative retry path, result=" + FormatHr(hr));
        return hr;
    }

    D3DPRESENT_PARAMETERS retryParams = *presentationParameters;
    FillMissingBorderlessDimensions(retryParams.hDeviceWindow, retryParams);
    ApplyPresentParams(retry, retryParams);

    const HRESULT retryHr = g_originalResetEx(device, &retryParams, nullptr);
    if (SUCCEEDED(retryHr))
    {
        g_lastDeviceAddress = reinterpret_cast<std::uintptr_t>(device);
        UpdateBackbufferDimensions(ToPresentParams(retryParams));
        InstallSwapChainPresentHook(device);
        InstallDeviceExHooks(device);
        InstallGlobalD3DCodeHooks();
        EnsureGameWindowHook(retryParams.hDeviceWindow);
        ApplyBorderlessWindow(retryParams.hDeviceWindow);
        ClearLostDeviceRecoveryWindow("reset_ex_retry");
    }
    else
    {
        NoteLostDeviceSignalFromApi("ResetExRetry", retryHr);
    }
    Log("ResetEx retry attempted, initial=" + FormatHr(hr) +
        ", retry=" + FormatHr(retryHr));
    return retryHr;
}

HRESULT STDMETHODCALLTYPE HookedPresent(IDirect3DDevice9* device, const RECT* sourceRect,
                                        const RECT* destRect, HWND destWindowOverride,
                                        const RGNDATA* dirtyRegion)
{
    ApplyCurrentRenderThreadExecutionPolicy();
    const auto originalPresent = reinterpret_cast<PresentFn>(ResolveRenderCallTarget(
        RenderHookTargets{
            .vtableOriginal = reinterpret_cast<std::uintptr_t>(g_originalPresent),
            .codeGateway = reinterpret_cast<std::uintptr_t>(g_originalPresentCodeGateway),
        }));
    if (originalPresent == nullptr)
    {
        return E_FAIL;
    }

    LARGE_INTEGER startCounter = {};
    const bool measuredStart = ::QueryPerformanceCounter(&startCounter) != FALSE;
    g_devicePresentHitCount.fetch_add(1, std::memory_order_relaxed);
    const HRESULT hr =
        originalPresent(device, sourceRect, destRect, destWindowOverride, dirtyRegion);
    double callDurationMs = -1.0;
    if (measuredStart)
    {
        LARGE_INTEGER endCounter = {};
        if (::QueryPerformanceCounter(&endCounter) != FALSE)
        {
            callDurationMs =
                MeasureHighPrecisionDurationMilliseconds(startCounter, endCounter);
        }
    }

    LogRenderCadenceSample("Device::Present", g_devicePresentCadence, callDurationMs);
    NoteLostDeviceSignalFromApi("Present", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE HookedPresentEx(IDirect3DDevice9Ex* device, const RECT* sourceRect,
                                          const RECT* destRect, HWND destWindowOverride,
                                          const RGNDATA* dirtyRegion, DWORD flags)
{
    ApplyCurrentRenderThreadExecutionPolicy();
    if (g_originalPresentEx == nullptr)
    {
        return E_FAIL;
    }

    LARGE_INTEGER startCounter = {};
    const bool measuredStart = ::QueryPerformanceCounter(&startCounter) != FALSE;
    g_devicePresentExHitCount.fetch_add(1, std::memory_order_relaxed);
    const HRESULT hr = g_originalPresentEx(device, sourceRect, destRect, destWindowOverride,
                                           dirtyRegion, flags);
    double callDurationMs = -1.0;
    if (measuredStart)
    {
        LARGE_INTEGER endCounter = {};
        if (::QueryPerformanceCounter(&endCounter) != FALSE)
        {
            callDurationMs =
                MeasureHighPrecisionDurationMilliseconds(startCounter, endCounter);
        }
    }

    LogRenderCadenceSample("Device::PresentEx", g_devicePresentExCadence, callDurationMs);
    NoteLostDeviceSignalFromApi("PresentEx", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE HookedSwapChainPresent(IDirect3DSwapChain9* swapChain,
                                                 const RECT* sourceRect,
                                                 const RECT* destRect,
                                                 HWND destWindowOverride,
                                                 const RGNDATA* dirtyRegion,
                                                 DWORD flags)
{
    ApplyCurrentRenderThreadExecutionPolicy();
    const auto originalSwapChainPresent =
        reinterpret_cast<SwapChainPresentFn>(ResolveRenderCallTarget(
            RenderHookTargets{
                .vtableOriginal =
                    reinterpret_cast<std::uintptr_t>(g_originalSwapChainPresent),
                .codeGateway = reinterpret_cast<std::uintptr_t>(
                    g_originalSwapChainPresentCodeGateway),
            }));
    if (originalSwapChainPresent == nullptr)
    {
        return E_FAIL;
    }

    LARGE_INTEGER startCounter = {};
    const bool measuredStart = ::QueryPerformanceCounter(&startCounter) != FALSE;
    g_swapChainPresentHitCount.fetch_add(1, std::memory_order_relaxed);
    const HRESULT hr = originalSwapChainPresent(swapChain, sourceRect, destRect,
                                                destWindowOverride, dirtyRegion, flags);
    double callDurationMs = -1.0;
    if (measuredStart)
    {
        LARGE_INTEGER endCounter = {};
        if (::QueryPerformanceCounter(&endCounter) != FALSE)
        {
            callDurationMs =
                MeasureHighPrecisionDurationMilliseconds(startCounter, endCounter);
        }
    }

    LogRenderCadenceSample("SwapChain::Present", g_swapChainPresentCadence, callDurationMs);
    NoteLostDeviceSignalFromApi("SwapChainPresent", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE HookedEndScene(IDirect3DDevice9* device)
{
    ApplyCurrentRenderThreadExecutionPolicy();
    const auto originalEndScene = reinterpret_cast<EndSceneFn>(ResolveRenderCallTarget(
        RenderHookTargets{
            .vtableOriginal = reinterpret_cast<std::uintptr_t>(g_originalEndScene),
            .codeGateway = reinterpret_cast<std::uintptr_t>(g_originalEndSceneCodeGateway),
        }));
    if (originalEndScene == nullptr)
    {
        return E_FAIL;
    }

    LARGE_INTEGER startCounter = {};
    const bool measuredStart = ::QueryPerformanceCounter(&startCounter) != FALSE;
    g_endSceneHitCount.fetch_add(1, std::memory_order_relaxed);
    const HRESULT hr = originalEndScene(device);
    double callDurationMs = -1.0;
    if (measuredStart)
    {
        LARGE_INTEGER endCounter = {};
        if (::QueryPerformanceCounter(&endCounter) != FALSE)
        {
            callDurationMs =
                MeasureHighPrecisionDurationMilliseconds(startCounter, endCounter);
        }
    }

    LogRenderCadenceSample("Device::EndScene", g_endSceneCadence, callDurationMs);
    return hr;
}

HRESULT STDMETHODCALLTYPE HookedSetTransform(IDirect3DDevice9* device,
                                             D3DTRANSFORMSTATETYPE state,
                                             const D3DMATRIX* matrix)
{
    if (g_originalSetTransform == nullptr || state != D3DTS_PROJECTION || matrix == nullptr)
    {
        return g_originalSetTransform != nullptr
                   ? g_originalSetTransform(device, state, matrix)
                   : E_FAIL;
    }

    D3DVIEWPORT9 viewport = {};
    if (FAILED(device->GetViewport(&viewport)) || viewport.Height == 0)
    {
        return g_originalSetTransform(device, state, matrix);
    }

    ProjectionMatrix projection = ToProjectionMatrix(*matrix);
    const float viewportAspect = static_cast<float>(viewport.Width) /
                                 static_cast<float>(viewport.Height);
    if (!AdjustProjectionMatrixForViewport(&projection, viewportAspect, g_config))
    {
        return g_originalSetTransform(device, state, matrix);
    }

    std::call_once(g_projectionLogOnce, [viewportAspect]() {
        std::ostringstream message;
        message << "Projection fix active for viewport aspect " << viewportAspect
                << ", ultrawide=" << g_config.enableUltrawideFovFix;
        Log(message.str());
    });

    const D3DMATRIX adjusted = ToD3DMatrix(projection);
    return g_originalSetTransform(device, state, &adjusted);
}

HRESULT STDMETHODCALLTYPE HookedSetViewport(IDirect3DDevice9* device,
                                            CONST D3DVIEWPORT9* viewport)
{
    if (g_originalSetViewport == nullptr)
    {
        return E_FAIL;
    }

    if (viewport == nullptr)
    {
        return g_originalSetViewport(device, viewport);
    }

    D3DVIEWPORT9 adjusted = *viewport;
    const float deviceWidth = static_cast<float>(g_backBufferWidth.load());
    const float deviceHeight = static_cast<float>(g_backBufferHeight.load());
    if (ClampHudViewport(&adjusted, deviceWidth, deviceHeight, g_config))
    {
        std::call_once(g_viewportClampLogOnce, []() {
            Log("HUD viewport clamp applied for widescreen.");
        });
        return g_originalSetViewport(device, &adjusted);
    }

    return g_originalSetViewport(device, viewport);
}

float GetPrimaryDisplayAspect()
{
    DEVMODEW displayMode = {};
    displayMode.dmSize = sizeof(displayMode);
    if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &displayMode) != FALSE &&
        displayMode.dmPelsWidth != 0 && displayMode.dmPelsHeight != 0)
    {
        return static_cast<float>(displayMode.dmPelsWidth) /
               static_cast<float>(displayMode.dmPelsHeight);
    }

    const int width = GetSystemMetrics(SM_CXSCREEN);
    const int height = GetSystemMetrics(SM_CYSCREEN);
    if (width <= 0 || height <= 0)
    {
        return 16.0f / 9.0f;
    }

    return static_cast<float>(width) / static_cast<float>(height);
}

bool IsWritableMemoryRange(void* address, std::size_t size)
{
    if (address == nullptr || size == 0)
    {
        return false;
    }

    MEMORY_BASIC_INFORMATION memoryInfo = {};
    if (VirtualQuery(address, &memoryInfo, sizeof(memoryInfo)) == 0 ||
        memoryInfo.State != MEM_COMMIT)
    {
        return false;
    }

    const DWORD protection = memoryInfo.Protect & 0xff;
    if (protection == PAGE_NOACCESS || protection == PAGE_EXECUTE || protection == PAGE_READONLY ||
        protection == PAGE_EXECUTE_READ || (memoryInfo.Protect & PAGE_GUARD) != 0)
    {
        return false;
    }

    const auto begin = reinterpret_cast<std::uintptr_t>(address);
    const auto end = begin + size;
    const auto regionBegin = reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress);
    const auto regionEnd = regionBegin + memoryInfo.RegionSize;
    return begin >= regionBegin && end <= regionEnd;
}

bool IsCommittedMemoryRange(void* address, std::size_t size)
{
    if (address == nullptr || size == 0)
    {
        return false;
    }

    MEMORY_BASIC_INFORMATION memoryInfo = {};
    if (VirtualQuery(address, &memoryInfo, sizeof(memoryInfo)) == 0 ||
        memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0)
    {
        return false;
    }

    const DWORD protection = memoryInfo.Protect & 0xff;
    if (protection == PAGE_NOACCESS)
    {
        return false;
    }

    const auto begin = reinterpret_cast<std::uintptr_t>(address);
    const auto end = begin + size;
    const auto regionBegin = reinterpret_cast<std::uintptr_t>(memoryInfo.BaseAddress);
    const auto regionEnd = regionBegin + memoryInfo.RegionSize;
    return begin >= regionBegin && end <= regionEnd;
}

bool ResolveEngineFrameBudgetPointers(HMODULE engineModule)
{
    if (engineModule == nullptr)
    {
        g_engineFrameDurationMs = nullptr;
        g_engineTargetFrameRate = nullptr;
        return false;
    }

    auto* frameDurationMs = reinterpret_cast<float*>(
        reinterpret_cast<std::uintptr_t>(engineModule) + kEngineFrameDurationMsRva);
    auto* targetFrameRate = reinterpret_cast<std::uint32_t*>(
        reinterpret_cast<std::uintptr_t>(engineModule) + kEngineTargetFrameRateRva);
    if (!IsWritableMemoryRange(frameDurationMs, sizeof(*frameDurationMs)) ||
        !IsWritableMemoryRange(targetFrameRate, sizeof(*targetFrameRate)))
    {
        g_engineFrameDurationMs = nullptr;
        g_engineTargetFrameRate = nullptr;
        return false;
    }

    g_engineFrameDurationMs = frameDurationMs;
    g_engineTargetFrameRate = targetFrameRate;
    return true;
}

bool ResolveSchedulerTimingPointers(HMODULE engineModule)
{
    if (engineModule == nullptr)
    {
        g_schedulerUpdateDeltaSeconds = nullptr;
        g_schedulerSleepMilliseconds = nullptr;
        return false;
    }

    auto* updateDeltaSeconds = reinterpret_cast<float*>(
        reinterpret_cast<std::uintptr_t>(engineModule) + kSchedulerUpdateDeltaSecondsRva);
    auto* sleepMilliseconds = reinterpret_cast<float*>(
        reinterpret_cast<std::uintptr_t>(engineModule) + kSchedulerSleepMillisecondsRva);
    if (!IsCommittedMemoryRange(updateDeltaSeconds, sizeof(*updateDeltaSeconds)) ||
        !IsCommittedMemoryRange(sleepMilliseconds, sizeof(*sleepMilliseconds)))
    {
        g_schedulerUpdateDeltaSeconds = nullptr;
        g_schedulerSleepMilliseconds = nullptr;
        return false;
    }

    g_schedulerUpdateDeltaSeconds = updateDeltaSeconds;
    g_schedulerSleepMilliseconds = sleepMilliseconds;

    std::ostringstream message;
    message << "Scheduler constants observed: delta=" << *g_schedulerUpdateDeltaSeconds
            << "s, sleep=" << *g_schedulerSleepMilliseconds << "ms";
    Log(message.str());
    return true;
}

void ApplyLegacyFramePacingBudget(std::uint32_t fallbackRequestedFrameRate)
{
    if (!g_config.enableFrameRateUnlock || g_engineFrameDurationMs == nullptr ||
        g_engineTargetFrameRate == nullptr)
    {
        return;
    }

    const std::uint32_t requestedFrameRate =
        *g_engineTargetFrameRate != 0 ? *g_engineTargetFrameRate : fallbackRequestedFrameRate;
    const std::uint32_t effectiveFrameRate =
        ResolveEngineFrameRate(g_config.enableFrameRateUnlock, g_config.targetFrameRate,
                               requestedFrameRate);
    const float desiredFrameDurationMs = ComputeFrameDurationMilliseconds(effectiveFrameRate);
    const bool frameRateChanged = *g_engineTargetFrameRate != effectiveFrameRate;
    const float frameDurationDelta = *g_engineFrameDurationMs - desiredFrameDurationMs;
    const bool frameDurationChanged =
        frameDurationDelta > 0.001f || frameDurationDelta < -0.001f;
    if (!frameRateChanged && !frameDurationChanged)
    {
        return;
    }

    *g_engineTargetFrameRate = effectiveFrameRate;
    *g_engineFrameDurationMs = desiredFrameDurationMs;

    std::call_once(g_engineFrameBudgetLogOnce, [requestedFrameRate, effectiveFrameRate,
                                                desiredFrameDurationMs]() {
        std::ostringstream message;
        message << "Engine frame budget override active: requested=" << requestedFrameRate
                << ", effective=" << effectiveFrameRate
                << ", frameMs=" << desiredFrameDurationMs;
        Log(message.str());
    });
}

void ApplyEngineFovPatch(void* settings)
{
    if (settings == nullptr)
    {
        return;
    }

    auto* base = static_cast<std::uint8_t*>(settings);
    auto* defaultFovDegrees = reinterpret_cast<float*>(base + kDefaultFovOffset);
    auto* currentFovDegrees = reinterpret_cast<float*>(base + kCurrentFovOffset);
    if (!IsWritableMemoryRange(defaultFovDegrees, sizeof(float)) ||
        !IsWritableMemoryRange(currentFovDegrees, sizeof(float)))
    {
        Log("Engine FOV patch skipped because settings memory was not writable.");
        return;
    }

    const float originalDefaultFov = *defaultFovDegrees;
    const float originalCurrentFov = *currentFovDegrees;
    const float displayAspect = GetPrimaryDisplayAspect();
    if (!AdjustEngineFovDegrees(currentFovDegrees, defaultFovDegrees, displayAspect, g_config))
    {
        return;
    }

    std::ostringstream message;
    message << "Engine FOV adjusted: aspect=" << displayAspect << ", current "
            << originalCurrentFov << " -> " << *currentFovDegrees << ", default "
            << originalDefaultFov << " -> " << *defaultFovDegrees;
    Log(message.str());
}

float MeasureGameplayElapsedSeconds(void* objectPointer)
{
    if (objectPointer == nullptr || g_highPrecisionTimerState.frequency == 0)
    {
        return 0.0f;
    }

    LARGE_INTEGER currentCounter = {};
    if (QueryPerformanceCounter(&currentCounter) == FALSE)
    {
        return 0.0f;
    }

    const auto objectAddress = reinterpret_cast<std::uintptr_t>(objectPointer);
    const auto currentCounterValue = static_cast<std::uint64_t>(currentCounter.QuadPart);

    std::scoped_lock lock(g_gameplayTimingMutex);
    float elapsedSeconds = 0.0f;
    const auto existing = g_gameplayLastCounterByOwner.find(objectAddress);
    if (existing != g_gameplayLastCounterByOwner.end() &&
        currentCounterValue > existing->second)
    {
        const std::uint64_t elapsedCounter = currentCounterValue - existing->second;
        elapsedSeconds = static_cast<float>(
            static_cast<double>(elapsedCounter) /
            static_cast<double>(g_highPrecisionTimerState.frequency));
    }

    g_gameplayLastCounterByOwner[objectAddress] = currentCounterValue;
    if (g_gameplayLastCounterByOwner.size() > 8)
    {
        g_gameplayLastCounterByOwner.clear();
        g_gameplayLastCounterByOwner[objectAddress] = currentCounterValue;
    }

    return elapsedSeconds;
}

float MeasureSchedulerElapsedSeconds(void* objectPointer)
{
    if (objectPointer == nullptr || g_highPrecisionTimerState.frequency == 0)
    {
        return 0.0f;
    }

    LARGE_INTEGER currentCounter = {};
    if (QueryPerformanceCounter(&currentCounter) == FALSE)
    {
        return 0.0f;
    }

    const auto objectAddress = reinterpret_cast<std::uintptr_t>(objectPointer);
    const auto currentCounterValue = static_cast<std::uint64_t>(currentCounter.QuadPart);

    std::scoped_lock lock(g_schedulerTimingMutex);
    float elapsedSeconds = 0.0f;
    const auto existing = g_schedulerLastCounterByOwner.find(objectAddress);
    if (existing != g_schedulerLastCounterByOwner.end() &&
        currentCounterValue > existing->second)
    {
        const std::uint64_t elapsedCounter = currentCounterValue - existing->second;
        elapsedSeconds = static_cast<float>(
            static_cast<double>(elapsedCounter) /
            static_cast<double>(g_highPrecisionTimerState.frequency));
    }

    g_schedulerLastCounterByOwner[objectAddress] = currentCounterValue;
    if (g_schedulerLastCounterByOwner.size() > 4)
    {
        g_schedulerLastCounterByOwner.clear();
        g_schedulerLastCounterByOwner[objectAddress] = currentCounterValue;
    }

    return elapsedSeconds;
}

void __fastcall HookedEngineSettingsInit(void* settings)
{
    if (g_originalEngineSettingsInit != nullptr)
    {
        g_originalEngineSettingsInit(settings);
    }

    ApplyEngineFovPatch(settings);
}

void __fastcall HookedSchedulerLoopUpdate(void* objectPointer, void* reserved, float deltaSeconds)
{
    TryInstallShvHooks();

    const float measuredElapsedSeconds = MeasureSchedulerElapsedSeconds(objectPointer);
    const GameplayDeltaPlan deltaPlan =
        BuildGameplayDeltaPlan(g_config.enableFrameRateUnlock, g_config.targetFrameRate,
                               measuredElapsedSeconds, deltaSeconds);
    const float effectiveDeltaSeconds =
        std::isfinite(deltaPlan.effectiveDeltaSeconds) && deltaPlan.effectiveDeltaSeconds >= 0.0f
            ? deltaPlan.effectiveDeltaSeconds
            : deltaSeconds;

    std::call_once(g_schedulerLoopTargetLogOnce, [objectPointer]() {
        LogSchedulerLoopTargets(objectPointer);
    });

    const std::uint32_t sampleIndex =
        g_schedulerUpdateLogCount.fetch_add(1, std::memory_order_relaxed);
    const float targetDeltaSeconds =
        g_config.targetFrameRate > 0 ? (1.0f / static_cast<float>(g_config.targetFrameRate))
                                     : (1.0f / 60.0f);
    const bool logOutlier =
        measuredElapsedSeconds > targetDeltaSeconds * 1.5f &&
        sampleIndex < (kSchedulerTimingLogSampleLimit + kRenderCadenceOutlierLogLimit);
    if (sampleIndex < kSchedulerTimingLogSampleLimit || logOutlier)
    {
        std::ostringstream message;
        message << "Scheduler update override "
                << (logOutlier && sampleIndex >= kSchedulerTimingLogSampleLimit ? "outlier "
                                                                                 : "sample ")
                << (sampleIndex + 1)
                << ": object=" << FormatAddress(reinterpret_cast<std::uintptr_t>(objectPointer))
                << ", delta=" << deltaSeconds << "->" << effectiveDeltaSeconds
                << "s, measured=" << measuredElapsedSeconds << "s, target=";
        if (g_config.targetFrameRate == 0)
        {
            message << "unlocked";
        }
        else
        {
            message << g_config.targetFrameRate;
        }
        Log(message.str());
    }

    {
        std::scoped_lock lock(g_timingDiagnosticsMutex);
        AccumulateCadenceSummary(&g_schedulerTimingSummary,
                                 static_cast<double>(measuredElapsedSeconds) * 1000.0,
                                 ResolveDiagnosticTargetFrameDurationMs());
        LogSchedulerTimingSummary(ResolveDiagnosticTargetFrameDurationMs());
    }

    if (g_originalSchedulerLoopUpdate != nullptr)
    {
        g_originalSchedulerLoopUpdate(objectPointer, reserved, effectiveDeltaSeconds);
    }
}

std::uint32_t __fastcall HookedGameplayUpdate(void* objectPointer, void* reserved,
                                              float* deltaSeconds, char allowAdvance)
{
    TryInstallShvHooks();

    const bool canPatchDelta =
        deltaSeconds != nullptr && IsWritableMemoryRange(deltaSeconds, sizeof(*deltaSeconds));
    const float originalDeltaSeconds = canPatchDelta ? *deltaSeconds : 0.0f;
    const float measuredElapsedSeconds = MeasureGameplayElapsedSeconds(objectPointer);
    const GameplayDeltaPlan deltaPlan =
        BuildGameplayDeltaPlan(g_config.enableFrameRateUnlock, g_config.targetFrameRate,
                               measuredElapsedSeconds, originalDeltaSeconds);

    std::call_once(g_gameplayThreadLogOnce, [objectPointer]() {
        std::ostringstream message;
        message << "Gameplay update active on thread " << ::GetCurrentThreadId()
                << ", object=" << FormatAddress(reinterpret_cast<std::uintptr_t>(objectPointer));
        Log(message.str());
    });

    const bool hasFiniteDelta =
        std::isfinite(deltaPlan.effectiveDeltaSeconds) && deltaPlan.effectiveDeltaSeconds >= 0.0f;
    const bool shouldOverrideDelta =
        canPatchDelta && hasFiniteDelta &&
        (deltaPlan.effectiveDeltaSeconds > 0.0f || originalDeltaSeconds > 0.0f);

    if (shouldOverrideDelta)
    {
        *deltaSeconds = deltaPlan.effectiveDeltaSeconds;
    }

    const std::uint32_t sampleIndex =
        g_gameplayDeltaLogCount.fetch_add(1, std::memory_order_relaxed);
    const float targetDeltaSeconds =
        g_config.targetFrameRate > 0 ? (1.0f / static_cast<float>(g_config.targetFrameRate))
                                     : (1.0f / 60.0f);
    const bool logOutlier =
        measuredElapsedSeconds > targetDeltaSeconds * 1.5f &&
        sampleIndex < (kGameplayDeltaLogSampleLimit + kRenderCadenceOutlierLogLimit);
    if (sampleIndex < kGameplayDeltaLogSampleLimit || logOutlier)
    {
        std::ostringstream message;
        message << "Gameplay delta override "
                << (logOutlier && sampleIndex >= kGameplayDeltaLogSampleLimit ? "outlier "
                                                                               : "sample ")
                << (sampleIndex + 1)
                << ": object=" << FormatAddress(reinterpret_cast<std::uintptr_t>(objectPointer))
                << ", delta=" << originalDeltaSeconds << "->"
                << deltaPlan.effectiveDeltaSeconds << "s, measured="
                << measuredElapsedSeconds << "s, target=";
        if (g_config.targetFrameRate == 0)
        {
            message << "unlocked";
        }
        else
        {
            message << g_config.targetFrameRate;
        }
        message << ", mode=" << static_cast<int>(allowAdvance);
        Log(message.str());
    }

    const std::uint32_t result =
        g_originalGameplayUpdate != nullptr
            ? g_originalGameplayUpdate(objectPointer, reserved, deltaSeconds, allowAdvance)
            : 0;

    if (shouldOverrideDelta)
    {
        *deltaSeconds = originalDeltaSeconds;
    }

    return result;
}

bool InstallEngineHooks(HMODULE engineModule)
{
    const auto targetAddress =
        reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(engineModule) +
                                kEngineSettingsInitRva);
    if (!InstallRelativeJumpDetour(targetAddress, kRelativeJumpLength,
                                   reinterpret_cast<void*>(&HookedEngineSettingsInit),
                                   reinterpret_cast<void**>(&g_originalEngineSettingsInit)))
    {
        Log("Failed to install engine FOV initializer hook.");
        return false;
    }

    Log("Engine FOV initializer hook installed.");
    return true;
}

bool InstallPreciseSleepHook(HMODULE engineModule)
{
    if (!g_config.enablePreciseSleepShim || engineModule == nullptr)
    {
        return false;
    }

    const auto targetAddress =
        reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(engineModule) + kEngineSleepExRva);
    if (!InstallRelativeJumpDetour(targetAddress, 8,
                                   reinterpret_cast<void*>(&HookedEngineSleepEx),
                                   reinterpret_cast<void**>(&g_originalEngineSleepEx)))
    {
        Log("Failed to install engine precise sleep shim.");
        return false;
    }

    Log("Engine precise sleep shim installed.");
    return true;
}

bool InstallSchedulerLoopUpdateHook(HMODULE engineModule)
{
    if (!g_config.enableFrameRateUnlock || engineModule == nullptr)
    {
        return false;
    }

    const auto targetAddress =
        reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(engineModule) +
                                kSchedulerLoopUpdateRva);
    if (!InstallRelativeJumpDetour(targetAddress, 8,
                                   reinterpret_cast<void*>(&HookedSchedulerLoopUpdate),
                                   reinterpret_cast<void**>(&g_originalSchedulerLoopUpdate)))
    {
        Log("Failed to install scheduler update hook.");
        return false;
    }

    Log("Scheduler update hook installed.");
    return true;
}

bool InstallGameplayUpdateHook(HMODULE engineModule)
{
    if (!g_config.enableFrameRateUnlock || engineModule == nullptr)
    {
        return false;
    }

    const auto targetAddress =
        reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(engineModule) +
                                kGameplayUpdateRva);
    if (!InstallRelativeJumpDetour(targetAddress, 6,
                                   reinterpret_cast<void*>(&HookedGameplayUpdate),
                                   reinterpret_cast<void**>(&g_originalGameplayUpdate)))
    {
        Log("Failed to install gameplay delta hook.");
        return false;
    }

    Log("Gameplay delta hook installed.");
    return true;
}

bool InstallBinkMovieHooks(HMODULE module, const char* moduleLabel)
{
    if (module == nullptr)
    {
        return false;
    }

    bool installedAnyHook = false;

    if (InstallIatHook(module, "binkw32.dll", "_BinkOpen@8",
                       reinterpret_cast<void*>(&HookedBinkOpen),
                       reinterpret_cast<void**>(&g_originalBinkOpen)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "binkw32.dll", "_BinkDoFrame@4",
                       reinterpret_cast<void*>(&HookedBinkDoFrame),
                       reinterpret_cast<void**>(&g_originalBinkDoFrame)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "binkw32.dll", "_BinkNextFrame@4",
                       reinterpret_cast<void*>(&HookedBinkNextFrame),
                       reinterpret_cast<void**>(&g_originalBinkNextFrame)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "binkw32.dll", "_BinkGoto@12",
                       reinterpret_cast<void*>(&HookedBinkGoto),
                       reinterpret_cast<void**>(&g_originalBinkGoto)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "binkw32.dll", "_BinkPause@8",
                       reinterpret_cast<void*>(&HookedBinkPause),
                       reinterpret_cast<void**>(&g_originalBinkPause)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "binkw32.dll", "_BinkWait@4",
                       reinterpret_cast<void*>(&HookedBinkWait),
                       reinterpret_cast<void**>(&g_originalBinkWait)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "binkw32.dll", "_BinkClose@4",
                       reinterpret_cast<void*>(&HookedBinkClose),
                       reinterpret_cast<void**>(&g_originalBinkClose)))
    {
        installedAnyHook = true;
    }

    if (installedAnyHook)
    {
        std::ostringstream message;
        message << "Bink movie hooks installed for " << moduleLabel << ".";
        Log(message.str());
    }

    return installedAnyHook;
}

bool InstallTimerHooks(HMODULE module, const char* moduleLabel)
{
    if (!g_config.enableHighPrecisionTiming || module == nullptr)
    {
        return false;
    }

    if (!InstallIatHook(module, "kernel32.dll", "GetTickCount",
                        reinterpret_cast<void*>(&HookedGetTickCount),
                        reinterpret_cast<void**>(&g_originalGetTickCount)))
    {
        std::ostringstream message;
        message << "Failed to install GetTickCount hook for " << moduleLabel << '.';
        Log(message.str());
        return false;
    }

    std::ostringstream message;
    message << "GetTickCount hook installed for " << moduleLabel << '.';
    Log(message.str());
    return true;
}

bool InstallLegacyRecoveryHooks(HMODULE module, const char* moduleLabel)
{
    if (module == nullptr)
    {
        return false;
    }

    bool installedAnyHook = false;
    bool installedThreadWrapperHook = false;
    if (InstallIatHook(module, "kernel32.dll", "ResumeThread",
                       reinterpret_cast<void*>(&HookedResumeThread),
                       reinterpret_cast<void**>(&g_originalResumeThread)))
    {
        installedAnyHook = true;
        installedThreadWrapperHook = true;
    }

    if (InstallIatHook(module, "kernel32.dll", "SuspendThread",
                       reinterpret_cast<void*>(&HookedSuspendThread),
                       reinterpret_cast<void**>(&g_originalSuspendThread)))
    {
        installedAnyHook = true;
        installedThreadWrapperHook = true;
    }

    if (InstallIatHook(module, "kernel32.dll", "TerminateThread",
                       reinterpret_cast<void*>(&HookedTerminateThread),
                       reinterpret_cast<void**>(&g_originalTerminateThread)))
    {
        installedAnyHook = true;
        installedThreadWrapperHook = true;
    }

    if (InstallIatHook(module, "kernel32.dll", "Sleep",
                       reinterpret_cast<void*>(&HookedEngineSleep),
                       reinterpret_cast<void**>(&g_originalEngineSleep)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "winmm.dll", "timeKillEvent",
                       reinterpret_cast<void*>(&HookedTimeKillEvent),
                       reinterpret_cast<void**>(&g_originalTimeKillEvent)))
    {
        installedAnyHook = true;
    }

    if (installedAnyHook)
    {
        std::ostringstream message;
        message << "Legacy recovery hooks installed for " << moduleLabel << '.';
        Log(message.str());
    }

    if (installedThreadWrapperHook)
    {
        std::ostringstream message;
        message << "Legacy thread wrapper hooks installed for " << moduleLabel << '.';
        Log(message.str());
    }

    return installedAnyHook;
}

bool InstallCrashDumpHooks(HMODULE module, const char* moduleLabel)
{
    if (!g_config.enableCrashDumps || module == nullptr)
    {
        return false;
    }

    if (!InstallIatHook(module, "kernel32.dll", "SetUnhandledExceptionFilter",
                        reinterpret_cast<void*>(&HookedSetUnhandledExceptionFilter), nullptr))
    {
        std::ostringstream message;
        message << "Failed to install SetUnhandledExceptionFilter hook for " << moduleLabel
                << '.';
        Log(message.str());
        return false;
    }

    ::SetUnhandledExceptionFilter(&HookedUnhandledExceptionFilter);

    std::ostringstream message;
    message << "Crash dump hook installed for " << moduleLabel << '.';
    Log(message.str());
    return true;
}

bool InstallDirectInputHooks(HMODULE module)
{
    if (!g_config.enableRawMouseInput || module == nullptr)
    {
        return false;
    }

    if (!InstallIatHook(module, "DINPUT8.dll", "DirectInput8Create",
                        reinterpret_cast<void*>(&HookedDirectInput8Create),
                        reinterpret_cast<void**>(&g_originalDirectInput8Create)))
    {
        Log("Failed to install DirectInput8Create hook.");
        return false;
    }

    Log("DirectInput8Create IAT hook installed.");
    return true;
}

bool InstallTimingDiagnosticsHooks(HMODULE module, const char* moduleLabel)
{
    if (module == nullptr)
    {
        return false;
    }

    bool installedAnyHook = false;
    if (InstallIatHook(module, "winmm.dll", "timeSetEvent",
                       reinterpret_cast<void*>(&HookedTimeSetEvent),
                       reinterpret_cast<void**>(&g_originalTimeSetEvent)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "kernel32.dll", "SetWaitableTimer",
                       reinterpret_cast<void*>(&HookedSetWaitableTimer),
                       reinterpret_cast<void**>(&g_originalSetWaitableTimer)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "kernel32.dll", "CreateThread",
                       reinterpret_cast<void*>(&HookedCreateThread),
                       reinterpret_cast<void**>(&g_originalCreateThread)))
    {
        installedAnyHook = true;
    }

    if (installedAnyHook)
    {
        std::ostringstream message;
        message << "Timing diagnostics hooks installed for " << moduleLabel << '.';
        Log(message.str());
    }

    return installedAnyHook;
}

bool InstallShutdownDiagnosticsHooks(HMODULE module, const char* moduleLabel)
{
    if (module == nullptr)
    {
        return false;
    }

    bool installedAnyHook = false;
    if (InstallIatHook(module, "kernel32.dll", "TerminateProcess",
                       reinterpret_cast<void*>(&HookedTerminateProcess),
                       reinterpret_cast<void**>(&g_originalTerminateProcess)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "kernel32.dll", "ExitProcess",
                       reinterpret_cast<void*>(&HookedExitProcess),
                       reinterpret_cast<void**>(&g_originalExitProcess)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "kernel32.dll", "RaiseException",
                       reinterpret_cast<void*>(&HookedRaiseException),
                       reinterpret_cast<void**>(&g_originalRaiseException)))
    {
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "user32.dll", "PostQuitMessage",
                       reinterpret_cast<void*>(&HookedPostQuitMessage),
                       reinterpret_cast<void**>(&g_originalPostQuitMessage)))
    {
        installedAnyHook = true;
    }

    if (installedAnyHook)
    {
        std::ostringstream message;
        message << "Shutdown diagnostics hooks installed for " << moduleLabel << '.';
        Log(message.str());
    }

    return installedAnyHook;
}

bool InstallShvHooks(HMODULE module)
{
    if (module == nullptr)
    {
        return false;
    }

    g_shvModule = module;
    Log("Detected shv.dll at " + FormatAddress(reinterpret_cast<std::uintptr_t>(module)));
    LogShvConfigSnapshot();

    bool installedAnyHook = false;
    installedAnyHook |= InstallBinkMovieHooks(module, "shv.dll");
    installedAnyHook |= InstallTimerHooks(module, "shv.dll");
    installedAnyHook |= InstallShutdownDiagnosticsHooks(module, "shv.dll");

    if (InstallIatHook(module, "kernel32.dll", "Sleep",
                       reinterpret_cast<void*>(&HookedShvSleep),
                       reinterpret_cast<void**>(&g_originalShvSleep)))
    {
        Log("shv.dll Sleep hook installed.");
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "user32.dll", "SetTimer",
                       reinterpret_cast<void*>(&HookedShvSetTimer),
                       reinterpret_cast<void**>(&g_originalShvSetTimer)))
    {
        Log("shv.dll SetTimer hook installed.");
        installedAnyHook = true;
    }

    if (InstallIatHook(module, "kernel32.dll", "GetProcAddress",
                       reinterpret_cast<void*>(&HookedShvGetProcAddress),
                       reinterpret_cast<void**>(&g_originalShvGetProcAddress)))
    {
        Log("shv.dll GetProcAddress hook installed.");
        installedAnyHook = true;
    }

    auto* mainLoopAddress = reinterpret_cast<void*>(
        reinterpret_cast<std::uintptr_t>(module) + kShvMainLoopRva);
    if (InstallRelativeJumpDetour(mainLoopAddress, kRelativeJumpLength,
                                  reinterpret_cast<void*>(&HookedShvMainLoop),
                                  reinterpret_cast<void**>(&g_originalShvMainLoop)))
    {
        Log("shv.dll MainLoop hook installed.");
        installedAnyHook = true;
    }

    if (installedAnyHook)
    {
        g_shvHooksInstalled.store(true, std::memory_order_relaxed);
    }

    return installedAnyHook;
}

bool TryInstallShvHooks()
{
    if (g_shvHooksInstalled.load(std::memory_order_relaxed))
    {
        return true;
    }

    HMODULE module = GetModuleHandleW(L"shv.dll");
    if (module == nullptr)
    {
        return false;
    }

    std::scoped_lock lock(g_hookMutex);
    if (g_shvHooksInstalled.load(std::memory_order_relaxed))
    {
        return true;
    }

    return InstallShvHooks(module);
}

bool WaitForAndInstallShvHooks(DWORD attempts, DWORD delayMs)
{
    for (DWORD attempt = 0; attempt < attempts; ++attempt)
    {
        if (TryInstallShvHooks())
        {
            return true;
        }

        Sleep(delayMs);
    }

    return false;
}

bool InstallHooks()
{
    HMODULE engineModule = nullptr;
    for (DWORD attempt = 0; attempt < kInitRetryCount; ++attempt)
    {
        engineModule = GetModuleHandleW(L"g_SilentHill.sgl");
        if (engineModule != nullptr)
        {
            break;
        }

        Sleep(kInitRetryDelayMs);
    }

    if (engineModule == nullptr)
    {
        Log("g_SilentHill.sgl was not available for IAT patching.");
        return false;
    }

    g_engineModule = engineModule;
    ResolveEngineFrameBudgetPointers(engineModule);
    ResolveSchedulerTimingPointers(engineModule);

    bool installedAnyHook = false;
    installedAnyHook |= InstallEngineHooks(engineModule);
    installedAnyHook |= InstallPreciseSleepHook(engineModule);
    installedAnyHook |= InstallSchedulerLoopUpdateHook(engineModule);
    installedAnyHook |= InstallGameplayUpdateHook(engineModule);
    installedAnyHook |= InstallBinkMovieHooks(engineModule, "g_SilentHill.sgl");
    installedAnyHook |= InstallTimerHooks(engineModule, "g_SilentHill.sgl");
    installedAnyHook |= InstallLegacyRecoveryHooks(engineModule, "g_SilentHill.sgl");
    installedAnyHook |= InstallCrashDumpHooks(engineModule, "g_SilentHill.sgl");
    installedAnyHook |= InstallShutdownDiagnosticsHooks(engineModule, "g_SilentHill.sgl");
    installedAnyHook |= InstallDirectInputHooks(engineModule);
    installedAnyHook |= InstallTimingDiagnosticsHooks(engineModule, "g_SilentHill.sgl");

    if (!InstallIatHook(engineModule, "d3d9.dll", "Direct3DCreate9",
                        reinterpret_cast<void*>(&HookedDirect3DCreate9),
                        reinterpret_cast<void**>(&g_originalDirect3DCreate9)))
    {
        Log("Failed to patch Direct3DCreate9 import.");
        return installedAnyHook;
    }

    Log("Direct3DCreate9 IAT hook installed.");
    if (!WaitForAndInstallShvHooks(kInitRetryCount, kInitRetryDelayMs))
    {
        Log("shv.dll was not available during the startup hook window.");
    }
    return true;
}

void InstallDirect3D9VtableHooks()
{
    HMODULE d3d9Module = GetModuleHandleW(L"d3d9.dll");
    if (d3d9Module == nullptr)
    {
        d3d9Module = LoadLibraryW(L"d3d9.dll");
    }

    if (d3d9Module == nullptr)
    {
        Log("d3d9.dll could not be loaded for vtable patching.");
        return;
    }

    Direct3DCreate9Fn create9 = g_originalDirect3DCreate9;
    if (create9 == nullptr)
    {
        create9 = reinterpret_cast<Direct3DCreate9Fn>(GetProcAddress(d3d9Module, "Direct3DCreate9"));
    }

    if (create9 == nullptr)
    {
        Log("Direct3DCreate9 export was unavailable.");
        return;
    }
    g_originalDirect3DCreate9 = create9;

    IDirect3D9* direct3d = create9(D3D_SDK_VERSION);
    if (direct3d == nullptr)
    {
        Log("Direct3DCreate9 dummy object creation failed.");
        return;
    }

    {
        std::scoped_lock lock(g_hookMutex);
        if (PatchVtableEntry(direct3d, kCreateDeviceVtableIndex,
                             reinterpret_cast<void*>(&HookedCreateDevice),
                             reinterpret_cast<void**>(&g_originalCreateDevice)))
        {
            Log("IDirect3D9::CreateDevice hook installed.");
        }
    }

    HWND dummyWindow =
        CreateWindowExW(0, L"STATIC", L"ShepherdPatchDummy", WS_POPUP, 0, 0, 1, 1, nullptr,
                        nullptr, GetModuleHandleW(nullptr), nullptr);
    if (dummyWindow != nullptr)
    {
        D3DPRESENT_PARAMETERS params = {};
        params.Windowed = TRUE;
        params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        params.hDeviceWindow = dummyWindow;

        IDirect3DDevice9* device = nullptr;
        const ScopedD3DProbeCreate probeCreate;
        const HRESULT hr = direct3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dummyWindow,
                                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &params,
                                                  &device);
        if (SUCCEEDED(hr) && device != nullptr)
        {
            device->Release();
        }
        else
        {
            Log("Dummy CreateDevice failed with " + FormatHr(hr));
        }

        DestroyWindow(dummyWindow);
    }
    else
    {
        Log("Dummy window creation failed.");
    }

    direct3d->Release();

    const auto create9Ex =
        reinterpret_cast<Direct3DCreate9ExFn>(GetProcAddress(d3d9Module, "Direct3DCreate9Ex"));
    if (create9Ex == nullptr)
    {
        Log("Direct3DCreate9Ex export was unavailable.");
        return;
    }
    g_originalDirect3DCreate9Ex = create9Ex;

    IDirect3D9Ex* direct3dEx = nullptr;
    if (FAILED(create9Ex(D3D_SDK_VERSION, &direct3dEx)) || direct3dEx == nullptr)
    {
        Log("Direct3DCreate9Ex dummy object creation failed.");
        return;
    }

    {
        std::scoped_lock lock(g_hookMutex);
        if (PatchVtableEntry(direct3dEx, kCreateDeviceExVtableIndex,
                             reinterpret_cast<void*>(&HookedCreateDeviceEx),
                             reinterpret_cast<void**>(&g_originalCreateDeviceEx)))
        {
            Log("IDirect3D9Ex::CreateDeviceEx hook installed.");
        }
    }

    HWND dummyWindowEx =
        CreateWindowExW(0, L"STATIC", L"ShepherdPatchDummyEx", WS_POPUP, 0, 0, 1, 1, nullptr,
                        nullptr, GetModuleHandleW(nullptr), nullptr);
    if (dummyWindowEx != nullptr)
    {
        D3DPRESENT_PARAMETERS params = {};
        params.Windowed = TRUE;
        params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        params.hDeviceWindow = dummyWindowEx;

        IDirect3DDevice9Ex* deviceEx = nullptr;
        const ScopedD3DProbeCreate probeCreate;
        const HRESULT hr =
            direct3dEx->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dummyWindowEx,
                                       D3DCREATE_SOFTWARE_VERTEXPROCESSING, &params, nullptr,
                                       &deviceEx);
        if (SUCCEEDED(hr) && deviceEx != nullptr)
        {
            deviceEx->Release();
        }
        else
        {
            Log("Dummy CreateDeviceEx failed with " + FormatHr(hr));
        }

        DestroyWindow(dummyWindowEx);
    }
    else
    {
        Log("Dummy Ex window creation failed.");
    }

    direct3dEx->Release();
}

DWORD WINAPI InitializeThread(void*)
{
    g_moduleDirectory = ResolveModuleDirectory(g_module);
    g_config = LoadConfigFromDisk();
    ApplyConfiguredEngineVarsOverrides();
    ApplyDpiAwareness();
    ApplyProcessExecutionPolicy();
    if (g_config.enableHighPrecisionTiming || g_config.enablePreciseSleepShim)
    {
        if (InitializeHighPrecisionTimingState())
        {
            Log("High precision timing state initialized.");
        }
        else
        {
            Log("High precision timing state initialization failed.");
        }
    }
    ApplyHighResolutionTimerRequest();

    const HMODULE exeModule = GetModuleHandleW(nullptr);
    const bool exeTimerHooksInstalled = InstallTimerHooks(exeModule, "SilentHill.exe");
    const bool exeCrashHooksInstalled = InstallCrashDumpHooks(exeModule, "SilentHill.exe");
    const bool exeShutdownHooksInstalled =
        InstallShutdownDiagnosticsHooks(exeModule, "SilentHill.exe");
    const bool exeTimingHooksInstalled = InstallTimingDiagnosticsHooks(exeModule, "SilentHill.exe");
    const bool exeIntroHooksInstalled = InstallBinkMovieHooks(exeModule, "SilentHill.exe");
    InstallDirect3D9VtableHooks();
    const bool engineHooksInstalled = InstallHooks();
    (void)exeTimerHooksInstalled;
    (void)exeCrashHooksInstalled;
    (void)exeShutdownHooksInstalled;
    (void)exeTimingHooksInstalled;
    (void)exeIntroHooksInstalled;
    (void)engineHooksInstalled;
    return 0;
}

BOOL CALLBACK StartRuntimeInitialization(PINIT_ONCE, PVOID parameter, PVOID*)
{
    g_module = static_cast<HMODULE>(parameter);

    HANDLE thread = CreateThread(nullptr, 0, &InitializeThread, nullptr, 0, nullptr);
    if (thread != nullptr)
    {
        CloseHandle(thread);
    }

    return TRUE;
}
} // namespace

void OnProcessAttach(HMODULE module)
{
    InitOnceExecuteOnce(&g_runtimeInitOnce, &StartRuntimeInitialization, module, nullptr);
}

void OnProcessDetach()
{
    CloseLegacyWaitableTimerWorkerHandle();

    if (g_renderThreadMmcssHandle != nullptr && g_avRevertMmThreadCharacteristics != nullptr)
    {
        g_avRevertMmThreadCharacteristics(g_renderThreadMmcssHandle);
        g_renderThreadMmcssHandle = nullptr;
        g_renderThreadMmcssTaskIndex = 0;
    }

    if (g_requestedHighResolutionTimer && g_timeEndPeriod != nullptr)
    {
        g_timeEndPeriod(1);
        g_requestedHighResolutionTimer = false;
    }
}
} // namespace shh

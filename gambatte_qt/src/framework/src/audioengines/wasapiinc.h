// Public domain header written by sinamas <sinamas at users.sourceforge.net>
#ifndef WASAPIINC_H
#define WASAPIINC_H

#include <objbase.h>
#include <objidl.h>

typedef LONGLONG REFERENCE_TIME;
typedef const GUID *LPCGUID;

enum EDataFlow { eRender, eCapture, eAll, EDataFlow_enum_count };
enum ERole { eConsole, eMultimedia, eCommunications, ERole_enum_count };
enum AUDCLNT_SHAREMODE { AUDCLNT_SHAREMODE_SHARED, AUDCLNT_SHAREMODE_EXCLUSIVE };
enum DeviceShareMode { DeviceShared, DeviceExclusive };
enum AudioSessionState { AudioSessionStateInactive, AudioSessionStateActive, AudioSessionStateExpired };

struct PROPERTYKEY {
	GUID fmtid;
	DWORD pid;
};

#define AUDCLNT_STREAMFLAGS_CROSSPROCESS   0x00010000
#define AUDCLNT_STREAMFLAGS_LOOPBACK       0x00020000
#define AUDCLNT_STREAMFLAGS_EVENTCALLBACK  0x00040000
#define AUDCLNT_STREAMFLAGS_NOPERSIST      0x00080000

#define DEVICE_STATE_ACTIVE      0x00000001
#define DEVICE_STATE_DISABLED    0x00000002
#define DEVICE_STATE_NOTPRESENT  0x00000004
#define DEVICE_STATE_UNPLUGGED   0x00000008
#define DEVICE_STATEMASK_ALL     0x0000000F

class IMMNotificationClient;

class IAudioClient : public IUnknown {
public:
	virtual HRESULT STDMETHODCALLTYPE Initialize(AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags,
		REFERENCE_TIME hnsBufferDuration, REFERENCE_TIME hnsPeriodicity, const WAVEFORMATEX *pFormat,
		LPCGUID AudioSessionGuid) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBufferSize(UINT32 *pNumBufferFrames) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetStreamLatency(REFERENCE_TIME *phnsLatency) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentPadding(UINT32 *pNumPaddingFrames) = 0;
	virtual HRESULT STDMETHODCALLTYPE IsFormatSupported(AUDCLNT_SHAREMODE ShareMode, const WAVEFORMATEX *pFormat,
		WAVEFORMATEX **ppClosestMatch) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMixFormat(WAVEFORMATEX **ppDeviceFormat) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDevicePeriod(REFERENCE_TIME *phnsDefaultDevicePeriod,
		REFERENCE_TIME *phnsMinimumDevicePeriod) = 0;
	virtual HRESULT STDMETHODCALLTYPE Start() = 0;
	virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
	virtual HRESULT STDMETHODCALLTYPE Reset() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetEventHandle(HANDLE eventHandle) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetService(REFIID riid, void **ppv) = 0;
};

class IAudioRenderClient : public IUnknown {
public:
	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT32 NumFramesRequested, BYTE **ppData) = 0;
	virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags) = 0;
};

class IAudioClock : public IUnknown {
public:
	virtual HRESULT STDMETHODCALLTYPE GetFrequency(UINT64 *pu64Frequency) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPosition(UINT64 *pu64Position, UINT64 *pu64QPCPosition) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCharacteristics(DWORD *pdwCharacteristics) = 0;
};

class IPropertyStore : public IUnknown {
public:
	virtual HRESULT STDMETHODCALLTYPE GetCount(DWORD *cProps) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAt(DWORD iProp, PROPERTYKEY *pkey) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetValue(const PROPERTYKEY &key, PROPVARIANT *pv) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetValue(const PROPERTYKEY &key, const PROPVARIANT &propvar) = 0;
	virtual HRESULT STDMETHODCALLTYPE Commit() = 0;
};

class IMMDevice : public IUnknown {
public:
	virtual HRESULT STDMETHODCALLTYPE Activate(REFIID iid, DWORD dwClsCtx, PROPVARIANT *pActivationParams,
		void **ppInterface) = 0;
	virtual HRESULT STDMETHODCALLTYPE OpenPropertyStore(DWORD stgmAccess, IPropertyStore **ppProperties) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetId(LPWSTR *ppstrId) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetState(DWORD *pdwState) = 0;
};

class IMMDeviceCollection : public IUnknown {
public:
	virtual HRESULT STDMETHODCALLTYPE GetCount(UINT *pcDevices) = 0;
	virtual HRESULT STDMETHODCALLTYPE Item(UINT nDevice, IMMDevice **ppDevice) = 0;
};

class IMMDeviceEnumerator : public IUnknown {
public:
	virtual HRESULT STDMETHODCALLTYPE EnumAudioEndpoints(EDataFlow dataFlow, DWORD dwStateMask,
		IMMDeviceCollection **ppDevices) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDefaultAudioEndpoint(EDataFlow dataFlow, ERole role,
		IMMDevice **ppEndpoint) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDevice(LPCWSTR pwstrId, IMMDevice **ppDevice) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterEndpointNotificationCallback(IMMNotificationClient *pClient) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterEndpointNotificationCallback(IMMNotificationClient *pClient) = 0;
};

#endif /* WASAPIINC_H */

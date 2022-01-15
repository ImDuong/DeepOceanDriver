#ifndef KRNL_GW
#define KRNL_GW

enum class Options {
	Unknown,
	BlockProg, UnBlockProg, ClearBlockProg,
};

class KernelGateway
{
private:
	HANDLE hDevice;
	bool OpenDevice(const wchar_t* deviceName);
	bool CloseDevice();

public:
	KernelGateway(const wchar_t* deviceName);
	~KernelGateway();
	bool IsGatewayOk();

	bool SendDataToKernel(Options option, LPVOID lpBuffer, DWORD nBufferSize);
	bool LooseReadFromKernel(LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead);
};

#endif // !KRNL_GW
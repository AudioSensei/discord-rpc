#ifdef DISCORD_WINDOWS
#include "connection.h"

#define WIN32_LEAN_AND_MEAN
#define NOMCX
#define NOSERVICE
#define NOIME
#include <assert.h>
#include <windows.h>
#include <sstream>

int GetProcessId()
{
    return static_cast<int>(GetCurrentProcessId());
}

struct BaseConnectionWin : public BaseConnection {
    HANDLE pipe{INVALID_HANDLE_VALUE};
};

static BaseConnectionWin Connection;

/*static*/ BaseConnection* BaseConnection::Create()
{
    return &Connection;
}

/*static*/ void BaseConnection::Destroy(BaseConnection*& c)
{
    auto self = reinterpret_cast<BaseConnectionWin*>(c);
    self->Close();
    c = nullptr;
}

bool BaseConnection::Open(int pipe, int& used_pipe)
{
    const std::wstring pipeTemplate = L"\\\\?\\pipe\\discord-ipc-";
    auto self = reinterpret_cast<BaseConnectionWin*>(this);
    used_pipe = -1;
    for (auto pipeNum = pipe;;) {
        auto pipeName = pipeTemplate + std::to_wstring(pipeNum);

        self->pipe = ::CreateFileW(
          pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (self->pipe != INVALID_HANDLE_VALUE) {
            self->isOpen = true;
            used_pipe = pipeNum;
            return true;
        }

        const auto lastError = GetLastError();
        if (lastError == ERROR_FILE_NOT_FOUND) {
            if (++pipeNum > 9) {
                return false;
            }
            continue;
        }
        if (lastError == ERROR_PIPE_BUSY) {
            if (WaitNamedPipeW(pipeName.c_str(), 10000)) {
                continue;
            }
        }
        return false;
    }
}

bool BaseConnection::Close()
{
    auto self = reinterpret_cast<BaseConnectionWin*>(this);
    ::CloseHandle(self->pipe);
    self->pipe = INVALID_HANDLE_VALUE;
    self->isOpen = false;
    return true;
}

bool BaseConnection::Write(const void* data, size_t length)
{
    if (length == 0) {
        return true;
    }
    auto self = reinterpret_cast<BaseConnectionWin*>(this);
    assert(self);
    if (!self) {
        return false;
    }
    if (self->pipe == INVALID_HANDLE_VALUE) {
        return false;
    }
    assert(data);
    if (!data) {
        return false;
    }
    const auto bytesLength = static_cast<DWORD>(length);
    DWORD bytesWritten = 0;
    return ::WriteFile(self->pipe, data, bytesLength, &bytesWritten, nullptr) == TRUE &&
      bytesWritten == bytesLength;
}

bool BaseConnection::Read(void* data, size_t length)
{
    assert(data);
    if (!data) {
        return false;
    }
    auto self = reinterpret_cast<BaseConnectionWin*>(this);
    assert(self);
    if (!self) {
        return false;
    }
    if (self->pipe == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD bytesAvailable = 0;
    if (::PeekNamedPipe(self->pipe, nullptr, 0, nullptr, &bytesAvailable, nullptr)) {
        if (bytesAvailable >= length) {
            auto bytesToRead = static_cast<DWORD>(length);
            DWORD bytesRead = 0;
            if (::ReadFile(self->pipe, data, bytesToRead, &bytesRead, nullptr) == TRUE) {
                assert(bytesToRead == bytesRead);
                return true;
            }
            else {
                Close();
            }
        }
    }
    else {
        Close();
    }
    return false;
}
#endif

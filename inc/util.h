#pragma once


// Provides Locked access to a container. Taken from https://github.com/microsoft/D3D12TranslationLayer/blob/master/include/ImmediateContext.hpp#L787-L807
template <typename T, typename mutex_t = std::mutex> class LockedContainer
{
    mutex_t m_mutex;
    T m_Obj;
public:
    class LockedAccess
    {
        std::unique_lock<mutex_t> m_Lock;
        T& m_Obj;
    public:
        LockedAccess(mutex_t& lock, T& Obj)
            : m_Lock(std::unique_lock<mutex_t>(lock))
            , m_Obj(Obj)
        {
        }
        T* operator->() { return &m_Obj; }
    };
    // Intended use: GetLocked()->member.
    // The LockedAccess temporary object ensures synchronization until the end of the expression.
    template <typename... Args> LockedContainer(Args&&... args) : m_Obj(std::forward<Args>(args)...) {}
    LockedAccess GetLocked() { return LockedAccess(m_mutex, m_Obj); }
};

template<UINT bufferSize = 256, typename... Args>
int MessageBoxHelper_FormattedBody(UINT messageBoxFlags, LPCWSTR title, LPCWSTR formatString, Args... args)
{
    WCHAR bodyBuff[bufferSize];
    std::swprintf(bodyBuff, bufferSize, formatString, args...);
    return MessageBox(NULL, bodyBuff, title, messageBoxFlags);
}
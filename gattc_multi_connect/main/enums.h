#if !defined(__ENUMS_H__)
#define __ENUMS_H__

enum profile_status {
    DISCONNECTING,
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCOVERING,
    DISCOVERED,
    NOTIFIED, // final state of the device
};

#endif // __ENUMS_H__

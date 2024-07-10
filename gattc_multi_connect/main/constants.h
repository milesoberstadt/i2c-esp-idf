#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#define GATTC_TAG "GATTC_MULTIPLE_CONNECTION"
#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01

/* register three profiles, each profile corresponds to one connection,
   which makes it easy to handle each connection event */
#define PROFILE_NUM 3
#define PROFILE_A_APP_ID 0
#define PROFILE_B_APP_ID 1
#define PROFILE_C_APP_ID 2
#define INVALID_HANDLE   0


#endif
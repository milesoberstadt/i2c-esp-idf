#if !defined(__UI_H__)
#define __UI_H__

#include <memory>

#include "Arduino.h"
#include "Arduino_GFX_Library.h"

#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3
#define GFX_BL 38

class UI 
{
    public:
        static UI& getInstance()
        {
            static UI instance;
            return instance;
        }

        UI(const UI&) = delete;
        UI& operator=(const UI&) = delete;

        Arduino_GFX* getGFX() { return gfx.get(); }

        void background();
        void titleStyle();
        void textStyle();
        void smallTextStyle();

    private:
        UI();
        ~UI();
        int32_t w, h, n, n1, cx, cy, cx1, cy1, cn, cn1;
        uint8_t tsa, tsb, tsc, ds;
        std::unique_ptr<Arduino_GFX> gfx;
        std::unique_ptr<Arduino_DataBus> bus;
};

inline UI& uiInstance()
{
    return UI::getInstance();
}

#endif // __UI_H__

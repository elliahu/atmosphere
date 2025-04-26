#pragma once
#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/keysym.h> 
#endif

namespace hammock{

    typedef unsigned long long Keycode;

    enum class KeyState {
        NONE, DOWN, UP
    };

    enum class ButtonState {
        NONE, DOWN, UP
    };

#define MOUSE_LEFT 0x0
#define MOUSE_RIGHT 0x1
#define MOUSE_MIDDLE 0x2

#if defined(_WIN32)
#define KEY_A 0x41
#define KEY_B 0x42
#define KEY_C 0x43
#define KEY_D 0x44
#define KEY_E 0x45
#define KEY_F 0x46
#define KEY_G 0x47
#define KEY_H 0x48
#define KEY_I 0x49
#define KEY_J 0x4A
#define KEY_K 0x4B
#define KEY_L 0x4C
#define KEY_M 0x4D
#define KEY_N 0x4E
#define KEY_O 0x4F
#define KEY_P 0x50
#define KEY_Q 0x51
#define KEY_R 0x52
#define KEY_S 0x53
#define KEY_T 0x54
#define KEY_U 0x55
#define KEY_V 0x56
#define KEY_W 0x57
#define KEY_X 0x58
#define KEY_Y 0x59
#define KEY_Z 0x5A

#define KEY_F1 VK_F1
#define KEY_F2 VK_F2
#define KEY_F3 VK_F3
#define KEY_F4 VK_F4
#define KEY_F5 VK_F5
#define KEY_F6 VK_F6
#define KEY_F7 VK_F7
#define KEY_F8 VK_F8
#define KEY_F9 VK_F9
#define KEY_F10 VK_F10
#define KEY_F11 VK_F11
#define KEY_F12 VK_F12

#define KEY_ESCAPE VK_ESCAPE
#define KEY_TAB VK_TAB
#define KEY_RETURN VK_RETURN
#define KEY_SHIFT VK_SHIFT
#define KEY_CONTROL VK_CONTROL
#define KEY_MENU VK_MENU
#define KEY_CAPITAL VK_CAPITAL
#define KEY_NUMLOCK VK_NUMLOCK
#define KEY_SCROLL VK_SCROLL
#define KEY_PAUSE VK_PAUSE

#define KEY_UP VK_UP
#define KEY_DOWN VK_DOWN
#define KEY_LEFT VK_LEFT
#define KEY_RIGHT VK_RIGHT

#define KEY_NUMPAD0 VK_NUMPAD0
#define KEY_NUMPAD1 VK_NUMPAD1
#define KEY_NUMPAD2 VK_NUMPAD2
#define KEY_NUMPAD3 VK_NUMPAD3
#define KEY_NUMPAD4 VK_NUMPAD4
#define KEY_NUMPAD5 VK_NUMPAD5
#define KEY_NUMPAD6 VK_NUMPAD6
#define KEY_NUMPAD7 VK_NUMPAD7
#define KEY_NUMPAD8 VK_NUMPAD8
#define KEY_NUMPAD9 VK_NUMPAD9
#define KEY_MULTIPLY VK_MULTIPLY
#define KEY_ADD VK_ADD
#define KEY_SEPARATOR VK_SEPARATOR
#define KEY_SUBTRACT VK_SUBTRACT
#define KEY_DECIMAL VK_DECIMAL
#define KEY_DIVIDE VK_DIVIDE

#define KEY_SPACE VK_SPACE
#define KEY_BACK VK_BACK
#define KEY_DELETE VK_DELETE
#define KEY_INSERT VK_INSERT
#define KEY_HOME VK_HOME
#define KEY_END VK_END
#define KEY_PAGEUP VK_PRIOR
#define KEY_PAGEDOWN VK_NEXT

#define KEY_LSHIFT VK_LSHIFT
#define KEY_RSHIFT VK_RSHIFT
#define KEY_LCONTROL VK_LCONTROL
#define KEY_RCONTROL VK_RCONTROL
#define KEY_LMENU VK_LMENU
#define KEY_RMENU VK_RMENU
#endif

#if defined(__linux__)
#define KEY_A XK_a
#define KEY_B XK_b
#define KEY_C XK_c
#define KEY_D XK_d
#define KEY_E XK_e
#define KEY_F XK_f
#define KEY_G XK_g
#define KEY_H XK_h
#define KEY_I XK_i
#define KEY_J XK_j
#define KEY_K XK_k
#define KEY_L XK_l
#define KEY_M XK_m
#define KEY_N XK_n
#define KEY_O XK_o
#define KEY_P XK_p
#define KEY_Q XK_q
#define KEY_R XK_r
#define KEY_S XK_s
#define KEY_T XK_t
#define KEY_U XK_u
#define KEY_V XK_v
#define KEY_W XK_w
#define KEY_X XK_x
#define KEY_Y XK_y
#define KEY_Z XK_z

#define KEY_F1 XK_F1
#define KEY_F2 XK_F2
#define KEY_F3 XK_F3
#define KEY_F4 XK_F4
#define KEY_F5 XK_F5
#define KEY_F6 XK_F6
#define KEY_F7 XK_F7
#define KEY_F8 XK_F8
#define KEY_F9 XK_F9
#define KEY_F10 XK_F10
#define KEY_F11 XK_F11
#define KEY_F12 XK_F12

#define KEY_ESCAPE XK_Escape
#define KEY_TAB XK_Tab
#define KEY_RETURN XK_Return
#define KEY_SHIFT XK_Shift_L
#define KEY_CONTROL XK_Control_L
#define KEY_MENU XK_Menu
#define KEY_CAPITAL XK_Caps_Lock
#define KEY_NUMLOCK XK_Num_Lock
#define KEY_SCROLL XK_Scroll_Lock
#define KEY_PAUSE XK_Pause

#define KEY_UP XK_Up
#define KEY_DOWN XK_Down
#define KEY_LEFT XK_Left
#define KEY_RIGHT XK_Right

#define KEY_NUMPAD0 XK_KP_0
#define KEY_NUMPAD1 XK_KP_1
#define KEY_NUMPAD2 XK_KP_2
#define KEY_NUMPAD3 XK_KP_3
#define KEY_NUMPAD4 XK_KP_4
#define KEY_NUMPAD5 XK_KP_5
#define KEY_NUMPAD6 XK_KP_6
#define KEY_NUMPAD7 XK_KP_7
#define KEY_NUMPAD8 XK_KP_8
#define KEY_NUMPAD9 XK_KP_9
#define KEY_MULTIPLY XK_KP_Multiply
#define KEY_ADD XK_KP_Add
#define KEY_SEPARATOR XK_KP_Separator
#define KEY_SUBTRACT XK_KP_Subtract
#define KEY_DECIMAL XK_KP_Decimal
#define KEY_DIVIDE XK_KP_Divide

#define KEY_SPACE XK_space
#define KEY_BACK XK_BackSpace
#define KEY_DELETE XK_Delete
#define KEY_INSERT XK_Insert
#define KEY_HOME XK_Home
#define KEY_END XK_End
#define KEY_PAGEUP XK_Prior
#define KEY_PAGEDOWN XK_Next

#define KEY_LSHIFT XK_Shift_L
#define KEY_RSHIFT XK_Shift_R
#define KEY_LCONTROL XK_Control_L
#define KEY_RCONTROL XK_Control_R
#define KEY_LMENU XK_Alt_L
#define KEY_RMENU XK_Alt_R
#endif


}
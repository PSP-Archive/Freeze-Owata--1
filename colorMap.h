// 色のフォーマット0xABGR(R:赤,G:緑,B:青の16進数)
// 例：青=0xFF00, 赤=0xF00F
#ifndef __COLOR_MAP_
#define __COLOR_MAP_

unsigned short __attribute__((aligned(16))) colorPalette[256] = {
	// 全体用 1 15, colorPalette[0]
	0x0000, // Transparent
	0xF999, // RGB(0x99, 0x99, 0x99)
	0xF333, // RGB(0x33, 0x33, 0x33)
	0xFFFF, // WHITE
	0xF000, // BLACK
	0xF00F, // RED
	0xF0F0, // GREEN
	0xFF00, // BLUE
	0xF0FF, // YELLOW
	0xFF0F, // MAGENTA
	0xFFF0, // CYAN
	0xFCCC, // GRAY
	0x8000, // DIALOG_BG_transparent
	0x0000,
	0x0000,
	0x0000,
	// レス本文 13 3 colorPalette[16]
	0xF00F,	// RES_NUMBER
	0xF000,	// RES_NAME_HEAD
	0xF0C0,	// RES_NAME_BODY
	0xF999,	// RES_MAIL
	0xF000,	// RES_DATE
	0xFF00,	// RES_ID_HEAD_1
	0xF00F,	// RES_ID_HEAD_2
	0xFF00,	// RES_ID_BODY
	0xF000,	// RES_TEXT
	0xFEEE,	// RES_BG
	0xFF00,	// RES_LINK
	0xF0FF,	// RES_BAR_SLIDER
	0xFF66,	// RES_BAR_BG
	0x0000,
	0x0000,
	0x0000,
	// レスアンカー 13 3 colorPalette[32]
	0xF00F,	// RES_A_NUMBER
	0xF000,	// RES_A_NAME_HEAD
	0xF0C0,	// RES_A_NAME_BODY
	0xF999,	// RES_A_MAIL
	0xF000,	// RES_A_DATE
	0xFF00,	// RES_A_ID_HEAD_1
	0xF00F,	// RES_A_ID_HEAD_2
	0xFF00,	// RES_A_ID_BODY
	0xF000,	// RES_A_TEXT
	0xFCFF,	// RES_A_BG
	0xFF00,	// RES_A_LINK
	0xFCF0,	// RES_A_BAR_SLIDER
	0xFFFC,	// RES_A_BAR_BG
	0x0000,
	0x0000,
	0x0000,
	// メニューバー 5 11 colorPalette[48]
	0xFFFF,	// MENU_TEXT
	0xF000,	// MENU_BG
	0xF0F0,	// MENU_BATTERY_1
	0xF0FF,	// MENU_BATTERY_2
	0xF00F,	// MENU_BATTERY_3
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	// スレ一覧・お気に入りスレ 14 2 colorPalette[64]
	0xF00F,	// THREAD_NUMBER
	0xF00F,	// THREAD_CATEGORY
	0xFF00,	// THREAD_TEXT_1
	0xF00F,	// THREAD_TEXT_2
	0xFCFC,	// THREAD_BG
	0xF000,	// THREAD_COUNT_1
	0xF00F,	// THREAD_COUNT_2
	0xF00F,	// THREAD_SELECT_NUMBER
	0xF009,	// THREAD_SELECT_CATEGORY
	0xF900,	// THREAD_SELECT_TEXT_1
	0xF009,	// THREAD_SELECT_TEXT_2
	0xFCCC,	// THREAD_SELECT_BG
	0xF000,	// THREAD_SELECT_COUNT_1
	0xF00F,	// THREAD_SELECT_COUNT_2
	0x8000, // THREAD_transparent
	0x0000,
	// カテゴリーにフォーカス 9 7 colorPalette[80]
	0xF03C,	// CATE_ON_TEXT
	0xFFFF,	// CATE_ON_BG
	0xFFFF,	// CATE_ON_S_TEXT
	0xF03C,	// CATE_ON_S_BG
	0xFF66,	// ITA_OFF_TEXT
	0xFCCC,	// ITA_OFF_BG
	0xFCCC,	// ITA_OFF_S_TEXT
	0xFF66,	// ITA_OFF_S_BG
	0xFFFF,	// CATE_ON_BASE
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	// 板一覧にフォーカス 9 7 colorPalette[96]
	0xF698,	// CATE_OFF_TEXT
	0xFCCC,	// CATE_OFF_BG
	0xFCCC,	// CATE_OFF_S_TEXT
	0xF698,	// CATE_OFF_S_BG
	0xFF00,	// ITA_ON_TEXT
	0xFFFF,	// ITA_ON_BG
	0xFFFF,	// ITA_ON_S_TEXT
	0xFF00,	// ITA_ON_S_BG
	0xFFFF,	// CATE_OFF_BASE
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	// 書き込みフォーム 3 13 colorPalette[112]
	0xFCCC,	// FORM_ITA_TEXT
	0xFFFF,	// FORM_TITLE_TEXT
	0xF00F,	// FORM_TITLE_BG
	0x8000, // FORM_BG_COLOR
	0xA000, // BOX_BG_COLOR
	0xFDDD, // BOX_TEXT_BG_COLOR
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	// メニューウィンドウ 4 12 colorPalette[128]
	0xFCCC,	// MENU_WIN_TEXT
	0xF000,	// MENU_WIN_BG
	0xFFFF,	// MENU_WIN_S_TEXT
	0xFF00,	// MENU_WIN_S_BG
	0xF0FF,	// MENU_WIN_DIR
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	// カーソル 3 13 colorPalette[144]
	0xF000,	// MENU_CURSOR_ARROW1
	0xFFFF,	// MENU_CURSOR_ARROW2
	0x3000, // MENU_CURSOR_transparent
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000
};

#endif

/*
* $Id: psp2chNet.c 157 2008-09-16 23:13:56Z bird_may_nike $
*/

#include "psp2ch.h"
#include <psputility.h>
#include <psprtc.h>
#include <pspwlan.h>
#include <pspnet_apctl.h>
#include <stdio.h>
#include "psp2chNet.h"
#include "pg.h"
#include "utf8.h"
#include "libCat/Cat_Network.h"

static int connected = 0;

// 描画処理
// ダイアログ表示のさいの背景を描画する
static void draw_callback( void* pvUserData )
{
	pgGuRender();
}

// 更新処理
// フレームバッファのスワップなど
static void screen_update_callback( void* pvUserData )
{
	flipScreen(0);
}

/*****************************
指定された設定番号のアクセスポイントに接続します
psp2chApConnect()から呼ばれます。
*****************************/
static int connect_to_apctl(int config)
{
    int state, ret = -1;
    clock_t time = sceKernelLibcClock() + 1000000 * 8; // 8秒後
	
    if (sceNetApctlConnect(config) != 0)
    {
        psp2chNormalError(NET_WLAN_ERR, "connect");
        time = 0;
    }
    while (sceKernelLibcClock() < time && ret < 0)
    {
        if (sceNetApctlGetState(&state) != 0)
        {
            psp2chNormalError(NET_WLAN_ERR, "state");
            break;
        }
        if (state < 3)
        {
            pgPrintMenuBar("　APに接続中");
            //stateLast = state;
        }
        else if (state == 3)
        {
            pgPrintMenuBar("　IPアドレスを取得中");
            //stateLast = state;
        }
        else if (state == 4)
        {
        	pgPrintMenuBar("　接続完了");
            ret = 0;  // connected with static IP
        }
		pgCopyMenuBar();
        flipScreen(0);
    }
    return ret;
}

/*****************************
PSP内のアクセスポイント設定を検索します
設定が2個以上あれば選択ダイアログを表示します。
設定が1個のみの場合はその値で接続します。
設定がない場合は-1が返されます。
*****************************/
int psp2chApConnect(void)
{
    struct
    {
        int index;
        char name[128];
    } ap[2];
    int count = 0;
    int i;
	
	if (connected) {
		// IP取得済み
		return 0;
	}
    if (sceWlanGetSwitchState() == 0) {
        pgPrintMenuBar("    LANのスイッチがオフ");
        pgCopyMenuBar();
        flipScreen(0);
        pgWaitVn(30);
        return -2;
    }
    if (sceWlanDevIsPowerOn() == 0) {
        pgPrintMenuBar("    LANの電源が入ってない");
        pgCopyMenuBar();
        flipScreen(0);
        pgWaitVn(30);
        return -2;
    }
    for (i = 1; i < 100; i++) // skip the 0th connection
    {
        if (sceUtilityCheckNetParam(i) != 0)
        	continue;  // more
        sceUtilityGetNetParam(i, 0, (netData*) ap[count].name);
        ap[count].index = i;
        count++;
        if (count > 1)
        	break;  // no more room
    }
    if (count == 1)
    {
        i = connect_to_apctl(ap[0].index);
    }
    else if (count > 1)
    {
        // 返り値 < 0 : エラー
        // 返り値 = 0 : 接続した
        // 返り値 > 0 : キャンセルされた
        i = Cat_NetworkConnect( draw_callback, screen_update_callback, 0 );
        // 全部0なのかな？？？
    }
    // 接続完了
    if (i == 0)
    	connected = 1;
    return i;
}

inline int psp2chIsConnect(void)
{
	int state;
	if (sceNetApctlGetState(&state) < 0) {
    	pgPrintMenuBar("    LAN状態の取得不可");
    	pgCopyMenuBar();
    	flipScreen(0);
    	pgWaitVn(30);
    	connected = 0;
    }
    if (state == PSP_NET_APCTL_STATE_GOT_IP)
    	connected = 1;
    else
    	connected = 0;
	return connected;
}

inline void psp2chDisconnect(void)
{
	sceNetApctlDisconnect();
	connected = 0;
}

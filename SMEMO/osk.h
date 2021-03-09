//==============================================================
// OSKŠÖ˜A
// STEAR 2009
//--------------------------------------------------------------

#ifndef OSK_H
#define OSK_H

typedef enum PspOskState_e {
	OSK_NONE = 0,
	OSK_INIT,
	OSK_VISIBLE,
	OSK_QUIT,
	OSK_FINISHED
} PspOskState;

typedef struct PspOskData_s {
    int unk_00;
    int unk_04;
    int language;
    int unk_12;
    int unk_16;
    int lines;
    int unk_24;
    unsigned short* desc;
    unsigned short* intext;
    int outtextlength;
    unsigned short* outtext;
    int rc;
    int outtextlimit;
} PspOskData;

typedef struct PspOsk_s {
    unsigned int size;
	int language;
	int buttonswap;
	int unk_12; // set 17
	int unk_16; // set 19
	int unk_20; // set 18
	int unk_24; // set 16
	int rc;
	int unk_32;
	int unk_36;
	int unk_40;
	int unk_44;
	int unk_48; // set 1, if 0 nothing happens, if 2 crash ...
	PspOskData* data;
	int unk_56;
	int unk_60;
} PspOsk;

void	oskSetupGu(void);
int		oskInput(char *title,char *intext,char *outtext);

#endif

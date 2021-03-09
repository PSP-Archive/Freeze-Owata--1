
#include "psp2ch.h"
#include <psputility.h>
#include <pspaudio.h>
#include <pspmp3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "psp2chAudio.h"

#define MP3_BUF_SIZE (1024 * 32 + 1472)
#define PCM_BUF_SIZE (1152 * 32)
#define FILE_END 0x80671402

extern S_2CH s2ch; // psp2ch.c

typedef struct {
	int mp3running;
	SceUID fd;
	int handle;
	char startDir[FILE_PATH];
	char playDir[FILE_PATH];
	char playName[FILE_PATH];
	int full_semaid;
	int empty_semaid;
	int mp3Loop;
	int bytesDecoded;
	short *outBuf;
} mp3_data;

static int playThread;
static int decodeThread;
static mp3_data playData = {1, -1, -1, "", "", "", -1, -1, -1, -1, NULL};

static int fillStreamBuffer(SceSize args, void *argp);
static int psp2chPlayThread(SceSize args, void *argp);
static int psp2chPlayEnd(void);

/**********************************
バッファ読み込み
**********************************/
static int fillStreamBuffer(SceSize args, void *argp)
{
	mp3_data *data = *(mp3_data**)argp;
	short *playBuf[2];
	SceUChar8* dst;
	int write, pos, swap = 0;
	SceInt64 ret;
	short *buf;
	char path[FILE_PATH];

	playBuf[0] = memalign(64, sizeof(short) * PCM_BUF_SIZE);
	playBuf[1] = memalign(64, sizeof(short) * PCM_BUF_SIZE);
	if (playBuf[0] == NULL || playBuf[1] == NULL)
	{
		free(playBuf[0]);
		free(playBuf[1]);
		return 0;
	}
	while (1)
	{
		sceKernelWaitSema(data->empty_semaid, 1, NULL);
		sceKernelSignalSema(data->empty_semaid, 0);
		if (sceMp3CheckStreamDataNeeded(data->handle) > 0)
		{
			if (sceMp3GetInfoToAddStreamData(data->handle, &dst, &write, &pos) < 0)
				break;
			sceIoLseek32(data->fd, pos, SEEK_SET);
			ret = sceIoRead(data->fd, dst, write);
			if (ret < 0) {
				break;
			}
			if (sceMp3NotifyAddStreamData(data->handle, ret) < 0)
				break;
		}
		if (data->bytesDecoded > 0)
		{
			data->bytesDecoded = sceMp3Decode(data->handle, &buf);
			memcpy(playBuf[swap], buf, data->bytesDecoded);
			data->outBuf = playBuf[swap];
			swap = 1 - swap;
		}
		sceKernelSignalSema(data->full_semaid, 1);
	}
	free(playBuf[0]);
	free(playBuf[1]);
	return (pos > 0);
}

/**********************************
MP3再生スレッド
**********************************/
static int psp2chPlayThread(SceSize args, void *argp)
{
	char *mp3Buf;
	short *pcmBuf;
	int lastDecoded = 0;
	int volume = PSP_AUDIO_VOLUME_MAX;
	SceMp3InitArg mp3Init;
	int start, end;
	int channel = -1;
	unsigned int numPlayed = 0;
	int samplingRate, numChannels, ret;
	char head[8], path[FILE_PATH];
	mp3_data *data = *(mp3_data**)argp;

	mp3Buf = memalign(64, sizeof(char) * (MP3_BUF_SIZE));
	pcmBuf = memalign(64, sizeof(short) * PCM_BUF_SIZE);
	if (mp3Buf == NULL || pcmBuf == NULL)
	{
		free(mp3Buf);
		free(pcmBuf);
		return -1;
	}
	do{
		// ファイルオープン
		strcpy(path, data->playDir);
		strcat(path, "/");
		strcat(path, data->playName);
		data->fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
		if(data->fd < 0)
		{
			break;
		}
		start = 0;
		end = sceIoLseek32(data->fd, 0, SEEK_END);
		sceIoLseek32(data->fd, 0, SEEK_SET);
		sceIoRead(data->fd, head, 4);
		if(head[0] == 'I' && head[1] == 'D' && head[2] == '3') // ID3v2
		{
			sceIoLseek32(data->fd, 6, SEEK_SET);
			sceIoRead(data->fd, head, 4);
			start = (head[0] << 21) + (head[1] << 14) + (head[2] << 7) + head[3] + 10;
		}
		sceIoLseek32(data->fd, -128, SEEK_END);
		sceIoRead(data->fd, head, 4);
		if(head[0] == 'T' && head[1] == 'A' && head[2] == 'G') // ID3v1
		{
			end -= 128;
		}
		// MP3の初期設定
		if(sceMp3InitResource() < 0)
		{
			sceIoClose(data->fd);
			break;
		}
		mp3Init.mp3StreamStart = start;
		mp3Init.mp3StreamEnd = end;
		mp3Init.unk1 = 0;
		mp3Init.unk2 = 0;
		mp3Init.mp3Buf = mp3Buf;
		mp3Init.mp3BufSize = MP3_BUF_SIZE;
		mp3Init.pcmBuf = pcmBuf;
		mp3Init.pcmBufSize = PCM_BUF_SIZE;
		data->handle = sceMp3ReserveMp3Handle(&mp3Init);
		if(data->handle < 0)
		{
			sceMp3TermResource();
			sceIoClose(data->fd);
			break;
		}
		data->bytesDecoded = 0;
		data->full_semaid = sceKernelCreateSema("full_sema", 0, 0, 1, 0);
		data->empty_semaid = sceKernelCreateSema("empty_sema", 0, 1, 1, 0);
		sceKernelStartThread(decodeThread, sizeof(mp3_data**), &data);
		sceKernelWaitSema(data->full_semaid, 1, NULL);
		sceKernelSignalSema(data->full_semaid, 0);
		ret = sceMp3Init(data->handle);
		if(ret < 0)
		{
			sceKernelTerminateThread(decodeThread);
			sceKernelDeleteSema(data->empty_semaid);
			sceKernelDeleteSema(data->full_semaid);
			sceMp3ReleaseMp3Handle(data->handle);
			sceMp3TermResource();
			sceIoClose(data->fd);
			break;
		}
		data->bytesDecoded = 1;
		sceKernelSignalSema(data->empty_semaid, 1);
		sceMp3SetLoopNum(data->handle, data->mp3Loop);
		samplingRate = sceMp3GetSamplingRate(data->handle);
		numChannels = sceMp3GetMp3ChannelNum(data->handle);
		// 再生開始
		for (;;)
		{
			if (data->mp3running)
			{
				if (data->mp3running == 1) // 終了
					break;
				sceKernelSleepThread(); // 一時停止
				continue;
			}
			SceUInt timeout = DISPLAY_WAIT;
			if (sceKernelWaitSema(data->full_semaid, 1, &timeout) < 0){ // 読み込み
				break;
			}
			sceKernelSignalSema(data->full_semaid, 0);
			if(data->bytesDecoded <= 0) // decode error
			{
				if (data->bytesDecoded == 0 || data->bytesDecoded == (int)FILE_END) // file end
				{
					sceMp3ResetPlayPosition(data->handle);
					numPlayed = 0;
					if (data->mp3Loop == -1)
						continue;
					data->mp3running = 3;
				}
				break;
			}
			else
			{
				if(channel < 0 || lastDecoded != data->bytesDecoded)
				{
					if(channel >= 0)
					{
						sceAudioChRelease(channel);
					}
					channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 1152, PSP_AUDIO_FORMAT_STEREO);
				}
				lastDecoded = data->bytesDecoded;
				while (sceAudioGetChannelRestLength(channel))
					sceKernelDelayThread(ACCESS_WAIT);
				numPlayed += sceAudioOutput(channel, volume, data->outBuf);
				sceKernelSignalSema(data->empty_semaid, 1);
			}
		}
		// 終了処理
		if(channel >= 0)
		{
			sceAudioChRelease(channel);
			channel = -1;
		}
		sceKernelTerminateThread(decodeThread);
		sceKernelDeleteSema(data->full_semaid);
		sceKernelDeleteSema(data->empty_semaid);
		sceKernelDelayThread(100000);
		sceMp3ReleaseMp3Handle(data->handle);
		sceMp3TermResource();
		sceIoClose(data->fd);
	}while(psp2chPlayEnd() == 0);
	free(pcmBuf);
	free(mp3Buf);
	sceKernelExitThread(0);
	return 0;
}

//===== サブフォルダを含めて次のmp3ファイルを探す =====

static void psp2chPlaySearch(char *playDir, int *next, char *play_1st, char *play_next)
{
	mp3_data	*data = &playData;
	SceUID		dfd;
	SceIoDirent	dir;
	char		*p, path[FILE_PATH];

	dfd = sceIoDopen(playDir);
	if (dfd < 0) return;

	memset(&dir, 0, sizeof(dir));
	while (sceIoDread(dfd, &dir) > 0) {
		if(strcmp(dir.d_name, ".") == 0) continue;
		if(strcmp(dir.d_name, "..") == 0) continue;
		if (dir.d_stat.st_attr & FIO_SO_IFDIR) {				// ディレクトリの場合
			strcpy(path, playDir);
			strcat(path, "/");
			strcat(path, dir.d_name);
			psp2chPlaySearch(path, next, play_1st, play_next);
			if (*next == 2) break;								// サブディレクトリ内で次のファイルを見つけた
		} else {												// ファイルの場合
			p = strrchr(dir.d_name, '.');
			if (p) {
				if (stricmp(p, ".mp3") == 0) {
					if (!play_1st[0]) {
						strcpy(play_1st, dir.d_name);
						strcpy(data->playDir, playDir);			// 取りあえず最初に見つけたファイルのディレクトリ位置にセット
					}
					if (*next) {
						*next = 2; 								// 発見
						strcpy(play_next, dir.d_name);
						strcpy(data->playDir, playDir);			// 対象ファイルのディレクトリ位置をセット
						break;
					}
					if (strcmp(dir.d_name, data->playName) == 0) {
						*next = 1; 								// 次の候補が対象
					}
				}
			}
		}
	}
	sceIoDclose(dfd);
}

//===== 次のmp3ファイルへ =====

static int psp2chPlayEnd(void)
{
	mp3_data *data = &playData;
	SceUID dfd;
	SceIoDirent dir;
	int next = 0;
	char play_1st[FILE_PATH] = {0}, play_next[FILE_PATH] = {0};
	char *p;

	if(data->mp3running != 3)
		return -1;

	if (s2ch.cfg.bgmLoop == 3){									// 指定フォルダ以下を検索
		psp2chPlaySearch(s2ch.cfg.bgmDir, &next, play_1st, play_next);
	} else if (s2ch.cfg.bgmLoop == 2){							// サブフォルダを含めて検索
		psp2chPlaySearch(data->startDir, &next, play_1st, play_next);
	} else {													// フォルダ内検索
		dfd = sceIoDopen(data->startDir);
		if (dfd >= 0)
		{
			memset(&dir, 0, sizeof(dir));
			while (sceIoDread(dfd, &dir) > 0)
			{
				p = strrchr(dir.d_name, '.');
				if (p){
					if (stricmp(p, ".mp3") == 0)
					{
						if (!play_1st[0])
						{
							strcpy(play_1st, dir.d_name);
						}
						if (next)
						{
							next = 2; // 発見
							strcpy(play_next, dir.d_name);
							break;
						}
						if (strcmp(dir.d_name, data->playName) == 0)
						{
							next = 1; // 次の候補が対象
						}
					}
				}
			}
			sceIoDclose(dfd);
		}
	}
	if(next != 1)
	{
		strcpy(data->playName, play_next);
	}
	else
	{
		strcpy(data->playName, play_1st);
	}
	data->mp3running = 0;
	return 0;
}

/**********************************
初期設定
**********************************/

int psp2chAudioInit(void)
{
	char msg[256];
	mp3_data *data = &playData;

	if (s2ch.cfg.bgmLoop) data->mp3Loop = 0;

	strcpy( msg,"AV module load error\n" );
	if(sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC) < 0)
	{
		strcat( msg, "  AV_AVCODEC\n" );
//		psp2chNormalError(MOD_LOAD_ERR, "AV_AVCODEC");
//		return -1;
	}
	if(sceUtilityLoadModule(PSP_MODULE_AV_MP3) < 0)
	{
		strcat( msg,"  AV_MP3\n" );
//		psp2chNormalError(MOD_LOAD_ERR, "AV_MP3");
//		return -1;
	}
	if (strlen(msg)!=21){
		strcat( msg,"\n\n以後、音楽再生で問題が起こる可能\性がありますがこのまま続行します。" );
		psp2chErrorDialog(MOD_LOAD_ERR, msg);
	}
	playThread = sceKernelCreateThread("play_thread", (SceKernelThreadEntry)&psp2chPlayThread, 0x11, 0x2000, 0, NULL);
	if(playThread < 0)
	{
		return -1;
	}
	decodeThread = sceKernelCreateThread("decode_thread", (SceKernelThreadEntry)&fillStreamBuffer, 0x11, 0x0800, 0, NULL);
	if (decodeThread < 0)
		return -1;
	return 0;
}

/**********************************
終了処理
**********************************/

void psp2chAudioTerm(void)
{
	mp3_data *data = &playData;
	if(data->mp3running != 1)
		psp2chAudioStop();
	if (decodeThread > 0)
		sceKernelDeleteThread(decodeThread);
	if (playThread > 0)
		sceKernelDeleteThread(playThread);
	sceUtilityUnloadModule(PSP_MODULE_AV_AVCODEC);
	sceUtilityUnloadModule(PSP_MODULE_AV_MP3);	
}

int psp2chAudioPlay(const char *path, const char *name)
{
	char old[FILE_PATH];
	static char file[FILE_PATH];
	mp3_data *data = &playData;
	
	strcpy(old, data->playDir);
	strcat(old, "/");
	strcat(old, data->playName);
	strcpy(file, path);
	strcat(file, "/");
	strcat(file, name);
	if (data->mp3running == 0 && strcmp(old, file) == 0) // 一時停止
	{
		data->mp3running = 2;
		return 0;
	}
	if (data->mp3running == 0) // 再生中なら停止
	{
		psp2chAudioStop();
	}
	if (data->mp3running == 1) // 再生
	{
		strcpy(data->startDir, path);
		strcpy(data->playDir, path);
		strcpy(data->playName, name);
	}
	data->mp3running = 2;
	sceKernelStartThread(playThread, sizeof(mp3_data**), &data);
	data->mp3running = 0;
	sceKernelWakeupThread(playThread);
	return 0;
}

void psp2chAudioStop(void)
{
	mp3_data *data = &playData;
	int temp = data->mp3running;
	if(data->mp3running != 1)
	{
		data->mp3running = 1;
    	if(temp == 2)
    		sceKernelWakeupThread(playThread);
    	sceKernelWaitThreadEnd(playThread, NULL);
	}
	data->playDir[0] = STREAM_END;
	data->playName[0] = STREAM_END;
}

void psp2chAudioSetLoop(int loop)
{
	mp3_data *data = &playData;
	data->mp3Loop = loop;
	if (data->handle > 0)
		sceMp3SetLoopNum(data->handle, data->mp3Loop);
}

int psp2chAudioGetLoop(void)
{
	mp3_data *data = &playData;
	return data->mp3Loop;
}


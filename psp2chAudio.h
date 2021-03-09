
#ifndef __PSP2CH_AUDIO_H__
#define __PSP2CH_AUDIO_H__

int psp2chAudioInit(void);
void psp2chAudioTerm(void);
int psp2chAudioPlay(const char *path, const char *name);
void psp2chAudioStop(void);
void psp2chAudioSetLoop(int loop);
int psp2chAudioGetLoop(void);

#endif

/*
* $Id: psp2chNet.h 111 2008-05-30 09:01:34Z bird_may_nike $
*/

#ifndef __PSP2CH_NET_H__
#define __PSP2CH_NET_H__

enum NET_METHOD
{
	NET_METHOD_GET,
	NET_METHOD_POST
};

typedef struct {
    int Content_Length;
    char Content_Type[64];
    char Last_Modified[64];
    char ETag[64];
    char location[FILE_PATH];
    int Range;
    int isGzip;
} HTTP_HEADERS;

typedef struct {
	int status;
	HTTP_HEADERS head;
	char* Header;
	char* body;
	int length;
	char *cook;
	// 送受信用データ
	int method;
	char *host;
	unsigned short port;
	char *path;
	char *requestText;
	char *referer;
	int recved;
	int aborting;
} S_NET;

int psp2chNetInit(void);
void psp2chNetTerm(void);
int psp2chGet(char* url, char* referer, char* cook, S_NET* net, int redirect);
int psp2chPost(char* url, char* referer, char* cook, char *body, S_NET* net);
int psp2chNetMaruLogin(char *dst, char *url, const char *body);

#endif

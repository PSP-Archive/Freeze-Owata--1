/*
* $Id: psp2chNet.c 157 2008-09-16 23:13:56Z bird_may_nike $
*/

#include "psp2ch.h"
#include <psprtc.h>
#include <pspwlan.h>
#include <pspnet.h>
#include <pspnet_apctl.h>
#include <pspnet_inet.h>
#include <pspnet_resolver.h>
#include <psputility_netmodules.h>
#include <psphttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/select.h>
#include <zlib.h>
#include <errno.h>
#include "psp2chNet.h"
#include "psp2chWlan.h"
#include "charConv.h"
#include "pg.h"
#include "utf8.h"
#include "libCat/Cat_Network.h"
#include "libCat/Cat_Resolver.h"

#define owataVer "freeze owata+1/1.00" // version
#define TCP_SIZE (4 * 1024) // TCP stack size
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

enum NET_RECV_PROCESS
{
	RECV_FINISH,
	READ_BODY,
	PARSE_HEADER,
	READ_HEADER
};

enum HTTP_RET
{
	SOCK_ABRT_ERR = -7,
	SOCK_MEM_ERR = -6,
	SOCK_TOUT_ERR = -5,
	SOCK_RECV_ERR = -4,
	SOCK_SEND_ERR = -3,
	SOCK_CNCT_ERR = -2,
	SOCK_OPEN_ERR = -1,
	SOCK_USING = 0,
	SOCK_COMP = 1,
	SOCK_HEAD = 2,
};

extern S_2CH s2ch;				// psp2ch.c
extern char keyWords[128];		// psp2ch.c

static int getHttpThread = -1;

// prototype
static int psp2chResolve(const char* host, struct in_addr* addr);
static int psp2chNetHttp(S_NET* net);
static int psp2chMakeRequestHeader(S_NET* net);
static int psp2chNetDataCtrl(SceSize args, void *argp);
static int psp2chConnect(S_NET* net, int sock);
static int psp2chSendRequest(S_NET* net, int sock);
static int psp2chRecvResponse(S_NET* net, int sock);
static int psp2chParseHeader(char* header, char*** hName, char*** hValue);
static int psp2chGetHttpHeader(int count, char** hName, char** hValue, S_NET* net);
static int gzipDecompress(char **out, const int isGzip, const char* src, const int srcSize);
static int psp2chNetDecodeChunk(char *src);
static int psp2chNetUrlParser(int *method, char **host, unsigned short *port, char **path, char *url);

/*****************************
�l�b�g���W���[���̃��[�h
������
*****************************/
int psp2chNetInit(void)
{
    int ret;
    char msg[256];

	strcpy( msg,"NET module load error\n" );
    ret = sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    if (ret < 0)
    {
		strcat( msg,"  COMMON\n" );
//        psp2chNormalError(MOD_LOAD_ERR, "COMMON");
//        return ret;
    }
    ret = sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
    if (ret < 0)
    {
		strcat( msg,"  INET\n" );
//        psp2chNormalError(MOD_LOAD_ERR, "INET");
//        return ret;
    }
    ret = sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEURI);
    if (ret < 0)
    {
		strcat( msg,"  PARSEURI\n" );
//        psp2chNormalError(MOD_LOAD_ERR, "PARSEURI");
//        return ret;
    }
    ret = sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEHTTP);
    if (ret < 0)
    {
		strcat( msg,"  PARSEHTTP\n" );
//        psp2chNormalError(MOD_LOAD_ERR, "PARSEHTTP");
//        return ret;
    }
    ret = sceUtilityLoadNetModule(PSP_NET_MODULE_HTTP);
    if (ret < 0)
    {
		strcat( msg,"  HTTP\n" );
//        psp2chNormalError(MOD_LOAD_ERR, "HTTP");
//        return ret;
    }
    ret = sceUtilityLoadNetModule(PSP_NET_MODULE_SSL);
    if (ret < 0)
    {
		strcat( msg,"  SSL\n" );
//        psp2chNormalError(MOD_LOAD_ERR, "SSL");
//        return ret;
    }
    ret = Cat_NetworkInit();
    if (ret < 0)
    {
		strcat( msg,"  Cat_NetworkInit\n" );
//        psp2chErrorDialog(0, "Cat_NetworkInit error");
//        return ret;
    }
	ret = Cat_ResolverInitEngine();
	if (ret < 0)
	{
		strcat( msg,"  Cat_ResolverInit\n" );
//		psp2chErrorDialog(0, "Cat_ResolverInit error");
//		return ret;
	}
	if (strlen(msg)!=22){
		strcat( msg,"\n\n�Ȍ�A�ʐM�֌W�Ŗ�肪�N����\\��������܂������̂܂ܑ��s���܂��B" );
		psp2chErrorDialog(MOD_LOAD_ERR, msg);
	}
	getHttpThread = sceKernelCreateThread("http_thread", (SceKernelThreadEntry)&psp2chNetDataCtrl, 0x14, 0x1800, 0, NULL);
	if(getHttpThread < 0)
		return getHttpThread;

	return 0;
}

/*****************************
�I������
*****************************/
void psp2chNetTerm(void)
{
	sceKernelDeleteThread(getHttpThread);
	Cat_ResolverTermEngine();
    Cat_NetworkTerm();
	sceUtilityUnloadNetModule(PSP_NET_MODULE_SSL);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_HTTP);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_PARSEHTTP);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_PARSEURI);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
	sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);
}

/*****************************
���O�����@�z�X�g����IP�A�h���X�ɕϊ�
Sfiya�L����̃p�b�`�g�p
*****************************/
static int psp2chResolve(const char* host, struct in_addr* addr)
{
    if(Cat_ResolverURL( host, addr ) < 0 )
    {
        pgPrintMenuBar("  Cat_ResolverURL error");
		pgCopyMenuBar();
        flipScreen(0);
        return -1;
    }
    return 0;
}

/*****************************
HTTP �� GET���N�G�X�g
�\�P�b�g���쐬
psp2chRequest();�Ń��N�G�X�g�w�b�_���M
�{����M
�\�P�b�g�����
�������ɂ�0��Ԃ�
*****************************/
int psp2chGet(char* url, char* referer, char* cook, S_NET* net, int redirect)
{
    char requestText[NET_REQUEST_HEADER], tmp[FILE_PATH];
    char *p;
    int ret, method;

	strcpy(tmp, url);
    do {
		if (psp2chNetUrlParser(&method, &(net->host), &(net->port), &(net->path), tmp)) {
			return -1;											// URL���s��
		}
		net->method = NET_METHOD_GET;
		net->cook = cook;
		net->requestText = requestText;
		net->referer = referer;
		net->body = NULL;
		// ���N�G�X�g�w�b�_�쐬
		psp2chMakeRequestHeader(net);
		ret = psp2chNetHttp(net);
		if (ret < 0)
			break;
		if (redirect) {
			if (net->status != 301 && net->status != 302)
				break;
			free(net->body);
			if (strstr(net->head.location, "http://"))
				strlcpy(tmp, net->head.location, FILE_PATH);
			else if (net->head.location[0] == '/')
				sprintf(tmp, "http://%s/%s", net->host, net->head.location);
			else {
				p = strrchr(net->path, '/');
				*p = '\0';
				sprintf(tmp,"http://%s/%s/%s", net->host, net->path, net->head.location);
			}
			memset(net, 0, sizeof(S_NET));
		}
	} while (redirect--);
	return ret;
}

/*****************************
HTTP �� POST ���M���܂��B
�\�P�b�g�쐬
psp2chRequest();�Ń��N�G�X�g�w�b�_���M
�{�f�B�𑗐M
�\�P�b�g�����
�F�ؗv�����ꂽ�ꍇ�̓��[�U�[���ƃp�X���[�h��t�����čđ��M
�i���Ӗ����ۂ��̂Ŏ������~�j
�������ɂ�0��Ԃ�
  -1 ���̑��̃G���[
  -2 ����LAN�̃X�C�b�`��OFF/�d����OFF
*****************************/
int psp2chPost(char* url, char* referer, char* cook, char *body, S_NET* net)
{
    char requestText[NET_REQUEST_HEADER], tmp[FILE_PATH];
	int method, ret, retry;

	do {														// �u���E�U�F�ؗp���[�v
		char user[64]={0}, pass[64]={0}, buf[FILE_PATH], base64[128];
		retry = 0;
		strcpy(tmp, url);
		psp2chNetUrlParser(&method, &(net->host), &(net->port), &(net->path), tmp);
		net->method = NET_METHOD_POST;
		net->cook = cook;
		net->requestText = requestText;
		net->referer = referer;
		net->body = body;
		// ���N�G�X�g�쐬
		psp2chMakeRequestHeader(net);
		ret = psp2chNetHttp(net);
		if (ret) return ret;									// �G���[���N���Ă����璆�f
		if (net->status == 401) {								// �F�ؗv����������
			if (psp2chInputDialog("�F�ؗp���[�U�[�������", "���[�U�[��", user)) {
				return -1;
			}
			strcpy(user, keyWords);
			pgRewrite();										// ��ʂ��ĕ`��
			if (psp2chInputDialog("�F�ؗp�p�X���[�h�����", "�p�X���[�h", pass)) {
				return -1;
			}
			pgRewrite();										// ��ʂ��ĕ`��
			if (strstr(net->head.Authenticate, "Basic")) {		// Basic�F�؂̏ꍇ
				retry = 1;
				strcpy(pass, keyWords);
				strcpy(buf, user);
				strcat(buf, ":");
				strcat(buf, pass);
				psp2chBase64(base64, buf);
				sprintf(buf, "Basic %s", base64);
				memset(net, 0, sizeof(S_NET));
				strcpy(net->head.Authorization, buf);
			} else if (strstr(net->head.Authenticate, "Digest")) {	// Digest�F�؂̏ꍇ�i�������j
				psp2chErrorDialog(1, "Digest�F�؂��v������܂����������𒆒f���܂�\n\n%s", net->head.Authenticate);
				return -1;
			}
		}
	} while (retry);

	return 0;
}

static int psp2chNetHttp(S_NET* net)
{
	char **hName, **hValue, buff[32];
	int ret, count, i;
	
	// �`��o�b�t�@���R�s�[
	pgReDraw();
	
	// �l�b�g���[�N�ڑ�
	if (!psp2chIsConnect()) {
		ret = psp2chApConnect();
		if (ret < 0 && ret != (int)0x80110405) {
			if (ret == -1) {
				psp2chErrorDialog(1, "�A�N�Z�X�|�C���g���ݒ肳��Ă��܂���");
			}
			return ret;
		}
		pgRewrite();											// �_�C�A���O�ŏ����ꂽ��ʂ��ĕ`��
	}
	
	// ����M�X���b�h�N��
	net->aborting = SOCK_USING;
	net->recved = -4;
	sceKernelStartThread(getHttpThread, sizeof(S_NET*), &net);
	// ���~���邽�߂̃��[�v
	while (net->aborting == SOCK_USING)
	{
		sceCtrlPeekBufferPositive(&s2ch.pad, 1);
		if (s2ch.pad.Buttons & PSP_CTRL_CROSS)
		{
			s2ch.oldPad = s2ch.pad;
			net->aborting = SOCK_ABRT_ERR;
		}
		switch (net->recved)
		{
			case -1: strcpy(buff, "�ڑ�����"); break;
			case -2: strcpy(buff, "���M����"); break;
			case -3: strcpy(buff, "��M����"); break;
			case -4: strcpy(buff, "�ڑ��J�n"); break;
			default: sprintf(buff, "��M %04d B", net->recved);
		}
		pgPrintMenuBar(buff);
		pgCopyMenuBar();
		flipScreen(0);
	}
	sceKernelWaitThreadEnd(getHttpThread, NULL);
	switch (net->aborting)
	{
		case SOCK_COMP: // �S��M
		case SOCK_HEAD: // �w�b�_�܂Ŏ�M
		case SOCK_ABRT_ERR: // ���f
			strcpy(buff, "err: abort"); break;
		case SOCK_TOUT_ERR:
			strcpy(buff, "err: timeout"); break;
		case SOCK_MEM_ERR:
			strcpy(buff, "err: malloc"); break;
		default:
			sprintf(buff, "err: socket %d", net->aborting); break;
	}
	if (net->aborting < 0)
	{
		pgPrintMenuBar(buff);
		pgCopyMenuBar();
		flipScreen(0);
		pgWaitVn(30);
		return -1;
	}
	if (0){ // for debug
		SceUID fd = sceIoOpen("ms0:/dump_header.txt", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
		if (fd > 0)
		{
			sceIoWrite(fd, net->requestText, strlen(net->requestText));
			sceIoWrite(fd, "\n", 1);
			sceIoWrite(fd, net->Header, strlen(net->Header));
			sceIoClose(fd);
		}
	}
	count = psp2chParseHeader(net->Header, &hName, &hValue);
	psp2chGetHttpHeader(count, hName, hValue, net);
	for (i = 0; i < count; i++)
	{
		free(hName[i]);
		free(hValue[i]);
	}
	ret = 0;
	if (net->head.isGzip)
	{
		char *dst = NULL;
		int size;
		size = gzipDecompress(&dst, net->head.isGzip, net->body, net->length);
		if (size < 0)
		{
			ret = -1;
		}
		net->length = size;
		free(net->body);										// psp2chRecvResponse()�Ŋm�ۂ������������J��
		net->body = dst;
	}
	free(hName);
	free(hValue);
	free(net->Header);
    return ret;
}

/* GET, POST �֐����ō쐬 */
static int psp2chMakeRequestHeader(S_NET* net)
{
	const char* userAgent = "Monazilla/1.00";
	const char* httpVer = "HTTP/1.1";
	char buf[NET_COOKIE_LENGTH], *p;
	
	p = strrchr(net->path, '#');
	if (p)
	{
		*p = STREAM_END;
	}
	// ���N�G�X�g�w�b�_�쐬
	switch(net->method)
	{
		case NET_METHOD_GET:
			sprintf(buf, "GET /%s %s\r\n", net->path, httpVer);
			break;
		case NET_METHOD_POST:
			sprintf(buf, "POST /%s %s\r\n", net->path, httpVer);
			break;
	}
	strcpy(net->requestText, buf);
	sprintf(buf, "Host: %s\r\n", net->host);
	strcat(net->requestText, buf);
	sprintf(buf, "User-Agent: %s (%s)\r\n", userAgent, owataVer);
	strcat(net->requestText, buf);
	strcat(net->requestText, "Accept-Language: ja, en\r\n");
	if (net->referer && *net->referer) {
		sprintf(buf, "Referer: %s\r\n", net->referer);
		strcat(net->requestText, buf);
	}
	if (net->head.Range == 0)
		strcat(net->requestText, "Accept-Encoding: gzip, deflate\r\n");
	if (net->cook  && *net->cook) {
		sprintf(buf, "Cookie: %s\r\n", net->cook);
		strcat(net->requestText, buf);
	}
	if (net->head.Last_Modified && *net->head.Last_Modified) {
		sprintf(buf, "If-Modified-Since: %s\r\n", net->head.Last_Modified);
		strcat(net->requestText, buf);
	}
	if (net->head.ETag && *net->head.ETag) {
		sprintf(buf, "If-None-Match: %s\r\n", net->head.ETag);
		strcat(net->requestText, buf);
	}
	if (net->head.Authorization && strlen(net->head.Authorization)) {
		sprintf(buf, "Authorization: %s\r\n", net->head.Authorization);
		strcat(net->requestText, buf);
	}
	if (net->head.Range) {
		sprintf(buf, "Range: bytes= %d-\r\n", net->head.Range);
		strcat(net->requestText, buf);
		net->head.Range = 0;
	}
	if (net->body) {
		int size = strlen(net->body);
		strcat(net->requestText, "Content-Type: application/x-www-form-urlencoded\r\n");
		sprintf(buf, "Content-Length: %d\r\n\r\n", size);
		strcat(net->requestText, buf);
	}
	else
		strcat(net->requestText, "Connection: close\r\n\r\n");

	if (strlen(net->requestText) > NET_REQUEST_HEADER)
	{
		psp2chErrorDialog(0, "over flow\n%s", buf);
		// �I�[�o�[�t���[
		return -1;
	}
	return 0;
}

/* �X���b�h�p */
static int psp2chNetDataCtrl(SceSize args, void *argp)
{
	S_NET *net = *((S_NET**)argp);
	int sock = -1, ret = 0;
	
	switch (ret)
	{
	case 0:	// �\�P�b�g�쐬
		sock = sceNetInetSocket(PF_INET, SOCK_STREAM, 0 );
		if (sock < 0) {
			ret = SOCK_OPEN_ERR;
			break;
		}
		pgWaitVn(1);
	case 1:	// �R�l�N�g
		ret = psp2chConnect(net, sock);
		if (ret != 0)
			break;
		net->recved = -1;
		pgWaitVn(1);
	case 2:	// ���N�G�X�g���M
		ret = psp2chSendRequest(net, sock);
		if (ret != 0)
			break;
		net->recved = -2;
		pgWaitVn(1);
	case 3:	// ���X�|���X��M
		ret = psp2chRecvResponse(net, sock);
		if (ret != 0)
			break;
		net->recved = -3;
		ret = SOCK_COMP;
		pgWaitVn(1);
	}
	// �I������
	sceNetInetShutdown(sock, SHUT_RDWR);
	sceNetInetClose(sock);
	net->aborting = ret;
	return ret;
}

static int psp2chConnect(S_NET* net, int sock)
{
	int ret, NoBlock, count = 30;
	struct in_addr addr;
	struct sockaddr_in sain0;
	fd_set wmask;
	struct timeval tv = {1, 0}; // 1�b
	
	// DNS
	ret = psp2chResolve(net->host, &addr);
	if (ret < 0) 
	{
	    return SOCK_CNCT_ERR;
	}
	// Tell the socket to connect to the IP address we found, on port 80 (HTTP)
	sain0.sin_family = AF_INET;
	sain0.sin_port = htons(net->port);
	sain0.sin_addr.s_addr = addr.s_addr;
	memset (sain0.sin_zero, (int)0, sizeof(sain0.sin_zero));
	// connect
	NoBlock = 1;
	sceNetInetSetsockopt(sock, SOL_SOCKET, SO_NONBLOCK, &NoBlock, sizeof(NoBlock));
	ret = sceNetInetConnect(sock, (struct sockaddr *)&sain0, sizeof(sain0) );
	if (sceNetInetGetErrno() != EINPROGRESS)
	{
		return SOCK_CNCT_ERR;
	}
	while (net->aborting == SOCK_USING)
	{
		FD_ZERO(&wmask);
		FD_SET(sock, &wmask);
		ret = sceNetInetSelect(sock + 1, NULL, &wmask, NULL, &tv);
		switch(ret)
		{
			case 1: // WR OK
				NoBlock = 0;
				sceNetInetSetsockopt(sock, SOL_SOCKET, SO_NONBLOCK, &NoBlock, sizeof(NoBlock));
				return 0;
			case 0: // Time Out
				if (--count == 0)
					return SOCK_TOUT_ERR;
				break;
			default: // error
				return SOCK_CNCT_ERR;
		}
		pgWaitVn(1);
	}
	return SOCK_ABRT_ERR;
}

static int psp2chSendRequest(S_NET* net, int sock)
{
	int ret, sendSize, limit, progress = 1, time = 30;
	char *p;
	fd_set wmask;
	struct timeval tv = {1, 0}; // 1�b
	
	if (net->method == NET_METHOD_POST)
	{
		sendSize = strlen(net->body);
		p = net->body;
	}
	// send request
	while (progress != 0)
	{
		FD_ZERO(&wmask);
		FD_SET(sock, &wmask);
		ret = sceNetInetSelect(sock + 1, NULL, &wmask, NULL, &tv);
		if (ret < 0) {
			net->body = NULL;
			return SOCK_SEND_ERR;
		} else if (ret == 0)
		{
			if (--time == 0) {
				net->body = NULL;
				return SOCK_TOUT_ERR;
			}
		}
		else {
			if (FD_ISSET(sock, &wmask))
			{
				switch(progress)
				{
				case 1: // request
					limit = strlen(net->requestText);
					if (sceNetInetSend(sock, net->requestText, limit, 0) != limit) {
						net->body = NULL;
						return SOCK_SEND_ERR;
					}
					progress = (net->method == NET_METHOD_POST) ? 2 : 0;
					break;
				case 2: // body
					limit = (sendSize > 1024) ? 1024 : sendSize;
					if (sceNetInetSend(sock, p, limit, 0) != limit) {
						net->body = NULL;
						return SOCK_SEND_ERR;
					}
					sendSize -= limit;
					p += limit;
					progress = (sendSize != 0) ? 2 : 0;
				}
			}
		}
		if (net->aborting != SOCK_USING) {
			net->body = NULL;
			return SOCK_ABRT_ERR;
		}
		pgWaitVn(1);
	}
	net->body = NULL;
    return 0;
}

static int psp2chRecvResponse(S_NET* net, int sock)
{
	static char __attribute__((aligned(16))) buf[TCP_SIZE + 4];
	char *recvHeader, *recvBody, *p, *q, *r;
	int ret, eCode, size, chunkSize, progress = READ_HEADER, time = 30, start = 0, chunked = 0;
	unsigned int recvSize, headerSize, bodySize;
	fd_set rmask;
	struct timeval tv = {1, 0};
	
	recvHeader = (char*)malloc(2048 * sizeof(char));
	recvBody = NULL;
	buf[TCP_SIZE] = STREAM_END;
	headerSize = 0;
	bodySize = 100 * 1024;
	recvSize = 0;
	chunkSize = 0;
	while (progress > 0)
	{
		FD_ZERO(&rmask);
		FD_SET(sock, &rmask);
		ret = sceNetInetSelect(sock + 1, &rmask, NULL, NULL, &tv);
		if (ret < 0)
		{
			eCode = SOCK_RECV_ERR;
			goto ERROR;
		}
		else if (ret == 0)
		{
			if (--time == 0)
			{
				eCode = SOCK_TOUT_ERR;
				goto ERROR;
			}
			pgWaitVn(6); // 0.1�b�҂�
		}
		else if (FD_ISSET(sock, &rmask))
		{
			size = sceNetInetRecv(sock, buf + start, TCP_SIZE - start, 0);
			buf[size + start] = STREAM_END;
			net->recved = size;
			if (0) {
				char buff[32];
				SceUID fd = sceIoOpen("ms0:/dump_recv.txt", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, FILE_PARMISSION);
				sceIoWrite(fd, buf + start, size);
				sprintf(buff, "%d\n", size);
				sceIoWrite(fd, buff, strlen(buff));
				sceIoClose(fd);
			}
			if (size == 0)
			{
				break;
			}
			else if (size < 0)
			{
				eCode = SOCK_RECV_ERR;
				goto ERROR;
			}
			p = buf;
			switch(progress)
			{
				case READ_HEADER:
					if ((p = strstr(buf, "\r\n\r\n")) == NULL)
					{
						start += size;
						break;
					}
					else
					{
						p += strlen("\r\n");
						*p = '\0';
						headerSize = strlen(buf);
						memcpy(recvHeader, buf, headerSize);
						recvHeader[headerSize] = '\0';
						p += strlen("\r\n");
						size -= headerSize - start + strlen("\r\n");
						start = 0;
						progress--;
					}
				case PARSE_HEADER:
					net->Header = recvHeader;
					// HTTP Status Code
					sscanf(recvHeader, "HTTP/%*f %d %*s", &net->status);
					// chunked�]��
					if ((q = strstr(recvHeader, "Transfer-Encoding")))
					{
						q += strlen("Transfer-Encoding") + 1;
						r = strchr(q, '\n');
						if (r)
						{
							*r = '\0';
							if (strstr(q, "chunked"))
								chunked = 1;
							*r = '\n';
						}
					}
					// Content-Length
					if ((q = strstr(recvHeader, "Content-Length")))
					{
						q += strlen("Content-Length") + 1;
						r = strchr(q, '\n');
						if (r)
						{
							*r = '\0';
							bodySize = strtoul(q, NULL, 10);
							*r = '\n';
						}
					}
					// body�m��
					recvBody = (char*)memalign(32, (bodySize + 1) * sizeof(char));
					if (recvBody == NULL)
					{
						eCode = SOCK_MEM_ERR;
						goto ERROR;
					}
					progress--;
					// �o�b�t�@�̎c�肪�Ȃ�
					if (size == 0)
					{
						break;
					}
				case READ_BODY:
					if (recvSize + size > bodySize)
					{
						char* tmp;
						bodySize += bodySize;
						tmp = memalign(32, bodySize);
						if (tmp == NULL)
						{
							eCode = SOCK_MEM_ERR;
							goto ERROR;
						}
						memcpy(tmp, recvBody, recvSize);
						free(recvBody);
						recvBody = tmp;
					}
					memcpy(recvBody + recvSize, p, size);
					recvSize += size;
					break;
			}
		}
		if (net->aborting == SOCK_ABRT_ERR)
		{
			eCode = SOCK_ABRT_ERR;
			goto ERROR;
		}
		pgWaitVn(1);
	}
	recvBody[recvSize] = '\0';
	if (chunked)
		recvSize = psp2chNetDecodeChunk(recvBody);
	net->body = recvBody;
	net->length = recvSize;
	return 0;
	
ERROR:
	net->aborting = eCode;
	free(recvHeader);
	if (recvBody)
		free(recvBody);
	return eCode;
}

static int psp2chParseHeader(char* header, char*** hName, char*** hValue)
{
	char *q = header, *r, *s, **name, **value;
	int count, i;
	
	// �S�w�b�_���
	q = strstr(q, "\r\n") + strlen("\r\n");
	r = q;
	count = 0;
	while (*r)
	{
		if (*r++ == LINE_END)
		{
			count++;
		}
	}
	name = (char**)malloc(sizeof(char*) * count);
	value = (char**)malloc(sizeof(char*) * count);
	for(i = 0; i < count; i++)
	{
		name[i] = (char*)malloc(sizeof(char) * 32);
		value[i] = (char*)malloc(sizeof(char) * 1024);
	}
	i = 0;
	while ((r = strstr(q, ": ")) && (s = strstr(r, "\r\n")))
	{
		*s = STREAM_END;
		*r = STREAM_END;
		strlcpy(name[i], q, 32);
		r += strlen(": ");
		strlcpy(value[i], r, 1024);
		q = s + strlen("\r\n");
		i++;
	}
	*hName = name;
	*hValue = value;
	return count;
}

static int psp2chGetHttpHeader(int count, char** hName, char** hValue, S_NET* net)
{
	int i;
	
	memset(&(net->head), 0, sizeof(HTTP_HEADERS));
	if (net->cook) {
		net->cook[0] = '\0';
	}
	for (i = 0; i < count; i++)
	{
		if (strcmp(hName[i], "Content-Length") == 0)
		{
			net->head.Content_Length = strtol(hValue[i], NULL, 10);
		}
		else if (strcmp(hName[i], "Content-Type") == 0)
		{
			strcpy(net->head.Content_Type, hValue[i]);
		}
		else if (strcmp(hName[i], "Last-Modified") == 0)
		{
			strcpy(net->head.Last_Modified, hValue[i]);
		}
		else if (strcmp(hName[i], "ETag") == 0)
		{
			strcpy(net->head.ETag, hValue[i]);
		}
		else if (net->cook && strcmp(hName[i], "Set-Cookie") == 0)
		{
			char *p = strchr(hValue[i], ';');
			if (p)
				*p = '\0';
			if (strlen(net->cook) + strlen(hValue[i]) < NET_COOKIE_LENGTH)
			{
				if (strlen(net->cook)) {
					strcat(net->cook, "; ");
				}
				strcat(net->cook, hValue[i]);
			}
		}
		else if (strcmp(hName[i], "Transfer-Encoding") == 0)
		{
			net->head.Content_Length = -1;
		}
		else if (strcmp(hName[i], "Accept-Ranges") == 0)
		{
			if (strstr(hValue[i], "bytes"))
			{
				net->head.Range = 1;
			}
		}
		else if (strcmp(hName[i], "Content-Encoding") == 0)
		{
			net->head.Content_Length = -1;
			if (strstr(hValue[i], "gzip"))
			{
				net->head.isGzip = 1;
			}
			else if (strstr(hValue[i], "deflate"))
			{
				net->head.isGzip = 2;
			}
		}
		else if (strcmp(hName[i], "Location") == 0)
		{
			strcpy(net->head.location, hValue[i]);
		}
		else if (strcmp(hName[i], "WWW-Authenticate") == 0)
		{
			strcpy(net->head.Authenticate, hValue[i]);
		}
	}
	return 0;
}

static int gzipDecompress(char **out, const int isGzip, const char* src, const int srcSize)
{
	z_stream z;
	int m_inflate_total, method, flags;
	int i, len, status, count, dstSize;
	unsigned char outbuf[TMP_BUF_SIZE];
	char *dst, *tmp, *p;
	
	// memory manager
	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;
	// initialize
	z.next_in = Z_NULL;
	z.avail_in = 0;
	m_inflate_total = 0;
	if(inflateInit2(&z, -MAX_WBITS) != Z_OK)
	{
		return -1;
	}
	i = 0;
	if(isGzip == 2)
		psp2chErrorDialog(0, "start deflate test");
	if(isGzip == 1)
	{
		// header check
		if((u8)src[0] != 0x1f || (u8)src[1] != 0x8b)
		{
			return -1;
		}
		i = 2;
		method = src[i++];
		flags = src[i++];
		if(method != Z_DEFLATED || (flags & RESERVED) != 0)
		{
			return -1;
		}
		i += 6;
		if(flags & EXTRA_FIELD)
		{
			len = (uInt)src[i++];
			len += ((uInt)src[i++]) << 8;
			i += len;
		}
		if(flags & ORIG_NAME)
		{
			while(src[i++] != 0){}
		}
		if(flags & COMMENT)
		{
			while(src[i++] != 0){}
		}
		if(flags & HEAD_CRC)
		{
			i += 2;
		}
	}
	z.next_out = outbuf;
	z.avail_out = TMP_BUF_SIZE;
	z.avail_in = srcSize - i;
	z.next_in = (unsigned char*)src + i;
	status = Z_OK;
	dstSize = (srcSize < TMP_BUF_SIZE) ? TMP_BUF_SIZE : sizeof(char) * srcSize * 2;
	dst = (char*)malloc(dstSize);
	if(dst == NULL)
	{
		psp2chNormalError(MEM_ALLC_ERR, "");
		return -1;
	}
	p = dst;
	// decompress start
	while(status != Z_STREAM_END)
	{
		status = inflate(&z, Z_NO_FLUSH);
		if(status == Z_STREAM_END)
		{
			break;
		}
		else if(status != Z_OK)
		{
			psp2chErrorDialog(0, "%d: %s", status, z.msg);
			inflateEnd(&z);
			free(dst);
			return -1;
		}
		if(z.avail_out == 0)
		{
			memcpy(p, outbuf, TMP_BUF_SIZE);
			p += TMP_BUF_SIZE;
			z.next_out = outbuf;
			z.avail_out = TMP_BUF_SIZE;
			m_inflate_total += TMP_BUF_SIZE;
			if(TMP_BUF_SIZE + m_inflate_total > dstSize)
			{
				dstSize += dstSize;
				tmp = realloc(dst, dstSize);
				if(tmp == NULL)
				{
					psp2chNormalError(MEM_ALLC_ERR, "");
					free(dst);
					return -1;
				}
				dst = tmp;
				p = dst + m_inflate_total;
			}
		}
	}
	// �c��
	if((count = TMP_BUF_SIZE - z.avail_out) != 0)
	{
		memcpy(p, outbuf, count);
		m_inflate_total += count;
	}
	if(inflateEnd(&z) != Z_OK)
	{
		psp2chErrorDialog(0, "%s", z.msg);
		free(dst);
		return -1;
	}
	dst[m_inflate_total] = STREAM_END;
	*out = dst;
	return m_inflate_total;
}

static int psp2chNetDecodeChunk(char *src)
{
	unsigned long chunkSize, bodySize = 0;
	char *dst = src;
	
	while (*src)
	{
		chunkSize = strtoul(src, &src, 16);
		if (chunkSize == 0)
		{
			break;
		}
		src += strlen("\r\n");
		memmove(dst, src, chunkSize);
		bodySize += chunkSize;
		dst += chunkSize;
		src += chunkSize;
	}
	return bodySize;
}

static int psp2chNetUrlParser(int *method, char **host, unsigned short *port, char **path, char *url)
{
	const char *types[] = {"http://", "https://"};
	char *p;
	
	if (memcmp(url, types[0], strlen(types[0])) == 0) {
		*host = url + strlen(types[0]);
		*method = 0;
	}
	else if (memcmp(url, types[1], strlen(types[0])) == 0) {
		*host = url + strlen(types[1]);
		*method = 1;
	}
	else
		return -1;
	
	p = strchr(*host, '/');
	if (p) {
		*p++ = '\0';
		*path = p;
	} else {
		*path = host;
	}
	
	if ((p = strchr(*host, ':'))) {
		*p++ = '\0';
		*port=strtoul(p, NULL, 10);
	}
	else if (*method == 1)
		*port = 443;
	else
		*port = 80;
	
	return 0;
}

/*
int psp2chNetMaruLogin(char *dst, char *url, const char *body)
{
	int ret;
	int s;
	int method;
	unsigned short port;
	struct in_addr addr;
	struct sockaddr_in sain0;

	SSL *ssl;
	SSL_CTX *ctx;

	char request[BUF_LENGTH * 4];
	char *host;
	char *path;

	psp2chNetUrlParser(&method, &host, &port, &path, url);

	// DNS
	ret = psp2chResolve(host, &addr);
	if (ret < 0) 
	    return SOCK_CNCT_ERR;
	sain0.sin_family = AF_INET;
	sain0.sin_port = htons(port);
	sain0.sin_addr.s_addr = addr.s_addr;
	memset (sain0.sin_zero, (int)0, sizeof(sain0.sin_zero));

	s = socket(AF_INET, SOCK_STREAM, 0); 
	if (s < 0)
		return -1;

	if (connect(s, (struct sockaddr*)&sain0, sizeof(sain0)) == -1){
		shutdown(s, SHUT_RDWR);
		close(s);
		return -2;
	}

	// �������炪 SSL
	SSL_library_init();
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL){
		shutdown(s, SHUT_RDWR);
		close(s);
		return -3;
	}

	ssl = SSL_new(ctx);
	if (ssl == NULL){
		SSL_CTX_free(ctx);
		shutdown(s, SHUT_RDWR);
		close(s);
		return -4;
	}

	ret = SSL_set_fd(ssl, s);
	if (ret == 0){
		SSL_free(ssl); 
		SSL_CTX_free(ctx);
		shutdown(s, SHUT_RDWR);
		close(s);
		return -5;
	}

	do {
		unsigned short rand_ret = rand() % 65536;
		RAND_seed(&rand_ret, sizeof(rand_ret));
	} while (RAND_status() == 0);

	// SSL �Őڑ�
	ret = SSL_connect(ssl);
	if (ret != 1){
		SSL_free(ssl); 
		SSL_CTX_free(ctx);
		shutdown(s, SHUT_RDWR);
		close(s);
		return -6;
	}

	// ���N�G�X�g���M
	sprintf(request, 
		"POST /%s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Referer: https://2chv.tora3.net/\r\n"
		"Accept-Language: ja\r\n"
		"User-Agent: DOLIB/1.00\r\n"
		"X-2ch-UA: %s\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"\r\n"
		"%s",
		path, host, owataVer, strlen(body), body);
	psp2chErrorDialog(0, "%s", request);

	ret = SSL_write(ssl, request, strlen(request));
	if (ret < 1){
		SSL_free(ssl); 
		SSL_CTX_free(ctx);
		shutdown(s, SHUT_RDWR);
		close(s);
		return -7;
	}

	// ���X�|���X��M
	while (1){
		char buf[BUF_LENGTH];
		int read_size;
		read_size = SSL_read(ssl, buf, sizeof(buf)-1);

		if (read_size > 0){
			buf[read_size] = '\0';
			memcpy(dst, buf, read_size);
			dst += read_size;
		} else if (read_size == 0){
			break;
		} else {
			SSL_free(ssl); 
			SSL_CTX_free(ctx);
			shutdown(s, SHUT_RDWR);
		close(s);
			return -8;
		}
	}

	ret = SSL_shutdown(ssl); 
	if (ret != 1){
		SSL_free(ssl); 
		SSL_CTX_free(ctx);
		shutdown(s, SHUT_RDWR);
		close(s);
		return -9;
	}
	shutdown(s, SHUT_RDWR);
	close(s);

	SSL_free(ssl); 
	SSL_CTX_free(ctx);

	return 0;
}
*/

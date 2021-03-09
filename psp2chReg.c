#include "psp2ch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oniguruma.h>
#include "psp2chReg.h"
#include "charConv.h"

typedef struct {
	char *src;
	OnigRegion *region;
	char *host;
	char *bbs;
	char *dir;
	char *dat;
} bbs_reg_data;

extern S_2CH s2ch; // psp2chRes.c

static int bbs_name_callback(const UChar* name, const UChar* name_end, int ngroup_num, int* group_nums, regex_t* reg, void* arg);
static int psp2chBBSGetStr(char *dst, char *src, char *host, char *bbs, char *dir, char *dat, int endRes, char *name, char *mail, char *message, char *subject, time_t time);

void psp2chRegGetStr(char *dst, const char *orig, OnigRegion *region, int num)
{
	memcpy(dst, orig + region->beg[num], region->end[num] - region->beg[num]);
	dst[region->end[num] - region->beg[num]] = '\0';
	return;
}

void psp2chRegReplace(char *dst, const char *base, const char *orig, OnigRegion *region)
{
	int i, num;
	
	while (*base) {
		switch (*base) {
		case '$':
			num = *(++base) - '0';
			for (i = region->beg[num]; i < region->end[num]; i++)
				*dst++ = *(orig + i);
			base++;
			break;
		case '\\':
			base++;
		default:
			*dst++ = *base++;
		}
	}
	
	*dst = '\0';
	return;
}

int psp2chRegPatGet(const char *host, int *code, char *pattern1, char *pattern2)
{
	SceUID fd;
	char path[FILE_PATH], str[NET_HOST_LENGTH];
	char *buf, *s, *e;
	int size, r;
	unsigned char *start, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	
	sprintf(path, "%s/dat/%s", s2ch.cwDir, "pattern.dat");
	fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
	if(fd < 0)
		return -1;
	
	size = sceIoLseek32(fd, 0, SEEK_END);
	sceIoLseek32(fd, 0, SEEK_SET);
	buf = malloc(sizeof(char) * (size + 1));
	psp2chFileRead(fd, buf, size);
	buf[size] = '\0';
	sceIoClose(fd);
	
	region = onig_region_new();
	s = buf;
	while ((e = strchr(s, '\n'))) {
		*e = '\0';
		sscanf(s, "%s %d %s %s", str, code, pattern1, pattern2);
		
		r = onig_new(&reg, (unsigned char*)str, (unsigned char*)str + strlen((char*)str),
			ONIG_OPTION_DEFAULT, ONIG_ENCODING_SJIS, ONIG_SYNTAX_DEFAULT, &einfo);
		if (r != ONIG_NORMAL) {
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str(s, r, &einfo);
			psp2chErrorDialog(0, "ERROR: %s\n", s);
			return -1;
		}
		
		end   = (unsigned char*)host + strlen((char*)host);
		start = (unsigned char*)host;
		r = onig_match(reg, (unsigned char*)host, end, start, region, ONIG_OPTION_NONE);
		
		if (r >= 0) {
			break;
		}
		onig_region_clear(region);
		s = e + 1;
	}
	free(buf);
	onig_region_free(region, 1);
	onig_free(reg);
	onig_end();
	
	return 0;
}

int psp2chUrlPatGet(char *dst, char *ref, int *mode, const char *src)
{
	SceUID fd;
	char path[FILE_PATH], url[3][FILE_PATH];
	char *buf, *s, *e;
	int size, r;
	unsigned char *start, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	
	sprintf(path, "%s/dat/%s", s2ch.cwDir, "image.dat");
	fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
	if(fd < 0)
		return -1;
	
	size = sceIoLseek32(fd, 0, SEEK_END);
	sceIoLseek32(fd, 0, SEEK_SET);
	buf = malloc(sizeof(char) * (size + 1));
	psp2chFileRead(fd, buf, size);
	buf[size] = '\0';
	sceIoClose(fd);
	
	region = onig_region_new();
	s = buf;
	while ((e = strchr(s, '\n'))) {
		*e = '\0';
		sscanf(s, "%s %s %s %d", url[0], url[1], url[2], mode);
		
		r = onig_new(&reg, (unsigned char*)url[0], (unsigned char*)url[0] + strlen((char*)url[0]),
			ONIG_OPTION_DEFAULT, ONIG_ENCODING_SJIS, ONIG_SYNTAX_DEFAULT, &einfo);
		if (r != ONIG_NORMAL) {
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str(s, r, &einfo);
			psp2chErrorDialog(0, "ERROR: %s\n", s);
			return -1;
		}
		
		end   = (unsigned char*)src + strlen((char*)src);
		start = (unsigned char*)src;
		r = onig_match(reg, (unsigned char*)src, end, start, region, ONIG_OPTION_NONE);
		
		if (r >= 0)
			break;
		s = e + 1;
		onig_region_clear(region);
	}
	psp2chRegReplace(dst, url[1], src, region);
	psp2chRegReplace(ref, url[2], src, region);
	free(buf);
	onig_region_free(region, 1);
	onig_free(reg);
	onig_end();
	
	return 0;
}

static int bbs_name_callback(const UChar* name, const UChar* name_end, int ngroup_num, int* group_nums, regex_t* reg, void* arg)
{
	int gn;
	bbs_reg_data* data = (bbs_reg_data*)arg;
	OnigRegion *region = data->region;

	gn = group_nums[0];
	if (strcmp((char*)name, "host") == 0) {
		psp2chRegGetStr(data->host, data->src, region, gn);
	}
	else if (strcmp((char*)name, "bbs") == 0) {
		psp2chRegGetStr(data->bbs, data->src, region, gn);
	}
	else if (strcmp((char*)name, "dir") == 0) {
		psp2chRegGetStr(data->dir, data->src, region, gn);
	}
	else if (strcmp((char*)name, "dat") == 0) {
		psp2chRegGetStr(data->dat, data->src, region, gn);
	}
	return 0;
}

int psp2chFormGetPattern(char *target_url, char *filename, char *target_ref, char *encode, char *src_url, char *name, char *mail, char *message, char *title)
{
	SceUID fd;
	char path[FILE_PATH], data[4][FILE_PATH] = {"", "", "", ""};
	char pattern[FILE_PATH], dst_url[FILE_PATH], ref[FILE_PATH], rep[FILE_PATH];
	char *buffer, *p, *q;
	int r, size;
	unsigned char *start, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	time_t time;
	
	// パターン情報を取得
	sprintf(path, "%s/dat/%s", s2ch.cwDir, filename);
	fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    	return -1;
    size = sceIoLseek32(fd, 0, SEEK_END);
    sceIoLseek32(fd, 0, SEEK_SET);
	buffer = malloc(sizeof(char) * (size + 1));
	if (buffer == NULL) {
		sceIoClose(fd);
		return -1;
	}
	psp2chFileRead(fd, buffer, size);
	buffer[size] = '\0';
	sceIoClose(fd);
	
	region = onig_region_new();
	p = buffer;
	while ((q = strchr(p, '\n'))) {
		*q = '\0';
		sscanf(p, "%s %s %s %s", pattern, dst_url, ref, rep);
		
		r = onig_new(&reg, (unsigned char*)pattern, (unsigned char*)pattern + strlen((char*)pattern),
			ONIG_OPTION_DEFAULT, ONIG_ENCODING_SJIS, ONIG_SYNTAX_DEFAULT, &einfo);
		if (r != ONIG_NORMAL) {
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str(s, r, &einfo);
			psp2chErrorDialog(0, "ERROR: %s\n", s);
			return -1;
		}
		
		end   = (unsigned char*)src_url + strlen((char*)src_url);
		start = (unsigned char*)src_url;
		r = onig_match(reg, (unsigned char*)src_url, end, start, region, ONIG_OPTION_NONE);
		
		if (r >= 0)
			break;
		onig_region_clear(region);
		p = q + 1;
	}
	bbs_reg_data fdata = {src_url, region, data[0], data[1], data[2], data[3]};
	r = onig_foreach_name(reg, bbs_name_callback, (void*)&fdata);
	onig_region_free(region, 1);
	onig_free(reg);
	onig_end();
	free(buffer);
	
	sceKernelLibcTime (&time);
	time -= 900; // 900秒前
    r = psp2chBBSGetStr(target_url, dst_url, data[0], data[1], data[2], data[3], 0, name, mail, message, title, time);
    if (r < 0)
    	return -1;
    r = psp2chBBSGetStr(target_ref, ref, data[0], data[1], data[2], data[3], 0, name, mail, message, title, time);
    if (r < 0)
    	return -1;
    r = psp2chBBSGetStr(encode, rep, data[0], data[1], data[2], data[3], 0, name, mail, message, title, time);
    if (r < 0)
    	return -1;
    return 0;
}

static int psp2chBBSGetStr(char *dst, char *src, char *host, char *bbs, char *dir, char *dat, int endRes, char *name, char *mail, char *message, char *subject, time_t time)
{
	char *buffer;
	
	buffer = (char*)malloc(RES_MESSAGE_LENGTH * 4);
    if (buffer == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return -1;
    }
	while (*src) {
		switch (*src) {
		case '$':
			src++;
			if (memcmp(src, "<host>", strlen("<host>")) == 0) {
				memcpy(dst, host, strlen(host));
				src += strlen("<host>");
				dst += strlen(host);
			}
			else if (memcmp(src, "<bbs>", strlen("<bbs>")) == 0) {
				memcpy(dst, bbs, strlen(bbs));
				src += strlen("<bbs>");
				dst += strlen(bbs);
			}
			else if (memcmp(src, "<dir>", strlen("<dir>")) == 0) {
				memcpy(dst, dir, strlen(dir));
				src += strlen("<dir>");
				dst += strlen(dir);
			}
			else if (memcmp(src, "<dat>", strlen("<dat>")) == 0) {
				memcpy(dst, dat, strlen(dat));
				src += strlen("<dat>");
				dst += strlen(dat);
			}
			else if (memcmp(src, "<endRes>", strlen("<endRes>")) == 0) {
				sprintf(buffer, "%d", endRes);
				memcpy(dst, buffer, strlen(buffer));
				src += strlen("<endRes>");
				dst += strlen(buffer);
			}
			else if (memcmp(src, "<name>", strlen("<name>")) == 0) {
				psp2chUrlEncode(buffer, name);
				memcpy(dst, buffer, strlen(buffer));
				src += strlen("<name>");
				dst += strlen(buffer);
			}
			else if (memcmp(src, "<mail>", strlen("<mail>")) == 0) {
				psp2chUrlEncode(buffer, mail);
				memcpy(dst, buffer, strlen(buffer));
				src += strlen("<mail>");
				dst += strlen(buffer);
			}
			else if (memcmp(src, "<text>", strlen("<text>")) == 0) {
				psp2chUrlEncode(buffer, message);
				memcpy(dst, buffer, strlen(buffer));
				src += strlen("<text>");
				dst += strlen(buffer);
			}
			else if (memcmp(src, "<time>", strlen("<time>")) == 0) {
				sprintf(buffer, "%lu", time);
				memcpy(dst, buffer, strlen(buffer));
				src += strlen("<time>");
				dst += strlen(buffer);
			}
			else if (memcmp(src, "<subject>", strlen("<subject>")) == 0) {
				psp2chUrlEncode(buffer, subject);
				memcpy(dst, buffer, strlen(buffer));
				src += strlen("<subject>");
				dst += strlen(buffer);
			}
			else
				while (*src++ != '>') {}
			break;
		case '\\':
			src++;
		default:
			*dst++ = *src++; break;
		}
	}
	*dst = '\0';
	free(buffer);
	return 0;
}

int psp2chBBSGetPattern(char *dst_url, char *referer, char *src_url, int endRes, char *filename)
{
	SceUID fd;
	char path[FILE_PATH], data[4][FILE_PATH] = {"", "", "", ""};
	char pattern[FILE_PATH], url[FILE_PATH], ref[FILE_PATH];
	char *buffer, *p, *q;
	int r, size;
	unsigned char *start, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	
	// パターン情報を取得
	sprintf(path, "%s/dat/%s", s2ch.cwDir, filename);
	fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    	return -1;
    size = sceIoLseek32(fd, 0, SEEK_END);
    sceIoLseek32(fd, 0, SEEK_SET);
	buffer = malloc(sizeof(char) * (size + 1));
	if (buffer == NULL) {
		sceIoClose(fd);
		return -1;
	}
	psp2chFileRead(fd, buffer, size);
	buffer[size] = '\0';
	sceIoClose(fd);
	
	region = onig_region_new();
	p = buffer;
	while ((q = strchr(p, '\n'))) {
		*q = '\0';
		sscanf(p, "%s %s %s", pattern, url, ref);
		
		r = onig_new(&reg, (unsigned char*)pattern, (unsigned char*)pattern + strlen((char*)pattern),
			ONIG_OPTION_DEFAULT, ONIG_ENCODING_SJIS, ONIG_SYNTAX_DEFAULT, &einfo);
		if (r != ONIG_NORMAL) {
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str(s, r, &einfo);
			psp2chErrorDialog(0, "ERROR: %s\n", s);
			return -1;
		}
		
		end   = (unsigned char*)src_url + strlen((char*)src_url);
		start = (unsigned char*)src_url;
		r = onig_match(reg, (unsigned char*)src_url, end, start, region, ONIG_OPTION_NONE);
		
		if (r >= 0)
			break;
		onig_region_clear(region);
		p = q + 1;
	}
	bbs_reg_data fdata = {src_url, region, data[0], data[1], data[2], data[3]};
	r = onig_foreach_name(reg, bbs_name_callback, (void*)&fdata);
	onig_region_free(region, 1);
	onig_free(reg);
	onig_end();
	free(buffer);
	
    r = psp2chBBSGetStr(dst_url, url, data[0], data[1], data[2], data[3], endRes, NULL, NULL, NULL, NULL, 0);
    if (r < 0)
    	return -1;
    r = psp2chBBSGetStr(referer, ref, data[0], data[1], data[2], data[3], endRes, NULL, NULL, NULL, NULL, 0);
    if (r < 0)
    	return -1;
    return 0;
}

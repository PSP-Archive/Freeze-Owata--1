/*
   miniunz.c
   Version 1.01b, May 30th, 2004

   Copyright (C) 1998-2004 Gilles Vollant

   2010 STEAR フリーズオワタ+1用にかなり修正を入れました
*/
#define unix

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <pspiofilemgr.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include "psp2ch.h"
#include "charConv.h"

extern S_2CH s2ch; // psp2ch.c

#define printf	pspDebugScreenPrintf 

#ifdef unix
# include <unistd.h>
# include <utime.h>
#else
# include <direct.h>
# include <io.h>
#endif

#include "unzip.h"

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (64)								// 展開時に使用する最大メモリ（*1024）
#define MAXFILENAME (256)

#ifdef WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif
/*
  mini unzip, demo of unzip package

  usage :
  Usage : miniunz [-exvlo] file.zip [file_to_extract] [-d extractdir]

  list the file in the zipfile, and print the content of FILE_ID.ZIP or README.TXT
    if it exists
*/


/* change_file_date : change the date/time of a file
    filename : the filename of the file where date/time must be modified
    dosdate : the new date at the MSDos format (4 bytes)
    tmu_date : the SAME new date at the tm_unz format */
void change_file_date(filename,dosdate,tmu_date)
    const char *filename;
    uLong dosdate;
    tm_unz tmu_date;
{
#ifdef WIN32
  HANDLE hFile;
  FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;

  hFile = CreateFile(filename,GENERIC_READ | GENERIC_WRITE,
                      0,NULL,OPEN_EXISTING,0,NULL);
  GetFileTime(hFile,&ftCreate,&ftLastAcc,&ftLastWrite);
  DosDateTimeToFileTime((WORD)(dosdate>>16),(WORD)dosdate,&ftLocal);
  LocalFileTimeToFileTime(&ftLocal,&ftm);
  SetFileTime(hFile,&ftm,&ftLastAcc,&ftm);
  CloseHandle(hFile);
#else
#ifdef unix
  /*
  struct utimbuf ut;
  struct tm newdate;
  newdate.tm_sec = tmu_date.tm_sec;
  newdate.tm_min=tmu_date.tm_min;
  newdate.tm_hour=tmu_date.tm_hour;
  newdate.tm_mday=tmu_date.tm_mday;
  newdate.tm_mon=tmu_date.tm_mon;
  if (tmu_date.tm_year > 1900)
      newdate.tm_year=tmu_date.tm_year - 1900;
  else
      newdate.tm_year=tmu_date.tm_year ;
  newdate.tm_isdst=-1;

  ut.actime=ut.modtime=mktime(&newdate);
  utime(filename,&ut);
  */
#endif
#endif
}


/* mymkdir and change_file_date are not 100 % portable
   As I don't know well Unix, I wait feedback for the unix portion */

int usePassword = 0;

void SetUsepassword(int p){usePassword = p;}
int GetUsePassword(){return usePassword;}

int mymkdir(dirname)
    const char* dirname;
{
    int ret=0;
#ifdef WIN32
    ret = mkdir(dirname);
#else
#ifdef unix
	ret = sceIoMkdir(dirname,0777);
    //ret = mkdir (dirname,0775);
#endif
#endif
    return ret;
}

int makedir (char *newdir)
{
	char *buffer;
	char *p;
	int  len = (int)strlen(newdir);

	if (len <= 0)
		return 0;

	buffer = (char*)malloc(len+1);
	strcpy(buffer,newdir);

	if (buffer[len-1] == '/') {
		buffer[len-1] = '\0';
	}
	if (mymkdir(buffer) == 0)
	{
		free(buffer);
		return 1;
	}

	p = buffer+1;
	while (1)
	{
		char hold;

		while(*p && *p != '\\' && *p != '/')
			p++;
		hold = *p;
		*p = 0;
		if ((mymkdir(buffer) == -1) && (errno == ENOENT))
		{
			printf("couldn't create directory %s\n",buffer);
			free(buffer);
			return 0;
		}
		if (hold == 0)
			break;
		*p++ = hold;
	}
	free(buffer);
	return 1;
}


void do_banner()
{
	printf("AnonymousTipster's .zip plugin, based on:\n");
    printf("MiniUnz 1.01b, demo of zLib + Unz package written by Gilles Vollant\n");
    printf("more info at http://www.winimage.com/zLibDll/unzip.html\n\n");
}

int do_list(uf)
    unzFile uf;
{
    uLong i;
    unz_global_info gi;
    int err;

    err = unzGetGlobalInfo (uf,&gi);
    if (err!=UNZ_OK)
        printf("error %d with zipfile in unzGetGlobalInfo \n",err);
    printf(" Length  Method   Size  Ratio   Date    Time   CRC-32     Name\n");
    printf(" ------  ------   ----  -----   ----    ----   ------     ----\n");
    for (i=0;i<gi.number_entry;i++)
    {
        char filename_inzip[256];
        unz_file_info file_info;
        uLong ratio=0;
        const char *string_method;
        char charCrypt=' ';
        err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
        if (err!=UNZ_OK)
        {
            printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
            break;
        }
        if (file_info.uncompressed_size>0)
            ratio = (file_info.compressed_size*100)/file_info.uncompressed_size;

        /* display a '*' if the file is crypted */
        if ((file_info.flag & 1) != 0)
            charCrypt='*';

        if (file_info.compression_method==0)
            string_method="Stored";
        else
        if (file_info.compression_method==Z_DEFLATED)
        {
            uInt iLevel=(uInt)((file_info.flag & 0x6)/2);
            if (iLevel==0)
              string_method="Defl:N";
            else if (iLevel==1)
              string_method="Defl:X";
            else if ((iLevel==2) || (iLevel==3))
              string_method="Defl:F"; /* 2:fast , 3 : extra fast*/
        }
        else
            string_method="Unkn. ";

        printf("%7lu  %6s%c%7lu %3lu%%  %2.2lu-%2.2lu-%2.2lu  %2.2lu:%2.2lu  %8.8lx   %s\n",
                file_info.uncompressed_size,string_method,
                charCrypt,
                file_info.compressed_size,
                ratio,
                (uLong)file_info.tmu_date.tm_mon + 1,
                (uLong)file_info.tmu_date.tm_mday,
                (uLong)file_info.tmu_date.tm_year % 100,
                (uLong)file_info.tmu_date.tm_hour,(uLong)file_info.tmu_date.tm_min,
                (uLong)file_info.crc,filename_inzip);
        if ((i+1)<gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err!=UNZ_OK)
            {
                printf("error %d with zipfile in unzGoToNextFile\n",err);
                break;
            }
        }
    }

    return 0;
}


//==============================================================
// ファイル展開処理
//--------------------------------------------------------------
// info : エラーが発生した時のログ
//--------------------------------------------------------------

int do_extract_currentfile(uf, popt_extract_without_path, popt_overwrite, password, entry, counter, info)
	unzFile		uf;
	const int*	popt_extract_without_path;
	int*		popt_overwrite;
	const char*	password;
	int			entry;
	int			counter;
	char		*info;
{
	char			filename_inzip[256], filename_inzip_sjis[256], msg[256];
	char*			filename_withoutpath;
	char*			p;
	int				i, err = UNZ_OK;
	FILE			*fot = NULL;
	SceUID			fout;
	u8*				buf;
	uInt			size_buf;
	unsigned short	ucs[256];
	unz_file_info	file_info;
	uLong			ratio = 0;

    err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
    if (err!=UNZ_OK)
    {
		sprintf(info, "error %d with zipfile in unzGetCurrentFileInfo\n（zipファイルが壊れてる…？）", err);
		return err;
    }

	strcpy(filename_inzip_sjis, filename_inzip);				// 画面表示用文字列
	if (s2ch.cfg.hbl){											// HBL環境下ではファイル名のシフトJIS→UTF8変換を行う
		psp2chSJIS2UCS(ucs, filename_inzip, 256);
		psp2chUCS2UTF8(filename_inzip, ucs);
	}

	for (i = WRITEBUFFERSIZE; i > 4; i-=2){						// WRITEBUFFERSIZE～4KBの範囲で出来るだけ大きくメモリ確保
		size_buf = i * 1024;
		buf = (void*)memalign(64,size_buf);
		if (buf) break;											// メモリが確保できたらループ終了
	}
	if (buf==NULL){												// 結局メモリ確保が出来なかった
		sprintf(info, "Error allocating memory\n（メモリが確保できなかった…）\n");
		return UNZ_INTERNALERROR;
	}

	p = filename_withoutpath = filename_inzip;
	while ((*p) != '\0'){										// パスの最後の'/'か'\'を探す
		if (((*p)=='/') || ((*p)=='\\'))
			filename_withoutpath = p+1;
		p++;
	}

    if ((*filename_withoutpath)=='\0')							// パスが'/'か'\'で終わっているなら
    {
        if ((*popt_extract_without_path)==0)
        {
			mymkdir(filename_inzip);
        }
    }
    else
    {
        const char* write_filename;
        int skip=0;

        if ((*popt_extract_without_path)==0)
            write_filename = filename_inzip;					// パス付きファイル名
        else
            write_filename = filename_withoutpath;				// ファイル名のみ

		if (usePassword == 1){
			err = unzOpenCurrentFilePassword(uf,password);
		} else {
			err = unzOpenCurrentFilePassword(uf,NULL);
		}
		if (err!=UNZ_OK){
			sprintf(info, "error %d with zipfile in unzOpenCurrentFilePassword\n（zipファイルが壊れてる…？）", err);
		}

/*		if (((*popt_overwrite)==0) && (err==UNZ_OK))			// 上書き確認
        {
            char rep=0;
            FILE* ftestexist;
            ftestexist = fopen(write_filename,"rb");
            if (ftestexist!=NULL)
            {
                fclose(ftestexist);
                do
                {
                    char answer[128];
                    int ret;

                    printf("The file %s will be overwritten ",write_filename);
                    ret = 'y';//scanf("%1s",answer);
                    if (ret != 1)
                    {
                       exit(EXIT_FAILURE);
                    }
                    rep = answer[0] ;
                    if ((rep>='a') && (rep<='z'))
                        rep -= 0x20;
                }
                while ((rep!='Y') && (rep!='N') && (rep!='A'));
            }

            if (rep == 'N')
                skip = 1;

            if (rep == 'A')
                *popt_overwrite=1;
        }
*/
		if ((skip==0) && (err==UNZ_OK))							// ディレクトリ構造を構築
        {
			fout = sceIoOpen(write_filename, O_RDWR, 0777);

            /* some zipfile don't contain directory alone before file */
            if ((fout<=0) && ((*popt_extract_without_path)==0) &&
                                (filename_withoutpath!=(char*)filename_inzip))
            {
                char c = *(filename_withoutpath-1);
                *(filename_withoutpath-1) = '\0';
                makedir(write_filename);
                *(filename_withoutpath-1) = c;
				fout = sceIoOpen(write_filename, O_RDWR, 0777);
            }
//			if (fout<=0)
//			{
//				printf("error opening %s\n",write_filename);
//			}
		}
		sceIoClose(fout);

		sprintf(msg, "%s (%d/%d %s)", info, counter, entry, filename_inzip_sjis);
		pgPrintMenuBar(msg);
		pgDrawTexture(-1);
		pgCopyMenuBar();
		flipScreen(0);											// 展開状況表示

		fout = sceIoOpen(write_filename,O_RDWR | O_CREAT,0777);
		if (fout > 0){
			do{													// カレントファイルを展開しつつ保存
				err = unzReadCurrentFile(uf,buf,size_buf);
				if (err<0)
				{
					sprintf(info, "error %d with zipfile in unzReadCurrentFile\n（パスワードが間違ってる…？）",err);
					break;
				}
				if (err>0){
					if (sceIoWrite(fout,buf,err)!=err)
					{
						sprintf(info, "error in writing extracted file\n（ディスク容量が足りない…？）\n");
						err = UNZ_ERRNO;
						break;
					}
				}
			}while (err>0);
			sceIoClose(fout);

			if (err==0){
				change_file_date(write_filename, file_info.dosDate, file_info.tmu_date);
			}
		}

		if (err==UNZ_OK){
			err = unzCloseCurrentFile(uf);
			if (err!=UNZ_OK)									// CRC Errorが発生する可能性あり
			{
				sprintf(info, "%s is CRC Error\n（zipファイルが壊れてる…？）\n", filename_inzip_sjis);
			}
		} else {
			unzCloseCurrentFile(uf);		/* don't lose the error */
		}
    }

	free(buf);
	return err;
}

//==============================================================
// アーカイブに含まれる全てのファイルを展開する
//--------------------------------------------------------------

int do_extract(uf, opt_extract_without_path, opt_overwrite, password, info)
	unzFile		uf;
	int			opt_extract_without_path;
	int			opt_overwrite;
	const char*	password;
	char		*info;
{
	uLong i;
	unz_global_info gi;
	int err, counter;

	err = unzGetGlobalInfo(uf, &gi);
	if (err!=UNZ_OK)
		sprintf(info, "error %d with zipfile in unzGetGlobalInfo \n", err);

	counter = 0;
	for (i=0; i<gi.number_entry; i++){
		counter++;
		err = do_extract_currentfile(uf,&opt_extract_without_path, &opt_overwrite, password, gi.number_entry, counter, info);
		if (err != UNZ_OK) break;

        if ((i+1)<gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err!=UNZ_OK)
            {
                sprintf(info, "error %d with zipfile in unzGoToNextFile\n", err);
                break;
            }
        }
    }

	return err;
}

//==============================================================
// 指定された一つのファイルのみを展開する
//--------------------------------------------------------------

int do_extract_onefile(uf,filename,opt_extract_without_path,opt_overwrite,password)
    unzFile uf;
    const char* filename;
    int opt_extract_without_path;
    int opt_overwrite;
    const char* password;
{
    int err = UNZ_OK;
    if (unzLocateFile(uf,filename,CASESENSITIVITY)!=UNZ_OK)
    {
        printf("file %s not found in the zipfile\n",filename);
        return 2;
    }

    if (do_extract_currentfile(uf,&opt_extract_without_path,
                                      &opt_overwrite,
                                      password) == UNZ_OK)
        return 0;
    else
        return 1;
}


//==============================================================
// zipファイル展開
//--------------------------------------------------------------
// 戻り値：1     展開されていない（destpathフォルダは生成されていない）
//         0     正常終了
//         0以下 エラー発生（destpathフォルダに一部展開されている）
//--------------------------------------------------------------
// destpathフォルダを作成し、その中にzippathアーカイブを展開する。
//--------------------------------------------------------------

int unzipToDir(const char *zippath, const char *destpath, const char *pass, char *info)
{
    const char *zipfilename = NULL;
    const char *filename_to_extract = NULL;
    const char *password = NULL;
    char filename_try[MAXFILENAME+16] = "", folder[1024];
    int opt_do_list = 0;
    int opt_do_extract = 1;
    int opt_do_extract_withoutpath = 0;
    int opt_overwrite = 1;
	int ret;
	char			dirname[256];
	unsigned short	ucs[256];
    unzFile uf = NULL;

	getcwd( folder,1024 );										// カレントディレクトリを取得

	zipfilename = zippath;		//"ms0:/testzip.zip";
	////filename_to_extract = "ms0:/zext/";
	strcpy(dirname, destpath);	//"ms0:/zext/";
	password = pass;
	if (strlen(password)==0){									// パスワードが指定されているか
		SetUsepassword(0);
	} else {
		SetUsepassword(1);
	}

    if (zipfilename!=NULL)
    {
        strncpy(filename_try, zipfilename,MAXFILENAME-1);
        /* strncpy doesnt append the trailing NULL, of the string is too long. */
        filename_try[ MAXFILENAME ] = '\0';

        uf = unzOpen(zipfilename);
        if (uf==NULL)
        {
            strcat(filename_try,".zip");
            uf = unzOpen(filename_try);
        }
    }

    if (uf==NULL)
    {
		sprintf(info, "Cannot open %s or %s.zip\n",zipfilename,zipfilename);
		return 1;
    }
//    printf("%s opened\n",filename_try);

	if (s2ch.cfg.hbl){											// HBL環境下ではファイル名のシフトJIS→UTF8変換を行う
		psp2chSJIS2UCS(ucs, dirname, 256);
		psp2chUCS2UTF8(dirname, ucs);
	}
	if (!makedir(dirname)){
		sprintf(info, "make dir error %s\n", dirname);
		return 1;
	}

    if (opt_do_list==1)
        return do_list(uf);
    else if (opt_do_extract==1)
    {
        if (chdir(dirname)){
			sprintf(info, "Error changing into %s, aborting\n", dirname);
			return UNZ_INTERNALERROR;
        }

		if (filename_to_extract == NULL){
			ret = do_extract(uf, opt_do_extract_withoutpath, opt_overwrite, password, info);
		} else {
			ret = do_extract_onefile(uf,filename_to_extract,
			                          opt_do_extract_withoutpath,opt_overwrite,password);
		}
		chdir(folder);											// カレントディレクトリを元に戻す
		return ret;
    }

    return 0;
}

void testPrint(){
	printf("hello!");
}
#include <citrus/app.hpp>
#include <citrus/core.hpp>
#include <citrus/fs.hpp>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <3ds.h>

Result http_download(httpcContext *context)//This error handling needs updated with proper text printing once ctrulib itself supports that.
{
	Result ret=0;
	//u8* framebuf_top;
	u32 statuscode=0;
	//u32 size=0;
        u32 contentsize=0, downloadsize=0, buffsize=262144;
	u8 *buf;
        char *obuf;

        // ret = httpcAddRequestHeaderField(context, (char*)"Accept-Encoding", (char*)"gzip");
	// if(ret!=0)return ret;

	ret = httpcBeginRequest(context);
	if(ret!=0)return ret;

	ret = httpcGetResponseStatusCode(context, &statuscode, 0);
	if(ret!=0)return ret;

	if(statuscode!=200)return -2;

	ret=httpcGetDownloadSizeState(context, &downloadsize, &contentsize);
	if(ret!=0)return ret;

	printf( "dlsize: %" PRId32 "\n",downloadsize);
	printf( "size: %" PRId32 "\n",contentsize);
	gfxFlushBuffers();

	buf = (u8*)malloc(buffsize);
	if(buf==NULL)return -1;
	memset(buf, 0, buffsize);

        obuf = (char*)malloc(64);

	ret = httpcGetResponseHeader(context, (char*)"Content-Encoding", obuf, 64);
	if(ret==0){
                printf( "Content-Encoding: %s\n", obuf);
                gfxFlushBuffers();
                free(obuf);
        }

      ctr::app::webinstall(ctr::fs::SD, context, NULL);

//    while (true){
//        ret=httpcGetDownloadSizeState(context, &downloadsize, &contentsize);
//        if(ret!=0)return ret;
//
//        printf("dlsize: %"PRId32"\n",downloadsize);
//        gfxFlushBuffers();
//
//        if(contentsize == downloadsize) break;
//
//	ret = httpcDownloadData(context, buf, buffsize, NULL);
//	if(ret!=0)
//	{
//		free(buf);
//		return ret;
//	}
//    }


//	size = contentsize;
//	if(size>(240*400*3*2))size = 240*400*3*2;

//	framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
//	memcpy(framebuf_top, buf, size);

//	gfxFlushBuffers();
//	gfxSwapBuffers();

//	framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
//	memcpy(framebuf_top, buf, size);

//	gfxFlushBuffers();
//	gfxSwapBuffers();
//	gspWaitForVBlank();

	free(buf);

	return 0;
}

int main(int argc, char **argv)
{
        ctr::core::init(argc);
	Result ret=0;
	httpcContext context;

	gfxInitDefault();
	httpcInit();

	consoleInit(GFX_BOTTOM,NULL);

	//Change this to your own URL.
	//char *url = (char*)"http://3ds.intherack.com/3ds_image.bin";
	char *url = (char*)"http://3ds.intherack.com/test.cia";

	printf( "Downloading %s\n",url);
	gfxFlushBuffers();

	ret = httpcOpenContext(&context, url, 1);
	printf( "return from httpcOpenContext: %" PRId32 "\n",ret);
	gfxFlushBuffers();

	if(ret==0)
	{
		ret=http_download(&context);
		printf( "return from http_download: %" PRId32 "\n",ret);
		gfxFlushBuffers();
		httpcCloseContext(&context);
	}

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		// Your code goes here

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	// Exit services
	httpcExit();
	gfxExit();
        ctr::core::exit();
	return 0;
}


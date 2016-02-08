#include <citrus/app.hpp>
#include <citrus/core.hpp>
#include <citrus/fs.hpp>
#include <citrus/hid.hpp>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <3ds.h>

bool onProgress(u64 pos, u64 size){       
        printf("pos: %" PRId32 "-%" PRId32 "\n", (u32)pos, (u32)size);
        gfxFlushBuffers();
        ctr::hid::poll();
        return !ctr::hid::pressed(ctr::hid::BUTTON_B);
}

Result http_download(httpcContext *context)
{
	Result ret=0;
	u32 statuscode=0;
        u32 contentsize=0, downloadsize=0, buffsize=262144;
	u8 *buf;
        char *obuf;

        // This is the app that will be uninstalled.
        ctr::app::App app;
        app.titleId = 0x000400000b198000;
        app.mediaType = ctr::fs::SD;

        printf("potato: 0x%llx\n", app.titleId);

        ret = httpcAddRequestHeaderField(context, (char*)"Accept-Encoding", (char*)"gzip, deflate");
        if(ret!=0)return ret;

	ret = httpcBeginRequest(context);
	if(ret!=0)return ret;

	ret = httpcGetResponseStatusCode(context, &statuscode, 0);
	if(ret!=0)return ret;

	if(statuscode!=200)return -2;

	ret=httpcGetDownloadSizeState(context, &downloadsize, &contentsize);
	if(ret!=0)return ret;

	buf = (u8*)malloc(buffsize);
	if(buf==NULL)return -1;
	memset(buf, 0, buffsize);

        obuf = (char*)malloc(16);

	if(httpcGetResponseHeader(context, (char*)"Content-Encoding", obuf, 16)==0){
                printf("Content-Encoding: %s\n", obuf);
                gfxFlushBuffers();
        }
        free(obuf);

	//u32 size=0, count=0;
	//ret = (s32)HTTPC_RESULTCODE_DOWNLOADPENDING;
	//while (ret==(s32)HTTPC_RESULTCODE_DOWNLOADPENDING) {
	//	ret=httpcDownloadData(context, buf, buffsize, &size);
	//	printf("return: 0x%lx size: %" PRId32 "\n", ret, size);
	//	gfxFlushBuffers();
	//	count += size;
	//}
	//printf("count: %lu\n", count);
	//ret=httpcGetDownloadSizeState(context, &size, NULL);
	//printf("size: %lu\n", size);

        // We should check if this title exists first, but...
        ctr::app::uninstall(app);
        ctr::app::install(ctr::fs::SD, context, 0, &onProgress);


	free(buf);

	return ret;
}

int main(int argc, char **argv)
{
	Result ret=0;
	httpcContext context;

	gfxInitDefault();
	httpcInit();
	hidInit();

	consoleInit(GFX_BOTTOM,NULL);

	//Change this to your own URL.
	char *url = (char*)"http://home.intherack.com/build.cia";

	printf("Downloading %s\n",url);
	gfxFlushBuffers();

	ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
	printf("return from httpcOpenContext: %" PRId32 "\n",ret);
	gfxFlushBuffers();

	if(ret==0)
	{
        	ctr::core::init(argc);
		ret = http_download(&context);
		printf("return from http_download: %" PRId32 "\n",ret);
		gfxFlushBuffers();
		ret = httpcCloseContext(&context);
                printf("return from httpcCloseContext: %" PRId32 "\n",ret);
		gfxFlushBuffers();
        	ctr::core::exit();
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
	hidExit();
	return 0;
}


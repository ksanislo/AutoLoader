#include <citrus/app.hpp>
#include <citrus/core.hpp>
#include <citrus/fs.hpp>
#include <citrus/hid.hpp>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <3ds.h>

bool onProgress(u64 pos, u64 size) {
        printf("pos: %" PRId32 "-%" PRId32 "\n", (u32)pos, (u32)size);
        gfxFlushBuffers();
        ctr::hid::poll();
        return !ctr::hid::pressed(ctr::hid::BUTTON_B);
}

Result http_getinfo(char *url, ctr::app::App *app) {
	Result ret=0;
	u32 statuscode=0;
	httpcContext context;

        app->mediaType = ctr::fs::SD;
	app->size = 0;
	app->titleId = 0x0000000000000000;

	ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 0);
	printf("return from httpcOpenContext: %" PRId32 "\n",ret);
        gfxFlushBuffers();

        // We should /probably/ make sure Range: is supported
	// before we try to do this, but these 8 bytes are the titleId
	ret = httpcAddRequestHeaderField(&context, (char*)"Range", (char*)"bytes=11292-11299");
        if(ret!=0)return ret;

	ret = httpcBeginRequest(&context);
	if(ret!=0)return ret;

	ret = httpcGetResponseStatusCode(&context, &statuscode, 0);
	if(ret!=0)return ret;

	if(statuscode!=206)return -2; // 206 Partial Content

	u8 *buf = (u8*)malloc(8); // Allocate u8*8 == u64
	if(buf==NULL)return -1;
	memset(buf, 0, 8); // Zero out

	ret=httpcDownloadData(&context, buf, 8, NULL);
        // Safely convert our 8 byte string into a u64
	app->titleId = ((u64)buf[0] << 56 | (u64)buf[1] << 48 | (u64)buf[2] << 40 | (u64)buf[3] << 32 | (u64)buf[4] << 24 | (u64)buf[5] << 16 | (u64)buf[6] << 8 | (u64)buf[7]);
	free(buf);

        buf = (u8*)malloc(64);

        if(httpcGetResponseHeader(&context, (char*)"Content-Range", (char*)buf, 64)==0){
		char *ptr = strchr((const char *)buf, 47);
	        printf("Content-Range: %s\n", &ptr[1]);
                gfxFlushBuffers();
		app->size = atoll(&ptr[1]);
        }
        free(buf);


	ret = httpcCloseContext(&context);
	printf("return from httpcCloseContext: %" PRId32 "\n",ret);
	gfxFlushBuffers();

	return 0;
}

Result http_download(char *url, ctr::app::App *app) {
	Result ret=0;
	httpcContext context;
	u32 statuscode=0;
        u32 contentsize=0, downloadsize=0, buffsize=262144;
	u8 *buf;
        char *obuf;

	ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 0);
	printf("return from httpcOpenContext: %" PRId32 "\n",ret);
	gfxFlushBuffers();

        ret = httpcAddRequestHeaderField(&context, (char*)"Accept-Encoding", (char*)"gzip, deflate");
        if(ret!=0)return ret;

	ret = httpcBeginRequest(&context);
	if(ret!=0)return ret;

	ret = httpcGetResponseStatusCode(&context, &statuscode, 0);
	if(ret!=0)return ret;

	if(statuscode!=200)return -2;

	ret=httpcGetDownloadSizeState(&context, &downloadsize, &contentsize);
	if(ret!=0)return ret;

	buf = (u8*)malloc(buffsize);
	if(buf==NULL)return -1;
	memset(buf, 0, buffsize);

        obuf = (char*)malloc(16);

	if(httpcGetResponseHeader(&context, (char*)"Content-Encoding", obuf, 16)==0){
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
        ctr::app::install(app->mediaType, &context, app->size, &onProgress);


	free(buf);

	ret = httpcCloseContext(&context);
	printf("return from httpcCloseContext: %" PRId32 "\n",ret);
	gfxFlushBuffers();

	return ret;
}

int main(int argc, char **argv)
{
	Result ret=0;

        ctr::core::init(argc);
	gfxInitDefault();
	httpcInit();
	hidInit();

	consoleInit(GFX_BOTTOM,NULL);

	ctr::app::App app;

	//Change this to your own URL.
	char *url = (char*)"http://home.intherack.com/build.cia";

	printf("Downloading %s\n",url);
	gfxFlushBuffers();

	ret = http_getinfo(url, &app);
	printf("return from http_getinfo: %" PRId32 "\n",ret);
	gfxFlushBuffers();

	if(app.titleId != 0 && ctr::app::installed(app)) { // Check if we have a titleId to remove
		printf("Uninstalling titleId: 0x%llx\n", app.titleId);
		gfxFlushBuffers();
		ctr::app::uninstall(app);
	}

	ret = http_download(url, &app);
	printf("return from http_download: %" PRId32 "\n",ret);
	gfxFlushBuffers();

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
	ctr::core::exit();

	return 0;
}


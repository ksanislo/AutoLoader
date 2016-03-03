#include <citrus/app.hpp>
#include <citrus/core.hpp>
#include <citrus/fs.hpp>
#include <citrus/gpu.hpp>
#include <citrus/hid.hpp>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#define AUTO_UPDATE_FILE "web-updater.url"
// If we don't have a file, we'll use this one.
#define DEFAULT_URL "http://3ds.intherack.com/files/web-updater_autoinstall.cia"

// Max URL length used in qr variation
#define QUIRC_MAX_PAYLOAD	8896


using namespace ctr;

bool onProgress(u64 pos, u64 size) {
        printf("pos: %" PRId64 "-%" PRId64 "\n", pos, size);
        gpu::flushBuffer();
        hid::poll();
        return !hid::pressed(hid::BUTTON_B);
}

Result http_getinfo(char *url, app::App *app) {
	Result ret=0;
	u32 statuscode=0;
	httpcContext context;

        app->mediaType = fs::SD;
	app->size = 0;
	app->titleId = 0x0000000000000000;

	ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 0);
        if(ret!=0)return ret;

        // We should /probably/ make sure Range: is supported
	// before we try to do this, but these 8 bytes are the titleId
	ret = httpcAddRequestHeaderField(&context, (char*)"Range", (char*)"bytes=11292-11299");
        if(ret!=0)return ret;

        // This disables the SSL certificate checks.
        ret = httpcSetSSLOpt(&context, 1<<9);
        if(ret!=0)return ret;

	ret = httpcBeginRequest(&context);
	if(ret!=0)return ret;

	ret = httpcGetResponseStatusCode(&context, &statuscode, 0);
	if(ret!=0)return ret;

        if(statuscode==301||statuscode==302) {
                if(httpcGetResponseHeader(&context, (char*)"Location", (char*)url, QUIRC_MAX_PAYLOAD-1)==0){
                        return http_getinfo(url, app);
                }       
        } 

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
		app->size = atoll(&ptr[1]);
        }
        free(buf);


	ret = httpcCloseContext(&context);
        if(ret!=0)return ret;

	return 0;
}

Result http_download(char *url, app::App *app) {
	Result ret=0;
	httpcContext context;
	u32 statuscode=0;
        u32 contentsize=0, downloadsize=0;
	char *buf;

	ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 0);
        if(ret!=0)return ret;

        ret = httpcAddRequestHeaderField(&context, (char*)"Accept-Encoding", (char*)"gzip, deflate");
        if(ret!=0)return ret;

        // This disables the SSL certificate checks.
        ret = httpcSetSSLOpt(&context, 1<<9);
        if(ret!=0)return ret;

	ret = httpcBeginRequest(&context);
	if(ret!=0)return ret;

	ret = httpcGetResponseStatusCode(&context, &statuscode, 0);
	if(ret!=0)return ret;

	if(statuscode!=200)return -2;

	ret=httpcGetDownloadSizeState(&context, &downloadsize, &contentsize);
	if(ret!=0)return ret;

        buf = (char*)malloc(16);
	if(buf==NULL)return -1;
	memset(buf, 0, 16);

	if(httpcGetResponseHeader(&context, (char*)"Content-Encoding", (char*)buf, 16)==0){
                printf("Content-Encoding: %s\n", buf);
        }

        app::install(app->mediaType, &context, app->size, &onProgress);

	free(buf);

	ret = httpcCloseContext(&context);
	if(ret!=0)return ret;

	return ret;
}

int main(int argc, char **argv)
{
	Result ret=0;

        core::init(argc);
	httpcInit(0x1000);

	consoleInit(GFX_BOTTOM,NULL);

	app::App app;

	char *url = (char*)malloc(QUIRC_MAX_PAYLOAD);
	FILE *fd = fopen(AUTO_UPDATE_FILE, "r");
	if(fd != NULL) {
		fread(url, sizeof(char), QUIRC_MAX_PAYLOAD - 1, fd);
		fclose(fd);
	} else
		strcpy(url, DEFAULT_URL);

	printf("Downloading %s\n", url);
	gpu::flushBuffer();

	ret = http_getinfo(url, &app);
	if(ret!=0)return ret;

	if(app.titleId != 0 && app::installed(app)) { // Check if we have a titleId to remove
		printf("Uninstalling titleId: 0x%llx\n", app.titleId);
		gpu::flushBuffer();
		app::uninstall(app);
	}

	ret = http_download(url, &app);
	if(ret!=0)return ret;
	free(url);

	// We're done with http:C
	httpcExit();

	printf("Install finished.\nLaunching title 0x%llx.\n", app.titleId);
	gpu::flushBuffer();

	ctr::app::launch(app); // Launch what we installed.

	// Main loop
	while (core::running())
	{
		hid::poll();

		// Your code goes here

		if (hid::pressed(hid::BUTTON_START))
			break; // break in order to return to hbmenu

		// Flush and swap framebuffers
		gpu::flushBuffer();
		gpu::swapBuffers(true);
	}

	// Exit services
	core::exit();

	return 0;
}

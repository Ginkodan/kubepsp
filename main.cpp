#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <arpa/inet.h> 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <pspgu.h>
#include <pspgum.h>

PSP_MODULE_INFO("K8s Dashboard GUI", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

// Pod structure
typedef struct
{
    char name[64];
    char status[32];
} Pod;

// Dashboard state
typedef struct
{
    int wifi_connected;
    char ip[16];
    Pod pods[10];
    int pod_count;
} DashboardState;

DashboardState state = {
    .wifi_connected = 0,
    .ip = "0.0.0.0",
    .pod_count = 2};

// Dummy data
void mock_pods()
{
    strcpy(state.pods[0].name, "nginx-deployment-1234");
    strcpy(state.pods[0].status, "Running");
    strcpy(state.pods[1].name, "api-server-5678");
    strcpy(state.pods[1].status, "Pending");
}

// GU Initialization
void setup_gu()
{
    void *list = sceGuGetMemory(256 * 1024);
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void *)0, 512);
    sceGuDispBuffer(480, 272, (void *)0x88000, 512);
    sceGuDepthBuffer((void *)0x110000, 512);
    sceGuOffset(2048 - (480 / 2), 2048 - (272 / 2));
    sceGuViewport(2048, 2048, 480, 272);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, 480, 272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuFrontFace(GU_CW);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuClearColor(0xff000000);
    sceGuClearDepth(0);
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

// Minimal text drawing via debug screen
void draw_text(int x, int y, const char *text)
{
    pspDebugScreenSetXY(x / 8, y / 8); // Character cell size is 8x8
    pspDebugScreenPrintf(text);
}

// Draw the dashboard
void draw_dashboard()
{
    sceGuStart(GU_DIRECT, sceGuGetMemory(256 * 1024));
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    draw_text(0, 0, "K8s Dashboard GUI");
    draw_text(0, 16, state.wifi_connected ? "WiFi: Connected" : "WiFi: Not Connected");

    char ipstr[32];
    sprintf(ipstr, "IP: %s", state.ip);
    draw_text(0, 32, ipstr);

    draw_text(0, 56, "Pods:");
    int y = 72;
    for (int i = 0; i < state.pod_count; i++)
    {
        char line[128];
        snprintf(line, sizeof(line), "%s | %s", state.pods[i].name, state.pods[i].status);
        draw_text(0, y, line);
        y += 16;
    }

    draw_text(0, 240, "X: Toggle WiFi | []: Refresh | O: Exit");

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
}

// Exit callback
int exit_callback(int arg1, int arg2, void *common)
{
    sceKernelExitGame();
    return 0;
}

int callback_thread(SceSize args, void *argp)
{
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setup_callbacks(void)
{
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, 0);
    return thid;
}

int init_network()
{
    sceNetInit(0x10000, 42, 4, 42, 4);
    sceNetInetInit();
    sceNetApctlInit(0x8000, 48);
    return 0;
}



int connect_to_wifi(char *ip_out, size_t ip_size)
{
    int stateLast = -1;

    sceNetInit(0x10000, 42, 4, 42, 4);
    sceNetInetInit();
    sceNetApctlInit(0x8000, 48);

    int connect_result = sceNetApctlConnect(1); // NetConf ID 1
    if (connect_result != 0)
        return 0;

    const char spinner[] = "|/-\\";
    int frame = 0;

    while (1)
    {
        int state;
        sceNetApctlGetState(&state);
        if (state != stateLast)
            stateLast = state;

        // Draw animation frame
        sceGuStart(GU_DIRECT, sceGuGetMemory(256 * 1024));
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        draw_text(0, 0, "K8s Dashboard GUI");
        draw_text(0, 16, "WiFi: Connecting");

        char spin[2] = {spinner[frame % 4], '\0'};
        char status_line[64];
        snprintf(status_line, sizeof(status_line), "Connecting to WiFi %s", spin);
        draw_text(0, 40, status_line);

        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();

        frame++;

        if (state == 4) break;
        sceKernelDelayThread(500 * 1000); // 500ms
    }

    union SceNetApctlInfo info;
    if (sceNetApctlGetInfo(8, &info) == 0)
    {
        strncpy(ip_out, info.ip, ip_size);
        return 1; // success
    }

    strncpy(ip_out, "error", ip_size);
    return 0;
}



int main()
{
    setup_callbacks();
    pspDebugScreenInit();
    setup_gu();

    mock_pods();
    draw_dashboard();

    SceCtrlData pad;
    while (1)
    {
        sceCtrlReadBufferPositive(&pad, 1);

        if (pad.Buttons & PSP_CTRL_CROSS)
        {
            if (!state.wifi_connected)
            {
                if (connect_to_wifi(state.ip, sizeof(state.ip)))
                    state.wifi_connected = 1;
                else
                    strcpy(state.ip, "error");
            }
            else
            {
                sceNetApctlDisconnect();
                state.wifi_connected = 0;
                strcpy(state.ip, "0.0.0.0");
            }

            draw_dashboard();
        }

        if (pad.Buttons & PSP_CTRL_SQUARE)
        {
            mock_pods(); // In real app, fetch data here
            draw_dashboard();
        }

        if (pad.Buttons & PSP_CTRL_CIRCLE)
            break;

        sceKernelDelayThread(100 * 1000);
    }

    return 0;
}

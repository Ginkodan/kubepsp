#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_adhoc.h> // Added for sceNetHtons
#include <arpa/inet.h>    // Added for inet_addr
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

PSP_MODULE_INFO("K8s Dashboard PSP", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

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

int connect_to_wifi()
{
    int stateLast = -1;
    sceNetApctlConnect(1); // NetConf ID 1 (configure via XMB first)

    while (1)
    {
        int state;
        sceNetApctlGetState(&state);
        if (state != stateLast)
        {
            stateLast = state;
            pspDebugScreenSetXY(0, 2);
            pspDebugScreenPrintf("WiFi state: %d\n", state);
        }
        if (state == 4)
            break;                        // connected
        sceKernelDelayThread(500 * 1000); // 500ms
    }
    return 0;
}

int fetch_k8s_data()
{
    int sock;
    struct sockaddr_in server;
    char buffer[512];

    sock = sceNetInetSocket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(3000);                       // port of your Node.js proxy
    server.sin_addr.s_addr = inet_addr("192.168.2.130"); // change to your PC IP

    if (sceNetInetConnect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        pspDebugScreenPrintf("Connection failed\n");
        sceNetInetClose(sock);
        return -2;
    }

    const char *request = "GET /pods HTTP/1.0\r\nHost: localhost\r\n\r\n";
    int sent = sceNetInetSend(sock, request, strlen(request), 0);
    pspDebugScreenPrintf("Sent: %d\n", sent);

    // Read the entire HTTP response
    int received = 0;
    int total = 0;
    char response[4096] = {0};
    while ((received = sceNetInetRecv(sock, buffer, sizeof(buffer) - 1, 0)) > 0 && total < sizeof(response) - 1)
    {
        buffer[received] = '\0';
        strcat(response, buffer);
        total += received;
    }

    // Move pointer to JSON body
    char *json = strstr(response, "\r\n\r\n");
    if (json)
        json += 4;
    else
        json = response;

    // Simple JSON parsing: print each pod's name and status
    int y = 6;
    char *pod = strstr(json, "{\"name\":");
    while (pod && y < 28)
    {
        char name[64] = {0}, status[32] = {0};
        char *name_start = strstr(pod, "\"name\":\"");
        char *status_start = strstr(pod, "\"status\":\"");
        if (name_start && status_start)
        {
            name_start += 8;
            char *name_end = strchr(name_start, '"');
            if (name_end && name_end - name_start < sizeof(name))
            {
                strncpy(name, name_start, name_end - name_start);
                name[name_end - name_start] = '\0';
            }
            status_start += 10;
            char *status_end = strchr(status_start, '"');
            if (status_end && status_end - status_start < sizeof(status))
            {
                strncpy(status, status_start, status_end - status_start);
                status[status_end - status_start] = '\0';
            }
            pspDebugScreenSetXY(0, y++);
            pspDebugScreenPrintf("Pod: %s | Status: %s\n", name, status);
        }
        pod = strstr(pod + 1, "{\"name\":");
    }

    sceNetInetClose(sock);
    return 0;
}

int main()
{
    setup_callbacks();
    pspDebugScreenInit();
    pspDebugScreenPrintf("K8s Dashboard PSP\n");
    pspDebugScreenPrintf("-----------------\n");
    pspDebugScreenPrintf("Press X to connect WiFi\n");
    pspDebugScreenPrintf("Press O to exit\n");

    SceCtrlData pad;
    int wifi_connected = 0;

    while (1)
    {
        sceCtrlReadBufferPositive(&pad, 1);
        if ((pad.Buttons & PSP_CTRL_CROSS) && !wifi_connected)
        {
            pspDebugScreenSetXY(0, 4);
            pspDebugScreenPrintf("Initializing network...\n");
            init_network();
            connect_to_wifi();

            union SceNetApctlInfo info;
            if (sceNetApctlGetInfo(8, &info) == 0)
                pspDebugScreenPrintf("PSP IP: %s\n", info.ip);
            else
                pspDebugScreenPrintf("PSP IP: (error)\n");

            pspDebugScreenPrintf("Press [] to fetch from Kubernetes\n");
            wifi_connected = 1;
        }
        if ((pad.Buttons & PSP_CTRL_SQUARE) && wifi_connected)
        {
            pspDebugScreenSetXY(0, 7);
            pspDebugScreenPrintf("Fetching data from Kubernetes...\n");
            fetch_k8s_data();
            pspDebugScreenPrintf("Done. Press O to exit.\n");
        }
        if (pad.Buttons & PSP_CTRL_CIRCLE)
            break;

        sceKernelDelayThread(100 * 1000); // 100ms
    }

    return 0;
}

// SPDX-License-Identifier: GPL-3.0-or-later

#include "comm.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"

#include "../hw/platform_esp/espnow.h"
#elif defined(__linux__)
#include <dirent.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#include "../hw/platform_sim/state.h"
#endif

#include "base.h"
#include "../hw/platform.h"
#include "../opt/utils.h"

static AGRXState_t p_rx_state = AG_RX_NONE;
static pthread_mutex_t p_tx_lock;
static pthread_mutex_t p_rx_lock;

static AG_FRAME_L0 p_tx_frame = {{0, 0}, {0, 0}, {0}};
static AG_FRAME_L0 p_rx_frame = {{0, 0}, {0, 0}, {0}};

void agComm_InitTXFrame(AG_FRAME_L0 *frame) {
    uint32_t mac[2];

    hw_GetIDCompact(mac);
    frame->dst_mac[0] = 0x00FFFFFF;
    frame->dst_mac[1] = 0x00FFFFFF;
    frame->src_mac[0] = mac[0];
    frame->src_mac[1] = mac[1];
    memset(frame->data, 0, AG_FRM_DATA_LEN * sizeof (uint8_t));
}

void agComm_InitRXFrame(AG_FRAME_L0 *frame) {
    frame->dst_mac[0] = 0x0;
    frame->dst_mac[1] = 0x0;
    frame->src_mac[0] = 0x0;
    frame->src_mac[1] = 0x0;
    memset(frame->data, 0, AG_FRM_DATA_LEN * sizeof (uint8_t));
}

AGRXState_t agComm_GetRXState(void) {
    return p_rx_state;
}

AGFrmType_t agComm_GetRXFrameType(void) {
    return (AGFrmType_t) p_rx_frame.data[AG_FRM_TYPE_OFFSET];
}

AGFrmCmd_t agComm_GetRXFrameCmd(void) {
    return (AGFrmCmd_t) ((p_rx_frame.data[AG_FRM_CMD_OFFSET + 1] << 8) +
                         p_rx_frame.data[AG_FRM_CMD_OFFSET]);
}

AG_FRAME_L0 * agComm_GetRXFrame(void) {
    return &p_rx_frame;
}

void agComm_CpRXFrame(AG_FRAME_L0 *frame) {
    pthread_mutex_lock(&p_rx_lock);

    frame->src_mac[0] = p_rx_frame.src_mac[0];
    frame->src_mac[1] = p_rx_frame.src_mac[1];
    frame->dst_mac[0] = p_rx_frame.dst_mac[0];
    frame->dst_mac[1] = p_rx_frame.dst_mac[1];
    memcpy(frame->data, p_rx_frame.data, AG_FRM_DATA_LEN);
    p_rx_state = AG_RX_NONE;

    pthread_mutex_unlock(&p_rx_lock);
}

int agComm_IsRXFrameFromMaster(void) {
    for (int i = 0; i < AG_MC_MAX_CNT; i++) {
        if ((g_RemoteMCs[i].mac[1] == p_rx_frame.src_mac[1])
                && (g_RemoteMCs[i].mac[0] == p_rx_frame.src_mac[0])) {
            if ((g_RemoteMCs[i].capsSW & AG_CAP_SW_TMC) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

int agComm_IsRXFrameFromBcast(void) {
    if ((p_rx_frame.dst_mac[1] == 0x00FFFFFF)
            && (p_rx_frame.dst_mac[0] == 0x00FFFFFF)) {
        return 1;
    }
    return 0;
}

#if defined(ESP_PLATFORM)
static void p_espnow_tx_cbk(const uint8_t *mac_addr,
                            esp_now_send_status_t status) {
    //char *appName = pcTaskGetName(NULL);
    //ESP_LOGI(appName, "TX to "MACSTR" status %d", MAC2STR(mac_addr), status);
    if (status != ESP_NOW_SEND_SUCCESS) {
        g_MCStats.cntTXDrop ++;
    }
}

static void p_espnow_rx_cbk(const uint8_t *mac_addr, const uint8_t *data,
                            int len) {
    if (len > AG_FRM_DATA_LEN) {
        printf("%s - frame TOO BIG\n", __func__);
        return;
    }

    //ESP_LOGI("COMM", "RX %d - " MACSTR " -> me", data[AG_FRM_TYPE_OFFSET], MAC2STR(mac_addr));

    pthread_mutex_lock(&p_rx_lock);

    memset(p_rx_frame.data, 0, AG_FRM_DATA_LEN * sizeof (uint8_t));
    memcpy(p_rx_frame.data, data, len * sizeof (uint8_t));
    p_rx_frame.src_mac[1] = (mac_addr[0] << 16) | (mac_addr[1] << 8) | mac_addr[2];
    p_rx_frame.src_mac[0] = (mac_addr[3] << 16) | (mac_addr[4] << 8) | mac_addr[5];
    p_rx_state = AG_RX_FULL;

    pthread_mutex_unlock(&p_rx_lock);

    g_MCStats.cntRX ++;
}
#elif defined(__linux__)
static void p_mq_notify(void);

static uint8_t p_recv_data[AG_FRM_DATA_LEN + 12];

static void p_linux_rx_cbk(void) {
    pthread_mutex_lock(&p_rx_lock);

    if (p_rx_state == AG_RX_FULL) {
        g_MCStats.cntRXFail ++;
    }
    agComm_InitRXFrame(&p_rx_frame);
//    p_rx_frame.dst_mac[1] = ((uint32_t) p_recv_data[5] << 16) | ((
//                                uint32_t) p_recv_data[4] << 8) | p_recv_data[3];
//    p_rx_frame.dst_mac[0] = ((uint32_t) p_recv_data[2] << 16) | ((
//                                uint32_t) p_recv_data[1] << 8) | p_recv_data[0];
    p_rx_frame.src_mac[1] = ((uint32_t) p_recv_data[11] << 16) | ((
                                uint32_t) p_recv_data[10] << 8) | p_recv_data[9];
    p_rx_frame.src_mac[0] = ((uint32_t) p_recv_data[8] << 16) | ((
                                uint32_t) p_recv_data[7] << 8) | p_recv_data[6];
    memcpy(p_rx_frame.data, &p_recv_data[12],
           (unsigned long) AG_FRM_DATA_LEN * sizeof (uint8_t));
    p_rx_state = AG_RX_FULL;

    pthread_mutex_unlock(&p_rx_lock);

    log_file("COMM", "RX %d - %06x:%06x -> %06x:%06x",
             p_rx_frame.data[AG_FRM_TYPE_OFFSET], p_rx_frame.src_mac[1],
             p_rx_frame.src_mac[0],
             p_rx_frame.dst_mac[1], p_rx_frame.dst_mac[0]);
    g_MCStats.cntRX ++;
}

static void p_mq_rx(union sigval sv) {
    uint32_t mac[2];

    hw_GetIDCompact(mac);

    mqd_t mq_des = *((mqd_t *) sv.sival_ptr);
    struct mq_attr attr;

    // determine msg count and msg size
    if (mq_getattr(mq_des, &attr) == -1) {
        perror("CANNOT get mq attr");
        exit(EXIT_FAILURE);
    }
    while (attr.mq_curmsgs != 0) {
        //log_file("COMM", "RX mq %d/%d", attr.mq_curmsgs, attr.mq_maxmsg);
        ssize_t nb_rx = mq_receive(mq_des, (char *)p_recv_data,
                                   (size_t) attr.mq_msgsize,
                                   NULL);
        if (nb_rx == -1) {
            perror("RX failure");
            continue;
        }
        if (nb_rx != (AG_FRM_DATA_LEN + 12)) {
            printf("INCORRECT number of RX bytes\n");
            exit(EXIT_FAILURE);
        }

        uint32_t dst_mac1 = ((uint32_t) p_recv_data[5] << 16) | ((
                                uint32_t) p_recv_data[4] << 8) | p_recv_data[3];
        uint32_t dst_mac0 = ((uint32_t) p_recv_data[2] << 16) | ((
                                uint32_t) p_recv_data[1] << 8) | p_recv_data[0];

        if ((dst_mac1 == 0x00FFFFFF) && (dst_mac0 == 0x00FFFFFF)) {
            log_file("COMM", "RX for ALL");
            p_linux_rx_cbk();
        } else if ((dst_mac1 == mac[1]) && (dst_mac0 == mac[0])) {
            log_file("COMM", "RX for me");
            p_linux_rx_cbk();
        } else {
            log_file("COMM", "RX NOT for me %06x:%06x", dst_mac1, dst_mac0);
        };

        if (mq_getattr(mq_des, &attr) == -1) {
            perror("CANNOT get mq attr");
            exit(EXIT_FAILURE);
        }
    }
    p_mq_notify();
}

static void p_mq_notify(void) {
    struct sigevent sev;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = p_mq_rx;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = &(SIM_STATE.msg_queue);
    if (mq_notify(SIM_STATE.msg_queue, &sev) == -1) {
        perror("mq_notify");
        exit(EXIT_FAILURE);
    }
}
#endif

#if defined(__linux__)
static char p_send_data[AG_FRM_DATA_LEN + 12] = {0};
#endif

void agComm_SendFrame(AG_FRAME_L0 *frame) {
    if ((g_MCState.flags & AG_FLAG_SW_TXOFF) != 0) {
        return;
    }

//    ESP_LOGI("COMM", "TX %d - %06lx:%06lx -> %06lx:%06lx",
//             frame->data[AG_FRM_TYPE_OFFSET],
//             frame->src_mac[1], frame->src_mac[0], frame->dst_mac[1], frame->dst_mac[0]);
    log_file("COMM", "TX %d - %06x:%06x -> %06x:%06x",
             frame->data[AG_FRM_TYPE_OFFSET],
             frame->src_mac[1], frame->src_mac[0], frame->dst_mac[1], frame->dst_mac[0]);

    pthread_mutex_lock(&p_tx_lock);

#if defined(ESP_PLATFORM)
    if (AG_FRM_DATA_LEN > ESP_NOW_MAX_DATA_LEN) {
        //printf("%s - frame TOO BIG\n", __func__);
        return;
    }

    uint8_t dst_mac[6] = {(uint8_t) (frame->dst_mac[1] >> 16), (uint8_t) (frame->dst_mac[1] >> 8),
                          (uint8_t) (frame->dst_mac[1]), (uint8_t) (frame->dst_mac[0] >> 16),
                          (uint8_t) (frame->dst_mac[0] >> 8), (uint8_t) (frame->dst_mac[0])
                         };
    espnow_tx(dst_mac, frame->data, AG_FRM_DATA_LEN);
#elif defined(__linux__)
    p_send_data[0] = (char) (frame->dst_mac[0] & 0xFF);
    p_send_data[1] = (char) (frame->dst_mac[0] >> 8);
    p_send_data[2] = (char) (frame->dst_mac[0] >> 16);
    p_send_data[3] = (char) (frame->dst_mac[1] & 0xFF);
    p_send_data[4] = (char) (frame->dst_mac[1] >> 8);
    p_send_data[5] = (char) (frame->dst_mac[1] >> 16);

    p_send_data[6] = (char) (frame->src_mac[0] & 0xFF);
    p_send_data[7] = (char) (frame->src_mac[0] >> 8);
    p_send_data[8] = (char) (frame->src_mac[0] >> 16);
    p_send_data[9] = (char) (frame->src_mac[1] & 0xFF);
    p_send_data[10] = (char) (frame->src_mac[1] >> 8);
    p_send_data[11] = (char) (frame->src_mac[1] >> 16);

    memcpy(&p_send_data[12], frame->data, AG_FRM_DATA_LEN);

    char mq_name[SIM_PATH_LEN] = "";
    char dst_name[SIM_PATH_LEN] = "";

    snprintf(mq_name, SIM_PATH_LEN, "%s%03d", SIM_MQ_PREFIX, SIM_STATE.id);

    DIR *d = opendir("/dev/mqueue");
    if (d == NULL) {
        printf("%s\n", "CANNOT open folder");
        exit(EXIT_FAILURE);
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        size_t tmp_len = strlen(SIM_MQ_PREFIX);
        if (strncmp(SIM_MQ_PREFIX, dir->d_name, tmp_len) != 0) {
            continue;
        }
        if (strncmp(mq_name, dir->d_name, SIM_PATH_LEN) == 0) {
            continue;
        }

        dst_name[0] = '/';
        dst_name[1] = '\0';
        strcat(dst_name, dir->d_name);
        mqd_t queue = mq_open(dst_name, (O_WRONLY | O_NONBLOCK));
        if (queue == -1) {
            perror("CANNOT create mq");
            continue;
        }

        if (mq_send(queue, (const char *) p_send_data, (AG_FRM_DATA_LEN + 12),
                    0) == -1) {
            perror("CANNOT send msg");
            continue;
        }
        mq_close(queue);
    }
    closedir(d);
    usleep(5000);  // add some delay to simulate propagation
#endif

    pthread_mutex_unlock(&p_tx_lock);

    g_MCStats.cntTX ++;

//    ESP_LOGI("COMM", "TX done");
    log_file("COMM", "TX done");
}

void agComm_SetFrameType(AGFrmType_t type, AG_FRAME_L0 *frame) {
    frame->data[AG_FRM_TYPE_OFFSET] = (uint8_t) type;
}

void agComm_SetFrameCmd(AGFrmCmd_t cmd, AG_FRAME_L0 *frame) {
    frame->data[AG_FRM_CMD_OFFSET] = (uint8_t) (cmd & 0xFF);
    frame->data[AG_FRM_CMD_OFFSET + 1] = (uint8_t) ((cmd >> 8) & 0xFF);
}

void agComm_Init(void) {
    //printf("DBG %s\n", __func__ );
    pthread_mutex_init(&p_tx_lock, NULL);
    pthread_mutex_init(&p_rx_lock, NULL);
#if defined(ESP_PLATFORM)
    espnow_init();
    espnow_set_tx_callback(p_espnow_tx_cbk);
    espnow_set_rx_callback(p_espnow_rx_cbk);
#elif defined(__linux)
    p_mq_notify();
#endif
}

void agComm_Exit(void) {
    pthread_mutex_destroy(&p_tx_lock);
    pthread_mutex_destroy(&p_rx_lock);
    //printf("DBG %s\n", __func__ );
}

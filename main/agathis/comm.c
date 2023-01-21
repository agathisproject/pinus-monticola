// SPDX-License-Identifier: GPL-3.0-or-later

#include "comm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

#include "../hw/platform_sim/state.h"
#endif

#include "base.h"
#include "../hw/platform.h"

static AG_FRAME_L0 p_tx_frame = {{0, 0}, {0, 0}, 0, AG_FRAME_DATA_LEN, NULL};
static AG_FRAME_L0 p_rx_frame = {{0, 0}, {0, 0}, 0, AG_FRAME_DATA_LEN, NULL};

int ag_comm_is_frame_master(AG_FRAME_L0 *frame) {
    for (int i = 0; i < AG_MC_MAX_CNT; i++) {
        if ((REMOTE_MODS[i].mac[1] == frame->src_mac[1])
                && (REMOTE_MODS[i].mac[0] == frame->src_mac[0])) {
            if ((REMOTE_MODS[i].caps & AG_CAP_SW_TMC) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

#if defined(ESP_PLATFORM)
static void p_espnow_tx_cbk(const uint8_t *mac_addr,
                            esp_now_send_status_t status) {
    //char *appName = pcTaskGetName(NULL);
    //ESP_LOGI(appName, "TX to "MACSTR" status %d", MAC2STR(mac_addr), status);
}

static void p_espnow_rx_cbk(const uint8_t *mac_addr, const uint8_t *data,
                            int len) {
    //char *appName = pcTaskGetName(NULL);
    //ESP_LOGI(appName, "RX from "MACSTR" %d B", MAC2STR(mac_addr), len);
    if (len > p_rx_frame.nb) {
        printf("%s - frame TOO BIG\n", __func__);
        return;
    }

    memset(p_rx_frame.data, 0, p_rx_frame.nb * sizeof (uint8_t));
    memcpy(p_rx_frame.data, data, len * sizeof (uint8_t));
    p_rx_frame.src_mac[1] = (mac_addr[0] << 16) | (mac_addr[1] << 8) | mac_addr[2];
    p_rx_frame.src_mac[0] = (mac_addr[3] << 16) | (mac_addr[4] << 8) | mac_addr[5];
    p_rx_frame.flags |= AG_FRAME_FLAG_VALID;
}
#elif defined(__linux__)
static void p_mq_notify(void);

static void p_mq_rx(union sigval sv) {
    mqd_t mq_des = *((mqd_t *) sv.sival_ptr);
    struct mq_attr attr;

    /* Determine max. msg size; allocate buffer to receive msg */
    if (mq_getattr(mq_des, &attr) == -1) {
        perror("CANNOT get mq attr");
        return;
    }

    uint8_t *buff;

    buff = (uint8_t *) malloc((size_t) attr.mq_msgsize);
    if (buff == NULL) {
        printf("CANNOT allocate RX buffer\n");
        return;
    }

    ssize_t nb_rx = mq_receive(mq_des, (char *) buff, (size_t) attr.mq_msgsize,
                               NULL);
    if (nb_rx == -1) {
        perror("RX failure");
        free(buff);
        return;
    }
    //printf("DBG RX@%d %zd bytes\n", SIM_STATE.id, nb_rx);
    //printf("DBG RX@%d dst: %02x:%02x:%02x:%02x:%02x:%02x\n", SIM_STATE.id, buff[5], buff[4], buff[3], buff[2], buff[1], buff[0]);
    //printf("DBG RX@%d src: %02x:%02x:%02x:%02x:%02x:%02x\n", SIM_STATE.id, buff[11], buff[10], buff[9], buff[8], buff[7], buff[6]);
    if ((nb_rx - 12) != AG_FRAME_DATA_LEN) {
        printf("INCORRECT number of bytes RX\n");
        free(buff);
        p_mq_notify();
        return;
    }

    p_rx_frame.dst_mac[1] = ((uint32_t) buff[5] << 16) | ((uint32_t) buff[4] << 8) |
                            buff[3];
    p_rx_frame.dst_mac[0] = ((uint32_t) buff[2] << 16) | ((uint32_t) buff[1] << 8) |
                            buff[0];
    p_rx_frame.src_mac[1] = ((uint32_t) buff[11] << 16) | ((
                                uint32_t) buff[10] << 8) | buff[9];
    p_rx_frame.src_mac[0] = ((uint32_t) buff[8] << 16) | ((uint32_t) buff[7] << 8) |
                            buff[6];

    memset(p_rx_frame.data, 0, p_rx_frame.nb * sizeof (uint8_t));
    memcpy(p_rx_frame.data, &buff[12],
           (unsigned long) (nb_rx - 12) * sizeof (uint8_t));
//    for (int i = 0; i < p_rx_frame.nb; i++) {
//        p_rx_frame.data[i] = buff[i + 12];
//    }
    p_rx_frame.flags |= AG_FRAME_FLAG_VALID;

    free(buff);
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

static void ag_comm_rx_process(AG_FRAME_L0 *frame) {
//    printf("DBG %s: %d B from %06x:%06x\n", __func__, frame->nb,
//           (unsigned int) frame->src_mac[1], (unsigned int) frame->src_mac[0]);
    if (frame->data[1] == AG_PKT_TYPE_STATUS) {
        ag_add_remote_mod(frame->src_mac, frame->data[4]);
    }

    if (ag_comm_is_frame_master(frame)) {
        //printf("DBG RX@%d msg from master\n", SIM_STATE.id);
        if ((frame->data[0] == AG_PROTO_VER1) && (frame->data[1] == AG_PKT_TYPE_CMD)) {
            //printf("DBG RX@%d cmd from master %d\n", SIM_STATE.id, frame->data[2]);
            switch (frame->data[2]) {
                case AG_CMD_ID: {
                    ag_id_external();
                    break;
                }
                case AG_CMD_RESET: {
                    ag_reset();
                    break;
                }
                case AG_CMD_POWER_OFF: {
                    ag_brd_pwr_off();
                    break;
                }
                case AG_CMD_POWER_ON: {
                    ag_brd_pwr_on();
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
    frame->flags &= (uint8_t) ~AG_FRAME_FLAG_VALID;
}

int ag_comm_tx(AG_FRAME_L0 *frame) {
    if ((frame->flags & AG_FRAME_FLAG_VALID) == 0) {
        //printf("%s - INVALID frame\n", __func__);
        return -1;
    }

#if defined(ESP_PLATFORM)
    if (frame->nb > ESP_NOW_MAX_DATA_LEN) {
        //printf("%s - frame TOO BIG\n", __func__);
        return -1;
    }

    uint8_t dst_mac[6] = {(uint8_t) (frame->dst_mac[1] >> 16), (uint8_t) (frame->dst_mac[1] >> 8), (uint8_t) (frame->dst_mac[1]),
                          (uint8_t) (frame->dst_mac[0] >> 16), (uint8_t) (frame->dst_mac[0] >> 8), (uint8_t) (frame->dst_mac[0])
                         };
    espnow_tx(dst_mac, frame->data, frame->nb);
#elif defined(__linux__)
    char mq_name[SIM_PATH_LEN] = "";
    char dst_name[SIM_PATH_LEN] = "";
    char *send_data = (char *) malloc((AG_FRAME_DATA_LEN + 12) * sizeof(char));

    if (send_data == NULL) {
        printf("%s - CANNOT malloc\n", __func__);
        exit(EXIT_FAILURE);
    }

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
        mqd_t queue = mq_open(dst_name, O_WRONLY);
        if (queue == -1) {
            perror("CANNOT create mq");
            continue;
        }

        //printf("DBG TX@%d to %s\n", SIM_STATE.id, dst_name);
        send_data[0] = (char) (frame->dst_mac[0] & 0xFF);
        send_data[1] = (char) (frame->dst_mac[0] >> 8);
        send_data[2] = (char) (frame->dst_mac[0] >> 16);
        send_data[3] = (char) (frame->dst_mac[1] & 0xFF);
        send_data[4] = (char) (frame->dst_mac[1] >> 8);
        send_data[5] = (char) (frame->dst_mac[1] >> 16);

        send_data[6] = (char) (frame->src_mac[0] & 0xFF);
        send_data[7] = (char) (frame->src_mac[0] >> 8);
        send_data[8] = (char) (frame->src_mac[0] >> 16);
        send_data[9] = (char) (frame->src_mac[1] & 0xFF);
        send_data[10] = (char) (frame->src_mac[1] >> 8);
        send_data[11] = (char) (frame->src_mac[1] >> 16);

        for (int i = 0; i < frame->nb; i++) {
            send_data[i + 12] = (char) frame->data[i];
        }
        if (mq_send(queue, (const char *) send_data, (AG_FRAME_DATA_LEN + 12),
                    0) == -1) {
            perror("CANNOT send msg");
            continue;
        }

        mq_close(queue);
    }
    closedir(d);
    free(send_data);
#endif
    frame->flags &= (uint8_t) ~AG_FRAME_FLAG_VALID;
    return 0;
}

AG_FRAME_L0 *ag_comm_get_tx_frame(void) {
    uint32_t my_mac[2];

    // if the frame is being transmitted, wait
    while ((p_tx_frame.flags & AG_FRAME_FLAG_VALID) != 0) {
#if defined(ESP_PLATFORM)
        vTaskDelay(1 / portTICK_PERIOD_MS);
#elif defined(__linux__)
        usleep(1000);
#endif
    }

    hw_GetIDCompact(my_mac);
    p_tx_frame.src_mac[0] = my_mac[0];
    p_tx_frame.src_mac[1] = my_mac[1];
    memset(p_tx_frame.data, 0, p_tx_frame.nb * sizeof (uint8_t));
    p_tx_frame.flags |= AG_FRAME_FLAG_VALID;
    return &p_tx_frame;
}

void ag_comm_init(void) {
    p_tx_frame.data = (uint8_t *) malloc((size_t) p_tx_frame.nb);
    if (p_tx_frame.data == NULL) {
        printf("CANNOT allocate TX buffer\n");
        return;
    }

    p_rx_frame.data = (uint8_t *) malloc((size_t) p_rx_frame.nb);
    if (p_rx_frame.data == NULL) {
        printf("CANNOT allocate RX buffer\n");
        return;
    }

#if defined(ESP_PLATFORM)
    espnow_init();
    espnow_set_tx_callback(p_espnow_tx_cbk);
    espnow_set_rx_callback(p_espnow_rx_cbk);
#elif defined(__linux)
    p_mq_notify();
#endif
}

void ag_comm_main(void) {
#if defined(ESP_PLATFORM)
    time_t ts_now = time(NULL);
    if ((ts_now % 5) == 0) {
        AG_FRAME_L0 *frame = ag_comm_get_tx_frame();
        frame->dst_mac[0] = 0x00FFFFFF;
        frame->dst_mac[1] = 0x00FFFFFF;
        frame->data[0] = AG_PROTO_VER1;
        frame->data[1] = AG_PKT_TYPE_STATUS;
        frame->data[4] = MOD_STATE.caps_sw;
        ag_comm_tx(frame);
    }
    if ((p_rx_frame.flags & AG_FRAME_FLAG_VALID) != 0) {
        ag_comm_rx_process(&p_rx_frame);
    }
#elif defined(__linux__)
    time_t ts_now = time(NULL);
    if ((ts_now % 5) == 0) {
        AG_FRAME_L0 *frame = ag_comm_get_tx_frame();
        frame->dst_mac[0] = 0x00FFFFFF;
        frame->dst_mac[1] = 0x00FFFFFF;
        frame->data[0] = AG_PROTO_VER1;
        frame->data[1] = AG_PKT_TYPE_STATUS;
        frame->data[4] = MOD_STATE.caps_sw;
        ag_comm_tx(frame);
    }
    if ((p_rx_frame.flags & AG_FRAME_FLAG_VALID) != 0) {
        ag_comm_rx_process(&p_rx_frame);
    }
#endif
}

// SPDX-License-Identifier: GPL-3.0-or-later

#include "comm.h"

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

#include "../hw/platform_sim/state.h"
#endif

#include "base.h"
#include "../hw/platform.h"

static AG_FRAME_L0 p_tx_frame = {{0, 0}, {0, 0}, 0, AG_FRM_DATA_LEN, NULL};
static AG_FRAME_L0 p_rx_frame = {{0, 0}, {0, 0}, 0, AG_FRM_DATA_LEN, NULL};

int agComm_IsFrameFromMaster(AG_FRAME_L0 *frame) {
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

int agComm_IsFrameSts(AG_FRAME_L0 *frame) {
    if (frame->data[1] == AG_FRM_TYPE_STS) {
        return 1;
    } else {
        return 0;
    }
}

int agComm_IsFrameCmd(AG_FRAME_L0 *frame) {
    if (frame->data[1] == AG_FRM_TYPE_CMD) {
        return 1;
    } else {
        return 0;
    }
}

int agComm_IsFrameAck(AG_FRAME_L0 *frame) {
    if (frame->data[1] == AG_FRM_TYPE_ACK) {
        return 1;
    } else {
        return 0;
    }
}

AG_FRAME_L0 *agComm_GetTXFrame(void) {
    uint32_t my_mac[2];

    // if the frame is being used, wait
    while (p_tx_frame.flags != 0) {
#if defined(ESP_PLATFORM)
        vTaskDelay(1 / portTICK_PERIOD_MS);
#elif defined(__linux__)
        usleep(1000);
#endif
    }

    p_tx_frame.flags |= AG_FRM_FLAG_APP_WR;
    hw_GetIDCompact(my_mac);
    p_tx_frame.src_mac[0] = my_mac[0];
    p_tx_frame.src_mac[1] = my_mac[1];
    memset(p_tx_frame.data, 0, p_tx_frame.nb * sizeof (uint8_t));
    return &p_tx_frame;
}

AG_FRAME_L0 *agComm_GetRXFrame(void) {
    while ((p_rx_frame.flags & AG_FRM_FLAG_APP_RD) == 0 ) {
#if defined(ESP_PLATFORM)
        vTaskDelay(1 / portTICK_PERIOD_MS);
#elif defined(__linux__)
        usleep(1000);
#endif
    }

    return &p_rx_frame;
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

    if (p_rx_frame.flags != 0) {
        printf("E (%s) dropping RX frame\n", __func__);
        return;
    }

    memset(p_rx_frame.data, 0, p_rx_frame.nb * sizeof (uint8_t));
    memcpy(p_rx_frame.data, data, len * sizeof (uint8_t));
    p_rx_frame.src_mac[1] = (mac_addr[0] << 16) | (mac_addr[1] << 8) | mac_addr[2];
    p_rx_frame.src_mac[0] = (mac_addr[3] << 16) | (mac_addr[4] << 8) | mac_addr[5];
    p_rx_frame.flags |= AG_FRM_FLAG_PHY_RX;
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
    if ((nb_rx - 12) != AG_FRM_DATA_LEN) {
        printf("INCORRECT number of bytes RX\n");
        free(buff);
        p_mq_notify();
        return;
    }

    if (p_rx_frame.flags != 0) {
        printf("E (%s) dropping RX frame\n", __func__);
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
    p_rx_frame.flags = AG_FRM_FLAG_PHY_RX;
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

void agComm_SendFrame(void) {
    if ((p_tx_frame.flags & AG_FRM_FLAG_PHY_TX) == 0) {
        return;
    }

#if defined(ESP_PLATFORM)
    if (p_tx_frame.nb > ESP_NOW_MAX_DATA_LEN) {
        //printf("%s - frame TOO BIG\n", __func__);
        return;
    }

    uint8_t dst_mac[6] = {(uint8_t) (p_tx_frame.dst_mac[1] >> 16), (uint8_t) (p_tx_frame.dst_mac[1] >> 8),
                          (uint8_t) (p_tx_frame.dst_mac[1]), (uint8_t) (p_tx_frame.dst_mac[0] >> 16),
                          (uint8_t) (p_tx_frame.dst_mac[0] >> 8), (uint8_t) (p_tx_frame.dst_mac[0])
                         };
    espnow_tx(dst_mac, p_tx_frame.data, p_tx_frame.nb);
#elif defined(__linux__)
    char mq_name[SIM_PATH_LEN] = "";
    char dst_name[SIM_PATH_LEN] = "";
    char *send_data = (char *) malloc((AG_FRM_DATA_LEN + 12) * sizeof(char));

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
        send_data[0] = (char) (p_tx_frame.dst_mac[0] & 0xFF);
        send_data[1] = (char) (p_tx_frame.dst_mac[0] >> 8);
        send_data[2] = (char) (p_tx_frame.dst_mac[0] >> 16);
        send_data[3] = (char) (p_tx_frame.dst_mac[1] & 0xFF);
        send_data[4] = (char) (p_tx_frame.dst_mac[1] >> 8);
        send_data[5] = (char) (p_tx_frame.dst_mac[1] >> 16);

        send_data[6] = (char) (p_tx_frame.src_mac[0] & 0xFF);
        send_data[7] = (char) (p_tx_frame.src_mac[0] >> 8);
        send_data[8] = (char) (p_tx_frame.src_mac[0] >> 16);
        send_data[9] = (char) (p_tx_frame.src_mac[1] & 0xFF);
        send_data[10] = (char) (p_tx_frame.src_mac[1] >> 8);
        send_data[11] = (char) (p_tx_frame.src_mac[1] >> 16);

        for (int i = 0; i < p_tx_frame.nb; i++) {
            send_data[i + 12] = (char) p_tx_frame.data[i];
        }
        if (mq_send(queue, (const char *) send_data, (AG_FRM_DATA_LEN + 12),
                    0) == -1) {
            perror("CANNOT send msg");
            continue;
        }

        mq_close(queue);
    }
    closedir(d);
    free(send_data);
#endif
    p_tx_frame.flags = 0;
    MOD_STATS.tx_cnt ++;
}

void agComm_RecvFrame(void) {
    if ((p_rx_frame.flags & AG_FRM_FLAG_PHY_RX) == 0) {
        return;
    }

//    printf("DBG %s: %d B from %06x:%06x\n", __func__, frame->nb,
//           (unsigned int) frame->src_mac[1], (unsigned int) frame->src_mac[0]);
    MOD_STATS.rx_cnt ++;

    if (agComm_IsFrameSts(&p_rx_frame) != 0) {
        ag_add_remote_mod(p_rx_frame.src_mac, p_rx_frame.data[4]);
        p_rx_frame.flags = 0;
    } else if (agComm_IsFrameAck(&p_rx_frame) != 0) {
        p_rx_frame.flags = AG_FRM_FLAG_APP_RD;
    } else if (agComm_IsFrameCmd(&p_rx_frame) != 0) {
        if (agComm_IsFrameFromMaster(&p_rx_frame) != 0) {
            //printf("DBG RX@%d msg from master\n", SIM_STATE.id);
            //printf("DBG RX@%d cmd from master %d\n", SIM_STATE.id, frame->data[2]);
            switch (agComm_GetFrameCmd(&p_rx_frame)) {
                case AG_FRM_CMD_ID: {
                    ag_id_external();
                    break;
                }
                case AG_FRM_CMD_RESET: {
                    ag_reset();
                    break;
                }
                case AG_FRM_CMD_POWER_OFF: {
                    ag_brd_pwr_off();
                    break;
                }
                case AG_FRM_CMD_POWER_ON: {
                    ag_brd_pwr_on();
                    break;
                }
                case AG_FRM_CMD_PING: {
                    AG_FRAME_L0 *frame = agComm_GetTXFrame();
                    frame->dst_mac[0] = p_rx_frame.src_mac[0];
                    frame->dst_mac[1] = p_rx_frame.src_mac[1];
                    frame->data[0] = AG_FRM_PROTO_VER1;
                    agComm_SetFrameType(AG_FRM_TYPE_ACK, frame);
                    agComm_SetFrameRTS(frame);
                    break;
                }
                default: {
                    printf("W (%s) UNRECOGNIZED command %d\n", __func__,
                           agComm_GetFrameCmd(&p_rx_frame));
                    break;
                }
            }
        }
        p_rx_frame.flags = 0;
    }
}

void agComm_SetFrameType(AGFrmType_t type, AG_FRAME_L0 *frame) {
    frame->data[1] = (uint8_t) type;
}

void agComm_SetFrameCmd(AGFrmCmd_t cmd, AG_FRAME_L0 *frame) {
    frame->data[2] = (uint8_t) (cmd & 0xFF);
    frame->data[3] = (uint8_t) ((cmd >> 8) & 0xFF);
}

AGFrmCmd_t agComm_GetFrameCmd(AG_FRAME_L0 *frame) {
    return (AGFrmCmd_t) ((frame->data[3] << 8) + frame->data[2]);
}

void agComm_SetFrameRTS(AG_FRAME_L0 *frame) {
    frame->flags = AG_FRM_FLAG_PHY_TX;
}

void agComm_SetFrameDone(AG_FRAME_L0 *frame) {
    frame->flags &= (uint8_t) ~AG_FRM_FLAG_APP_RD;
}

void agComm_Init(void) {
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

void agComm_SendStatus(void) {
    AG_FRAME_L0 *frame = agComm_GetTXFrame();
    frame->dst_mac[0] = 0x00FFFFFF;
    frame->dst_mac[1] = 0x00FFFFFF;
    frame->data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_STS, frame);
    frame->data[4] = MOD_STATE.caps_sw;
    agComm_SetFrameRTS(frame);
}

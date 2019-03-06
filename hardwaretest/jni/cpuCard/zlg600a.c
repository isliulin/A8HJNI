#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "serial/serialServer.h"
#include "cpuCard/zlg600a.h"
#include "common/debugLog.h"

// ic card interface ON/OFF (however, these are command codes, fuck!!!)
enum card_interface_state {
	INTERFACE_ON = 0x42, INTERFACE_OFF = 0x43,
};

struct ic_reader_ctl {
	unsigned char *respons;
	pSerialOps uart;
};

static struct ic_reader_ctl *ctl;
static const int serial_baud = 115200;
static const int uart_data_bit = 8;
static const int uart_stop_bit = 1;
static const int uart_parity_bit = 0;
static const int time_out_ms = 200;
// Function forward decleration
static int ctl_serial_baud_sync(void);
static int ctl_get_dev_info(unsigned char *info);
static int ctl_card_interface_cfg(enum card_interface_state state);

// ================================================================================
// Utility Function
// ================================================================================
static unsigned char _get_bcc(const unsigned char *buf, unsigned char len) {
	unsigned char bcc = 0;
	unsigned char i = 0;

	while (i < len)
		bcc ^= buf[i++];

	return ~bcc;
}

// Function:	send command to controller, check respons from controller
// @buf:		buffer of command to be send
// @len:		length of command
// @info:		buffer for the info field of the respons frame
// @size:		size of the info buffer
// Return:		length of info data, negative error code if failed
static int _send_command(const unsigned char *cmd, size_t len) {
	int ret;
	unsigned char respons_len;
	unsigned char status;
	unsigned char info_len;
	unsigned char bcc;
	unsigned char *pinfo;
	int try = 0;
	memset(ctl->respons, 0, MAX_BUF_SIZE);

	ctl->uart->write(ctl->uart, cmd, len);
	ret = ctl->uart->read(ctl->uart, ctl->respons, MAX_BUF_SIZE, time_out_ms);

	if (ret == -ERR_UART_TIME_OUT) { // timeout / uart read error
		LOGE("%s: Respons time out, reset!!!\n", __func__);
		return ret;
	}

	if (ret == -1) {
		LOGE("%s: Uart unknow read error!\n", __func__);
		return ret;
	}

	respons_len = ctl->respons[0];
	if (ret < respons_len) {
		LOGE("%s: uncompleted respons frame\n", __func__);
		return -ERR_UART_UNKNOW_ERR;
	}

	if (ret > respons_len) {
		LOGE("%s: more than one respons frame recieved\n", __func__);
	}

	if (ctl->respons[respons_len - 2]
			!= _get_bcc(ctl->respons, respons_len - 2)) {
		LOGE("%s: Respons frame check error!\n", __func__);
		return -ERR_CTL_RESPONS_ERROR;
	}
	return ctl->respons[2];
}

// ================================================================================
// Reader Controller init & close
// ================================================================================
int ic_reader_ctl_init(const char *path) {
	int ret;
	int restartCount = 0;
	unsigned char info[MAX_BUF_SIZE];
	LOGE("ic_reader_ctl_init!");
	if (NULL == path)
		return -1;
	if (ctl)
		ic_reader_ctl_close();

	ctl = calloc(1, sizeof(*ctl));
	if (NULL == ctl) {
		ret = -ERR_MEM_ALLOC_FAILED;
		goto end;
	}

	ctl->respons = calloc(MAX_BUF_SIZE, sizeof(ctl->respons[0]));
	if (NULL == ctl->respons) {
		ret = -ERR_MEM_ALLOC_FAILED;
		goto free_ctl;
	}

	ctl->uart = createSerialServer(path, serial_baud, uart_data_bit,
			uart_stop_bit, uart_parity_bit);
	if (NULL == ctl->uart) {
		LOGE("fail to  createSerialServer!");
		ret = -ERR_UART_SERVER_CREATE;
		goto free_respons;
	}

	reset:
		restartCount++;
	ret = ctl_get_dev_info(info);
	if (0 != ret) {
			LOGE("%s: ctl_get_dev_info error!\n", __func__);
			ret = ctl_serial_baud_sync();
			if(ret != 0 )
			{
				LOGW("fail to ctl_serial_baud_sync, reset :%d! ",restartCount);
				usleep(1000);
				if(restartCount > 3)
				{
					goto close_uart;
				}
				goto reset;
			}
	}
	memset(info, 0, MAX_BUF_SIZE);

	ret = ctl_card_interface_cfg(INTERFACE_ON);
	if (0 == ret) {
		LOGE("%s: Success!!!\n", __func__);
		goto end;
	}
	LOGE("%s: ctl_card_interface_cfg ON failed!\n", __func__);
	close_uart: if (ctl->uart) {
		destroySerialServer(&ctl->uart);
		ctl->uart = NULL;
	}
	free_respons: if (ctl->respons) {
		free(ctl->respons);
		ctl->respons = NULL;
	}
	free_ctl: if (ctl) {
		free(ctl);
		ctl = NULL;
	}
	end: return ret;
}

void ic_reader_ctl_close(void) {
	if (ctl) {
		if (ctl->uart && ctl->respons)
			ctl_card_interface_cfg(INTERFACE_OFF);

		if (ctl->uart) {
			destroySerialServer(&ctl->uart);
			ctl->uart = NULL;
		}
		if (ctl->respons) {
			free(ctl->respons);
			ctl->respons = NULL;
		}
		free(ctl);
		ctl = NULL;
	}
}

// ================================================================================
// Reader Controller Command
// ================================================================================

 int ctl_reset_dev(void) {
	int ret;
	unsigned char info[MAX_BUF_SIZE] = { 0 };
	ctl_serial_baud_sync();
	memset(info, 0, MAX_BUF_SIZE);
	ret = ctl_get_dev_info(info);
	if (0 != ret) {
		LOGE("%s: ctl_get_dev_info error!\n", __func__);
		goto fail0;
	}
	ret = ctl_card_interface_cfg(INTERFACE_ON);
	if (ret) {
		LOGE("fail to ctl_card_interface_cfg");
		goto fail0;
	}
	LOGD("%s: Success!!!\n", __func__);
	return 0;
	fail0: return ret;
}
static int ctl_serial_baud_sync(void) {
	unsigned char val = 0x20;
	int i, ret;

	ctl->respons[0] = 0x00;

	for (i = 0; i < 3; i++) {
		ctl->uart->write(ctl->uart, &val, 1);
		usleep(5000);
		ctl->uart->write(ctl->uart, &val, 1);

		if ((ret = ctl->uart->read(ctl->uart, ctl->respons, MAX_BUF_SIZE,
				time_out_ms)) == 1)
			break;
	}

	if (ret > 0) {
		if (0x06 != ctl->respons[0]) {
			LOGE("%s: failed!\n", __func__);
			return -ERR_CTL_RESPONS_ERROR;
		} else {
			LOGE("%s: Scuccess!\n", __func__);
		}
	} else if (ret == -ERR_UART_TIME_OUT) {
		LOGE("%s: Respons time out!\n", __func__);
		return ret;
	} else {
		LOGE("%s: Unknow Uart Read Error!\n", __func__);
		return ret;
	}

	return 0;
}

static int ctl_get_dev_info(unsigned char *info) {
	const unsigned char cmd[] = { 0x06, 0x01, 0x41, 0x00, 0xB9, 0x03 };
	int ret = _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));

	if (ret) {
		LOGE("%s: failed!\n", __func__);
		return ret;
	}
	memcpy(info, ctl->respons + 4, ctl->respons[3]);
	return 0;
}

static int ctl_card_interface_cfg(enum card_interface_state state) {
	unsigned char cmd[] = { 0x06, 0x01, 0x42, 0x00, 0xBA, 0x03 };

	cmd[2] = state;
	cmd[4] = _get_bcc(cmd, 4);
	return _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));
}

int ctl_set_picc_type(enum picc_type type) {
	unsigned char cmd[] = { 0x07, 0x01, 0x44, 0x01, 0x04, 0xB8, 0x03 };

	cmd[4] = type;
	cmd[5] = _get_bcc(cmd, 5);
	return _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));
}

// ================================================================================
// ISO14443-A PICC Command
// ================================================================================
int picc_A_request(void) {
	const unsigned char cmd[] = { 0x07, 0x06, 0x41, 0x01, 0x26, 0x98, 0x03 };
	return _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));
}

int picc_A_active(unsigned short *atq, unsigned char *sak, unsigned char *len,
		unsigned char *uid) {
	const unsigned char cmd[] =
			{ 0x08, 0x06, 0x4D, 0x02, 0x00, 0x26, 0x98, 0x03 };
	int ret = _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));

	if (0 == ret) {
		*atq = ctl->respons[4] + (ctl->respons[5] << 8);
		*sak = ctl->respons[6];
		*len = ctl->respons[7];
		memcpy(uid, ctl->respons + 8, *len);
	}

	return ret;
}

int picc_A_halt(void) {
	const unsigned char cmd[] = { 0x06, 0x06, 0x44, 0x00, 0xBB, 0x03 };
	return _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));
}

// ================================================================================
// ISO14443-B PICC Command
// ================================================================================
int picc_B_request(void) {
	const unsigned char cmd[] = { 0x09, 0x06, 0x50, 0x03, 0x00, 0x00, 0x00,
			0xA3, 0x03 };
	return _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));
}

int picc_B_active(unsigned char *info, unsigned char *len __unused) {
	const unsigned char cmd[] =
			{ 0x08, 0x06, 0x4E, 0x02, 0x00, 0x00, 0xBD, 0x03 };
	int ret = _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));

	if (0 == ret)
		memcpy(info, ctl->respons + 4, ctl->respons[3]);

	return ret;
}

int picc_B_halt(void) {
	const unsigned char cmd[] = { 0x0A, 0x06, 0x53, 0x04, 0x95, 0x22, 0x49,
			0x38, 0x62, 0x03 };
	return _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));
}

int picc_B_resident_ID_read(unsigned char *uid, unsigned char *len) {
	const unsigned char cmd[] = { 0x07, 0x06, 0x54, 0x01, 0x08, 0xA3, 0x03 };
	int ret = _send_command(cmd, sizeof(cmd) / sizeof(cmd[0]));

	if (ret) {
		LOGE("%s: failed!\n", __func__);
		return ret;
	}

	*len = ctl->respons[3];
	memcpy(uid, ctl->respons + 4, *len);
	return 0;
}


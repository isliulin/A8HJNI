#include <stdio.h>
#include <unistd.h>
#include "taskManage/threadManage.h"
#include "cpuCard/cpu_card.h"
#include "cpuCard/zlg600a.h"
#include "common/debugLog.h"
#include "hwInterface/hwInterfaceManage.h"

struct ic_reader {
	pThreadOps thread;
	RecvFunc id_report_handler;
};

union card_id_buf {
	char buf[sizeof(uint32_t)];
	uint32_t id;
};
typedef struct CpuCardServer {
	CpuCardOps ops;
	struct ic_reader *reader;
} CpuCardServer, *pCpuCardServer;

// Function forward declaration
static void picc_A_card_catch_proc(RecvFunc upfunc);
static void picc_B_card_catch_proc(RecvFunc upfunc);
static int GetWeiGendCardId(const unsigned char *hexIn, int len, uint32_t *id);
// Poll thread function
static void *poll_thread_func(void *para);

// Export interface implemetation

pCpuCardOps createZLG600AServer(const char *devPath, RecvFunc IDHandler) {
	int ret;
	pCpuCardServer server = NULL;
	if (!devPath || !IDHandler) {
		LOGE("%s: Parameter can not be nul\n", __func__);
		return NULL;
	}
	if (server)
		destroyCpuCardServer();

	server = calloc(1, sizeof(*server));
	if (!server) {
		LOGE("%s: Alloc memory failed!\n", __func__);
		ret = -ERR_MEM_ALLOC_FAILED;
		goto end;
	}
	server->reader = calloc(1, sizeof(struct ic_reader));
	if (!server->reader) {
		LOGE("%s: Alloc memory failed!\n", __func__);
		goto free_server;
	}

	ret = ic_reader_ctl_init(devPath);
	if (ret) {
		LOGE("%s: ic_reader_ctl_init failed!\n", __func__);
		goto free_reader;
	}

	server->reader->id_report_handler = IDHandler;
	server->reader->thread = pthread_register(poll_thread_func, &server,
			sizeof(void*), NULL);
	if (!server->reader->thread) {
		LOGE("%s: pthread_register failed!\n", __func__);
		goto close_ctl;
	}

	if (server->reader->thread->start(server->reader->thread) != 0)
		goto destroy_thread;

	return (pCpuCardOps) server;
	destroy_thread:
	LOGE("%s: pthread_start failed!\n", __func__);
	pthread_destroy(&(server->reader->thread));
	close_ctl: ic_reader_ctl_close();
	free_reader: if (server->reader) {
		free(server->reader);
		server->reader = NULL;
	}
	free_server: if (server) {
		free(server);
		server = NULL;
	}
	end: return NULL;
}

void destroyZLG600AServer(pCpuCardOps *pthis) {
	if (pthis != NULL && *pthis != NULL)
		return;
	pCpuCardServer server = (pCpuCardServer) *pthis;
	if (server->reader) {
		if (server->reader->thread) {
			server->reader->thread->stop(server->reader->thread);
			pthread_destroy(&server->reader->thread);
			server->reader->thread = NULL;
		}
		ic_reader_ctl_close();
		free(server->reader);
		server->reader = NULL;
		free(server);
		*pthis = NULL;
	}
}

static void picc_A_card_catch_proc(RecvFunc upfunc) {
	CARD_TYPE card_type;
	unsigned short atq;
	unsigned char sak;
	unsigned char len;
	unsigned char uid[32];
	int i, id_len;

	if (picc_A_active(&atq, &sak, &len, uid) != 0) {
		//LOGE("%s: picc_A_active failed!\n", __func__);
		return;
	}
	if (picc_A_halt() != 0)
		LOGE("%s: picc_A_halt failed!\n", __func__);

	LOGE("atq = %04X, sak = %02X, len = %d\n uid:", atq, sak, len);
	for (i = 0; i < len; i++)
		LOGE(" %02X", uid[i]);
	LOGE("\n");

	if (0x0004 == atq && 0x08 == sak)
		card_type = IC_CARD;
	else
		card_type = CPU_CARD;

//	union card_id_buf id_buf;

//id_len = GetWeiGendCardId(uid, len, &id_buf.id);
//LOGE("Card ID length: %d, ID: ", id_len);

	if (upfunc)
		upfunc(card_type, uid, len);
}

static void picc_B_card_catch_proc(RecvFunc upfunc) {
	CARD_TYPE card_type;
	unsigned char info[32], id[32];
	unsigned char info_len, id_len = 0;
	int i;

	if (picc_B_resident_ID_read(id, &id_len) == 0) {
		card_type = ID_CARD;
		LOGE("%s: Resident ID Card: ", __func__);
		if (id_len) {
			for (i = 0; i < id_len; i++)
				LOGE(" %02X", id[i]);
			LOGE("\n");
		}
	} else {
		LOGE("%s: picc_B_resident_ID_read failed!\n", __func__);
	}

	if (picc_B_active(info, &info_len) == 0) {
		if (0 == id_len) {
			card_type = CPU_CARD;
			id_len = info_len;
			memcpy(id, info, info_len);
			LOGE("%s: PICC_B Card: ", __func__);
			if (id_len) {
				for (i = 0; i < id_len; i++)
					LOGE(" %02X", id[i]);
				LOGE("\n");
			}
		}
	} else {
		LOGE("%s: picc_B_active failed!\n", __func__);
	}

	if (picc_B_halt())
		LOGE("%s: picc_B_halt failed!\n", __func__);
	if (id_len && upfunc)
		upfunc(card_type, id, id_len);
}

static void *poll_thread_func(void *para) {

	pCpuCardServer server = (*(pCpuCardServer*) para);
	int ret = 0;
	while (1) {
		if (Thread_Stop
				== server->reader->thread->check(server->reader->thread))
			break;
		usleep(1000 * 50);
		ret = ctl_set_picc_type(PICC_TYPE_A);
		if (ret) {
			LOGE("%s: ctl_set_picc_type A failed!\n", __func__);
			if (ret == -ERR_UART_TIME_OUT) {
				LOGE("send cmd timeout!");
				goto reset;
			}
		}
		ret = picc_A_request();
		if (0 == ret) {
			picc_A_card_catch_proc(server->reader->id_report_handler);
		} else if (ret == -ERR_UART_TIME_OUT) {
			LOGE("send cmd timeout!");
			goto reset;
		} else {
			//LOGE("fail to picc_A_request!");
		}

		ret = ctl_set_picc_type(PICC_TYPE_B);
		if (ret == -ERR_UART_TIME_OUT) {
			LOGE("send cmd timeout!");
			goto reset;
		}
		ret = picc_B_request();
		if (ret == 0)
			picc_B_card_catch_proc(server->reader->id_report_handler);
		else if (ret == -ERR_UART_TIME_OUT) {
			LOGE("send cmd timeout!");
			goto reset;
		}
		continue;
	reset:
		LOGW("reset dev!");
		sleep(2);
		ret = ctl_reset_dev();
		if (ret) {
			LOGE("fail to ctl_reset_dev!");
		}
	}
	return NULL;
}

static int GetWeiGendCardId(const unsigned char *hexIn, int len, uint32_t *id) {
	char s[6];
	if (hexIn == NULL || len > 6)
		return -1;

	union {
		char buf[sizeof(uint32_t)];
		uint32_t id;
	} intChar_union;

	union {
		char buf[sizeof(uint16_t)];
		uint16_t id;
	} shortChar_union;

	int i = 0;
	for (i = 0; i < len; i++) {
		//LOGE(" 0x%x",hexIn[i]);
		LOGE(" 0x%x", hexIn[i]);
	}

	//	printf("buf point=%p, int point=%p\n", &intChar_union.buf[0], &intChar_union.id);
	//	printf("hex[0]=%#x, hex[1]=%#x, hex[2]=%#x, hex[3]=%#x, \n",
	//									hexIn[0], hexIn[1], hexIn[2], hexIn[3]);

	sprintf(s, "%03u", hexIn[2]);
	//	printf("3bit=%s\n", s);
	intChar_union.buf[0] = (s[0] & 0x0f) << 4;
	intChar_union.buf[0] |= (s[1] & 0x0f);
	intChar_union.buf[1] = (s[2] & 0x0f) << 4;

	memcpy(shortChar_union.buf, hexIn, sizeof(uint16_t));
	sprintf(s, "%05u", shortChar_union.id);
	//	printf("5bit=%s\n", s);
	intChar_union.buf[1] |= (s[0] & 0x0f);
	intChar_union.buf[2] = (s[1] & 0x0f) << 4;
	intChar_union.buf[2] |= (s[2] & 0x0f);
	intChar_union.buf[3] = (s[3] & 0x0f) << 4;
	intChar_union.buf[3] |= (s[4] & 0x0f);
	*id = intChar_union.id;

	return len;
}


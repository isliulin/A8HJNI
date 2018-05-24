#ifndef __ZLG600A_H__
#define __ZLG600A_H__

// ================================================================================
// Controller command parameter
// ================================================================================
// PICC TYPE
enum picc_type {
	PICC_TYPE_A	= 0x00,
	PICC_TYPE_B	= 0x04
};

#define MAX_BUF_SIZE 128	// max frame length is 70 bytes

// ================================================================================
// Export function
// ================================================================================
int ic_reader_ctl_init(const char *path);

void ic_reader_ctl_close(void);
// Reader Controller Command
int ctl_set_picc_type(enum picc_type type);
int ctl_reset_dev(void);
// ISO14443-A PICC Command
int picc_A_request(void);
int picc_A_active(unsigned short *atq, unsigned char *sak,
		unsigned char *len, unsigned char *uid);
int picc_A_halt(void);

// ISO14443-B PICC Command
int picc_B_request(void);
int picc_B_active(unsigned char *info, unsigned char *len);
int picc_B_halt(void);
int picc_B_resident_ID_read(unsigned char *uid, unsigned char *len);


// ================================================================================
// Error code
// ================================================================================
// Serial port
#define ERR_UART_READ_ERROR			0x01
#define ERR_UART_TIME_OUT			0x02
#define ERR_UART_SERVER_CREATE		0x03
#define ERR_UART_UNKNOW_ERR			0x04
#define ERR_UART_SET_BAUD			0x05
#define ERR_UART_WRITE_FAILED		0x08
// Controller
#define ERR_CTL_INIT_FAILED			0x11
#define ERR_CTL_RESPONS_TIMEOUT		0x12
#define ERR_CTL_RESPONS_ERROR		0x13
#define ERR_CTL_CMD_FAILED			0x14
// Memory error
#define ERR_MEM_ALLOC_FAILED		0x20

#endif

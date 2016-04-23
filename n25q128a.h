#ifndef N25Q128A
#define N25Q128A

#include <linux/limits.h>
#include <sys/types.h>
#include "mpsse.h"
#include <string.h>

#define N25Q128A_MANUFACTURER_ID   0x20
#define N25Q128A_DEVICE_ID         0x18ba
#define N25Q128A_ID                0x9f
#define N25Q128A_READ              0x03
#define N25Q128A_READ_STATUS       0x05
#define N25Q128A_READ_FLAG_STATUS  0x70
#define N25Q128A_WRITE_ENABLE      0x06
#define N25Q128A_WRITE_DISABLE     0x04
#define N25Q128A_BULK_ERASE        0xc7
#define N25Q128A_PAGE_PROGRAM      0x02
#define N25Q128A_SECTOR_ERASE      0xd8

#define N25Q128A_STATUS_WRITING    0x01
#define N25Q128A_FLAG_ERASE_FAIL   0x06
#define N25Q128A_FLAG_PROGRAM_FAIL 0x05

#define N25Q128A_PAGES             65536
#define N25Q128A_BYTES_PER_PAGE    256
#define N25Q128A_TOTAL_BYTES       16777216
#define N25Q128A_SECTORS           256
#define N25Q128A_BYTES_PER_SECTOR  65536

struct n25q128a_id_data {
	unsigned char manufacturer_id;
	unsigned short device_id;
	unsigned char unique_id[17];
};

int n25q128a_id(struct mpsse_context *mpsse, struct n25q128a_id_data *id_data);
int n25q128a_read(struct mpsse_context *mpsse, char *data[],
		  unsigned int address, unsigned int length);
int n25q128a_read_status(struct mpsse_context *mpsse, char *status);
int n25q128a_read_flag_status(struct mpsse_context *mpsse, char *flag_status);
int n25q128a_write_enable(struct mpsse_context *mpsse);
int n25q128a_write_disable(struct mpsse_context *mpsse);
int n25q128a_bulk_erase(struct mpsse_context *mpsse);
int n25q128a_sector_erase(struct mpsse_context *mpsse, unsigned char sector);
int n25q128a_page_program(struct mpsse_context *mpsse, unsigned short page,
			  char *data, unsigned short length);

#endif

#include "n25q128a.h"

int n25q128a_id(struct mpsse_context *mpsse, struct n25q128a_id_data *id_data) {
	int ret;
	char command[1];

	command[0] = (char)N25Q128A_ID;

	Start(mpsse);

	ret = Write(mpsse, command, 1);
	if (ret != MPSSE_OK)
		goto done;

	char *data = Read(mpsse, 20);
	if (!data) {
		ret = MPSSE_FAIL;
		goto done;
	}

	id_data->manufacturer_id = data[0];
	id_data->device_id = (data[2] & 0xff) << 8 | (data[1] & 0xff) << 0;
	memcpy(&id_data->unique_id, &data[3], 17);
	free(data);

done:
	Stop(mpsse);
	return ret;
}

int n25q128a_read(struct mpsse_context *mpsse, char *data[],
		  unsigned int address, unsigned int length) {
	int ret;
	char command[4];

	command[0] = (char)N25Q128A_READ;
	command[1] = (address & 0x00ff0000) >> 16;
	command[2] = (address & 0x0000ff00) >>  8;
	command[3] = (address & 0x000000ff) >>  0;

	Start(mpsse);

	ret = Write(mpsse, command, 4);
	if (ret != MPSSE_OK)
		goto done;

	*data = Read(mpsse, length);
	if (!data)
		ret = MPSSE_FAIL;

done:
	Stop(mpsse);
	return ret;
}

int n25q128a_read_status(struct mpsse_context *mpsse, char *status) {
	int ret;
	char command[1];

	command[0] = (char)N25Q128A_READ_STATUS;

	Start(mpsse);

	ret = Write(mpsse, command, 1);
	if (ret != MPSSE_OK)
		goto done;

	char *s = Read(mpsse, 1);
	if (!s) {
		ret = MPSSE_FAIL;
		goto done;
	}

	*status = s[0];
	free(s);

done:
	Stop(mpsse);
	return ret;
}

int n25q128a_read_flag_status(struct mpsse_context *mpsse, char *flag_status) {
	int ret;
	char command[1];

	command[0] = (char)N25Q128A_READ_FLAG_STATUS;

	Start(mpsse);

	ret = Write(mpsse, command, 1);
	if (ret != MPSSE_OK)
		goto done;

	char *s = Read(mpsse, 1);
	if (!s) {
		ret = MPSSE_FAIL;
		goto done;
	}

	*flag_status = s[0];
	free(s);

done:
	Stop(mpsse);
	return ret;
}

int n25q128a_write_enable(struct mpsse_context *mpsse) {
	int ret;
	char command[1];

	command[0] = (char)N25Q128A_WRITE_ENABLE;

	Start(mpsse);
	ret = Write(mpsse, command, 1);
	Stop(mpsse);

	return ret;
}

int n25q128a_write_disable(struct mpsse_context *mpsse) {
	int ret;
	char command[1];

	command[0] = (char)N25Q128A_WRITE_DISABLE;

	Start(mpsse);
	ret = Write(mpsse, command, 1);
	Stop(mpsse);

	return ret;
}

int n25q128a_bulk_erase(struct mpsse_context *mpsse) {
	int ret;
	char command[1];
	char status;
	char flag_status;

	command[0] = (char)N25Q128A_BULK_ERASE;

	Start(mpsse);
	ret = Write(mpsse, command, 1);
	if (ret != MPSSE_OK)
		goto error;
	Stop(mpsse);

	do {
		ret = n25q128a_read_status(mpsse, &status);
		if (ret != MPSSE_OK)
			goto done;
	} while (status & N25Q128A_STATUS_WRITING);

	if (n25q128a_read_flag_status(mpsse, &flag_status) != MPSSE_OK)
		goto done;
	if (flag_status & N25Q128A_FLAG_ERASE_FAIL) {
		ret = MPSSE_FAIL;
		mpsse->ftdi.error_str = "flash bulk erase failed";
		goto done;
	}

error:
	Stop(mpsse);
done:
	return ret;
}

int n25q128a_sector_erase(struct mpsse_context *mpsse, unsigned char sector) {
	int ret;
	char command[4];
	char status;
	char flag_status;
	unsigned int sector_address = sector * N25Q128A_BYTES_PER_SECTOR;

	command[0] = (char)N25Q128A_SECTOR_ERASE;
	command[1] = (sector_address & 0x00ff0000) >> 16;
	command[2] = (sector_address & 0x0000ff00) >>  8;
	command[3] = (sector_address & 0x000000ff) >>  0;

	Start(mpsse);
	ret = Write(mpsse, command, 4);
	if (ret != MPSSE_OK)
		goto error;
	Stop(mpsse);

	do {
		ret = n25q128a_read_status(mpsse, &status);
		if (ret != MPSSE_OK)
			goto done;
	} while (status & N25Q128A_STATUS_WRITING);

	if (n25q128a_read_flag_status(mpsse, &flag_status) != MPSSE_OK)
		goto done;
	if (flag_status & N25Q128A_FLAG_ERASE_FAIL) {
		ret = MPSSE_FAIL;
		mpsse->ftdi.error_str = "flash sector erase failed";
		goto done;
	}

error:
	Stop(mpsse);
done:
	return ret;
}

int n25q128a_page_program(struct mpsse_context *mpsse, unsigned short page,
			  char *data, unsigned short length) {
	int ret;
	char command[4];
	char status;
	char flag_status;
	unsigned int page_address = page * N25Q128A_BYTES_PER_PAGE;

	command[0] = N25Q128A_PAGE_PROGRAM;
	command[1] = (page_address & 0x00ff0000) >> 16;
	command[2] = (page_address & 0x0000ff00) >>  8;
	command[3] = (page_address & 0x000000ff) >>  0;

	Start(mpsse);
	ret = Write(mpsse, command, 4);
	if (ret != MPSSE_OK)
		goto error;

	ret = Write(mpsse, data, length);
	if (ret != MPSSE_OK)
		goto error;
	Stop(mpsse);

	do {
		ret = n25q128a_read_status(mpsse, &status);
		if (ret != MPSSE_OK)
			goto done;
	} while (status & N25Q128A_STATUS_WRITING);

	if (n25q128a_read_flag_status(mpsse, &flag_status) != MPSSE_OK)
		goto done;
	if (flag_status & N25Q128A_FLAG_PROGRAM_FAIL) {
		ret = MPSSE_FAIL;
		mpsse->ftdi.error_str = "flash program failed";
		goto done;
	}

	goto done;

error:
	Stop(mpsse);
done:
	return ret;
}

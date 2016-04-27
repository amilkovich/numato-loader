#include <stdio.h>
#include "n25q128a.h"
#include <string.h>
#include <signal.h>

#define VID 0x0403
#define PID 0x6010
#define DESCRIPTION "Saturn Spartan 6 FPGA Module"

#define OPTIONS_SKIP_VALIDATION 0x10
#define OPTIONS_LIST_DEVICES    0x08
#define OPTIONS_ERASE_CHIP      0x04
#define OPTIONS_HELP            0x02
#define OPTIONS_VERSION         0x01

int erase_flash(struct mpsse_context *mpsse) {
	int ret;

	for (unsigned char sector = 0; sector < N25Q128A_SECTORS - 1;
	     sector++) {
		char *sector_data;
		ret = n25q128a_read(mpsse, &sector_data,
				    sector * N25Q128A_BYTES_PER_SECTOR,
				    N25Q128A_BYTES_PER_SECTOR);
		if (ret != MPSSE_OK)
			goto done;

		unsigned char matched = 1;
		for (unsigned short i = 0; i < N25Q128A_BYTES_PER_SECTOR - 1;
		     i++) {
			if (sector_data[i] != (char)0xff) {
				matched = 0;
				break;
			}
		}

		free(sector_data);

		if (!matched) {
			ret = n25q128a_write_enable(mpsse);
			if (ret != MPSSE_OK)
				goto done;
			ret = n25q128a_sector_erase(mpsse, sector);
			if (ret != MPSSE_OK)
				goto done_attempt_write_disable;
			ret = n25q128a_write_disable(mpsse);
			if (ret != MPSSE_OK)
				goto done;
		} else {
			goto done;
		}
	}

	goto done;

done_attempt_write_disable:
	n25q128a_write_disable(mpsse);
done:
	return ret;
}

int program_flash(struct mpsse_context *mpsse, char *data,
		  unsigned int length) {
	int ret;

	unsigned int full_pages = length / N25Q128A_BYTES_PER_PAGE;
	unsigned char partial_page = length % N25Q128A_BYTES_PER_PAGE;

	for (unsigned int page = 0; page < full_pages; page++) {
		ret = n25q128a_write_enable(mpsse);
		if (ret != MPSSE_OK)
			goto done_attempt_write_disable;
		ret = n25q128a_page_program(mpsse, page,
				&data[N25Q128A_BYTES_PER_PAGE * page],
				N25Q128A_BYTES_PER_PAGE);
		if (ret != MPSSE_OK)
			goto done_attempt_write_disable;
	}

	if (partial_page) {
		ret = n25q128a_write_enable(mpsse);
		if (ret != MPSSE_OK)
			goto done_attempt_write_disable;
		ret = n25q128a_page_program(mpsse, full_pages,
				&data[N25Q128A_BYTES_PER_PAGE * full_pages],
				partial_page);
		if (ret != MPSSE_OK)
			goto done_attempt_write_disable;
	}

	ret = n25q128a_write_disable(mpsse);

	goto done;

done_attempt_write_disable:
	n25q128a_write_disable(mpsse);
done:
	return ret;
}

int validate_flash(struct mpsse_context *mpsse, char *data, unsigned int length,
		   unsigned char *matched) {
	char *read_data;

	*matched = 0;

	n25q128a_read(mpsse, &read_data, 0, length);

	if (!read_data)
		return MPSSE_FAIL;

	if (memcmp(data, read_data, length) == 0)
		*matched = 1;

	free(read_data);

	return MPSSE_OK;
}

void list_devices() {
	struct ftdi_context *ftdi;
	struct ftdi_device_list *dev_list;
	struct ftdi_device_list *dev_current;
	int num_devices;

	ftdi = ftdi_new();
	if (!ftdi) {
		fprintf(stderr, "failed to initialize ftdi context\n");
		exit(1);
	}

	num_devices = ftdi_usb_find_all(ftdi, &dev_list, VID, PID);
	if (num_devices < 0) {
		fprintf(stderr, "ftdi error: %s\n",
			ftdi_get_error_string(ftdi));
		goto error;
	}

	if (!num_devices) {
		printf("unable to find saturn device!\n");
		goto done;
	}

	dev_current = dev_list;
	num_devices = 0;
	while (dev_current) {
		char manufacturer[255];
		char description[255];
		char serial[255];
		if (ftdi_usb_get_strings(ftdi, dev_current->dev,
					 manufacturer, 255,
					 description, 255,
					 serial, 255) < 0) {
			fprintf(stderr, "ftdi error: %s\n",
				ftdi_get_error_string(ftdi));
			goto done;
		}

		if (strcmp(DESCRIPTION, description) == 0)
			printf("%d: %s (%s, %s)\n", num_devices++, description,
			       manufacturer, serial);

		dev_current = dev_current->next;
	}

	if (!num_devices)
		printf("unable to find saturn device!\n");

done:
	ftdi_list_free(&dev_list);
error:
	ftdi_free(ftdi);
}

struct mpsse_context *initialize_mpsse() {
	struct mpsse_context *mpsse;
	struct n25q128a_id_data id_data;

	printf("initializing saturn..");
	fflush(stdout);

	mpsse = OpenIndex(VID, PID, SPI0, THIRTY_MHZ, MSB, IFACE_A,
			  DESCRIPTION, NULL, 1);
	if (mpsse && mpsse->open) {
		printf("failed!\n");
		fprintf(stderr, "error: too many saturn devices connected\n");
		goto error;
	} else if (mpsse) {
		Close(mpsse);
	}

	mpsse = OpenIndex(VID, PID, SPI0, THIRTY_MHZ, MSB, IFACE_A,
			  DESCRIPTION, NULL, 0);
	if (!mpsse || !mpsse->open) {
		printf("failed!\n");
		fprintf(stderr, "mpsse error: %s\n", ErrorString(mpsse));
		goto error;
	}

	if (n25q128a_id(mpsse, &id_data) != MPSSE_OK) {
		printf("failed!\n");
		fprintf(stderr, "mpsse error: %s\n", ErrorString(mpsse));
		fprintf(stderr, "error: flash id read failed\n");
		goto error;
	}

	if (id_data.manufacturer_id != N25Q128A_MANUFACTURER_ID ||
	    id_data.device_id != N25Q128A_DEVICE_ID) {
		printf("failed!\n");
		fprintf(stderr, "error: flash id mismatch\n");
		goto error;
	}

	printf("\n");

	return mpsse;

error:
	if (mpsse) Close(mpsse);
	return NULL;
}

int set_spartan_program_b(struct mpsse_context *mpsse, unsigned char value) {
	int ret;

	if (!value)
		ret = PinHigh(mpsse, GPIOL3);
	else
		ret = PinLow(mpsse, GPIOL3);

	return ret;
}

void erase_chip() {
	struct mpsse_context *mpsse = initialize_mpsse();

	if (!mpsse)
		exit(1);

	printf("erasing chip..");
	fflush(stdout);

	if (set_spartan_program_b(mpsse, 0) != MPSSE_OK)
		goto program_b_error;
	if (n25q128a_write_enable(mpsse) != MPSSE_OK)
		goto error;
	if (n25q128a_bulk_erase(mpsse) != MPSSE_OK)
		goto error;
	if (n25q128a_write_disable(mpsse) != MPSSE_OK)
		goto error;
	if (set_spartan_program_b(mpsse, 1) != MPSSE_OK)
		goto program_b_error;

	Close(mpsse);
	printf("success!\n");
	return;

error:
	printf("failed!\n");
	fprintf(stderr, "mpsse error: %s\n", ErrorString(mpsse));
	if (set_spartan_program_b(mpsse, 1) != MPSSE_OK)
		goto program_b_error_quiet;
	Close(mpsse);
	exit(1);
program_b_error:
	printf("failed!\n");
program_b_error_quiet:
	fprintf(stderr, "error: unable to change PROGRAM_B pin!\n");
	fprintf(stderr, "mpsse error: %s\n", ErrorString(mpsse));
	Close(mpsse);
	exit(1);
}

void flash(char *file, unsigned char skip_validation) {
	FILE *fp;
	long flash_length;
	char *flash_data;
	struct mpsse_context *mpsse;

	printf("reading input file..");
	fflush(stdout);

	fp = fopen(file, "r");
	if (!fp) {
		printf("failed!\n");
		fprintf(stderr, "error: could not open '%s'", file);
		exit(1);
	}

	fseek(fp, 0L, SEEK_END);
	flash_length = ftell(fp);

	if (!flash_length) {
		printf("failed!\n");
		fprintf(stderr, "error: file length was 0 bytes\n");
		fclose(fp);
		exit(1);
	}

	if (flash_length > N25Q128A_TOTAL_BYTES) {
		printf("failed!\n");
		fprintf(stderr, "error: file length exceeds flash capacity\n");
		fclose(fp);
		exit(1);
	}

	rewind(fp);

	flash_data = (char *)malloc(flash_length * sizeof(char));

	if (!flash_data) {
		printf("failed!\n");
		fprintf(stderr, "error: could not allocate memory");
		fclose(fp);
		exit(1);
	}

	if (fread(flash_data, 1, flash_length, fp) != flash_length) {
		printf("failed!\n");
		fprintf(stderr, "error: could not read entire file");
		free(flash_data);
		fclose(fp);
		exit(1);
	}

	fclose(fp);

	printf("\n");

	mpsse = initialize_mpsse();

	if (!mpsse)
		exit(1);

	printf("flashing chip..");
	fflush(stdout);

	if (set_spartan_program_b(mpsse, 0) != MPSSE_OK)
		goto program_b_error;
	if (erase_flash(mpsse) != MPSSE_OK)
		goto error;
	if (program_flash(mpsse, flash_data, flash_length) != MPSSE_OK)
		goto error;

	if (!skip_validation) {
		unsigned char matched;
		printf("\nvalidating..");
		fflush(stdout);
		if (validate_flash(mpsse, flash_data, flash_length,
			     &matched) != MPSSE_OK)
			goto error;
		if (!matched) {
			printf("failed!\n");
			fprintf(stderr, "error: flash mismatch\n");
			goto validation_error;
		}
	}

	printf("success!\n");
	if (set_spartan_program_b(mpsse, 1) != MPSSE_OK)
		goto program_b_error_quiet;
	free(flash_data);
	Close(mpsse);
	return;

error:
	printf("failed!\n");
	fprintf(stderr, "mpsse error: %s\n", ErrorString(mpsse));
validation_error:
	if (set_spartan_program_b(mpsse, 1) != MPSSE_OK)
		goto program_b_error_quiet;
	free(flash_data);
	Close(mpsse);
	exit(1);
program_b_error:
	printf("failed!\n");
program_b_error_quiet:
	fprintf(stderr, "error: unable to change PROGRAM_B pin!\n");
	fprintf(stderr, "mpsse error: %s\n", ErrorString(mpsse));
	free(flash_data);
	Close(mpsse);
	exit(1);
}

void usage(void) {
	fprintf(stderr, "saturn-loader\n"
			"usage: saturn-loader <file> [-s]\n"
			"       saturn-loader [-l | -e | -h | -v]\n"
			"options:\n"
			"-s     skip validation\n"
			"-l     list devices\n"
			"-e     erase chip\n"
			"-h     show this screen\n"
			"-v     show version\n");
	exit(1);
}

void sigint_handler(int s) {
	char msg[] = "\nplease wait..";
	signal(SIGINT, sigint_handler);
	write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(int argc, char *argv[]) {
	char *file_name = NULL;
	unsigned char options = 0x00;

	signal(SIGINT, sigint_handler);

	if (argc < 2)
		usage();

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (!file_name)
				file_name = argv[i];
			else
				usage();
		} else {
			if (options || strlen(argv[i]) > 2)
				usage();

			switch (argv[i][1]) {
				case 's':
					options |= OPTIONS_SKIP_VALIDATION;
				break;
				case 'l':
					options |= OPTIONS_LIST_DEVICES;
				break;
				case 'e':
					options |= OPTIONS_ERASE_CHIP;
				break;
				case 'h':
					options |= OPTIONS_HELP;
				break;
				case 'v':
					options |= OPTIONS_VERSION;
				break;
				default:
					usage();
				break;
			}
		}
	}

	if (!file_name && (options & OPTIONS_SKIP_VALIDATION))
		usage();
	if (file_name && (options & ~OPTIONS_SKIP_VALIDATION))
		usage();

	switch (options) {
		case OPTIONS_LIST_DEVICES:
			list_devices();
		break;
		case OPTIONS_ERASE_CHIP:
			erase_chip();
		break;
		case OPTIONS_HELP:
			usage();
		break;
		case OPTIONS_VERSION:
			printf("saturn-loader 0.5 beta, © 2016 Andrew Milkovich"
			       ", see LICENSE for details\n");
			exit(0);
		break;
		default:
			flash(file_name, options & OPTIONS_SKIP_VALIDATION);
		break;
	}

	return 0;
}

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include "wiringPi.h"
#include "computrol-mock.h"

#define FIFO "cm-fifo"

static bool awake = false;
static bool ignore_wake = false;
void wakeup(void);
void do_cmd(void);
void do_alarms(void);
unsigned int get_millis(void);

modbus_mapping_t *mb_mapping;
int fifo_fd;
volatile sig_atomic_t end_flag = 0;

void sig_end_handler(int sig)
{
	end_flag = 1;
}


int main(int argc, char*argv[])
{
	modbus_t *ctx;
	int rc;
	int i;
	uint8_t *query;
	int header_length;
	char *dev = NULL;
	int spd = 9600;
	char default_dev[] = "/dev/ttyUSB0";
	int c;

	//initialise Ctrl-C handler
	signal(SIGINT, sig_end_handler);

	//Initialise wiringPi
	if(wiringPiSetupGpio())
	{
		fprintf(stderr, "Failed to init wiringPi:");
		return -1;
	}
	pinMode(17,OUTPUT);
	pinMode(27,INPUT);
	wiringPiISR (27, INT_EDGE_BOTH, &wakeup);
	while((c = getopt(argc, argv, "D:B:i")) != -1)
	{
		switch (c)
		{
		case 'D':
			dev = optarg;
			break;
		case 'B':
			spd = atoi(optarg);
			break;
		case 'i':
			//ignore the awake signal, respond to all requests.
			fprintf(stderr, "Ignoring WAKEUP signal, responding to all requests.\r\n");
			ignore_wake = true;
			break;
		case '?':
			if (optopt == 'D')
			{
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			}
			else if (isprint (optopt))
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
			return 1;
		default:
			abort ();
		}
	}
	if(!dev)
		dev = default_dev;
	ctx = modbus_new_rtu(dev, spd, 'N', 8, 1);
	modbus_set_slave(ctx, SERVER_ID);
	query = malloc(MODBUS_RTU_MAX_ADU_LENGTH);
	header_length = modbus_get_header_length(ctx);

	modbus_set_debug(ctx, TRUE);

	mb_mapping = modbus_mapping_new_start_address(
			BITS_ADDRESS, BITS_NB,
			INPUT_BITS_ADDRESS, INPUT_BITS_NB,
			REGISTERS_ADDRESS, REGISTERS_NB_MAX,
			INPUT_REGISTERS_ADDRESS, INPUT_REGISTERS_NB);
	if (mb_mapping == NULL) {
		fprintf(stderr, "Failed to allocate the mapping: %s\n",
				modbus_strerror(errno));
		modbus_free(ctx);
		return -1;
	}

	/* Examples from PI_MODBUS_300.pdf.
       Only the read-only input values are assigned. */

	/* Initialize input values that's can be only done server side. */
	modbus_set_bits_from_bytes(mb_mapping->tab_input_bits, 0, INPUT_BITS_NB,
			INPUT_BITS_TAB);

	/* Initialize values of REGISTERS */
	for (i=0; i < REGISTERS_NB_MAX; i++) {
		mb_mapping->tab_registers[i] = REGISTERS_TAB[i];;
	}

	/* Initialize values of INPUT REGISTERS */
	for (i=0; i < INPUT_REGISTERS_NB; i++) {
		mb_mapping->tab_input_registers[i] = INPUT_REGISTERS_TAB[i];;
	}

	rc = modbus_connect(ctx);
	if (rc == -1) {
		fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
		modbus_free(ctx);
		return -1;
	}
	// Initialise the fifo file descriptor
	mknod(FIFO, S_IFIFO | 0666, 0);
	fifo_fd = open(FIFO, O_RDWR); // RDWR to avoid EOF return to select
	chmod(FIFO, 0666);

	for (;;)
	{
		do
		{
			if (end_flag)
			{
				rc=-1;
				break;
			}
			do_cmd();
			do_alarms();
			if(awake || ignore_wake) {
				rc = modbus_receive(ctx, query);
			}
			/* Filtered queries return 0 */
		} while (rc == 0 || rc == -2);

		/* The connection is not closed on errors which require on reply such as
       bad CRC in RTU. */
		if (rc == -1 && errno != EMBBADCRC)
		{
			/* Quit */
			break;
		}

		//printf("query[header_length] is %02x, query[header_length +2] is %02x\r\n", query[header_length], query[header_length + 2]);
		// emulate Alarm Clear
		if (query[header_length] == 0x10 && query[header_length +2] == 0x09)
		{
			printf("Got a write register command to 0x09\r\n");
			mb_mapping->tab_registers[0x03] = 0;
			printf("Resetting Register 3 to %i\r\n", mb_mapping->tab_registers[0x03]);
		}
		rc = modbus_reply(ctx, query, rc, mb_mapping);
		if (rc == -1)
		{
			break;
		}
	}

	printf("Quiting the loop: %s\n", modbus_strerror(errno));

	close(fifo_fd);
	unlink(FIFO);
	modbus_mapping_free(mb_mapping);
	free(query);
	/* For RTU */
	modbus_close(ctx);
	modbus_free(ctx);

	return 0;
}

void wakeup(void)
{
	awake = digitalRead(27);
	printf("Wakeup now %i\r\n", awake);
	delayMicroseconds(100);
	//printf("setting awake to match\r\n");
	digitalWrite(17,awake);
}

void set_alarm(int num)
{
	int tmpval = 0;
	tmpval = (num << 1);
	if((mb_mapping->tab_registers[7] && tmpval) != 0)
	{
		printf("Setting register 3 to %i\r\nWaking GT.\r\n", mb_mapping->tab_registers[3]);
		mb_mapping->tab_registers[3] |= (tmpval & 0x1f);
		digitalWrite(17, true);
	}
	return;
}

void reset_rbe(void)
{
	printf("Setting register 12 to 255\r\n");
	mb_mapping->tab_registers[12] = 0x00ff;
	return;
}



void do_cmd(void)
{
	int sval = 0;
	int rval = 0;
	char cmd;
	char cmd_buf[1024] = {0};
	char *data = cmd_buf;
	fd_set fds;
	int max_fd;
	unsigned int offset = 0;
	struct timeval tv = {0L, 0L};
	bool write = false;

	FD_ZERO(&fds);
	FD_SET(fileno(stdin), &fds);
	FD_SET(fifo_fd, &fds);

	max_fd = fileno(stdin) > fifo_fd ? fileno(stdin) : fifo_fd;

	if(select(max_fd + 1,&fds, NULL, NULL, &tv) > 0)
	{
		if(FD_ISSET(fifo_fd, &fds))
		{
			read(fifo_fd, &cmd_buf,sizeof(cmd_buf));
		}
		if(FD_ISSET(fileno(stdin), &fds))
		{
			read(1, &cmd_buf,sizeof(cmd_buf));
		}
		FD_SET(fileno(stdin), &fds);
		FD_SET(fifo_fd, &fds);
		//while (sscanf(data, "%[RWVIT] %i %i\n%n", &cmd, &rval, &sval, &offset) > 0)
		while (data[0] != 0 && data[0] != '\n')
		{
			switch (data[0])
			{
			case 'V':
				sscanf(data, "V %i%n", &sval, &offset);
				rval = 101;
				write = true;
				break;
			case 'I':
				sscanf(data, "I %i%n", &sval, &offset);
				rval = 100;
				write = true;
				break;
			case 'T':
				sscanf(data, "T %i%n", &sval, &offset);
				rval = 84;
				write = true;
				break;
			case 'W':
				sscanf(data, "W %i %i%n", &rval, &sval, &offset);
				write = true;
				break;
			case 'R':
				sscanf(data, "R %i%n", &rval, &offset);
				break;
			}
			if( write )
			{
				mb_mapping->tab_registers[rval] = sval;
				write = false;
			}
			printf("Reg %i = %i\r\n", rval, mb_mapping->tab_registers[rval]);
			data += offset+1;
			cmd = 0;
			rval = 0;
			sval = 0;
		}
	}
}

void do_alarms(void)
{
	static unsigned int level1time = 0;
	static unsigned int level2time = 0;
	static unsigned int level3time = 0;
	static unsigned int level4time = 0;
	static unsigned int lowvolttime = 0;
	static unsigned int normvolttime = 0;
	unsigned int thisms;
	static int last_r3;

	thisms = get_millis();
	// check current levels
	// Level 1
	if( mb_mapping-> tab_registers[20 + mb_mapping->tab_registers[7]] > 0 &&
			mb_mapping->tab_registers[100] > mb_mapping->tab_registers[20 + mb_mapping->tab_registers[7]] )
	{
		level1time += thisms;
		if (level1time > mb_mapping->tab_registers[60 + mb_mapping->tab_registers[7]] &&
				(mb_mapping->tab_registers[3] & 0x0001) == 0) {
			mb_mapping->tab_registers[3] |= 0x0001;
		}
	}
	else if(mb_mapping-> tab_registers[20 + mb_mapping->tab_registers[7]] > 0 &&
			mb_mapping->tab_registers[100] <= mb_mapping->tab_registers[20 + mb_mapping->tab_registers[7]])
	{
		mb_mapping->tab_registers[3] &= 0xfffe;
		level1time = 0;
	}
	// Level 2
	if( mb_mapping-> tab_registers[21 + mb_mapping->tab_registers[7]] > 0 &&
			mb_mapping->tab_registers[100] > mb_mapping->tab_registers[21 + mb_mapping->tab_registers[7]] )
	{
		level2time += thisms;
		if (level2time > mb_mapping->tab_registers[61 + mb_mapping->tab_registers[7]] &&
				(mb_mapping->tab_registers[3] & 0x0002) == 0) {
			mb_mapping->tab_registers[3] |= 0x0002;
		}
	}
	else if( mb_mapping-> tab_registers[21 + mb_mapping->tab_registers[7]] > 0 &&
			mb_mapping->tab_registers[100] <= mb_mapping->tab_registers[21 + mb_mapping->tab_registers[7]] )
	{
		mb_mapping->tab_registers[3] &= 0xfffd;
		level2time = 0;
	}
	// Level 3
	if( mb_mapping-> tab_registers[22 + mb_mapping->tab_registers[7]] > 0 &&
			mb_mapping->tab_registers[100] > mb_mapping->tab_registers[22 + mb_mapping->tab_registers[7]] )
	{
		level3time += thisms;
		if (level3time > mb_mapping->tab_registers[60 + mb_mapping->tab_registers[7]] &&
				(mb_mapping->tab_registers[3] & 0x0004) == 0) {
			mb_mapping->tab_registers[3] |= 0x0004;
		}
	}
	else if( mb_mapping-> tab_registers[22 + mb_mapping->tab_registers[7]] > 0 &&
			mb_mapping->tab_registers[100] <= mb_mapping->tab_registers[22 + mb_mapping->tab_registers[7]] )
	{
		mb_mapping->tab_registers[3] &= 0xfffb;
		level3time = 0;
	}
	// Level 4
	if( mb_mapping-> tab_registers[23 + mb_mapping->tab_registers[7]] > 0 &&
			mb_mapping->tab_registers[100] > mb_mapping->tab_registers[23 + mb_mapping->tab_registers[7]] )
	{
		level4time += thisms;
		if (level4time > mb_mapping->tab_registers[60 + mb_mapping->tab_registers[7]] &&
				(mb_mapping->tab_registers[3] & 0x0008) == 0) {
			mb_mapping->tab_registers[3] |= 0x0008;
		}
	}
	else if( mb_mapping-> tab_registers[23 + mb_mapping->tab_registers[7]] > 0 &&
			mb_mapping->tab_registers[100] <= mb_mapping->tab_registers[23 + mb_mapping->tab_registers[7]] )
	{
		mb_mapping->tab_registers[3] &= 0xfff7;
		level4time = 0;
	}
	// Voltage limits
	// Low Voltage Limit
	if( mb_mapping-> tab_registers[90] > 0 &&
			mb_mapping->tab_registers[101] < mb_mapping->tab_registers[90] )
	{
		lowvolttime += thisms;
		if (lowvolttime > mb_mapping->tab_registers[92] &&
				(mb_mapping->tab_registers[3] & 0x0010) == 0)
		{
			//clear normal voltage bit
			mb_mapping->tab_registers[3] &= 0xffcf;
			//set low voltage bit
			mb_mapping->tab_registers[3] |= 0x0010;
			normvolttime = 0;
		}
	}
	//Normal Voltage Limit
	if( mb_mapping-> tab_registers[91] > 0 &&
			mb_mapping->tab_registers[101] >= mb_mapping->tab_registers[91] )
	{
		normvolttime += thisms;
		if (normvolttime > mb_mapping->tab_registers[93] &&
				(mb_mapping->tab_registers[3] & 0x0020) == 0)
		{
			//clear low voltage bit
			mb_mapping->tab_registers[3] &= 0xffef;
			//set normal voltage bit
			mb_mapping->tab_registers[3] |= 0x0020;
			lowvolttime = 0;
		}
	}
	if (mb_mapping->tab_registers[3] != last_r3)
	{
		digitalWrite(17, true);
	}
	last_r3 = mb_mapping->tab_registers[3];
}

unsigned int get_millis(void)
{
	static struct timeval last_time;
	struct timeval this_time;
	unsigned int ms = 0;
	if (!last_time.tv_sec)
	{
		gettimeofday(&last_time, NULL);
		return 0;
	}
	gettimeofday(&this_time, NULL);
	ms = (this_time.tv_sec - last_time.tv_sec) * 1000;
	if (this_time.tv_usec > last_time.tv_usec)
	{
		ms += (this_time.tv_usec - last_time.tv_usec) / 1000;
	}
	else
	{
		ms += ((this_time.tv_usec +1000) - last_time.tv_usec) / 1000;
	}
	if (ms > 0)
	{
		last_time = this_time;
	}
	return ms;
}

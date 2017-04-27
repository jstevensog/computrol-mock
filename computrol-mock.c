#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include "wiringPi.h"
#include "computrol-mock.h"

static bool awake = false;
void wakeup(void);
void set_alarm(int num);
void reset_rbe(void);
void do_cmd(void);
modbus_mapping_t *mb_mapping;

//signal handler
void sig_handler(int signo)
{
  if (signo == SIGRTMIN+1)
  {
    printf("Received Current Alarm 1 signal. \r\n");
    set_alarm(0x01);
  }
  //if (signo == SIGUSR2)
  if (signo == SIGRTMIN+2)
  {
    printf("Received Current Alarm 2 signal. \r\n");
    set_alarm(0x02);
  }
  if (signo == SIGRTMIN+3)
  {
    printf("Received Current Alarm 3 signal. \r\n");
    set_alarm(0x03);
  }
  if (signo == SIGRTMIN+4)
  {
    printf("Received Voltage Low Alarm signal. \r\n");
    set_alarm(0x04);
  }
  if (signo == SIGRTMIN+5)
  {
    printf("Received Voltage Normal Alarm signal. \r\n");
    set_alarm(0x08);
  }
  if (signo == SIGRTMIN+6)
  {
    printf("Received reset LFI RBE address. \r\n");
    reset_rbe();
  }
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

  //Initialise wiringPi
  if(wiringPiSetupGpio())
  {
    fprintf(stderr, "Failed to init wiringPi:");
    return -1;
  }
  pinMode(17,OUTPUT);
  pinMode(27,INPUT);
  wiringPiISR (27, INT_EDGE_BOTH, &wakeup);
  while((c = getopt(argc, argv, "D:B:")) != -1)
  {
    switch (c)
    {
      case 'D':
        dev = optarg;
        break;
      case 'B':
        spd = atoi(optarg);
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
  //set up signal handler
  if (signal(SIGRTMIN+1, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+1\n");
  if (signal(SIGRTMIN+2, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+2\n");
  if (signal(SIGRTMIN+3, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+3\n");
  if (signal(SIGRTMIN+4, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+4\n");
  if (signal(SIGRTMIN+5, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+5\n");
  if (signal(SIGRTMIN+6, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+6\n");


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

  for (;;) 
  {
    do 
    {
      do_cmd();
      rc = modbus_receive(ctx, query);
      /* Filtered queries return 0 */
    } while (( rc == -2 || rc == 0 )  && !awake);

    /* The connection is not closed on errors which require on reply such as
       bad CRC in RTU. */
    if (rc == -1 && errno != EMBBADCRC) 
    {
        /* Quit */
        break;
    }

    printf("query[header_length] is %02x, query[header_length +2] is %02x\r\n", query[header_length], query[header_length + 2]);
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

  printf("Quit the loop: %s\n", modbus_strerror(errno));

  modbus_mapping_free(mb_mapping);
  free(query);
  /* For RTU */
  modbus_close(ctx);
  modbus_free(ctx);

  return 0;
}

void wakeup(void)
{
  delayMicroseconds(100);
  awake = digitalRead(27);
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

int get_cmd(void)
{
  fd_set fds;
  int r = 0;
  struct timeval tv = {0L, 0L};
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  r = select(1, &fds, NULL, NULL, &tv);
  return r;
}

void do_cmd(void)
{
  int sval = 0;
  int rval = 0;
  char cmd;
  char cmd_buf[128] = {0};
  if(get_cmd())
  {
    read(1, &cmd_buf,sizeof(cmd_buf)); 
    sscanf(cmd_buf, "%[RW] %i %i", &cmd, &rval, &sval);
    if( cmd == 'W' )
    {
      mb_mapping->tab_registers[rval] = sval;
    }
    printf("Reg %i = %i\r\n", rval, mb_mapping->tab_registers[rval]);
  }
}

void do_alarms(void)
{
	static int level1time = 0;
	static int level2time = 0;
	static int level3time = 0;
	static int level4time = 0;
	static int lowvolttime = 0;
	static int normvolttime = 0;
	static int lastms;

	if(!lastms) {
		lastms = millis();
	}
	// check current levels
	// Level 1
	if( mb_mapping-> tab_registers[20 + mb_mapping->tab_registers[7]] > 0 &&
		mb_mapping->tab_registers[100] > mb_mapping->tab_registers[20 + mb_mapping->tab_registers[7]] )
	{
		level1time += millis() - lastms;
		if (level1time > mb_mapping->tab_registers[60 + mb_mapping->tab_registers[7]] &&
			(mb_mapping->tab_registers[3] & 0x0008) == 0) {
			mb_mapping->tab_registers[3] |= 0x0008;
		}
		else
		{
			mb_mapping->tab_registers[3] &= 0xfff7;
			level1time = 0;
		}
	}
	// Level 2
	if( mb_mapping-> tab_registers[21 + mb_mapping->tab_registers[7]] > 0 &&
		mb_mapping->tab_registers[100] > mb_mapping->tab_registers[21 + mb_mapping->tab_registers[7]] )
	{
		level2time += millis() - lastms;
		if (level2time > mb_mapping->tab_registers[61 + mb_mapping->tab_registers[7]] &&
			(mb_mapping->tab_registers[3] & 0x0004) == 0) {
			mb_mapping->tab_registers[3] |= 0x0004;
		}
		else
		{
			mb_mapping->tab_registers[3] &= 0xfffb;
			level2time = 0;
		}
	}
	// Level 3
	if( mb_mapping-> tab_registers[22 + mb_mapping->tab_registers[7]] > 0 &&
		mb_mapping->tab_registers[100] > mb_mapping->tab_registers[22 + mb_mapping->tab_registers[7]] )
	{
		level3time += millis() - lastms;
		if (level3time > mb_mapping->tab_registers[60 + mb_mapping->tab_registers[7]] &&
			(mb_mapping->tab_registers[3] & 0x000c) == 0) {
			mb_mapping->tab_registers[3] |= 0x000c;
		}
		else
		{
			mb_mapping->tab_registers[3] &= 0xfff9;
			level3time = 0;
		}
	}
	// Level 4
	if( mb_mapping-> tab_registers[23 + mb_mapping->tab_registers[7]] > 0 &&
		mb_mapping->tab_registers[100] > mb_mapping->tab_registers[23 + mb_mapping->tab_registers[7]] )
	{
		level4time += millis() - lastms;
		if (level4time > mb_mapping->tab_registers[60 + mb_mapping->tab_registers[7]] &&
			(mb_mapping->tab_registers[3] & 0x0002) == 0) {
			mb_mapping->tab_registers[3] |= 0x0002;
		}
		else
		{
			mb_mapping->tab_registers[3] &= 0xfffd;
			level4time = 0;
		}
	}
	// Voltage limits
	// Low Voltage Limit
	if( mb_mapping-> tab_registers[90] > 0 &&
			mb_mapping->tab_registers[101] < mb_mapping->tab_registers[90] )
	{
		lowvolttime += millis() - lastms;
		if (lowvolttime > mb_mapping->tab_registers[92] &&
			(mb_mapping->tab_registers[3] & 0x0010) > 0) {
			mb_mapping->tab_registers[3] |= 0x0010;
		}
		else
		{
			mb_mapping->tab_registers[3] &= 0xffef;
			lowvolttime = 0;
		}
	}
	//Normal Voltage Limit
	if( mb_mapping-> tab_registers[91] > 0 &&
			mb_mapping->tab_registers[101] < mb_mapping->tab_registers[91] )
	{
		normvolttime += millis() - lastms;
		if (normvolttime > mb_mapping->tab_registers[93] &&
			(mb_mapping->tab_registers[3] & 0x0020) > 0) {
			mb_mapping->tab_registers[3] |= 0x0020;
		}
		else
		{
			mb_mapping->tab_registers[3] &= 0xffcf;
			normvolttime = 0;
		}
	}

}

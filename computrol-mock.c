#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <sys/socket.h>
#include <signal.h>
#include "wiringPi.h"
#include "computrol-mock.h"

static bool awake = false;
void wakeup(void);
void set_alarm(int num);
modbus_mapping_t *mb_mapping;

//signal handler
void sig_handler(int signo)
{
	//if (signo == SIGUSR1)
	if (signo == SIGRTMIN+1)
	{
		printf("Setting Current Alarm 1, and waking GT. \r\n");
		set_alarm(0x01);
	}
	//if (signo == SIGUSR2)
	if (signo == SIGRTMIN+2)
	{
		printf("Setting Current Alarm 2, and waking GT. \r\n");
		set_alarm(0x02);
	}
	if (signo == SIGRTMIN+3)
	{
		printf("Setting Current Alarm 3, and waking GT. \r\n");
		set_alarm(0x03);
	}
	if (signo == SIGRTMIN+4)
	{
		printf("Setting Current Alarm 4, and waking GT. \r\n");
		set_alarm(0x04);
	}
	if (signo == SIGRTMIN+5)
	{
		printf("Setting Voltage Low Alarm, and waking GT. \r\n");
		set_alarm(0x08);
	}
	digitalWrite(17, true);
}
int main(int argc, char*argv[])
{
    modbus_t *ctx;
    int rc;
    int i;
    uint8_t *query;
    int header_length;

	//Initialise wiringPi
	if(wiringPiSetupGpio())
	{
		fprintf(stderr, "Failed to init wiringPi:");
		return -1;
	}
	pinMode(17,OUTPUT);
	pinMode(27,INPUT);
	wiringPiISR (27, INT_EDGE_BOTH, &wakeup);
    ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);
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
/*	if (signal(SIGUSR1, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGUSR1\n");
	if (signal(SIGUSR2, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGUSR2\n");
*/	if (signal(SIGRTMIN+1, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+1\n");
	if (signal(SIGRTMIN+2, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+2\n");
	if (signal(SIGRTMIN+3, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+3\n");
	if (signal(SIGRTMIN+4, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+4\n");
	if (signal(SIGRTMIN+5, sig_handler) == SIG_ERR)
        printf("\ncan't catch SIGRTMIN+5\n");


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
            rc = modbus_receive(ctx, query);
            /* Filtered queries return 0 */
        } while (rc == 0 && !awake);

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
    	
    	for (i=13; i < REGISTERS_NB_MAX; i++) {
        	mb_mapping->tab_registers[i]++ ;
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
	mb_mapping->tab_registers[3] |= (tmpval & 0x1f);
	printf("Setting register 3 to %i\r\n", mb_mapping->tab_registers[3]);
	return;
}

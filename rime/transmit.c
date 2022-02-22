/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "board-peripherals.h"
#include <stdio.h>
#include "net/rime/rime.h"
#include <stdio.h>
#include <math.h>
/*---------------------------------------------------------------------------*/


static char message[50];
static void send(char message[], int size);

PROCESS(transmit_process, "unicasting...");
AUTOSTART_PROCESSES(&transmit_process);

static const struct unicast_callbacks unicast_callbacks = {};
static struct unicast_conn uc;
static int length = 0;
static char message[50];
static int i;

static void send(char message[], int size){
	linkaddr_t addr;
	//printf("%s\n",message);
   packetbuf_copyfrom(message, strlen(message));

   // COMPUTE THE ADDRESS OF THE RECEIVER FROM ITS NODE ID, FOR EXAMPLE NODEID 0xBA04 MAPS TO 0xBA AND 0x04 RESPECTIVELY
   // In decimal, if node ID is 47620, this maps to 186 (higher byte) AND 4 (lower byte)
   addr.u8[0] = 46; // HIGH BYTE or 186 in decimal
   addr.u8[1] = 2; // LOW BYTE or 4 in decimal
   if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
         unicast_send(&uc, &addr);
   }

}

PROCESS_THREAD(transmit_process, ev, data) {

	PROCESS_EXITHANDLER(unicast_close(&uc);)
	PROCESS_BEGIN();
	unicast_open(&uc, 146, &unicast_callbacks);
   static struct etimer et;

   for(i = 0;i < 160;i++){
      /* Delay 0.25 seconds */
      etimer_set(&et, CLOCK_SECOND * 0.0625);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      
      sprintf(message, "%d",i+1);
      length = (int)((ceil(log10(i+1))+1));
      send(message, length);
   }


   PROCESS_END();
}

/*---------------------------------------------------------------------------*/

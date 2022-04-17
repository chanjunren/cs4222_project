#include "contiki.h"
#include "dev/leds.h"
#include <stdio.h>
#include "core/net/rime/rime.h"
#include "dev/serial-line.h"
#include "dev/uart1.h"
#include "node-id.h"
#include "defs_and_types.h"
#include "net/netstack.h"
#include "random.h"
#ifdef TMOTE_SKY
#include "powertrace.h"
#endif
/*---------------------------------------------------------------------------*/
#define WAKE_TIME RTIMER_SECOND/10    // 10 HZ, 0.1s
/*---------------------------------------------------------------------------*/
#define SLEEP_CYCLE  9        	      // 0 for never sleep
#define SLEEP_SLOT RTIMER_SECOND/10   // sleep slot should not be too large to prevent overflow
/*---------------------------------------------------------------------------*/
// duty cycle = WAKE_TIME / (WAKE_TIME + SLEEP_SLOT * SLEEP_CYCLE)
/*---------------------------------------------------------------------------*/
// sender timer
static struct rtimer rt;
static struct pt pt;
/*---------------------------------------------------------------------------*/
static data_packet_struct received_packet;
static data_packet_struct data_packet;
unsigned long curr_timestamp;
/*---------------------------------------------------------------------------*/
PROCESS(cc2650_nbr_discovery_process, "cc2650 neighbour discovery process");
AUTOSTART_PROCESSES(&cc2650_nbr_discovery_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  leds_on(LEDS_GREEN);
  memcpy(&received_packet, packetbuf_dataptr(), sizeof(data_packet_struct));

  printf("Send seq# %lu  @ %8lu  %3lu.%03lu\n", data_packet.seq, curr_timestamp, curr_timestamp / CLOCK_SECOND, ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

  printf("Received packet from node %lu with sequence number %lu and timestamp %3lu.%03lu\n", received_packet.src_id, received_packet.seq, received_packet.timestamp / CLOCK_SECOND, ((received_packet.timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);
  leds_off(LEDS_GREEN);
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
char sender_scheduler(struct rtimer *t, void *ptr) {
  static uint16_t i = 0;
  static int NumSleep=0;
  PT_BEGIN(&pt);

  curr_timestamp = clock_time(); 
  printf("Start clock %lu ticks, timestamp %3lu.%03lu\n", curr_timestamp, curr_timestamp / CLOCK_SECOND, ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

  while(1){

    // radio on
    NETSTACK_RADIO.on();

    for(i = 0; i < NUM_SEND; i++){
      leds_on(LEDS_RED);
      
      data_packet.seq++;
      curr_timestamp = clock_time();
      data_packet.timestamp = curr_timestamp;

      printf("Send seq# %lu  @ %8lu ticks   %3lu.%03lu\n", data_packet.seq, curr_timestamp, curr_timestamp / CLOCK_SECOND, ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

      packetbuf_copyfrom(&data_packet, (int)sizeof(data_packet_struct));
      broadcast_send(&broadcast);
      leds_off(LEDS_RED);

      if(i != (NUM_SEND - 1)){
        rtimer_set(t, RTIMER_TIME(t) + WAKE_TIME, 1, (rtimer_callback_t)sender_scheduler, ptr);
        PT_YIELD(&pt);
      }
    }

    if(SLEEP_CYCLE != 0){
      leds_on(LEDS_BLUE);
      // radio off
      NETSTACK_RADIO.off();

      // SLEEP_SLOT cannot be too large as value will overflow,
      // to have a large sleep interval, sleep many times instead

      // get a value that is uniformly distributed between 0 and 2*SLEEP_CYCLE
      // the average is SLEEP_CYCLE 
      NumSleep = random_rand() % (2 * SLEEP_CYCLE + 1);  
      printf(" Sleep for %d slots \n",NumSleep);

      // NumSleep should be a constant or static int
      for(i = 0; i < NumSleep; i++){
        rtimer_set(t, RTIMER_TIME(t) + SLEEP_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
        PT_YIELD(&pt);
      }
      leds_off(LEDS_BLUE);
    }
  }
  
  PT_END(&pt);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc2650_nbr_discovery_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  random_init(54222);

  #ifdef TMOTE_SKY
  powertrace_start(CLOCK_SECOND * 5);
  #endif

  broadcast_open(&broadcast, 129, &broadcast_call);

  // for serial port
  #if !WITH_UIP && !WITH_UIP6
  uart1_set_input(serial_line_input_byte);
  serial_line_init();
  #endif

  printf("CC2650 neighbour discovery\n");
  printf("Node %d will be sending packet of size %d Bytes\n", node_id, (int)sizeof(data_packet_struct));

  // radio off
  NETSTACK_RADIO.off();

  // initialize data packet
  data_packet.src_id = node_id;
  data_packet.seq = 0;

  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

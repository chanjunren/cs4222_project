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
// duty cycle = (2 * N_SIZE - 1) / (N_SIZE * N_SIZE)
/*---------------------------------------------------------------------------*/
#define TIME_SLOT RTIMER_SECOND/10
#define N_SIZE 13

static int row;
static int col;
static int currRow = 0;
static int currCol = 0;
/*------------------------------------0--------------------------------------*/
// sender timer
static struct rtimer rt;
static struct pt pt;
/*---------------------------------------------------------------------------*/
static data_packet_struct received_packet;
static data_packet_struct data_packet;
unsigned long curr_timestamp;
/*----------------------------NODE MANAGEMENT--------------------------------*/
device_node head;
MEMB(nodes, struct device_info, sizeof(struct device_info));

#define ABSENT_LIMIT 10
#define MIN_CONTACT 15
#define RSSI_THRESHOLD 58

void add_node(int id, unsigned long timestamp, signed short rssi)
{
  device_node new_node;
  new_node = memb_alloc(&nodes);
  new_node->id = id;
  new_node->timestamp = timestamp;
  new_node->is_printed = false;
  if (rssi < RSSI_THRESHOLD)
  {
    new_node->is_detect = true;
  }
  else
  {
    new_node->is_detect = false;
  }
  if (head == NULL)
  {
    head = new_node;
    return;
  }
  device_node ptr = head;
  while (ptr->next != NULL)
    ptr = ptr->next;
  ptr->next = new_node;
}

void remove_node(device_node prev, device_node to_remove)
{
  // removed node is head
  if (to_remove == head)
  {
    head = head->next;
    memb_free(&nodes, to_remove);
    return;
  }

  // removed node is tail
  if (to_remove->next == NULL)
  {
    prev->next = NULL;
    memb_free(&nodes, to_remove);
    return;
  }
  // node to remove is in the middle of the list
  prev->next = to_remove->next;
  memb_free(&nodes, to_remove);
}

void process_node(int id, unsigned long curr_timestamp, signed short rssi)
{
  if (head == NULL)
  {
    // First node detected
    return add_node(id, curr_timestamp, rssi);
  }
  printf("CURR RSSI Value : %d\n", rssi);
  device_node prev = NULL, ptr = head;
  while (ptr != NULL)
  {
    // Updating last timestamp if node is currently connected
    if (ptr->id == id)
    {
      if (ptr->is_detect && rssi < RSSI_THRESHOLD)
      {
        // printf("curr_timestamp : %ld , ptr->timestamp : %ld\n", curr_timestamp, ptr->timestamp);
        if ((curr_timestamp - ptr->timestamp) > MIN_CONTACT && !ptr->is_printed)
        {
          printf("%ld DETECT %d\n", ptr->timestamp, ptr->id);
          ptr->is_printed = true;
        }
        ptr->timestamp = curr_timestamp;
      }
      else if (!ptr->is_detect && rssi > RSSI_THRESHOLD)
      {
        if ((curr_timestamp - ptr->timestamp) > ABSENT_LIMIT && ptr->is_printed)
        {
          printf("%ld ABSENT %d\n", ptr->timestamp, ptr->id);
          remove_node(prev, ptr);
        }
      }
      else if (!ptr->is_detect && rssi < RSSI_THRESHOLD)
      {
        ptr->is_detect = true;
        ptr->timestamp = curr_timestamp;
      }
      else if (ptr->is_detect && rssi > RSSI_THRESHOLD)
      {
        ptr->is_detect = false;
        ptr->timestamp = curr_timestamp;
      }
      return;
    }
    prev = ptr;
    ptr = ptr->next;
  }
  // Node is detected for the first time
  return add_node(id, curr_timestamp, rssi);
}

void check_for_absence(unsigned long curr_timestamp)
{
  device_node ptr = head, prev = NULL;
  while (ptr != NULL)
  {
    if (ptr->is_printed && (curr_timestamp - ptr->timestamp > ABSENT_LIMIT))
    {
      printf("%ld ABSENT %d\n", ptr->timestamp, ptr->id);
      remove_node(prev, ptr);
    }
    prev = ptr;
    ptr = ptr->next;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS(cc2650_nbr_discovery_process, "cc2650 neighbour discovery process");
AUTOSTART_PROCESSES(&cc2650_nbr_discovery_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  leds_on(LEDS_GREEN);
  memcpy(&received_packet, packetbuf_dataptr(), sizeof(data_packet_struct));

  // printf("Send seq# %lu  @ %8lu  %3lu.%03lu\n",
  //   data_packet.seq,
  //   curr_timestamp,
  //   curr_timestamp / CLOCK_SECOND,
  //   ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

  // printf("Received packet from node %lu with sequence number %lu and timestamp %3lu.%03lu\n",
  //   received_packet.src_id,
  //   received_packet.seq,
  //   received_packet.timestamp / CLOCK_SECOND,
  //   ((received_packet.timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);
  process_node(received_packet.src_id, curr_timestamp / CLOCK_SECOND,
               (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI) * -1);
  leds_off(LEDS_GREEN);
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
char sender_scheduler(struct rtimer *t, void *ptr) {
  static uint16_t i = 0;
  PT_BEGIN(&pt);

  curr_timestamp = clock_time();
  printf("Start clock %lu ticks, timestamp %3lu.%03lu\n",
    curr_timestamp,
    curr_timestamp / CLOCK_SECOND,
    ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

  while(1) {
    check_for_absence(curr_timestamp / CLOCK_SECOND);
    // radio on
    NETSTACK_RADIO.on();
    if (currRow == row || currCol == col) {
      for(i = 0; i < NUM_SEND; i++) {
        leds_on(LEDS_RED);
        
        data_packet.seq++;
        curr_timestamp = clock_time();
        data_packet.timestamp = curr_timestamp;

        packetbuf_copyfrom(&data_packet, (int)sizeof(data_packet_struct));
        broadcast_send(&broadcast);
        leds_off(LEDS_RED);

        if(i != (NUM_SEND - 1)) {
          rtimer_set(
            t,
            RTIMER_TIME(t) + TIME_SLOT,
            1,
            (rtimer_callback_t)sender_scheduler,
            ptr);
          PT_YIELD(&pt);
        }
      }
    }
    else {
      leds_on(LEDS_BLUE);
      // radio off
      NETSTACK_RADIO.off();
      rtimer_set(t, RTIMER_TIME(t) + TIME_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
      PT_YIELD(&pt);
      leds_off(LEDS_BLUE);
    }
      
    currCol = (currCol + 1) % N_SIZE;
    if (currCol == 0) currRow = (currRow + 1) % N_SIZE;
  }
  
  PT_END(&pt);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc2650_nbr_discovery_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  random_init(54222);

  row = random_rand() % N_SIZE;
  col = random_rand() % N_SIZE;

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
  // printf("Node %d will be sending packet of size %d Bytes\n",
    // node_id, (int)sizeof(data_packet_struct));
  // printf("N_SIZE: %d row: %d col: %d\n", N_SIZE, row, col);
  // radio off
  NETSTACK_RADIO.off();

  // initialize data packet
  data_packet.src_id = node_id;
  data_packet.seq = 0;

  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);

  PROCESS_END();
}
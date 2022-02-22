#include "contiki.h"
#include "net/rime/rime.h"
#include <stdio.h>
#include "net/netstack.h"

static int counter_etimer;
static int recv_count = 0;
static signed short total_rssi;
static signed short avg_rssi;

/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "unicast receiver");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/
static void recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  clock_time_t t;
  t = clock_time();
  int s = t / CLOCK_SECOND;

  recv_count++;
  total_rssi = total_rssi + (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI);
  avg_rssi = total_rssi / recv_count;
  counter_etimer++;
  char message[50];
  strcpy(message, (char *)packetbuf_dataptr());
  message[packetbuf_datalen()] = '\0';
  printf("t: %i, recv_count: %i | RSSI: %d | AVG_RSSI %d\n", s, recv_count, (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI), avg_rssi);
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)

  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);

  while (1)
  {
    static struct etimer et;
    etimer_set(&et, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

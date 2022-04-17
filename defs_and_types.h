/*	Author: ebramkw
	Typedef and definitions	*/

/*---------------------------------------------------------------------------*/
#define NUM_SEND 2
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
typedef struct {
  unsigned long src_id;
  unsigned long timestamp;
  unsigned long seq;
} data_packet_struct;
/*---------------------------------------------------------------------------*/

struct device_info {
    int id;
    unsigned long timestamp;
    unsigned long last_pkt_recv_timestamp;
    int rssi_1, rssi_2, rssi_3;
    bool in_proximity;
    bool is_printed;
    struct device_info *next;
};

typedef struct device_info *device_node;
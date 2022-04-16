/*	Author: ebramkw
	Typedef and definitions	*/

/*---------------------------------------------------------------------------*/
#define NUM_SEND 2
/*---------------------------------------------------------------------------*/
typedef struct {
  unsigned long src_id;
  unsigned long timestamp;
  unsigned long seq;
} data_packet_struct;
/*---------------------------------------------------------------------------*/

struct device_info {
    int id;
    unsigned long timestamp; // timestamp of last received packet 
    unsigned long first_close_prox_timestamp; // timestamp of first received packet in proximity
    unsigned long last_close_prox_timestamp; // timestamp of first received packet in proximity
    bool in_proximity;
    bool is_printed;
    struct device_info *next;
};

typedef struct device_info *device_node;
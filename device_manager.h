typedef struct {
    int device_id;
    unsigned long first_timestamp;
    unsigned long last_timestamp;
    struct device_node* next;
} device_node;

typedef struct {
    device_node *head;
    int my_id;
} device_manager;

device_manager* init_device_manager(int id);
void add_node(device_manager *manager, int id, unsigned long timestamp);
void remove_node(device_manager *manager, device_node *prev, device_node *to_remove);
void node_detected(device_manager *manager, int id, unsigned long timestamp);
void check_for_absence(device_manager *manager, unsigned long curr_timestamp);
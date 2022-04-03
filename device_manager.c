#define ABSENT_LIMIT 30
#define MIN_CONTACT 15

device_manager* init_device_manager(int id) {
    device_manager *manager = (device_manager*)malloc(sizeof(struct device_manager));
    manager->head = NULL;
    manager->my_id = id;
}

void add_node(device_manager *manager, int id, unsigned long timestamp) {
    device_node *new_node;
    new_node = (device_node*) malloc(sizeof(struct device_node));
    new_node->device_id = id;
    new_node->first_timestamp = timestamp;
    if (head == NULL) {
        head = new_node;
        return; 
    }
    device_node *temp = head;
    while (temp->next != NULL) temp = temp->next;
    temp->next = new_node;
}

void remove_node(device_manager *manager, device_node *prev, device_node *to_remove) {
    // removed node is head
    if (to_remove == head) {
        head = head->next;
        // free(to_remove);
        return;
    }

    // removed node is tail
    if (to_remove->next == NULL) {
        prev->next = NULL;
        // free(to_remove);
        return;
    }
    // node to remove is in the middle of the list
    prev->next = to_remove->next;
    // free(to_remove);
    return;
}

void node_detected(device_manager *manager, int id, unsigned long timestamp) {
    if (head == NULL) {
        // First node detected
        return add_node(id, timestamp); 
    }
    
    device_node *ptr = manager->head;
    while (ptr != NULL) {
        // Updating last timestamp if node is currently connected
        if (ptr->id == id) {
            ptr->last_timestamp = timestamp;
            return;
        }
        ptr = ptr->next;
    }
    // Node is detected for the first time
    return add_node(id, timestamp);
}

void check_for_absence(device_manager *manager, unsigned long curr_timestamp) {
    printf("Checking for absence...\n");
    device_node *ptr = head, *prev = null;
    while (ptr != NULL) {
        if (curr_timestamp - ptr->last_timestamp > ABSENT_LIMIT) {
            printf("%d ABSENT %d\n", manager->my_id, ptr->id);
            remove_node(prev, ptr);
        }
    }
}
/**
 * \file
 *    Header file for the Contiki netflow engine
 *
 * \author
 *         Ndizera Eddy <eddy.ndizera@student.uclouvain.be>
 *         Ivan Ahad <ivan.abdelahad@student.uclouvain.be>
 */
/*---------------------------------------------------------------------------*/

#include "contiki.h"
#include "contiki-lib.h"
#include "sys/node-id.h"
#include "net/ipv6/ipv6flow/ipflow.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

#define LIST_NAME flow_list
#define MAX_LENGTH 10

static uint16_t seqno = 1;
process_event_t netflow_event;
static short initialized = 0;

struct flow_node {
  struct flow_node *next;
  ipflow_record_t record;
};

LIST(LIST_NAME);

/*---------------------------------------------------------------------------*/
PROCESS(flow_process, "Ip flows");
/*---------------------------------------------------------------------------*/
void 
initialize_ipflow()
{
  PRINTF("Initialize ipflow\n");
  list_init(LIST_NAME);
  process_start(&flow_process, NULL);
  initialized = 1;
}
/*---------------------------------------------------------------------------*/
int
is_launched()
{
	return initialized;
}
/*---------------------------------------------------------------------------*/
int 
flow_update(uip_ipaddr_t *ripaddr, int size)
{
	if(initialized == 0){
		PRINTF("Not initialized\n");
		return 0;
	}	
  // Update flow if already existent
  struct flow_node *current_node; 
  for(current_node = list_head(LIST_NAME); 
      current_node != NULL;
      current_node = list_item_next(current_node)) {
    ipflow_record_t record = current_node -> record;
	PRINTF("Record destination %d \n", record.destination);
    if (ripaddr -> u8[0] == record.destination){
      PRINTF("Update existent flow record\n");
      record.size = record.size + size;
      record.packets++;
      return 1;
    }
  }

  // Cannot add anymore flow
  if (list_length(LIST_NAME) >= MAX_LENGTH){
    PRINTF("Cannot add new flow record. Flow table is full.\n");
    return 0;
  }

  // Create new flow
  PRINTF("Create new flow record\n");
  PRINTF("Address: %d \n", ripaddr -> u8[0]);
  PRINTF("Length of flow tables %d \n", list_length(LIST_NAME));
  ipflow_record_t new_record = {ripaddr -> u8[0], size, 1};
  // TODO allocate memory and free
  static struct flow_node new_node;
  new_node.record = new_record;
  list_push(LIST_NAME, &new_node);
  return 1;
}
/*---------------------------------------------------------------------------*/
ipflow_t *
create_message()
{
	if(initialized == 0){
		PRINTF("Not initialized\n");
		return 0;
	}
  PRINTF("Create message\n");
  // Create header 
  // TODO get real battery value, get rpl parent
  int length = HDR_BYTES + FLOW_BYTES * list_length(LIST_NAME);
  ipflow_hdr_t header = {node_id, seqno, 80, length, 1};

  // Create flow records
  ipflow_record_t records[list_length(LIST_NAME)];
  struct flow_node *current_node;
  int n = 0; 
  for(current_node = (struct flow_node*)list_head(LIST_NAME); 
      current_node != NULL;
      current_node = list_item_next(current_node)) {
    records[n] = current_node->record;
    n++;
  }
  // TODO malloc memory for message
  ipflow_t message = {header, records};

  seqno ++;

  return &message;

}
/*---------------------------------------------------------------------------*/
void
flush(){
	if(initialized == 0){
		PRINTF("Not initialized\n");
		return ;
	}
  PRINTF("Flush records list\n");
  while(list_pop(LIST_NAME) != NULL){}
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(flow_process, ev, data)
{
  PROCESS_BEGIN();

  ipflow_p = PROCESS_CURRENT();
  netflow_event = process_alloc_event();

  while(1){
    PROCESS_YIELD();
    PRINTF("Event to treat\n");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
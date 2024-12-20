#ifndef ORDER_H_
#define ORDER_H_
#include <stdlib.h>
#include <stdio.h>
#include "amount_set.h"
#include "list.h"

/** Type for defining the order struct */
//define Order_t struct
typedef struct Order_t {
	unsigned int order_id;
	AmountSet order_products;
}*Order;

/*
copyOrder - returns a copy of source_element as order
INPUT:
	@param source_element - order to copy
OUTPUT:
	@param dest_order - copied order, NULL if failed
*/
ASElement copyOrder(ListElement source_element);

/*
freeOrder - frees the given element as order
INPUT:
	@param element_to_free - element to free
*/
void freeOrder(ListElement element_to_free);

/*
orderCreate - creates new order
INPUT:
	@param id - order id
	@param copyElement - copy product func 
    @param freeElement - free product func
    @param compareElements -compare products func
OUTPUT:
	the created order. if error returns NULL
*/
Order orderCreate(unsigned int id,CopyASElement copyElement,
                  FreeASElement freeElement,
                  CompareASElements compareElements);

/*
orderDestroy - destroys given order
INPUT:
	@param order - order to detroy
-the func will use the static freeOrder
*/
void orderDestroy(Order order);


#endif //ORDER_H_
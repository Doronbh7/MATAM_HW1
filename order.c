#include "order.h"
#include <stdlib.h>
#include <assert.h>

//func implementation

ASElement copyOrder(ListElement source_element) {

	//creates new order and checks if valid
	Order dest_order = malloc(sizeof(*dest_order));
	if (dest_order == NULL)
	{
		return NULL;
	}

	//convert element to order
	Order source_order = (Order)source_element;

	//regular copy
	dest_order->order_id = source_order->order_id;

	//deep copy
	dest_order->order_products = asCopy(source_order->order_products);
	//check if asCopy valid(assuming source_order->order_products!=NULL)
	if (dest_order->order_products == NULL &&
		source_order->order_products != NULL) {
		//garunteed fail in asCopy, frees allocated memory
		//if source_order->order_products==NULL asCopy returns NULL
		free(dest_order);

		return NULL;
	}

	//passed all copies, returns dest product
	return dest_order;

}


void freeOrder(ListElement element_to_free) {
	Order order_to_free;

	if (element_to_free != NULL) {

		//converts element to order
		order_to_free = (Order)element_to_free;
		//frees allocated amount set in order
		asDestroy(order_to_free->order_products);

		//frees the allocated order
		free(order_to_free);
	}
}


Order orderCreate(unsigned int id,CopyASElement copyElement,
                  FreeASElement freeElement,
                  CompareASElements compareElements ){
    
	//allocates new order and checks if valid
	Order new_order = malloc(sizeof(*new_order));
    if(new_order == NULL){
        return NULL;
    }

    new_order->order_id=id;
	
	//creates amount set of products and checks if valid
	new_order->order_products = asCreate(copyElement,
            freeElement,
            compareElements);
    if (new_order->order_products == NULL)
    {
        //if fail - frees memory
		free(new_order);
        return NULL;
    }

    return new_order;
}

void orderDestroy(Order order){
	freeOrder(order);
}



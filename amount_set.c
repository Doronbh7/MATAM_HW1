#include "amount_set.h"
#include <stdlib.h>
#include <assert.h>

#define NO_SIZE -1

/** Type for defining the element node struct */
typedef struct ASElementNode_t* ASElementNode;

//defining element node
struct ASElementNode_t {
	ASElement element;//element inside node
	double amount;//amount of elements
	ASElementNode next;//next node in linked list
};

//defining amount set
struct AmountSet_t {
	int size;//size of lement linked list
	ASElementNode head;//first item in element linked list 
	ASElementNode iterator;//iterator for user usage
	CopyASElement copyASElement;//copy function for ASElement
	FreeASElement freeASElement;//free function for ASElement
	CompareASElements cmpASElement;//compare function for ASElement
};

//defining static functions to use with ASElementNode
static ASElementNode ASElementNodeCreate(CopyASElement copyElement,
	                                     ASElementNode element);
static ASElementNode getASElementNode(AmountSet set, ASElement element);       

/*
ASElementNodeCreate: create function for struct ASElementNode
INPUT:
	@param copyElement - function to copy the given element 
	@param element - the element value in the created node
OUTPUT:
	@param allocated_node - created node with its new values
*/
static ASElementNode ASElementNodeCreate(CopyASElement copyElement,
	                                     ASElementNode element) {

	//allocating node and checking if allocation is valid
	ASElementNode allocated_node = malloc(sizeof(*allocated_node));
	if (allocated_node == NULL) {
		return NULL;
	}

	//setting values
	allocated_node->amount = 0.0;
	allocated_node->element = copyElement(element);
	allocated_node->next = NULL;
	
	return allocated_node;
}

/*
getASElementNode: returns the requested element node from a given node
INPUT:
	@param set - set of unique elements
	@param element - element to search for
OUTPUT:
	@param node_ptr - the requested element node if found one
	else, returns null if element not in set
NOTE: we use this function when we know that set and element arent NULL
*/
static ASElementNode getASElementNode(AmountSet set, ASElement element) {

	assert(set != NULL && element != NULL);//asserting the ptrs arent null
	
	//while loop that searches for the element on the linked list
	ASElementNode node_ptr = set->head;
	while (node_ptr != NULL) {

		if (set->cmpASElement(node_ptr->element, element) == 0) { //match
			return node_ptr;
		}

		node_ptr = node_ptr->next;//forwarding
	}

	return NULL;
}

//amount set functions with comments on amount_set.h

AmountSet asCreate(CopyASElement copyElement,FreeASElement freeElement,
	               CompareASElements compareElements) {

	if (copyElement == NULL || freeElement == NULL || compareElements == NULL){
		return NULL;
	}

	//allocating new amount set
	AmountSet allocated_as = malloc(sizeof(*allocated_as));
	if (allocated_as == NULL) {
		return NULL;
	}

	//setting variables
	allocated_as->cmpASElement = compareElements;
	allocated_as->copyASElement = copyElement;
	allocated_as->freeASElement = freeElement;
	allocated_as->head = NULL;
	allocated_as->iterator = NULL;
	allocated_as->size = 0;

	return allocated_as;
}

void asDestroy(AmountSet set){

	//clears set and frees it, no need to check NULL
	asClear(set);
	free(set);
}

AmountSet asCopy(AmountSet set) {
	
	if (set == NULL) {
		return NULL;
	}

	//creates new target set for copy and checks allocaiton
	AmountSet target_set = asCreate(set->copyASElement, set->freeASElement,
														set->cmpASElement);
	if (target_set == NULL) {
		return NULL;
	}
	
	//while loop that copies the elements from set to target_set
	double target_amount = 0;
	ASElementNode elem_ptr = set->head;
	while (elem_ptr != NULL){
		
		//register elements and amount to to target_set and checking if valid
		if (asRegister(target_set, elem_ptr->element) != AS_SUCCESS ||
		    asGetAmount(set, elem_ptr->element, &target_amount) != AS_SUCCESS
			|| asChangeAmount(target_set, elem_ptr->element, target_amount)
			!= AS_SUCCESS) {

			asDestroy(target_set);//if failed, destroys set and exit with null
			return NULL;
		}
	
		elem_ptr = elem_ptr->next;//forwarding
	}

	//resets iterators
	target_set->iterator = NULL;
	set->iterator = NULL;

	return target_set;
}

int asGetSize(AmountSet set){
	return (set != NULL) ? set->size : NO_SIZE;
}

bool asContains(AmountSet set, ASElement element){
	
	if (set == NULL || element == NULL) {
		return false;
	}
	
	//if the function gets the element, return true
	return (getASElementNode(set, element) != NULL) ? true : false;
}

AmountSetResult asGetAmount(AmountSet set, ASElement element,
	                        double* outAmount) {

	if (set == NULL || element == NULL || outAmount == NULL) {
		return AS_NULL_ARGUMENT;
	}
	
	//pointer for requested node
	ASElementNode temp_node = getASElementNode(set, element);
	if (temp_node == NULL){
		return AS_ITEM_DOES_NOT_EXIST;
	}

	*outAmount = temp_node->amount;//points at amount of requested element
	return AS_SUCCESS;
}

AmountSetResult asRegister(AmountSet set, ASElement element) {
	if (set == NULL || element == NULL) {
		return AS_NULL_ARGUMENT;
	}
	if (asContains(set, element)){
		return AS_ITEM_ALREADY_EXISTS;
	}
	//allocation of new _node with given element and memory check
	ASElementNode new_node = ASElementNodeCreate(set->copyASElement, element);
	if (new_node == NULL){
		return AS_OUT_OF_MEMORY;
	}
	bool flag = false;//exit flag from while, true when new node registered
	if (set->size == 0) {//register when linked list is mpty
		set->head = new_node;
		flag = true;
	}
	else { //register when linked list isnt empty
		ASElementNode cur_node = set->head;
		while (cur_node != NULL && flag == false) {
			//if new node is smaller than first node
			if (cur_node == set->head && 
				set->cmpASElement(cur_node->element, new_node->element) >= 0) {
				set->head = new_node;
				new_node->next = cur_node;
				flag = true;
			}
			//if new node is bigger than next node
			else if (cur_node->next == NULL && 
				     set->cmpASElement(cur_node->element,
						               new_node->element) <= 0) {
				cur_node->next = new_node;
				flag = true;
			}
			else if (cur_node->next != NULL && 
				     set->cmpASElement(cur_node->element,new_node->element) <= 0
				     &&set->cmpASElement(cur_node->next->element,
						               new_node->element) >= 0) {
				new_node->next = cur_node->next;
				cur_node->next = new_node;
				flag = true;
			}
			cur_node = cur_node->next;//forwarding
		}
	}
	assert(flag == true);
	set->size++;
	set->iterator = NULL; //reset iterator
	return AS_SUCCESS;
}

AmountSetResult asChangeAmount(AmountSet set, ASElement element,
	                           const double amount) {

	//getting the amount of given element and checking return value
	double elem_amount = 0;
	AmountSetResult result_value = asGetAmount(set, element, &elem_amount);
	if (result_value != AS_SUCCESS)
	{
		return result_value;//returns specific case of function error
	}

	//checking if amount valid
	if (amount + elem_amount < 0)
	{
		return AS_INSUFFICIENT_AMOUNT;//if not return error
	}

	//else we increase the node amount and return success
	getASElementNode(set, element)->amount += amount;
	return AS_SUCCESS;
}

AmountSetResult asDelete(AmountSet set, ASElement element) {

	if (set == NULL || element == NULL) {
		return AS_NULL_ARGUMENT;
	}

	//while loop that finds the element node and deletes it
	ASElementNode node_ptr = set->head;
	ASElementNode prev_node_ptr = NULL;
	while (node_ptr != NULL) {
		
		if (set->cmpASElement(node_ptr->element, element) == 0) {//match
			
			//free element
			set->freeASElement(node_ptr->element);

			//sets head and prev node in list accordingly(compensate)
			if (node_ptr == set->head){
				set->head = node_ptr->next;
			}
			else
			{
				prev_node_ptr->next = node_ptr->next;
			}
			
			//frees the node 
			free(node_ptr);
			
			//asserting that the list size atm has to be positive
			assert(set->size > 0);
			set->size--;
			set->iterator = NULL; //reset iterator
			return AS_SUCCESS;
		}
		
		//forwarding
		prev_node_ptr = node_ptr;
		node_ptr = node_ptr->next;
	}

	return AS_ITEM_DOES_NOT_EXIST;
}

//funct implementation similar to delete func in stack(popping head elements)
AmountSetResult asClear(AmountSet set) {
	
	if (set == NULL) {
		return AS_NULL_ARGUMENT;
	}

	//while loop that delets the link list
	AmountSetResult ret_value;
	ASElementNode head_ptr = set->head;
	while (head_ptr != NULL){
		
		//deletes the head and checks if valid
		ret_value = asDelete(set, head_ptr->element);
		if (ret_value == AS_NULL_ARGUMENT) {
			return ret_value;//returns specific function result
		}

		//**set->head is set now to the previous set->head->next value
		head_ptr = set->head;//therefore thats the forwarding
	}
	
	//size, iterator, head values of set were effected, no need to reset them 

	return AS_SUCCESS;
}

ASElement asGetFirst(AmountSet set) {
	
	if (set == NULL) {
		return NULL;
	}
	
	//setting iterator to be the head node and returns its element
	set->iterator = set->head;
	return (set->iterator == NULL) ? NULL : set->iterator->element;
}

ASElement asGetNext(AmountSet set) {
	
	if (set == NULL || set->iterator == NULL) {
		return NULL;
	}
	
	//setting iterator to be the next node and returns its element
	set->iterator = getASElementNode(set, set->iterator->element)->next;
    return (set->iterator == NULL) ? NULL : set->iterator->element;
}


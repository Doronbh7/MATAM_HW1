#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "matamazom.h"
#include "list.h"
#include "order.h"
#include "matamazom_print.h"

#define ERROR_RANGE 0.001
#define HALF_INT 0.5
#define CAPITAL_A 'A'
#define CAPITAL_Z 'Z'
#define SMALL_A 'a'
#define SMALL_Z 'z'
#define HIGHEST_DIGIT '9' 
#define SMALLEST_DIGIT '0'
#define ORDER_ERROR 0
#define SINGLE 1
#define NEGETIVE(x) (-1*x)
#define DEF_PROFIT -1

/** Type for defining the product struct */
typedef struct Product_t* Product;

//defining product
struct Product_t {
	char* product_name;//name
	unsigned int product_id;//id
	unsigned int amount_sold;
	MatamazomAmountType measurement_type;//product measurement
	MtmProductData additional_data;//additional info
    MtmCopyData copyData;//copy product data function
    MtmFreeData freeData;//free product data function
    MtmGetProductPrice prodPrice;//get product price function

};

//defining matamazom warehouse struct
struct Matamazom_t {
	AmountSet products_storage;//amount set of products
	List order_list;//list  of orders
	unsigned int num_orders;//number of orders
};

//defining static functions
//for product
static ASElement copyProduct(ASElement source_element);
static void freeProduct(ASElement element_to_free);
static int compareProduct(ASElement element1, ASElement element2);

//additional static funcs
static bool inRange(double n, double high, double low);
static bool isAmountConsistentWithAmountType(const double amount,
                                         const MatamazomAmountType amountType);
static Product searchProductById(AmountSet products_storage,
	                             unsigned int product_id);
static Order searchOrderById(List order,
                              unsigned int order_id);
static void clearProductFromOrders(Matamazom matamazom, const unsigned int id);
static void clearProductFromOrder(Order order, const unsigned int id);
static void changeOrderProductAmount(Order ret_order,Product ret_product,
        const double amount);
static bool checkInsufficientAmount(Matamazom matamazom,Order order);
static void decreaseProductFromStorageByOrder(Matamazom matamazom,
                                              Order ret_order);
//for printing
static void printProductsInAmountSet(AmountSet product_storage, bool flag ,
        FILE* output);
//for price calculation
static double getOrderPrice(Order order);
static double getOrdersTotalPrice(Matamazom matamazom);
static Product getBestProfitableProduct(Matamazom matamazom);


/*
copyProduct - returns a copy of source_element as product
INPUT:
	@param source_element - product to copy
OUTPUT:
	@param dest_product - copied product, NULL if failed
*/
static ASElement copyProduct(ASElement source_element) {
	
	//creates new product and checks if valid
	Product dest_product = malloc(sizeof(*dest_product));
	if (dest_product == NULL)
	{
		return NULL;
	}

	//convert element to product
	Product source_product = (Product)source_element;
	
	//regular copy
	dest_product->product_id = source_product->product_id;
	dest_product->measurement_type = source_product->measurement_type;
	dest_product->copyData = source_product->copyData;
	dest_product->freeData = source_product->freeData;
	dest_product->prodPrice = source_product->prodPrice;
	dest_product->amount_sold = source_product->amount_sold;
	
	//deep copy
	dest_product->additional_data = 
		source_product->copyData(source_product->additional_data);
	
	//creates new name and checks if valid
	dest_product->product_name = 
		malloc(strlen(source_product->product_name)+1);
	if (dest_product->product_name == NULL)
	{
		//if out of memory, destroys dest product and returns NULL
		dest_product->freeData(dest_product->additional_data);
		free(dest_product);
		return NULL;
	}

	else {//allocation valid - copies string
		strcpy(dest_product->product_name, source_product->product_name);
	}

	//passed all copies, returns dest product
	return dest_product;
}

/*
freeProduct - frees the given element as product
INPUT:
	@param element_to_free - element to free
*/
static void freeProduct(ASElement element_to_free) {

	Product product_to_free;

	if (element_to_free != NULL){
		
		//converts element to product
		product_to_free = (Product)element_to_free;
		//frees allocated data in product
		product_to_free->freeData(product_to_free->additional_data);
		free(product_to_free->product_name);
		
		//frees the allocated product
		free(product_to_free);
	}
}

/*
compareProduct - compares between 2 elements as products
INPUT:
	@param element1 - referance element
	@param element2 - addtional element
OUTPUT:
	returns the sub result of their id's.
	if output < 0 -> element1 < element2
	if output > 0 -> element1 > element2
	if output = 0 -> element1 = element2
*/
static int compareProduct(ASElement element1, ASElement element2) {

	//converts elements to products
	Product ref_product = (Product)element1;
	Product non_ref_product = (Product)element2;

	return ref_product->product_id - non_ref_product->product_id;//seb result
}



/*
inRange - checks if number is in range of 2 numbers
INPUT:
	@param n - number to check
	@param high - highest number in range
	@param low - lowest number in range
OUTPUT:
	if n in range - true, else false
*/
static bool inRange(double n, double high, double low) {
    return (n <= high) && (n >= low);
}
/*
isAmountConsistentWithAmountType - checks if amount is valid 
with given  amount type
INPUT:
	@param amount - given amount to check
	@param amountType - the type of amount
OUTPUT:
	true if valid, false if not
*/
static bool isAmountConsistentWithAmountType(const double amount,              
                                         const MatamazomAmountType amountType){

	if (amountType == MATAMAZOM_ANY_AMOUNT){

	    return true;
	}

	double amount_floor = floor(amount);
	double amount_ceil = ceil(amount);

	//checks if amount is in error range of a natural number
	if (((amount<= (amount_floor + ERROR_RANGE))&&(amount>= amount_floor)) ||
            ((amount<=amount_ceil)&&amount>=(amount_ceil - ERROR_RANGE))) {
		return true;
	}
	//checks if amount is in error range of a X.5 number
	else if (amountType == MATAMAZOM_HALF_INTEGER_AMOUNT)
	{
		if (((amount<= (amount_floor + ERROR_RANGE+HALF_INT))&&
		(amount>= amount_floor+HALF_INT)) ||
            ((amount<=amount_ceil-HALF_INT+ERROR_RANGE)&&amount>=(amount_ceil
            - ERROR_RANGE-HALF_INT))) {
			return true;
		}
	}

	return false;
}

/*
searchProductById - searches the product in given AS with given id
INPUT:
	@param product_id - id of requested product
	@param products_storage - amount set to search in
OUTPUT:
	returns the product if found, if not NULL
*/
static Product searchProductById(AmountSet products_storage,
	                             unsigned int product_id){
	//using macro to search inside amount set
	//note that current_product is set to be Product obj
	AS_FOREACH(Product,currrent_product,products_storage){
        if(currrent_product->product_id == product_id){
            return currrent_product;
        }
    }
    return NULL;
}

/**
 * searches the order in given order list with given id
 * @param order id of requested order
 * @param order_id list of orders to search in
 * @return the order if found, if not NULL
 */
static Order searchOrderById(List order_list,unsigned int order_id){
    //using macro to search inside the list

    LIST_FOREACH(Order,current_order,order_list){
        if(current_order->order_id == order_id){
            return current_order;
        }
    }
    return NULL;
}

/*
clearProductFromOrders - clears product from all orders
INPUT:
	 @param matamzom - the mighty matamzon
	 @param id - product id
the function will call clearProductFromOrder to operate
-it clears the product from single order
NOTE: we relay on the fact that id is valid
*/
static void clearProductFromOrders(Matamazom matamazom, const unsigned int id){
	//using macro to search inside order list
	//note that current_order is set to be order obj
	LIST_FOREACH(Order,current_order,matamazom->order_list){
        clearProductFromOrder(current_order, id);
    }
}

/*
changeOrderProductAmount - changes amount of given product in given order
INPUT:
    @param ret_order - given order
    @param ret_product - given product to change amount of
    @param amount - given amount to change
*/
static void changeOrderProductAmount(Order ret_order,Product ret_product,
        const double amount){
    //assert(!(ret_order->order_products==NULL));
    if(asContains(ret_order->order_products,ret_product)==false){
        if(amount>0)
        {
            asRegister(ret_order->order_products,ret_product);
            asChangeAmount(ret_order->order_products,ret_product,amount);
        }
        return;
    }
    if(asChangeAmount(ret_order->order_products,ret_product,amount)
    ==AS_INSUFFICIENT_AMOUNT){
        asDelete(ret_order->order_products,ret_product);
    }
}


/*
clearProductFromOrder - clears product from given order
INPUT:
	 @param order - the given order
	 @param id - product id
NOTE: we relay on the fact that id,asDelete() are valid 
*/
static void clearProductFromOrder(Order order, const unsigned int id){
	//using macro to search inside order products amount
	//note that current_product is set to be product obj
	AS_FOREACH(Product, current_product, order->order_products) {
        if(current_product->product_id==id){
            //current_product->freeData(current_product->additional_data);
            asDelete(order->order_products,
				            current_product);
        }
    }
}

/**
 * @param matamazom a warehouse
 * @param order a selected order to check
 * @return false-if the order contains a product with an amount
 *         that is larger than its amount in matamazom.
 *         true-else
 */
static bool checkInsufficientAmount(Matamazom matamazom,Order order){
    double order_amount=0;
    double storage_amount=0;
    AS_FOREACH(Product,current_product_order,order->order_products){
        asGetAmount(order->order_products,current_product_order,&order_amount);
        Product current_product_storage=searchProductById
                (matamazom->products_storage,current_product_order->product_id);
        asGetAmount(matamazom->products_storage,current_product_storage,
                &storage_amount);
        if(order_amount>storage_amount){
            return false;
        }
    }
    return true;
}
/**
 * remove the order products from the storage
 * @param matamazom a warehouse
 * @param ret_order the shipping order
 */

static void decreaseProductFromStorageByOrder(Matamazom matamazom,
        Order ret_order){
    double order_amount=0;
    AS_FOREACH(Product,current_product,ret_order->order_products){
        asGetAmount(ret_order->order_products,current_product,&order_amount);
		searchProductById(matamazom->products_storage,
		        current_product->product_id)->amount_sold += order_amount;
		mtmChangeProductAmount(matamazom,current_product->product_id,
                               (NEGETIVE(order_amount)));
    }

}
/*
printProductsInAmountSet - prints all products in given amount set
that contains only products
INPUT:
@param product_storage - amount set of products
@param - output - open stream we printing into
*/
static void printProductsInAmountSet(AmountSet product_storage, bool flag,
        FILE* output){

    if (product_storage == NULL) {
        return;
    }

    assert(output != NULL);
    double cur_amount = 0;
	double amount_to_price = 0;
    //using macro to search inside storage
    //note that cur_product is set to be Product obj
    AS_FOREACH(Product, cur_product, product_storage) {
        //gets amount from storage
        asGetAmount(product_storage, cur_product,&cur_amount);
        //prints details
		amount_to_price = (flag == true) ? cur_amount : SINGLE;
        mtmPrintProductDetails(cur_product->product_name,
                cur_product->product_id, cur_amount,
                cur_product->prodPrice(cur_product->additional_data,
                        amount_to_price),output);
    }
}
/*
getOrderPrice - gets order price
INPUT:
	@param order
OUTPUT:
	price of order
*/
static double getOrderPrice(Order order) {

    assert(order != NULL);

    double cur_amount = 0;
    double price = 0;
    AS_FOREACH(Product, cur_product, order->order_products) {
        asGetAmount(order->order_products, cur_product, &cur_amount);
        price += cur_product->prodPrice(cur_product->additional_data,
                                        cur_amount);
    }
    return price;
}
/*
getOrdersTotalPrice - gets all orders price
INPUT:
	@param matamazom - mighty matamazom
OUTPUT:
	price of all orders
*/
static double getOrdersTotalPrice(Matamazom matamazom) {
    assert(matamazom != NULL);
    double price = 0;
    LIST_FOREACH(Order, cur_order, matamazom->order_list) {
        price += getOrderPrice(cur_order);
    }
    return price;
}
/*
getBestSellingProduct - gets the product who has the highest amount sold value
INPUT:
	@param matamazom - mighty matamazom
OUTPUT:
	best selling product (when 2 same -choses the lower id porduct)
	NULL if storage empty/no sells
*/
static Product getBestProfitableProduct(Matamazom matamazom) {
    if (matamazom->products_storage == NULL)
    {
        return NULL;
    }
    double max_profit = 0;
    double product_profit = 0;
	int max_profit_id = DEF_PROFIT;
    AS_FOREACH(Product, cur_product, matamazom->products_storage) {
        product_profit = cur_product->prodPrice(cur_product->additional_data,
                                                cur_product->amount_sold);
		
		if (product_profit > max_profit || (product_profit = max_profit
		        && product_profit > 0 &&
		        cur_product->product_id < max_profit_id)) {
			max_profit_id = cur_product->product_id;
			max_profit = product_profit;
		}
    }
    return (max_profit_id == DEF_PROFIT) ? NULL :
		          searchProductById(matamazom->products_storage, max_profit_id);
}

Matamazom matamazomCreate(){
	
	//allocates matamzom and checks if valid
	Matamazom allocated_matamazom = malloc(sizeof(*allocated_matamazom));
    if (allocated_matamazom == NULL){//if fail - frees memory
        return NULL;
    }

	//allocates amount set for products and checks if valid
	allocated_matamazom->products_storage=asCreate(copyProduct, freeProduct,
											       compareProduct);
    if (allocated_matamazom->products_storage == NULL){//if fail - frees memory
        free(allocated_matamazom);
        return NULL;
    }

	//allocates list for products and checks if valid
	allocated_matamazom->order_list =listCreate(copyOrder,freeOrder);
    if(allocated_matamazom->order_list==NULL){//if fail - frees memory
        asDestroy(allocated_matamazom->products_storage);
        free(allocated_matamazom);
        return NULL;
    }

	allocated_matamazom->num_orders = 0;
	return allocated_matamazom;

}

void matamazomDestroy(Matamazom matamazom){

    if(matamazom==NULL){
        return;
    }
    
	//frees allocated memory in matamzom
	asDestroy(matamazom->products_storage);
    listDestroy(matamazom->order_list);
	//frees allocated matamazom
	free(matamazom);
}

//!!!!!!!!! check if customData gets free'd in main.c/mtm tests
//for now, we use the copy data func to copy the custom data
MatamazomResult mtmNewProduct(Matamazom matamazom, const unsigned int id,
        const char *name,const double amount,const MatamazomAmountType
        amountType,const MtmProductData customData,
        MtmCopyData copyData,MtmFreeData freeData,
        MtmGetProductPrice prodPrice){
	if(matamazom == NULL || name == NULL || customData == NULL ||
	   copyData == NULL || freeData == NULL || prodPrice == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
	}
	if (searchProductById(matamazom->products_storage, id) != NULL) {
		return MATAMAZOM_PRODUCT_ALREADY_EXIST;
	}
	if (name == NULL || !(inRange(name[0], SMALL_Z, SMALL_A) ||
		inRange(name[0], CAPITAL_Z, CAPITAL_A) ||
		inRange(name[0], HIGHEST_DIGIT, SMALLEST_DIGIT))) {
		return MATAMAZOM_INVALID_NAME;
	}
    if (amount < 0 || !isAmountConsistentWithAmountType(amount, amountType)){
        return MATAMAZOM_INVALID_AMOUNT;
    }
    Product new_product = malloc(sizeof(*new_product));
    if(new_product == NULL){
        return MATAMAZOM_OUT_OF_MEMORY;
    }
    new_product->product_id = id;
    new_product->freeData = freeData;
    new_product->copyData = copyData;
    new_product->measurement_type = amountType;
	new_product->prodPrice = prodPrice;
	new_product->amount_sold=0;
	new_product->additional_data = new_product->copyData(customData);
	new_product->product_name = malloc(strlen(name) + 1);
	if (new_product->product_name == NULL){
		new_product->freeData(new_product->additional_data);
		free(new_product);
		return MATAMAZOM_OUT_OF_MEMORY;
	}
	else {//allocation valid - copies string
		strcpy(new_product->product_name, name);
	}
    AmountSetResult result_value = asRegister(matamazom->products_storage, 
		                                      new_product);
    if(result_value == AS_OUT_OF_MEMORY){
        freeProduct(new_product);
        return MATAMAZOM_OUT_OF_MEMORY;
    }
	asChangeAmount(matamazom->products_storage, new_product, amount);
    freeProduct(new_product);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmChangeProductAmount(Matamazom matamazom,
        const unsigned int id, const double amount){
    
	//check null arguments
	if(matamazom==NULL){
         return MATAMAZOM_NULL_ARGUMENT;
    }
    
	//if storage empty - no product
	if(matamazom->products_storage == NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    
	//gets product with id and checks if valid
	Product ret_product=searchProductById(matamazom->products_storage, id);
    if(ret_product == NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }

	//check if amount consistent with type
    if(!isAmountConsistentWithAmountType(amount,
		                                 ret_product->measurement_type)){
        return MATAMAZOM_INVALID_AMOUNT;
    }

	//changes amount and checks if amount insuffisient
    if(asChangeAmount(matamazom->products_storage,ret_product,amount)==
    AS_INSUFFICIENT_AMOUNT){
        return MATAMAZOM_INSUFFICIENT_AMOUNT;
    }

	//passed all tests-success
    return MATAMAZOM_SUCCESS;
}


MatamazomResult mtmClearProduct(Matamazom matamazom, const unsigned int id){
    
	//check null arguments
	if(matamazom == NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    
	//if storage empty - no product
	if(matamazom->products_storage==NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    
	//searches for product and checks if valid
	Product ret_product=searchProductById(matamazom->products_storage, id);
    if(ret_product==NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }

	//clears product from all orders
    clearProductFromOrders(matamazom, id);

    //ret_product->freeData(ret_product->additional_data);
    
	//clears product from matamzom storage
	asDelete(matamazom->products_storage,ret_product);

    return MATAMAZOM_SUCCESS;

}


unsigned int mtmCreateNewOrder(Matamazom matamazom){

    if(matamazom==NULL){
        return ORDER_ERROR;
    }
    //creates a new order and checks if valid
	Order new_order=orderCreate(matamazom->num_orders+1, copyProduct,
							    freeProduct,compareProduct);
    if(new_order==NULL){//if failed returns 0
		return ORDER_ERROR;
    }

    //inserts order in list and checks if valid
	ListResult result = listInsertLast(matamazom->order_list, new_order);
    if((result != LIST_SUCCESS)){
		//if failed frees memory and returns 0
		orderDestroy(new_order);
        return ORDER_ERROR;
    }
    orderDestroy(new_order);
	//else return new number of orders
    return ++matamazom->num_orders;
}

MatamazomResult mtmChangeProductAmountInOrder(Matamazom matamazom,
                                              const unsigned int orderId,
                                              const unsigned int productId,
                                              const double amount){
    if(matamazom==NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    if(matamazom->order_list==NULL){
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    Product ret_product=searchProductById(matamazom->products_storage,
            productId);
    if (ret_product==NULL){
        return MATAMAZOM_PRODUCT_NOT_EXIST;
    }
    if(isAmountConsistentWithAmountType(amount,ret_product->measurement_type)
    ==false){
        return MATAMAZOM_INVALID_AMOUNT;
    }
    Order ret_order=searchOrderById(matamazom->order_list,
                                          orderId);
    if (ret_order==NULL){
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    if(amount==0){
        return MATAMAZOM_SUCCESS;
    }
    changeOrderProductAmount(ret_order,ret_product,amount);

    return MATAMAZOM_SUCCESS;


}

MatamazomResult mtmShipOrder(Matamazom matamazom, const unsigned int orderId) {
    if (matamazom == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Order ret_order = searchOrderById(matamazom->order_list, orderId);
    if (ret_order == NULL) {
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    if (checkInsufficientAmount(matamazom, ret_order) == false) {
		return MATAMAZOM_INSUFFICIENT_AMOUNT;
    }
    decreaseProductFromStorageByOrder(matamazom,ret_order);
    mtmCancelOrder(matamazom,orderId);
    return MATAMAZOM_SUCCESS;
}


MatamazomResult mtmCancelOrder(Matamazom matamazom, const unsigned int orderId){
    if(matamazom==NULL){
        return MATAMAZOM_NULL_ARGUMENT;
    }
    Order ret_order=searchOrderById(matamazom->order_list,orderId);
    if(ret_order==NULL){
        return MATAMAZOM_ORDER_NOT_EXIST;
    }
    LIST_FOREACH(Order,current_order,matamazom->order_list){
        if(current_order->order_id==orderId){
			listRemoveCurrent(matamazom->order_list);
        }
    }
    return MATAMAZOM_SUCCESS;
}


MatamazomResult mtmPrintInventory(Matamazom matamazom, FILE* output) {

    if (matamazom == NULL || output == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }

    //prints headers
    fprintf(output, "Inventory Status:\n");

    if (matamazom->products_storage == NULL)
    {
        return MATAMAZOM_SUCCESS;
    }

    printProductsInAmountSet(matamazom->products_storage, false,output);
    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmPrintOrder(Matamazom matamazom, const unsigned int orderId,
        FILE* output) {

    if (matamazom == NULL || output == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }

    //gets the order and checks if valid
    Order requested_order = searchOrderById(matamazom->order_list, orderId);
    if (requested_order == NULL) {
        return MATAMAZOM_ORDER_NOT_EXIST;
    }

    //prints header
    mtmPrintOrderHeading(orderId, output);
    //prints all products in order
    printProductsInAmountSet(requested_order->order_products,true, output);
    //prints end
    mtmPrintOrderSummary(getOrdersTotalPrice(matamazom), output);

    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmPrintBestSelling(Matamazom matamazom, FILE* output) {

    if (matamazom == NULL || output == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }

	//print header
	fprintf(output,"Best Selling Product:\n");
    Product best_seller = getBestProfitableProduct(matamazom);
    
	if (best_seller != NULL)
	{
		mtmPrintIncomeLine(best_seller->product_name, best_seller->product_id,
			best_seller->prodPrice(best_seller->additional_data,
				best_seller->amount_sold)
			,output);
	}
	else
	{
		fprintf(output,"none\n");
	}
	

    return MATAMAZOM_SUCCESS;
}

MatamazomResult mtmPrintFiltered(Matamazom matamazom,
        MtmFilterProduct customFilter, FILE* output) {

    if (matamazom == NULL || customFilter == NULL || output == NULL) {
        return MATAMAZOM_NULL_ARGUMENT;
    }

    double cur_amount = 0;

    //using macro to search inside order products amount
    //note that cur_product is set to be product obj
    AS_FOREACH(Product, cur_product, matamazom->products_storage) {
        AS_FOREACH(Product, cur_product, matamazom->products_storage) {
            asGetAmount(matamazom->products_storage, cur_product,
                        &cur_amount);
            if (customFilter(cur_product->product_id,
                             cur_product->product_name, cur_amount,
                             cur_product->additional_data)) {

                mtmPrintProductDetails(cur_product->product_name,
                                       cur_product->product_id, cur_amount,
                                       cur_product->prodPrice
                                               (cur_product->additional_data,
                                                SINGLE),
                                       output);
            }
        }
    }
    return MATAMAZOM_SUCCESS;

}





/**
	******************************************************************************
	* @file    can_mvs.h
	* @author  Victor S. Camargo
	* @brief   CanMvs header file.
	******************************************************************************

	[English]:

	Class created for the specific application of syncronize known data and
	resquest function execution between two STM32 microcontrolers.
	This class is designed to be used with HAL libraries and STM32CubeMX.

	What does it do:
	- Sends known data structs (adjacent in memory) to the other device and updates it;
	- Requests known data structs from the other device;
	- Sends function execution requisitions to the other device;
	- Receives function execution requisitions from the other device;

	How does it ships and syncronizes the data:
	- The sending device generates a copy of the data that you wish to send and
	splits it into packages (since CAN BUS protocol can only send 8 bytes of data per
	shipment). First, it will send a header that contains information about what the board
	is going to be sending. Then, the data packages.
	- The receiving device unloads the package of data to a space in the memory. When all
	data is received, it will overwrite the target struct.

	Tasks to do before using this class:
	- Configure and generate HAL's CAN basic code in the STM32CubeMX software, because
	different boards requires different configurations. Both boards should have the same
	baud rate. I suggest seeing this guide if you dont know how to setup the basics:
	https://controllerstech.com/can-protocol-in-stm32/
	- Configure what is needed in the @file can_mvs_config.h for your application;
	- Add the needed IDs in the @file can_mvs_config.h for each data you would 
	like to synchronize.
	- Give the adress of the main's @var CAN_HandleTypeDef hcan to the @method init;
	- Associate each ID to the desired data with the @method connect_struct_to_id;
	- Run the @method receive_data_callback in a definition of 
	HAL_CAN_RxFifo1MsgPendingCallback in main's file (where @var CAN_HandleTypeDef hcan is);
	- Run the @method shipping_application in the main's while loop or how you would like.

	The algorythm works based on flags. It marks IDs as "CAN_MARK_TO_BE_PROCESSED" or 
	"CAN_MARK_TO_BE_REQUESTED". The first one, as the name suggests, means that the board 
	should deal with the flag, executing a function or sending the requested data. The other
	one means it should send the marked flags to the other board, requesting a function
	execution or data.
	The @method shipping_application will deal with flags marked as CAN_MARK_TO_BE_REQUESTED
	automatically. But the algorythm was build in a way that you need to implement 
	funcionalities in your main's code to deal with data and function execution flags marked 
	as CAN_MARK_TO_BE_PROCESSED, using @method start_shipping_data on the data marked ones or
	executing a requested function. Use the example project as reference (or copy it).

	Also, there is the loopback mode for very basic testing in the case of using only 
	one board. You should configure it on @file can_mvs_config.h and in STM32CubeMX's
	MX_CAN_Init generated function (that usually is on main file).

*/

#ifndef __CAN_MVS_H
#define __CAN_MVS_H

#include <stdlib.h>
#include <string.h>

/* open and edit this one */
#include "can_mvs_config.h"

#ifdef __cplusplus
 extern "C" {
#endif
	 
	typedef enum
	{
		CAN_STATUS_OK = 0,
		CAN_STATUS_ERROR,
		CAN_STATUS_BUSY,
		CAN_STATUS_TIMEOUT,
		CAN_STATUS_NULL_POINTER,
		CAN_STATUS_INVALID_ID
	} CAN_status;
	
	typedef enum
	{
		CAN_DATA_TYPE_STANDARD = 0,
		CAN_DATA_TYPE_HEADER,
		CAN_DATA_TYPE_ASK_FOR_DATA,
		CAN_DATA_TYPE_HAVE_A_FUNCTION_RUN
	} CAN_data_type;
	
	typedef enum
	{
		CAN_MARK_TO_BE_PROCESSED = 0,
		CAN_MARK_TO_BE_REQUESTED,
		CAN_MARK_MAX
	} CAN_mark_type;

	class CanMvs
	{
	public:
		CanMvs();

		void init(CAN_HandleTypeDef*);

		CAN_status connect_struct_to_id(CAN_MVS_data_id, uint8_t*, uint8_t);

		CAN_status start_shipping_data(CAN_MVS_data_id);

		CAN_status shipping_application(void);

		void receive_data_callback(void);

		bool is_there_any_data_id_flag_marked(CAN_mark_type);
		bool is_data_id_flag_marked(CAN_mark_type, CAN_MVS_data_id);
		void mark_data_id_flag(CAN_mark_type, CAN_MVS_data_id);
		void unmark_data_id_flag(CAN_mark_type, CAN_MVS_data_id);
		void clear_pending_data_flags(CAN_mark_type);

		bool is_there_any_func_id_flag_marked(CAN_mark_type);
		bool is_func_id_flag_marked(CAN_mark_type, CAN_MVS_functions_id);
		void mark_func_id_flag(CAN_mark_type, CAN_MVS_functions_id);
		void unmark_func_id_flag(CAN_mark_type, CAN_MVS_functions_id);
		void clear_pending_func_flags(CAN_mark_type);

	private:

		CAN_HandleTypeDef *ptr_hcan;

		CAN_TxHeaderTypeDef TxHeader;
		CAN_RxHeaderTypeDef RxHeader;

		uint8_t TxData[8];
		uint8_t RxData[8];
		uint32_t TxMailbox;

		uint32_t flags_data_requisition[CAN_MARK_MAX];
		uint32_t flags_func_requisition[CAN_MARK_MAX];

		uint8_t count_received_bytes;
		uint8_t count_sent_bytes;

		uint8_t max_receivable_bytes;
		uint8_t max_sendable_bytes;

		CAN_MVS_data_id id_shipping_struct;
		CAN_MVS_data_id id_receipt_struct;

		uint8_t *ptr_struct_to_send;
		uint8_t *ptr_struct_to_receive;

		uint8_t *ptr_struct_to_send_bytes;
		uint8_t *ptr_struct_to_receive_bytes;

		uint8_t *ptr_structs[ID_DATA_MAX];
		uint16_t size_of_structs[ID_DATA_MAX];

		void set_package_data_type(CAN_data_type);
		void set_package_struct_id(CAN_MVS_data_id);

		void prepare_package_to_be_send(CAN_data_type);
		CAN_status send_package(void);

		CAN_MVS_data_id identify_received_struct_id(void);
		CAN_status start_receiving(CAN_MVS_data_id);
		void process_received_package(void);
		void unload_received_package(void);
		void copy_built_struct_to_destiny(void);

		void callback_set_asked_flags(CAN_data_type);

		bool sent_header;
		bool is_HAL_CAN_busy(void);
		bool were_all_pending_data_packages_sent(void);
		bool were_all_pending_data_packages_received(void);
		bool is_struct_id_valid(CAN_MVS_data_id);
		CAN_data_type identify_received_data_type(void);
		bool am_i_receiving_this_id(CAN_MVS_data_id);
	};
	
#ifdef __cplusplus
}
#endif

#endif

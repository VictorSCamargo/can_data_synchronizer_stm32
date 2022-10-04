#include "can_mvs.h"

#ifdef CAN_MVS_LOOPBACK_MODE

	typedef struct
	{
		int v1;
		float v2;
		int v3;
	}struct_loopback_mode;
	
	struct_loopback_mode board_1_data, board_2_data;
	
#endif

CanMvs::CanMvs(void)
{
	count_received_bytes = 0;
	max_receivable_bytes = 0;

	count_sent_bytes = 0;
	max_sendable_bytes = 0;

	id_shipping_struct = ID_DATA_MAX;
	id_receipt_struct = ID_DATA_MAX;

	sent_header = false;

  TxHeader.DLC = 8;
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.StdId = CAN_MVS_SENDER_ID;

	ptr_hcan = NULL;

	for(int i = 0; i < ID_DATA_MAX; i++)
	{
		ptr_structs[i] = NULL;
	}
	
#ifdef CAN_MVS_LOOPBACK_MODE
	
	board_1_data.v1 = 372;
	board_1_data.v2 = 7.5f;
	board_1_data.v3 = -32;

	board_2_data.v1 = 0;
	board_2_data.v2 = 0;
	board_2_data.v3 = 0;

#endif
	
}

/* Start of public methods ---------------------------------- */

void CanMvs::init(CAN_HandleTypeDef *hcan)
{
	ptr_hcan = hcan;
	
	HAL_CAN_Start(ptr_hcan);

  HAL_CAN_ActivateNotification(ptr_hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
}

bool CanMvs::is_struct_id_valid(CAN_MVS_data_id struct_id)
{
	int id_value = struct_id;

	if((id_value < 0) || (id_value >= (uint8_t)ID_DATA_MAX))
	{
		return false;
	}
	return true;
}

CAN_status CanMvs::connect_struct_to_id(
	CAN_MVS_data_id struct_id,
	uint8_t *ptr_to_struct,
	uint8_t size_of_struct
)
{
	if(!is_struct_id_valid(struct_id))
	{
		return CAN_STATUS_INVALID_ID;
	}
	ptr_structs[struct_id] = ptr_to_struct;
	size_of_structs[struct_id] = size_of_struct;
	return CAN_STATUS_OK;
}

CAN_status CanMvs::shipping_application(void)
{
	CAN_status status = CAN_STATUS_OK;

	if(is_there_any_data_id_flag_marked(CAN_MARK_TO_BE_REQUESTED))
	{
		if(is_HAL_CAN_busy())
		{
			return CAN_STATUS_BUSY;
		}
		prepare_package_to_be_send(CAN_DATA_TYPE_ASK_FOR_DATA);
		status = send_package();

		if(status == CAN_STATUS_OK)
		{
			clear_pending_data_flags(CAN_MARK_TO_BE_REQUESTED);
		}
	}

	if(is_there_any_func_id_flag_marked(CAN_MARK_TO_BE_REQUESTED))
	{
		if(is_HAL_CAN_busy())
		{
			return CAN_STATUS_BUSY;
		}
		prepare_package_to_be_send(CAN_DATA_TYPE_HAVE_A_FUNCTION_RUN);
		status = send_package();

		if(status == CAN_STATUS_OK)
		{
			clear_pending_func_flags(CAN_MARK_TO_BE_REQUESTED);
		}
	}

	if(!were_all_pending_data_packages_sent())
	{
		if(is_HAL_CAN_busy())
		{
			return CAN_STATUS_BUSY;
		}

		if(!sent_header)
		{
			prepare_package_to_be_send(CAN_DATA_TYPE_HEADER);
			status = send_package();

			if(status == CAN_STATUS_OK)
			{
				sent_header = true;
			}
		}
		else
		{
			prepare_package_to_be_send(CAN_DATA_TYPE_STANDARD);
			status = send_package();
		}
	}
	return status;
}

void CanMvs::receive_data_callback(void)
{
	/* desativa temporariamente o callback */
	HAL_CAN_DeactivateNotification(ptr_hcan, CAN_IT_RX_FIFO1_MSG_PENDING);

	/* recebe dado */
	HAL_CAN_GetRxMessage(ptr_hcan, CAN_RX_FIFO1, &RxHeader, RxData);

	process_received_package();
	
	HAL_CAN_ActivateNotification(ptr_hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
}

CAN_status CanMvs::start_shipping_data(CAN_MVS_data_id id)
{
	if(!were_all_pending_data_packages_sent() || am_i_receiving_this_id(id))
	{
		return CAN_STATUS_BUSY;
	}

	uint16_t struct_size;
	uint8_t* ptr_choosen_struct;
	
#ifndef CAN_MVS_LOOPBACK_MODE

	if(ptr_structs[id] == NULL)
	{
		return CAN_STATUS_NULL_POINTER;
	}

	struct_size = size_of_structs[id];
	ptr_choosen_struct = ptr_structs[id];

#else

	struct_size = sizeof(board_1_data);
	ptr_choosen_struct = (uint8_t*)&board_1_data;

#endif
	
	ptr_struct_to_send = (uint8_t*) malloc(struct_size);
	
	if(ptr_struct_to_send == NULL)
	{
		return CAN_STATUS_ERROR;
	}

	memcpy(ptr_struct_to_send, ptr_choosen_struct, struct_size);

	ptr_struct_to_send_bytes = ptr_struct_to_send;

	max_sendable_bytes = struct_size;
	count_sent_bytes = 0;
	
	id_shipping_struct = id;
	sent_header = false;

	return CAN_STATUS_OK;
}

bool CanMvs::is_there_any_data_id_flag_marked(CAN_mark_type source)
{
	if(flags_data_requisition[source] != 0)
	{
		return true;
	}		
	return false;
}

bool CanMvs::is_data_id_flag_marked(CAN_mark_type source, CAN_MVS_data_id id)
{
	if(flags_data_requisition[source] & 1<<(uint8_t)id)
	{
		return true;
	}
	return false;
}

void CanMvs::mark_data_id_flag(CAN_mark_type source, CAN_MVS_data_id id)
{
	flags_data_requisition[source] |= 1<<id;
}

void CanMvs::unmark_data_id_flag(CAN_mark_type source, CAN_MVS_data_id id)
{
	flags_data_requisition[source] &= ~(1<<id);
}

bool CanMvs::is_there_any_func_id_flag_marked(CAN_mark_type source)
{
	if(flags_func_requisition[source] != 0)
	{
		return true;
	}		
	return false;
}
bool CanMvs::is_func_id_flag_marked(CAN_mark_type source, CAN_MVS_functions_id id)
{
	if(flags_func_requisition[source] & 1<<(uint8_t)id)
	{
		return true;
	}
	return false;
}

void CanMvs::mark_func_id_flag(CAN_mark_type source, CAN_MVS_functions_id id)
{
	flags_func_requisition[source] |= 1<<id;
}

void CanMvs::unmark_func_id_flag(CAN_mark_type source, CAN_MVS_functions_id id)
{
	flags_func_requisition[source] &= ~(1<<id);
}

void CanMvs::clear_pending_data_flags(CAN_mark_type source)
{
	flags_data_requisition[source] = 0;
}

void CanMvs::clear_pending_func_flags(CAN_mark_type source)
{
	flags_func_requisition[source] = 0;
}

/* End of public methods ------------------------------------ */

bool CanMvs::am_i_receiving_this_id(CAN_MVS_data_id id)
{
	if((id == id_receipt_struct) && !were_all_pending_data_packages_received())
	{
		return true;
	}
	return false;
}

CAN_data_type CanMvs::identify_received_data_type(void)
{
	return (CAN_data_type)RxData[0];
}

CAN_MVS_data_id CanMvs::identify_received_struct_id(void)
{
	return (CAN_MVS_data_id)RxData[1];
}

CAN_status CanMvs::send_package(void)
{
	if(HAL_CAN_AddTxMessage(ptr_hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK)
	{
		return CAN_STATUS_ERROR;
	}
	return CAN_STATUS_OK;
}

bool CanMvs::is_HAL_CAN_busy(void)
{
	if(HAL_CAN_IsTxMessagePending(ptr_hcan, TxMailbox) == 0U)
	{
		return false;
	}
	return true;
}

void CanMvs::set_package_data_type(CAN_data_type type)
{
	TxData[0] = (uint8_t)type;
}

void CanMvs::set_package_struct_id(CAN_MVS_data_id id)
{
	TxData[1] = (uint8_t)id;
}

void CanMvs::prepare_package_to_be_send(CAN_data_type data_type)
{
	if(data_type == CAN_DATA_TYPE_STANDARD)
	{
		set_package_data_type(CAN_DATA_TYPE_STANDARD);

		/* first byte is reserved*/
		for(int i = 1; i < 8; i++)
		{
			TxData[i] = *ptr_struct_to_send_bytes;
			count_sent_bytes++;
			ptr_struct_to_send_bytes++;

			if(were_all_pending_data_packages_sent())
			{
				free(ptr_struct_to_send); //do not remove
				break;
			}
		}
	}

	else if(data_type == CAN_DATA_TYPE_HEADER)
	{
		set_package_data_type(CAN_DATA_TYPE_HEADER);
		set_package_struct_id(id_shipping_struct);
	}

	else if(data_type == CAN_DATA_TYPE_ASK_FOR_DATA)
	{	
		uint8_t *conversor_ptr = (uint8_t*) &flags_data_requisition[CAN_MARK_TO_BE_REQUESTED];
		set_package_data_type(CAN_DATA_TYPE_ASK_FOR_DATA);
		TxData[1] = conversor_ptr[3]; //Little-endian is the default memory format for ARM processors.
		TxData[2] = conversor_ptr[2];
		TxData[3] = conversor_ptr[1];
		TxData[4] = conversor_ptr[0];
	}

	else if(data_type == CAN_DATA_TYPE_HAVE_A_FUNCTION_RUN)
	{
		uint8_t *conversor_ptr = (uint8_t*) &flags_func_requisition[CAN_MARK_TO_BE_REQUESTED];
		set_package_data_type(CAN_DATA_TYPE_HAVE_A_FUNCTION_RUN);
		TxData[1] = conversor_ptr[3];
		TxData[2] = conversor_ptr[2];
		TxData[3] = conversor_ptr[1];
		TxData[4] = conversor_ptr[0];
	}
}

bool CanMvs::were_all_pending_data_packages_sent(void)
{
	if(count_sent_bytes != max_sendable_bytes)
	{
		return false;
	}
	return true;
}

bool CanMvs::were_all_pending_data_packages_received(void)
{
	if(count_received_bytes != max_receivable_bytes)
	{
		return false;
	}
	return true;
}

void CanMvs::process_received_package(void)
{
	switch(identify_received_data_type())
	{
		case CAN_DATA_TYPE_STANDARD:
			if(!were_all_pending_data_packages_received())
			{
				unload_received_package();
			}
			break;

		case CAN_DATA_TYPE_HEADER:
			start_receiving(identify_received_struct_id());
			break;
		
		case CAN_DATA_TYPE_ASK_FOR_DATA:
			callback_set_asked_flags(CAN_DATA_TYPE_ASK_FOR_DATA);
			break;
		
		case CAN_DATA_TYPE_HAVE_A_FUNCTION_RUN:
			callback_set_asked_flags(CAN_DATA_TYPE_HAVE_A_FUNCTION_RUN);
			break;
		default:
			break;
	}
}

void CanMvs::callback_set_asked_flags(CAN_data_type received_data_type)
{
	uint32_t united_data = (
		RxData[4] | (RxData[3] << 8) | (RxData[2] << 16) | (RxData[1] << 24)
	);

	if(received_data_type == CAN_DATA_TYPE_ASK_FOR_DATA)
	{
		flags_data_requisition[CAN_MARK_TO_BE_PROCESSED] |= united_data; //sets 1 to those that arent
	}
	else
	{
		flags_func_requisition[CAN_MARK_TO_BE_PROCESSED] |= united_data;
	}
}

CAN_status CanMvs::start_receiving(CAN_MVS_data_id id)
{
	uint16_t struct_size;
	uint8_t* ptr_choosen_struct;
	
#ifndef CAN_MVS_LOOPBACK_MODE

	struct_size = size_of_structs[id];
	ptr_choosen_struct = ptr_structs[id];

#else

	struct_size = sizeof(board_2_data);
	ptr_choosen_struct = (uint8_t*)&board_2_data;

#endif

	ptr_struct_to_receive = (uint8_t*) malloc(struct_size);
	
	if(ptr_struct_to_receive == NULL)
	{
		return CAN_STATUS_ERROR;
	}

	memcpy(ptr_struct_to_receive, ptr_choosen_struct, struct_size);

	ptr_struct_to_receive_bytes = ptr_struct_to_receive;
	
	id_receipt_struct = id;
	max_receivable_bytes = struct_size;
	count_received_bytes = 0;

	return CAN_STATUS_OK;
}

void CanMvs::copy_built_struct_to_destiny(void)
{
	uint8_t* ptr_destiny; 

#ifndef CAN_MVS_LOOPBACK_MODE

	ptr_destiny = ptr_structs[id_receipt_struct];

#else

	ptr_destiny = (uint8_t*)&board_2_data;

#endif

	memcpy(
		ptr_destiny,
		ptr_struct_to_receive,
		max_receivable_bytes
	);
}

void CanMvs::unload_received_package(void)
{
	/* first byte is reserved*/
	for(int i = 1; i < 8; i++)
	{
		*ptr_struct_to_receive_bytes = RxData[i];
		count_received_bytes++;
		ptr_struct_to_receive_bytes++;

		if(were_all_pending_data_packages_received())
		{
			copy_built_struct_to_destiny();
			free(ptr_struct_to_receive); //do not remove
			break;
		}
	}
}

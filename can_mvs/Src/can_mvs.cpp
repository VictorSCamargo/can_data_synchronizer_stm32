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

	id_shipping_struct = ID_STRUCT_MAX;
	id_receipt_struct = ID_STRUCT_MAX;

	sent_header = false;

  TxHeader.DLC = 8;
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.StdId = CAN_MVS_SENDER_ID;

	ptr_hcan = NULL;

	for(int i = 0; i < ID_STRUCT_MAX; i++)
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
}

bool CanMvs::is_struct_id_valid(CAN_MVS_struct_id struct_id)
{
	int id_value = struct_id;

	if((id_value < 0) || (id_value >= (uint8_t)ID_STRUCT_MAX))
	{
		return false;
	}
	return true;
}

CAN_MVS_status CanMvs::get_app_status(void)
{
	if(!was_all_pending_data_sent())
	{
		return CAN_MVS_BUSY;
	}
	return CAN_MVS_OK;
}

CAN_MVS_status CanMvs::connect_struct_to_id(
	CAN_MVS_struct_id struct_id,
	uint8_t *ptr_to_struct,
	uint8_t size_of_struct
)
{
	if(!is_struct_id_valid(struct_id))
	{
		return CAN_MVS_INVALID_ID;
	}
	ptr_structs[struct_id] = ptr_to_struct;
	size_of_structs[struct_id] = size_of_struct;
	return CAN_MVS_OK;
}

CAN_MVS_status CanMvs::shipping_application(void)
{
	CAN_MVS_status status = CAN_MVS_OK;

	if(!was_all_pending_data_sent())
	{
		if(!is_HAL_CAN_busy())
		{
			if(!sent_header)
			{
				send_header();
			}
			else
			{
				prepare_package();
				status = send_package();
			}
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

CAN_MVS_status CanMvs::start_shipping(CAN_MVS_struct_id id)
{
	if(!was_all_pending_data_sent())
	{
		return CAN_MVS_BUSY;
	}

	uint16_t struct_size;
	uint8_t* ptr_choosen_struct;
	
#ifndef CAN_MVS_LOOPBACK_MODE

	if(ptr_structs[id] == NULL)
	{
		return CAN_MVS_NULL_POINTER;
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
		return CAN_MVS_ERROR;
	}

	memcpy(ptr_struct_to_send, ptr_choosen_struct, struct_size);

	ptr_struct_to_send_bytes = ptr_struct_to_send;

	max_sendable_bytes = struct_size;
	count_sent_bytes = 0;
	
	id_shipping_struct = id;
	sent_header = false;

	return CAN_MVS_OK;
}

/* End of public methods ------------------------------------ */

bool CanMvs::is_rxdata_header(void)
{
	if(RxData[0] == ID_DATA_HEADER)
	{
		return true;
	}
	return false;
}

CAN_MVS_struct_id CanMvs::identify_received_struct_id(void)
{
	return (CAN_MVS_struct_id)RxData[1];
}

CAN_MVS_status CanMvs::send_package(void)
{
	if(HAL_CAN_AddTxMessage(ptr_hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK)
	{
		return CAN_MVS_ERROR;
	}
	return CAN_MVS_OK;
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

void CanMvs::set_package_struct_id(CAN_MVS_struct_id id)
{
	TxData[1] = (uint8_t)id;
}

CAN_MVS_status CanMvs::send_header(void)
{	
	set_package_data_type(ID_DATA_HEADER);
	set_package_struct_id(id_shipping_struct);
	
	if(send_package() != CAN_MVS_OK)
	{
		return CAN_MVS_ERROR;
	}

	set_package_data_type(ID_DATA_STANDARD);
				
	sent_header = true;

	return CAN_MVS_OK;
}

void CanMvs::prepare_package(void)
{
	/* first byte is reserved*/
	for(int i = 1; i < 8; i++)
	{
		TxData[i] = *ptr_struct_to_send_bytes;
		count_sent_bytes++;
		ptr_struct_to_send_bytes++;

		if(was_all_pending_data_sent())
		{
			free(ptr_struct_to_send); //do not remove
			break;
		}
	}
}

bool CanMvs::was_all_pending_data_sent(void)
{
	if(count_sent_bytes != max_sendable_bytes)
	{
		return false;
	}
	return true;
}

bool CanMvs::was_all_pending_data_received(void)
{
	if(count_received_bytes != max_receivable_bytes)
	{
		return false;
	}
	return true;
}

void CanMvs::process_received_package(void)
{
	if(is_rxdata_header())
	{
		start_receiving(identify_received_struct_id());
	}
	else
	{
		if(!was_all_pending_data_received())
		{
			unload_received_package();
		}
	}
}

CAN_MVS_status CanMvs::start_receiving(CAN_MVS_struct_id id)
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
		return CAN_MVS_ERROR;
	}

	memcpy(ptr_struct_to_receive, ptr_choosen_struct, struct_size);

	ptr_struct_to_receive_bytes = ptr_struct_to_receive;
	
	id_receipt_struct = id;
	max_receivable_bytes = struct_size;
	count_received_bytes = 0;

	return CAN_MVS_OK;
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

		if(was_all_pending_data_received())
		{
			copy_built_struct_to_destiny();
			free(ptr_struct_to_receive); //do not remove
			break;
		}
	}
}

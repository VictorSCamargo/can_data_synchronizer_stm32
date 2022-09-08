/**
  ******************************************************************************
  * @file    can_mvs_config.h
  * @brief   CanMvs configuration file.
  ******************************************************************************
  */

#ifndef __CAN_MVS_CONFIG_H
#define __CAN_MVS_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

/* to debug with only one STM32 board */
#define CAN_MVS_LOOPBACK_MODE

/* this should be the ID that the other device's CAN filter was configured to receive*/
#define CAN_MVS_SENDER_ID 0x103

/* the device will receive messages that have this ID */
#define CAN_MVS_FILTER_ID 0x407

#ifdef CAN_MVS_LOOPBACK_MODE
	#define HAL_CAN_FILTER_ID CAN_MVS_SENDER_ID //is the same in the loopback mode
#else
	#define HAL_CAN_FILTER_ID CAN_MVS_FILTER_ID
#endif

/**
  * Add the IDs for your structs in this enum.
  * - first ID should be equal to zero.
  * - do not remove ID_STRUCT_MAX.
  */
typedef enum
{
	ID_STRUCT_EXEMPLO = 0,
	ID_STRUCT_MAX
}CAN_MVS_struct_id;

#ifdef __cplusplus
}
#endif

#endif

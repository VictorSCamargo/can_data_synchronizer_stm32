# can_data_synchronizer_stm32
 Class created to syncronize data structs between two STM32 devices using CAN protocol and HAL libraries given by STM.
 The CAN protocol lets us send packages with up to 8 bytes that should be received by the other device and processed. This class makes it easy to syncronize known structs between two devides and request function executions, providing functionalities as:
 - Send known data structs to the other device and updates it;
	- Request known data structs from the other device;
	- Send function execution requisitions to the other device;
	- Receive function execution requisitions from the other device;

 Details can be found in the header file.
 If you wish to use it, check the example project made by me: https://github.com/VictorSCamargo/can_data_synchronizer_test_code.git

Made by Victor. Documented in english.

# can_data_synchronizer_stm32 - CanMvs class
This class was made during the university by me for a research lab project. I am making it public for those who would like to use it and to show some of the skills I got up to this moment with C/C++ and embedded programming.

## About it
Class created to syncronize data structs and to make requests between two STM32 devices using CAN protocol and HAL libraries given by STM. This class can:
- Send and request known data structs between devices;
- Send and receive function execution requisitions between devices;

## How it works
Basically you create IDs for data structs and function in the `can_mvs_config.h` file and, in your code, associate the data scructs to the respective IDs you created with the method `connect_struct_to_id`. You do it in both devices and then, when you want to send the data struct or request the data struct between devices using the ID you created, they will know where in memory it is.
If you want to use it, there are details in the `can_mvs.h` file and you should check out [this example](https://github.com/VictorSCamargo/can_data_synchronizer_test_code.git) made by me.

## Details about how it works
### About the HAL libraries
The CAN protocol lets us send data between CAN devices with only 2 wires. What you should worry about is that you can send up to 8 bytes of data in every shipment.
The HAL libraries provides only basic features to implement CAN functionalities. I used around 5 functions from the HAL libraries.
#### Should i be concerned about the low-level stuff of the CAN protocol?
The programmer should only need to initialize the device correctly as it says in the `can_mvs.h` file.

### About how does it ships and syncronizes the data
When you call `start_shipping_data` with sucess, the sending device generates a copy of the data struct that the device will send and splits it into packages. First, it will send a header that contains information about what the board is going to be sending. Then, the data packages. The receiving device unloads the package of data to a space in the memory. When all data is received, it will overwrite the target struct. 
Also, data packages are indentified by the first byte. So the boards knows if it is receiving a header, a piece of the struct data, a function execution request or a request for a data struct.

#### How it was designed to be used
The algorythm works based on flags. You can mark IDs as `CAN_MARK_TO_BE_PROCESSED` or `CAN_MARK_TO_BE_REQUESTED` and interpret those flags with the public methods that can be seen in the `can_mvs.h` file.
`CAN_MARK_TO_BE_PROCESSED` means that the board should deal with the flag, executing a function or sending the requested data.
`CAN_MARK_TO_BE_REQUESTED` means it should send the marked flags to the other board, requesting a function execution or data.
The method `shipping_application` will deal with flags marked as `CAN_MARK_TO_BE_REQUESTED` automatically. But the algorythm was build in a way that you need to implement funcionalities in your main's code to deal with data and function execution flags marked as `CAN_MARK_TO_BE_PROCESSED` by using the method `start_shipping_data` (for the data struct) or executing a requested function. Use the example project as reference (or copy it).

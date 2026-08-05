## Software Requirements

 - SWR-PROTO-001 : The TargetCommand payload shall be exactly 8 bytes.
	- > std::array<uint8_t, TARGET_COMMAND_WIRE_SIZE>
 - SWR-PROTO-002 : All 16-bit fields shall use Little Endian byte oreder.
	- > test_known_byte_pattern()
 - SWR-PROTO-003 : Signed target coordinates shall use 16-bit two's complement representation.
	- > test_negative_cordinates()
	- > test_boundary_values()
 - SWR-PROTO-004 : The codec shall reject null pointers and buffers smaller.
	- > test_invalid_arguments()
 - SWR-PROTO-005 : Structure memory shall not be transmitted directly.


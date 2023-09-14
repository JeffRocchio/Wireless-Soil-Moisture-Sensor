# nRF24 Arduino Library Notes

## Some Thoughts on the Use of Addresses:

I think when they're saying it's "best to think of addresses as a path," they mean that instead of a device transmitting its device ID (address), it is instead tagging it's outgoing transmission with the ID (address) of the recipient, ie the "path to the target." That's why the comment says: "0 uses address[0] transmit." It's not saying that I am device 0, it's saying: "This message is intended for the device whose address is zero." And I'm thinking the reason for this is for mesh network type situations where any node could receive a message and if that message is not intended for itself, per the address, then it would retransmit it forward so that after multiple hops it would finally reach its intended target - and that's the notion of a "path."




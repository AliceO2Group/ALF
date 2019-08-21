# ALF

## Introduction
ALF(Alice Low-level Frontend) spawns DIM services as an interface with detector FEEs(Front-End Electronics). The DIM services can be access through DCS's [FRED](https://gitlab.cern.ch/alialfred/FREDServer).

## Requirements
In order to run ALF a DIM Nameserver has to be up and running.

## Usage

### o2-alf
o2-alf is the binary of the ALF server. The only option it expects is the address of the DIM Nameserver. It can be passed either as a command-line argument or as an environmental variable.

`
DIM_DNS_NODE=thedimdns o2-alf
o2-alf --dim-dns-node thedimdns
`

### o2-alf-client
o2-alf-client is the binary of an ALF client used solely for testing purposes. On top of the DIM Nameserver it expects the hostname of the node hosting the ALF server, as a command-line argument.

`
o2-alf-client --dim-dns-node thedimdns --alf-id thealfserver
`

## Services

Service names are identified by the server's hostname, the card's serial number and the link, as follows:

`
ALF_[hostname]/SERIAL_[serial_number]/LINK_[link]/[service_name]
`

## DIM RPC services

The services are DIM RPC services. Every RPC is called with a string and expects a string in return. For these strings the following requirements are defined:

* Arguments are newline-separated '\n'.
* The return string is prefixed by "success\n" or "failure\n"
* Address and values are hex strings with leading zeros for 32-bit unsigned integers. (e.g. 0x0000f00d)
  * An exception is made for SWT words which are 76-bit unsigned integers. (e.g. 0x00000000000badc0ffee)
  
### REGISTER_READ

### REGISTER_WRITE

### SCA_SEQUENCE

### SWT_SEQUENCE

### IC_SEQUENCE

### IC_GBT_I2C_WRITE

### Examples

Examples on making the RPC calls can be seen under [ProgramAlfClient.cxx](apps/ProgramAlfClient.cxx) and [AlfClient.h](src/AlfClient.h).

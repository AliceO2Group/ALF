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

* Arguments are newline-separated ('\n').
* The return string is prefixed by "success\n" or "failure\n"
* Address and values are hex strings with leading zeros for 32-bit unsigned integers. (e.g. 0x0000f00d)
  * An exception is made for SWT words which are 76-bit unsigned integers. (e.g. 0x00000000000badc0ffee)
* Lines prefixed with `#` are disregarded as comments.
  
### REGISTER_READ
* Parameter:
  * Register address
* Returns:
  * Register value

* Example:
  * DIM input: `0x0000f00d \n`
  * DIM output: `0x0000beef \n`

### REGISTER_WRITE
* Parameters:
  * Register address
  * Register value
* Returns:
  * empty
  
* Example:
  * DIM input: `0x0000f00d,0x0000beef \n`
  * DIM output: ` `

### SCA_SEQUENCE
* Parameters:
  * Sequence of SCA command and data pairs
* Returns:
  * Sequence of SCA command, and SCA read pairs
  
* Example:
  * DIM input: `0x00000010,0x00000011\n0x000000020,0x00000021\n`
  * DIM output: `0x00000010,0x00000111\n0x00000020,0x00000221\n`

### SWT_SEQUENCE
* Parameters:
  * Sequence of SWT word and operation pairs as follows:
    * Operations may be:
    * SWT word with `write`
    * `reset` (no SWT word)
    * `read` (no SWT word)
* Returns:
  * Sequence of SWT output as follows:
    * `write` always retuns `0`
    * `read` returns an SWT word
    * `reset` returns nothing
    
* Example:
  * DIM input `reset\n0x0000000000badc0ffee,write\nread\n`
  * DIM output `0\n0x0000000000badc0ffee\n`

### IC_SEQUENCE

* Parameters:
  * Sequence of IC operations as follows:
    * Operations may be:
    * Address, Value and `write`
    * Address and `read`
    
* Returns:
  * Value on `write` (echo)
  * Value on `read`

* Example:
  * DIM input: `0x54,0xff,write\n0x54,read\n`
  * DIM output: `0xff\n0xff\n`
  
### IC_GBT_I2C_WRITE

* Parameters:
  * Value

* Returns:
  * empty
  
* Example:
  * DIM input `0x3`
  * DIM output ` `

### Examples

Examples on making the RPC calls can be seen under [ProgramAlfClient.cxx](apps/ProgramAlfClient.cxx) and [AlfClient.h](src/AlfClient.h).

# ALF

## Introduction
ALF(Alice Low-level Frontend) spawns DIM services as an interface with detector FEEs(Front-End Electronics). The DIM services can be accessed through DCS's [FRED](https://gitlab.cern.ch/alialfred/FREDServer).

## Requirements
In order to run ALF a DIM Nameserver has to be up and running. For performance reasons, it is recommended to run the DIM Nameserver on the host running the FRED server.

## Install ALF
Under the specified directory run: 

`
git clone https://github.com/AliceMCH/ALF.git
`

`
cd ALF
`

`
cmake3 -DInfoLogger_ROOT=/home/flp/alice/sw/slc7_x86-64/libInfoLogger/latest-o2-dataflow
`

`
make all
`

## Usage

### o2-alf
o2-alf is the binary of the ALF server. The only option it expects is the address of the DIM Nameserver. It can be passed either as a command-line argument or as an environmental variable.

`
o2-alf --dim-dns-node thedimdns.cern.ch
`

`
DIM_DNS_NODE=thedimdns.cern.ch o2-alf
`

### o2-alf-client
o2-alf-client is the binary of an ALF client used solely for testing purposes. On top of the DIM Nameserver it expects the hostname of the node hosting the ALF server, the card's serial and the link number as command-line arguments.

`
o2-alf-client --dim-dns-node thedimdns.cern.ch --alf-id thealfserver --serial 0 --link 4
`

## Services

Service names are identified by the server's hostname, the card's serial number and the link, as follows:

`
ALF_[hostname]/SERIAL_[serial_number]/LINK_[link]/[service_name]
`

### DIM RPC services

The services are DIM RPC services. Every RPC is called with a string and expects a string in return. For these strings the following requirements are defined:

* Arguments are newline-separated ('\n').
* The return string is prefixed by "success\n" or "failure\n"
* Address and values are hex strings with leading zeros for 32-bit unsigned integers. (e.g. 0x0000f00d)
  * An exception is made for SWT words which are 76-bit unsigned integers. (e.g. 0x0000000000badc0ffee)
  * Input needs to be prefixed with "0x" but not necessarily with leading zeros.
* Lines prefixed with `#` are disregarded as comments.
  
#### REGISTER_READ
* Parameter:
  * Register address
* Returns:
  * Register value

* Example:
  * DIM input: `0x0000f00d`
  * DIM output: `0x0000beef`

#### REGISTER_WRITE
* Parameters:
  * Register address
  * Register value
* Returns:
  * empty
  
* Example:
  * DIM input: `0x0000f00d,0x0000beef`
  * DIM output: ` `

#### SCA_SEQUENCE
* Parameters:
  * Sequence of SCA command and data pairs
* Returns:
  * Sequence of SCA command, and SCA read pairs
  
* Example:
  * DIM input: `0x00000010,0x00000011\n0x000000020,0x00000021`
  * DIM output: `0x00000010,0x00000111\n0x00000020,0x00000221\n`

#### SWT_SEQUENCE
* Parameters:
  * Sequence of SWT word and operation pairs as follows:
    * Operations may be:
    * `write` with SWT prefix (e.g. `0x0000f00d,write`)
    * `reset` (without SWT word)
    * `read` with optional TimeOut prefix (e.g. `2,read`)
* Returns:
  * Sequence of SWT output as follows:
    * `write` always retuns `0`
    * `read` returns the SWT words present in the CRU SWT FIFO
    * `reset` returns nothing
    
* Example:
  * DIM input `reset\n0x0000000000badc0ffee,write\nread\n0xbadf00d,write\n4,read`
  * DIM output `0\n0x0000000000badc0ffee\n0x000000000000badf00d\n`

#### IC_SEQUENCE

* Parameters:
  * Sequence of IC operations as follows:
    * Operations may be:
    * Address, Value and `write`
    * Address and `read`
    
* Returns:
  * Value on `write` (echo)
  * Value on `read`

* Example:
  * DIM input: `0x54,0xff,write\n0x54,read`
  * DIM output: `0x000000ff\n0x000000ff\n`
  
#### IC_GBT_I2C_WRITE

* Parameters:
  * Value

* Returns:
  * empty
  
* Example:
  * DIM input `0x3\n`
  * DIM output ` `

# ALF

## Introduction
ALF(Alice Low-level Frontend) spawns DIM services as an interface with detector FEEs(Front-End Electronics). The DIM services can be accessed through DCS's [FRED](https://gitlab.cern.ch/alialfred/FREDServer).

## Requirements
In order to run ALF a DIM Nameserver has to be up and running. For performance reasons, it is recommended to run the DIM Nameserver on the host running the FRED server.

## Usage

### o2-alf
o2-alf is the binary of the ALF server. It expects the address of the DIM Nameserver, which can be passed either as a command-line argument or as an environmental variable.

`
o2-alf --dim-dns-node thedimdns.cern.ch
`

`
DIM_DNS_NODE=thedimdns.cern.ch
`

### o2-alf-client
o2-alf-client is the binary of an ALF client used solely for testing purposes. On top of the DIM Nameserver it expects the hostname of the node hosting the ALF server, the card's serial and endpoint, and the link number as command-line arguments. Different arguments to test different types of services are available (run with `--help`).

`
o2-alf-client --dim-dns-node thedimdns.cern.ch --alf-id thealfserver --serial 1041 --endpoint 1 --link 4
`

### o2-alf-lib-client
o2-alf-lib-client is the binary of an ALF SC library client used solely for testing purposes. It expects parameters for the SC modules to test, the card and link ID (run with `--help` for the different options).

`
o2-alf-lib-client --card-id=#1 --serial=1041 --endpoint=1 --swt
`

## DIM Services

Service names may refer to the card or the link level, depending on the functionality published.

Card-level services are identified by the server's hostname, and the card's serial:

`
ALF_[hostname]/SERIAL_[serial]/[service_name]
`

Link-level services are identified by the server's hostname, the card's serial and endpoint pair, and the link, as follows:

`
ALF_[hostname]/SERIAL_[serial]/ENDPOINT_[endpoint]/LINK_[link]/[service_name]
`

### DIM RPC services

The services are DIM RPC services. Every RPC is called with a string and expects a string in return. For these strings the following requirements are defined:

* Arguments are newline-separated ('\n').
* The return string is prefixed by "success\n" or "failure\n"
* Address and values are hex strings with leading zeros for 32-bit unsigned integers. (e.g. 0x0000f00d)
  * An exception is made for SWT words which are 76-bit unsigned integers. (e.g. 0x0000000000badc0ffee)
  * Input needs to be prefixed with "0x" but not necessarily with leading zeros.
* Lines prefixed with `#` are disregarded as comments.

#### CRU
##### REGISTER_SEQUENCE
* Parameters:
   * Operations may be:
     * `write` with address and value (e.g. `0x0000f00d,0x0000beef`)
     * `read` with address (e.g `0x0000cafe`)

* Returns:
   * `write` always retuns `0`
   * `read` returns the value read from the register
    
* Example:
  * DIM input `0xc00004\n0x00c00008, 0x0000beef\n0x00c00008`
  * DIM output `0xcafe\n0\n0xbeef\n`

##### SCA_SEQUENCE
* Parameters:
  * Sequence of SCA operations as follows:
    * Operations may be:
    * An SCA command and data pair (e.g. `0x0000f00d,0x0000cafe`)
    * A wait operation (e.g. `30,wait`) in ms, defaults to 3
    * An SCA supervisory level connect operation (e.g. `svl_connect`)
    * An SCA supervisory level reset operation (`svl_reset`)
    * An SC global reset operation (`sc_reset`)
    * An instruction to execute the sequence atomically (`lock` - needs to lead the sequence)
* Returns:
  * Sequence of SCA output as follows: 
    * SCA command and SCA read pairs
    * Wait confirmations with time waited
    * Connect confirmations made up of a "svl_connect" string
    * No entries for `svl_reset`, `sc_reset`, and `lock` directives
  
* Example:
  * DIM input: `0x00000010,0x00000011\n3\n0x000000020,0x00000021`
  * DIM input (atomic): `lock\n0x00000010,0x00000011\n3\n0x000000020,0x00000021`
  * DIM output: `0x00000010,0x00000111\n3\n0x00000020,0x00000221\n`

##### SCA_MFT_PSU_SEQUENCE

This service is uniquely registered for the MFT PSU CRU.
It extends the `SCA_SEQUENCE` to add the following functionality:
* Parameters:
  * Operations:
    * Select master (e.g. `master`)
    * Select slave (e.g. `slave`)

* Returns:
  * Echo for both `master` & `slave`
 
##### SWT_SEQUENCE
* Parameters:
  * Sequence of SWT word and operation pairs as follows:
    * Operations may be:
    * `write` with SWT prefix (e.g. `0x0000f00d,write`)
    * `sc_reset` without prefix, is global
    * `read` with optional TimeOut prefix (e.g. `2,read`)
    * `wait` with optional WaitTime prefix in ms (e.g. `5,wait`), defaults to 3
    * `lock` which instructs ALF to execute the sequence atomically (needs to lead the sequence)
* Returns:
  * Sequence of SWT output as follows:
    * `write` always retuns `0`
    * `read` returns the SWT words present in the CRU SWT FIFO
    * `sc_reset` returns nothing
    * `wait` returns time waited
    * `lock` returns nothing
    
* Example:
  * DIM input `sc_reset\n0x0000000000badc0ffee,write\nread\n0xbadf00d,write\n4,read`
  * DIM input (atomic) `lock\nsc_reset\n0x0000000000badc0ffee,write\nread\n0xbadf00d,write\n4,read`
  * DIM output `0\n0x0000000000badc0ffee\n0\n0x000000000000badf00d\n`

##### IC_SEQUENCE

* Parameters:
  * Sequence of IC operations as follows:
    * Operations may be:
    * Address, Value and `write`
    * Address and `read`
    * `lock` which instructs ALF to execute the sequence atomically (needs to lead the sequence)
    
* Returns:
  * Value on `write` (echo)
  * Value on `read`
  * Nothing on `lock`

* Example:
  * DIM input: `0x54,0xff,write\n0x54,read`
  * DIM input (atomic): `lock\n0x54,0xff,write\n0x54,read`
  * DIM output: `0x000000ff\n0x000000ff\n`
  
##### IC_GBT_I2C_WRITE

* Parameters:
  * Value

* Returns:
  * empty
  
* Example:
  * DIM input `0x3\n`
  * DIM output ` `

##### PATTERN_PLAYER

* Parameters
  * Sync Pattern
  * Reset Pattern
  * Idle Pattern
  * Sync Length
  * Sync Delay
  * Reset Length
  * Reset Trigger Select
  * Sync Trigger Select
  * Sync At Start
  * Trigger Sync
  * Trigger Reset

* Returns
  * empty

* Example:
  * DIM input `0x23456789abcdef123456\n0x5678\n0x9abc\n42\n0\n53\n30\n29\n#a comment\nfalse\ntrue\nfalse`
  * DIM output ` `

##### LLA_SESSION_START

* Parameters
  * Session Name
  * Timeout for timed start in ms (optional)

* Returns
  * empty

* Examples:
  *  DIM input `FRED\n` 
  *  DIM output ` ` 

##### LLA_SESSION_STOP

* Parameters
  * No parameters

* Returns
  * empty

* Examples:
  *  DIM input ` ` 
  *  DIM output ` ` 


#### CRORC

##### REGISTER_SEQUENCE
* Parameters:
   * Operations may be:
     * `write` with address and value (e.g. `0x0000f00d,0x0000beef`)
     * `read` with address (e.g `0x0000cafe`)

* Returns:
   * `write` always retuns `0`
   * `read` returns the value read from the register
    
* Example:
  * DIM input `0xf00d\n0x0000f00d, 0x0000beef\n0x0000f00d`
  * DIM output `0xcafe\n0\n0xbeef\n`

##### RESET_CARD
* Parameters:
   * empty

* Returns:
   * empty

* Example:
  * DIM input ` `
  * DIM output ` `

## Slow Control library
ALF can also be used as a C++ library to access the Slow Control interface of the CRU. The three available interfaces (IC, SCA & SWT) can be accessed through single operations, or sequences of operations.

For each Slow Control (SC) class a handle can be acquired by passing the card ID as an `std::string` argument and, optionally, the SC channel to use as an `int`. Constructors have no side-effects; an SC reset would need to be performed manually before starting operations (e.g. `swt.scReset()`).

### Single operations
Depending on the type, an SC class offers a different interface for single operation execution. `SWT` and `IC` offer `read()` and `write()` standalone operations, while `SCA` only offers `executeCommand()`.

All the above offer **no implicit locking** and should be manually locked through the use of the [LLA](https://github.com/AliceO2Group/LLA) library, if needed. The recommended way to execute atomic operations in one go is the one described in the following paragraph.

### Sequences of operations
All SC classes offer a function to execute a sequence of their respective operations. This function receives an `std::vector`, consisting of an `std::pair` made up of the compatible SC operation and SC data, as these are defined in their headers.

For example, `SWT` offers `Read, Write, Wait, and SCReset` operations which expect a `TimeOut`, an `SwtWord`, a `WaitTime`, and no argument, respectively.

```
typedef int TimeOut;

/// Typedef for the Data type of an SWT sequence operation.
/// Variant of TimeOut for reads, SwtWord for writes, std::string for Errors
typedef boost::variant<boost::blank, TimeOut, WaitTime, SwtWord, std::string> Data;

/// Enum for the different SWT operation types
enum Operation { Read,
                 Write,
                 Wait,
                 SCReset,
                 Error };
                 
std::vector<std::pair<Operation, Data>> executeSequence(const std::vector<std::pair<Operation, Data>>& operations, bool lock = false);
```

The above function also optionally accepts a boolean, enabling atomic execution. This should not be used with an explicit LLA session started, as it will lead to a deadlock due to the lack of communication between the ALF library and the aforementioned LLA session instance.


More details and examples on the API can be found in the doxygen docs in the header files or in [this](apps/AlfLibClient.cxx) code example.

### Using ALF as a library

To use ALF as a library the "Alf/Alf.h" convenience header may be used, as seen in [this](src/example.cxx) example. To build, it is necessary to load the ALF environment (`module load ALF`) and run the following g++ command. Make sure to adjust the versions according to `module list` output, when the environment is loaded.

```
g++ -Wall \
  -I /opt/alisw/el7/ALF/v0.7.0-1/include \
  -I /opt/alisw/el7/Common-O2/v1.4.9-27/include/ \
  -I /opt/alisw/el7/LLA/v0.1.0-1/include \
  -I /opt/alisw/el7/ReadoutCard/v0.21.3-1/include \
  -I /opt/alisw/el7/boost/v1.72.0-alice1-36/include/ \
  -lO2Alf \
  -lCommon \
  -lO2Lla \
  -lReadoutCard \
  -L /opt/alisw/el7/ALF/v0.7.0-1/lib \
  -L /opt/alisw/el7/Common-O2/v1.4.9-27/lib \
  -L /opt/alisw/el7/LLA/v0.1.0-1/lib \
  -L /opt/alisw/el7/ReadoutCard/v0.21.3-1/lib \
  alf-lib-example.cxx -o ale
```

### Python bindings
The ALF library also offers a python interface through the use of `Boost.Python`. To use it the ALF environment needs to be loaded and the `libO2Alf` library imported. 

To read the documentation for the python bindings:
```
module load ALF
python -c "import libO2Alf; help(libO2Alf)"
```

Non-exhaustive examples on usage may be found in the [test](test) directory.

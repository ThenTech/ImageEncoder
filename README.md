
# ImageEncoder
#### William Thenaers - v0.0.0.5


## Build
1. Install g++\-7 with the commands in install_g++.sh
2. Build the encoder with:
    `make encoder`
    
    Build the decoder with:
    `make decoder`
    
    Or build both with:
    `make` or `make all`
3. Got to the ./bin folder and run the encoder/decoder 
    with a file containing the settings.


## Info
- Linux builds (through Win10 bash) and Windows builds are provided in ./bin
- Everything was implemented according to the assignment
- The only extra thing is an offset for each pixel before the DCT step (and after the iDCT),
here the value of 128 is subtracted from the pixels (and added during decoding) to make the DCT
components smaller and easier to fit in less space.
- The encoded image has the following structure:

| Property                          | Amount of bits |
|-----------------------------------|:--------------:|
| Bit length for quant matrix coeff | `5` |
| Quant matrix coeffs               | `16 * bit_len` |
| Whether to use RLE                | `1` |
| Image width                       | `15` |
| Image height                      | `15` |
| Block data:                       | different for every block |
| Bit length for data in block      | `5` |
| Data length (if using RLE)        | `block bit_len` |
| Data                              | `block bit_len * data_len` |

For the example quant matrix in the assignment, the header is 20.5 bytes of data.

- The encoder/decoder will give a compression percentage after writing the resulting file. (`< 100.0`: result is smaller, `> 100.0`: result is bigger )


## Example
- Original image:

![Original image](doc/ex6.png)

- The decoder had an "error" where signed encoded data was interpreted as unsigned, and
consequently every pixel that would contain signed data was wrongly transformed during iDCT,
giving the weird uniform areas.

![Signed errors](doc/ex6_dec2.png)

- This was solved by providing a more specific function to check the minimal required bit length to represent
a data value.
e.g. The new function would check if a value could be encoded in x bits, by shifting the value to the left of an int16, and reshifting it back (to create a signed value), if the result is the same as the original, the value can be encoded using x bits. If not, increase x until it does.
This way the decoder could do the same thing, because it knows the bit length, and the original value is properly decoded (signed or unsigned).

![Final result](doc/ex6_dec.png)


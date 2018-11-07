# [ImageEncoder](https://github.com/Wosser1sProductions/ImageEncoder) ![version](https://img.shields.io/badge/release-v0.0.0.7-brightgreen.svg)
#### William Thenaers

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
- Linux builds (through Win10 bash) and Windows builds are provided in  `./bin`

- A QtCreator .qbs project file is included for debugging, the makefile will always build for release by default.

- Everything was implemented according to the assignment, i.e. nothing is excluded.

- One extra thing is an offset for each pixel before the DCT step (and after the iDCT), here the value of 128 is subtracted from the pixels (and added during decoding) to make the DCT components smaller and easier to fit in less space.

- The encoded image has the following structure:

    | Property                          | Amount of bits |
    |-----------------------------------|:--------------:|
    | Bit length for quant matrix coeff | `5` |
    | Quant matrix coeffs               | `16 * bit_len` |
    | Whether to use RLE                | `1` |
    | Image width                       | `15` |
    | Image height                      | `15` |
    | Block data                        | different for every block |
    | Bit length for data in block      | `5` |
    | Data length (if using RLE)        | `block bit_len` |

    For the example quant matrix in the assignment, the header is 20.5 bytes of data.

- The en/decoder will give a compression percentage after writing the resulting file. (`< 100.0`: result is smaller, `> 100.0`: result is bigger )

- The Block size is provided as a Template argument and can be changed in Block.hpp.
  Everything should work as expected, only an 8x8 quant matrix is needed to continue.
  However, resulting images do not seem as good in comparison with a 4x4 block size.

- A single example is provided (the image from the iPyhton notebooks) for convenience.
  Other testing images used during development can be added on request.

### Extra
- The encoder now has an additional step to apply Huffman encoding on the final bitstream. This will also be decompressed by the decoder, if set in the encoded image. The inclusion of Huffman encoding can be toggled through the `ENABLE_HUFFMAN` macro in `main.hpp` or through the makefile.

    When enabled, the first bit of the resulting stream will be set to `1`. The following structure will appear instead of the encoded image:

    | Property                          | Amount of bits |
    |-----------------------------------|:--------------:|
    | Huffman encoding used             | `1` |
    | Huffman table header group        | `12` |
    | **[HDR]** Has sequence            | `1 of 12` |
    | **[HDR]** Sequence length         | `7 of 12` |
    | **[HDR]** Amount of bits for vals | `4 of 12` |
    | Sequence entry {key:val}          | `(8 + val_bit_len) * seq_len` |
    | Huffman encoded data              | rest |

    The occurrences of every byte in the stream are counted, and a heap is constructed to build a tree from. This tree is then converted to a dictionary containing keys and the paths to follow in the tree.
    After creating the Huffman dictionary, the frequencies of each path length are determined.
    `{key: value}`-pairs are grouped by their path length. 

    A header contains `1` bit to indicate whether there is a sequence, 7 bits for the length of the sequence (= amount of `{key: value}`-pairs until the next header) and 4 bits for the amount of bits for each value in this group.
    Afterwards, every byte (size can be changed with template arguments, but using 8 bits limits the
    Huffman dictionary to a manageable 256 entries) is encoded according to the Huffman dictionary.

    If the addition of Huffman encoding results in a bigger image than the already encoded image, the Huffman dictionary will not be included and the original encoded stream will be restored.

- An elapsed time in milliseconds is now provided after en/decoding.

- A progress bar indicates how many blocks are already done.

- By including the `-DENABLE_OPENMP` compiler flag, the code will build with OpenMP enabled (set in `main.hpp` or in the makefile). This causes Block functions to run in parallel (where possible) and improves the en/decoding speed. This is however not supported with MSVC, since the loop uses iterators and support for iterators came in OpenMP 3.0 while MSVC only implements features up to version 2.0.

- Timings and [test results](#Test-Results) were added below.

- A [class diagram](#Class-diagram) giving an overview of the code was generated through Visual Studio.

## Example
- Original image:

    ![Original image](doc/ex6.png)

- The decoder had an "error" where signed encoded data was interpreted as unsigned, and
  consequently every pixel that would contain signed data was wrongly transformed during iDCT, giving these weird uniform areas:

    ![Signed errors](doc/ex6_dec2.png)

- This was solved by providing a more specific function to check the minimal required bit length to represent a data value.

    e.g. The new function would check if a value could be encoded in x bits, by shifting the value to the left of an int16, and re-shifting it back (to create a signed value), if the result is the same as the original, the value can be encoded using x bits. If not, increase x until it does.
    This way the decoder could do the same thing, because it knows the bit length, and the original value is properly decoded (signed or unsigned).

    ![Final result](doc/ex6_dec.png)

## Test results
### Size statistics

| Example image | Resolution     | Raw size   | Encoded    | Ratio | Huffman    | Ratio |
|---------------|:--------------:|-----------:|-----------:|------:|-----------:|------:|
| ex0           | `8x8`          |        64b |        82b |  128% |        82b |  128% |
| ex1           | `936x936`      |   876 096b |   413 210b |   47% |   327 658b |   37% |
| ex2           | `512x512`      |   262 144b |   104 597b |   40% |    83 274b |   32% |
| ex3           | `400x400`      |   160 000b |    76 033b |   48% |    61 230b |   38% |
| ex4           | `4096x912`     | 3 735 552b | 1 834 256b |   49% | 1 473 058b |   39% |
| ex5           | `2160x2160`    | 4 665 600b | 1 598 931b |   34% | 1 369 376b |   29% |
| ex6           | `512x256`      |   131 072b |    42 198b |   32% |    34 191b |   26% |

### Timing statistics

| Example image | Resolution     | Enc Time | Dec Time | Huffman enc | Huffman dec | OpenMP enc | OpenMP dec |
|---------------|:--------------:|---------:|---------:|------------:|------------:|-----------:|-----------:|
| ex0           | `8x8`          |    7.0ms |    5.1ms |       8.0ms |       5.5ms |     11.9ms |      5.6ms |
| ex1           | `936x936`      |  239.8ms |  201.1ms |     266.4ms |     220.2ms |    126.0ms |     79.5ms |
| ex2           | `512x512`      |   74.8ms |   65.6ms |      85.7ms |      67.9ms |     43.0ms |     29.0ms |
| ex3           | `400x400`      |   49.0ms |   42.0ms |      57.7ms |      45.6ms |     29.5ms |     20.6ms |
| ex4           | `4096x912`     | 1019.0ms |  842.1ms |    1139.5ms |    851.26ms |    461.9ms |    327.3ms |
| ex5           | `2160x2160`    | 1241.7ms | 1060.8ms |    1334.7ms |    1046.7ms |    506.1ms |    373.1ms |
| ex6           | `512x256`      |   38.5ms |   33.8ms |      46.2ms |      35.3ms |     23.9ms |     17.4ms |

*Regular*, *With extra Huffman compression* and *Huffman with OpenMP*. All were determined on a machine with an `Intel i7 7700k` CPU with the default turbo boost enabled (3.5 to 4.90 GHz clock speed).

## Class diagram

![Class Diagram](doc/ClassDiagram.png)


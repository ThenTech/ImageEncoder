# Target specific options
# When compiling with "make encoder" or "make decoder",
# a symbol ENCODER or DECODER will be defined globally
# to signal the compiler which target code is expected.
ENCODER_TGT = encoder
ENCODER_DEF = -DENCODER
DECODER_TGT = decoder
DECODER_DEF = -DDECODER

# Extra options for encoder compilation
# -DENABLE_HUFFMAN : Enable additional Huffman compression step
# -DENABLE_OPENMP  : Enable Block parallelisation with OpenMP
ECFLAGS = -DENABLE_HUFFMAN -DENABLE_OPENMP

# Output folder for binaries
OUTPUT = ./bin

# Link libs such as: -pthread -lm
LIBS = -fopenmp

# Use a g++ executable with c++17 support (see install_g++.sh)
CC = g++-7

# Extra flags to strip unused symbols: -Wl,--strip-all,--gc-sections -fdata-sections -ffunction-sections
# Debug
# CFLAGS = $(ECFLAGS) -std=c++17 -Wall -Og -fopenmp
# Release
CFLAGS = $(ECFLAGS) -std=c++17 -Wall -O3 -Wl,--strip-all,--gc-sections -fdata-sections -ffunction-sections -fopenmp

# Default target
TARGET   = $(ENCODER_TGT)
CODE_DEF = $(ENCODER_DEF)

# Tools
createout = @mkdir -p $(OUTPUT)
cleanobj  = @-rm -f *.o

##################################################################

.PHONY: all default $(ENCODER_TGT) $(DECODER_TGT) clean

all:
	@$(MAKE) --no-print-directory $(ENCODER_TGT)
	@$(MAKE) --no-print-directory $(DECODER_TGT)

pre: clean
	$(createout)

default: pre compile
	$(cleanobj)

# Build encoder target
$(ENCODER_TGT): TARGET   = $(ENCODER_TGT)
$(ENCODER_TGT): CODE_DEF = $(ENCODER_DEF)
$(ENCODER_TGT): default

# Build decoder target
$(DECODER_TGT): TARGET   = $(DECODER_TGT)
$(DECODER_TGT): CODE_DEF = $(DECODER_DEF)
$(DECODER_TGT): default

##################################################################

# Look for .hpp/.cpp files to compile and link
OBJECTS = $(patsubst %.cpp,%.o, $(wildcard *.cpp))
HEADERS = $(wildcard *.hpp)

# Compile each .cpp file to its object
%.o: %.cpp $(HEADERS)
	@$(CC) $(CFLAGS) $(CODE_DEF) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

# Call compiler for linking
compile: $(OBJECTS)
	@$(CC) $(OBJECTS) -Wall $(CFLAGS) $(LIBS) -o $(OUTPUT)/$(TARGET)

# Clean targets
cleantg:
	@-rm -r -f $(OUTPUT)/$(ENCODER_TGT)
	@-rm -r -f $(OUTPUT)/$(DECODER_TGT)

# Clean all?
clean:
	$(cleanobj)

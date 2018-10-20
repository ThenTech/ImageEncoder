#include "Huffman.hpp"
#include <numeric>
#include <queue>

#include "utils.hpp"
#include "Logger.hpp"

////////////////////////////////////////////////////////////////////////////////
///  Private functions
////////////////////////////////////////////////////////////////////////////////

/**
 *  @brief  A comparator to sort Codeword pairs by bit length.
 */
struct CodewordComparator {
    bool operator()(const std::pair<uint8_t, algo::Codeword>& first,
                    const std::pair<uint8_t, algo::Codeword>& second)
    {
        return first.second.len > second.second.len;
    }
};

/**
 *  @brief  Add the given settings to the output stream according to the amount of bits
 *          specified in the Huffman class.
 *  @param  length
 *      The length of the sequence that will follow this header.
 *      If the length is 0, only one '0' bit will be written.
 *  @param  bit_length
 *      The amount of bits needed for every data element in the sequence following this header.
 *      Keys always use KEY_BITS as length, and values use bit_length, which is different for each group.
 *      This is done to minimize the amoutn of bits needed to save the Huffman dictionary.
 *  @param  writer
 *      The outputstream to write to.
 */
template<class T>
void algo::Huffman<T>::add_huffman_dict_header(uint32_t length, uint32_t bit_length, util::BitStreamWriter& writer) {
    if (length > 0) {
        writer.put(algo::Huffman<>::DICT_HDR_HAS_ITEMS_BITS + algo::Huffman<>::DICT_HDR_SEQ_LENGTH_BITS,
                   0x80 | (length & 0x7F)); // MSB is HAS_ITEMS setting + 7 bits length
        writer.put(algo::Huffman<>::DICT_HDR_ITEM_BITS,
                   bit_length & 0xF);       // 4 bits for bit length of every dict item
    } else {
        writer.put_bit(0);
    }
}

/**
 *  @brief  Read a dictionary header from the inputstream and set the given variables.
 *
 *  @param  reader
 *      The inputstream to read from.
 *  @param  length
 *      The length of the sequence that will follow this header (will be set).
 *  @param  bit_length
 *      The amount of bits for every value element in the following sequence (will be set).
 *
 *  @return Returns true if there is data after this header. (first bit was set)
 */
template<class T>
bool algo::Huffman<T>::read_huffman_dict_header(util::BitStreamReader& reader, uint32_t& length, uint32_t& bit_length) {
    if (reader.get_bit()) {
        length     = reader.get(algo::Huffman<>::DICT_HDR_SEQ_LENGTH_BITS);
        bit_length = reader.get(algo::Huffman<>::DICT_HDR_ITEM_BITS);
        return true;
    }

    return false;
}

/**
 *  @brief  Traverse Huffman tree starting from node and
 *          add codes for leafs to the dictionary.
 *  @param  node
 *      The starting node.
 *  @param  stream
 *      The current stream of bits for a path in the tree.
 */
template<class T>
void algo::Huffman<T>::buildDict(const algo::Node<> * const node, std::vector<bool> stream) {
    if (node == nullptr) {
        return;
    }

    // Check if leaf
    if (node->isLeaf()) {
        this->dict[node->data] = Codeword {
            // Concatenate the bits in stream to a value
            std::accumulate(stream.begin(), stream.end(), uint32_t(0u),
                            [=](uint32_t x, uint32_t y) { return (x << 1u) | y; }),
            // Get the amount of bits
            uint32_t(stream.size())
        };

        return;
    }

    std::vector<bool> lstream(stream);
    lstream.push_back(false);   // Go left  => '0'
    stream.push_back(true);     // Go right => '1'

    this->buildDict(node->left , lstream);
    this->buildDict(node->right, stream);
}

/**
 *  @brief  Read the dictionary from the given stream to build a Huffman tree structure.
 *          Clear the previous tree and overwrite with the data from the dict.
 *
 *          Optionally also save dict internally, but only tree is needed for
 *          proper decoding.
 *
 *          If first bit was '0', no key: val sequence follows and reader
 *          already contains uncompressed data.
 *
 *  @param  reader
 *      The stream to read from.
 */
template<class T>
void algo::Huffman<T>::buildTree(util::BitStreamReader& reader) {
    uint32_t dseq_len = 0u, dbit_len = 0u;

    util::deallocVar(this->tree_root);
    this->tree_root = util::allocVar<algo::Node<>>(-1);
    this->dict.clear();

    // While header is followed by sequence
    while (this->read_huffman_dict_header(reader, dseq_len, dbit_len)) {
        while (dseq_len--) {
            // For each element, read {key: val}
            const T        key = T(reader.get(algo::Huffman<>::KEY_BITS));
            const Codeword val { reader.get(dbit_len), dbit_len };

            const std::pair<T, algo::Codeword>& entry = std::make_pair(key, val);

            #if 0  // Optionally add to dict, but not necessary since decoding only needs tree.
                dict.insert(entry);
                // util::Logger::WriteLn(std::string_format("%02X: %8X (%d bits)", entry.first, entry.second.word, entry.second.len));
            #endif
            this->treeAddLeaf(entry);
        }
    }
}

/**
 *  @brief  Add a single <T, Codeword> leaf to the current tree.
 *
 *  @param  pair
 *      The <T, Codeword> pair to add.
 *      Follow Codeword.word from MSB to LSB and create new Nodes on the way,
 *      ending withe a Node that has the data T.
 */
template<class T>
void algo::Huffman<T>::treeAddLeaf(const std::pair<T, Codeword>& pair) {
    const size_t mask = (1 << (pair.second.len - 1));  // Mask the pair.second.len'th bit
          size_t dirs = pair.second.word;              // The directions to follow in the tree

    algo::Node<> *current = this->tree_root;

    // Grow the tree according to the dirs path, starting from MSB, except for last dir
    for (size_t bits = pair.second.len; --bits; dirs <<= 1) {
        // Follow the direction and create a dummy Node if none exists
        if (dirs & mask) {
            if (current->right == nullptr)
                current->right = util::allocVar<algo::Node<>>(-1);
            current = current->right;
        } else {
            if (current->left == nullptr)
                current->left = util::allocVar<algo::Node<>>(-1);
            current = current->left;
        }
    }

    // Create a new Node at the correct position (the last one in dirs)
    if (dirs & mask) {
        current->right = util::allocVar<algo::Node<>>(pair.first);
    } else {
        current->left  = util::allocVar<algo::Node<>>(pair.first);
    }
}

/**
 *  @brief  Traverse Huffman tree starting from node and
 *          decode symbols according to the dictionary.
 *  @param  node
 *      The starting node.
 *  @param  reader
 *      The bytestream to read from.
 */
template<class T>
void algo::Huffman<T>::decode(util::BitStreamReader& reader, util::BitStreamWriter& writer) {
    if (this->tree_root == nullptr) {
        return;
    }

    algo::Node<> *current = this->tree_root;

    while (!current->isLeaf()) {
        // Read next bit and go left or right untill a leaf is reached
        current = (reader.get_bit() ? current->right : current->left);
    }

    writer.put(algo::Huffman<>::KEY_BITS, current->data);
}

////////////////////////////////////////////////////////////////////////////////

/**
 *  @brief  Default ctor
 */
template<class T>
algo::Huffman<T>::Huffman(void) : tree_root(nullptr) {
    // Empty
}

/**
 *  @brief  Default dtor
 */
template<class T>
algo::Huffman<T>::~Huffman(void) {
    util::deallocVar(this->tree_root);
}

/**
 *  @brief  Encode bits of length sizeof(T) with Huffman encoding and
 *          write the Huffman dict and the encoded data to an outputstream.
 *
 *  @param  reader
 *      The bytestream to read from.
 *  @return Returns a new bitstream with the encoded data.
 */
template<class T>
util::BitStreamWriter* algo::Huffman<T>::encode(util::BitStreamReader& reader) {    
    const size_t length = reader.get_size() * 8u;

    // Calculate frequencies
    std::unordered_map<T, uint32_t> freqs;

    reader.reset();
    while(reader.get_position() != length) {
        const T word = T(reader.get(algo::Huffman<>::KEY_BITS));
        freqs[word]++;
    }

    // Create priority queue to sort tree with Nodes with data from frequency
    std::priority_queue<algo::Node<>*, std::vector<algo::Node<>*>, algo::Node<>::comparator> pq;

    for (const auto& pair: freqs) {
        pq.push(util::allocVar<algo::Node<>>(pair.first, pair.second));
        // util::Logger::WriteLn(std::string_format("%02X: %d", pair.first, pair.second), false);
    }

    while (pq.size() > 1) {
        // Empty out queue and build leaves, starting with lowest freq
        // Result is a single Node with references to other Nodes in tree structure.
        algo::Node<> *left  = pq.top(); pq.pop();
        algo::Node<> *right = pq.top(); pq.pop();

        pq.push(util::allocVar<algo::Node<>>(-1, left->freq + right->freq, left, right));
    }

    // Huffman tree root
    this->tree_root = pq.top();

    // Create dictionary by tree traversal
    this->buildDict(this->tree_root, std::vector<bool>());

    // Create new list with dict elements sorted by bit length for saving to stream
    std::vector<std::pair<uint8_t, algo::Codeword>> sorted_dict(this->dict.begin(), this->dict.end());

    // Sort the dictionary by value bit length
    std::sort(sorted_dict.begin(), sorted_dict.end(), CodewordComparator());

    // Determine frequencies of each bit length with {bit_length: freq}
    std::unordered_map<uint32_t, uint32_t> bit_freqs;
    for (const auto& w : sorted_dict) {
        bit_freqs[w.second.len]++;
    }

    // Calculate total needed length for dict
    size_t h_dict_total_length = (algo::Huffman<>::KEY_BITS * this->dict.size())  // Amount of bits needed for keys
                               + ((algo::Huffman<>::DICT_HDR_HAS_ITEMS_BITS + algo::Huffman<>::DICT_HDR_ITEM_BITS + algo::Huffman<>::DICT_HDR_SEQ_LENGTH_BITS)
                                  * bit_freqs.size())                             // Amount of bits for each header
                               + 1;                                               // Stop bit
    for (const auto& f : bit_freqs) {
        h_dict_total_length += f.first * f.second;  // Amount of bits for each header group
    }

    util::Logger::WriteLn(std::string_format("[Huffman] Table overhead with %d entries: %.1f bytes.",
                                             this->dict.size(), float(h_dict_total_length) / 8.0f));

    // Save the Huffman dictionary to a stream
    util::BitStreamWriter *writer = util::allocVar<util::BitStreamWriter>((h_dict_total_length + length) / 8 + 1);
    uint32_t seq_len = 0u, bit_len = 0u;

    // Add headers for each group of same length key:val pairs
    // and write them to the stream
    for (const auto& w : sorted_dict) {
        if (seq_len == 0) {
            // New group
            bit_len = w.second.len;
            seq_len = bit_freqs[bit_len];
            this->add_huffman_dict_header(seq_len, bit_len, *writer);
        }

        writer->put(algo::Huffman<>::KEY_BITS, w.first);  // Put Key
        writer->put(bit_len, w.second.word);              // Put Val
        seq_len--;
    }

    this->add_huffman_dict_header(0, 0, *writer);  // Stop bit

    // Encode
    reader.reset();
    while(reader.get_position() < length) {
        const T word = T(reader.get(algo::Huffman<>::KEY_BITS));
        const auto& pair = this->dict[word];
        writer->put(pair.len, pair.word);
    }

    const size_t original_length = reader.get_size();
    const size_t total_length    = writer->get_last_byte_position();

    util::Logger::WriteLn(std::string_format("[Huffman]         Encoded file size: %8d bytes", original_length));
    util::Logger::WriteLn(std::string_format("[Huffman]           Compressed size: %8d bytes  => Ratio: %.2f%%",
                                             total_length,
                                             float(total_length) / original_length * 100.0f));

    if (original_length < total_length) {
        util::Logger::WriteLn("[Huffman] No extra compression achieved, reverting stream to encoded.");
        util::deallocVar(writer);
        writer = util::allocVar<util::BitStreamWriter>(original_length);
        writer->put_bit(0);

        reader.reset();
        while(reader.get_position() < length) {
            writer->put(algo::Huffman<>::KEY_BITS, reader.get(algo::Huffman<>::KEY_BITS));
        }

        return writer;
    }

    return writer;
}

/**
 *  @brief  Read the Huffman dict from the stream and
 *          write the decoded data to an outputstream.
 *
 *  @param  reader
 *      The bytestream to read from.
 *  @return Returns true if current Node has higher frequency.
 */
template<class T>
util::BitStreamReader* algo::Huffman<T>::decode(util::BitStreamReader& reader) {
    this->buildTree(reader);

    const size_t raw_bits   = (reader.get_size() * 8u);
          size_t data_bits  = raw_bits - reader.get_position();
    const size_t data_bytes = util::round_to_byte(data_bits);

    if (this->tree_root->isLeaf()) {
        // No tree was build => No Huffman used, just use passthrough of buffer by setting pointer

        util::BitStreamReader *result = util::allocVar<util::BitStreamReader>(reader.get_buffer(),
                                                                              data_bytes);
        result->set_position(reader.get_position());

        util::Logger::WriteLn("[Huffman] No Huffman table present in file. Skipping decompression.");

        return result;
    } else {
        // Consume all other data, bit by bit and traverse Huffman tree to find word
        util::BitStreamWriter *writer = util::allocVar<util::BitStreamWriter>(data_bytes);

        while (reader.get_position() < raw_bits) {
            this->decode(reader, *writer);

            if (writer->get_position() + 16 > data_bits) {
                // Resize buffer if decompression reaches buffer size.
                data_bits = writer->resize() * 8u;
            }
        }

        const size_t original_length = reader.get_size();
        const size_t total_length    = writer->get_last_byte_position();

        util::BitStreamReader *result = util::allocVar<util::BitStreamReader>(writer->get_buffer(), total_length);

        // Transfer ownership of buffer from writer to result stream
        writer->set_managed(false);
        result->set_managed(true);

        util::Logger::WriteLn(std::string_format("[Huffman]           Input file size: %8d bytes", original_length));
        util::Logger::WriteLn(std::string_format("[Huffman]         Decompressed size: %8d bytes  => Ratio: %.2f%%",
                                                 total_length,
                                                 float(total_length) / original_length * 100.0f));
        util::deallocVar(writer);

        return result;
    }
}

template<class T>
void algo::Huffman<T>::printDict(void) {
    util::Logger::WriteLn("[Huffman] Dictionary:");

    for (const auto& pair : this->dict) {
        util::Logger::WriteLn(std::string_format("%02X: %8X (%d bits)", pair.first, pair.second.word,pair.second.len),
                              false);
    }
}

template<class T>
void algo::Huffman<T>::printTree(void) {
    util::Logger::WriteLn("[Huffman] Tree:");
    algo::Node<>::printTree(this->tree_root);
}

template class algo::Node<uint8_t>;
template class algo::Huffman<uint8_t>;

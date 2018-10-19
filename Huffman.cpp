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
        writer.put(algo::Huffman<T>::DICT_HDR_HAS_ITEMS_BITS + algo::Huffman<T>::DICT_HDR_SEQ_LENGTH_BITS,
                   0x80 | (length & 0x7F)); // MSB is HAS_ITEMS setting + 7 bits length
        writer.put(algo::Huffman<T>::DICT_HDR_ITEM_BITS,
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
        length     = reader.get(algo::Huffman<T>::DICT_HDR_SEQ_LENGTH_BITS);
        bit_length = reader.get(algo::Huffman<T>::DICT_HDR_ITEM_BITS);
        return true;
    }

    return false;
}

/**
 *  @brief  Deallocate every node in the given tree.
 *  @param  root
 *      The node to start with and delete its children.
 */
template<class T>
void algo::Huffman<T>::deleteTree(algo::Node<> *root) {
    if (root == nullptr) return;

    this->deleteTree(root->left);
    this->deleteTree(root->right);
    delete root;
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
    if (node->left == nullptr && node->right == nullptr) {
        this->dict[node->data] = Codeword {
            std::accumulate(stream.begin(), stream.end(), uint32_t(0u),
                            [=](uint32_t x, uint32_t y) { return (x << 1u) | y; }),
            uint32_t(stream.size())
        };
    }

    std::vector<bool> lstream(stream);
    lstream.push_back(false);
    stream.push_back(true);

    this->buildDict(node->left , lstream);
    this->buildDict(node->right, stream);
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
void algo::Huffman<T>::decode(const algo::Node<> * const node, util::BitStreamReader& reader,util::BitStreamWriter& writer) {
    if (node == nullptr) {
        return;
    }

    // Check if leaf
    if (node->left == nullptr && node->right == nullptr) {
        writer.put(algo::Huffman<>::KEY_BITS, node->data);
        return;
    }

    const uint8_t bit = reader.get_bit();

    if (bit) {
        this->decode(node->right, reader, writer);
    } else {
        this->decode(node->left , reader, writer);
    }
}

////////////////////////////////////////////////////////////////////////////////

template<class T>
algo::Huffman<T>::Huffman(void) {

}

template<class T>
algo::Huffman<T>::~Huffman(void) {
    this->deleteTree(this->tree_root);
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
    reader.set_position(0);

    while(reader.get_position() != length) {
        const T word = T(reader.get(algo::Huffman<>::KEY_BITS));
        freqs[word]++;
    }

    // Create priority queue to sort tree with Nodes with data from frequency
    std::priority_queue<algo::Node<>*, std::vector<algo::Node<>*>, algo::Node<>::comparator> pq;

    for (const auto& pair: freqs) {
        pq.push(util::allocVar<algo::Node<>>(pair.first, pair.second));

        util::Logger::WriteLn(std::string_format("%02X: %d", pair.first, pair.second), false);
    }

    while (pq.size() > 1) {
        // Empty out queue and build leaves, starting with lowest freq
        // Result is a single Node with references to other Nodes in tree structure.
        algo::Node<> *left  = pq.top(); pq.pop();
        algo::Node<> *right = pq.top(); pq.pop();

        pq.push(util::allocVar<algo::Node<>>(-1, left->freq + right->freq, left, right));
    }

    this->tree_root = pq.top();

    this->buildDict(this->tree_root, std::vector<bool>());

    // Create new list with dict elements sorted by bit length for saving to stream
    // Sort the dictionary by value bit length
    std::vector<std::pair<uint8_t, algo::Codeword>> sorted_dict(this->dict.begin(), this->dict.end());
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

    util::Logger::WriteLn(std::string_format("[Huffman] Dict{key:%d, val:*} for %d entries + hdr bits: %.1f total bytes.",
                                             algo::Huffman<>::KEY_BITS, this->dict.size(),
                                             float(h_dict_total_length) / 8.0f));

    //*** Save the Huffman dictionary to a stream ***//
    util::BitStreamWriter *writer = util::allocVar<util::BitStreamWriter>((h_dict_total_length + length) / 8 + 1);
    uint32_t seq_len = 0u, bit_len = 0u;

    // Add headers for each group of same length key:val pairs
    // and write them to the stream
    for (const auto& w : sorted_dict) {
        if (seq_len == 0) {
            // New group
            bit_len = w.second.len;
            seq_len = bit_freqs[bit_len];
            add_huffman_dict_header(seq_len, bit_len, *writer);
        }

        writer->put(algo::Huffman<>::KEY_BITS, w.first);  // Put Key
        writer->put(bit_len, w.second.word);              // Put Val
        seq_len--;
    }

    add_huffman_dict_header(0, 0, *writer);


    /*******************************************************************************/

    /*ori*/
    reader.set_position(0);
//    while(reader.get_position() != length) {
//        const T word = T(reader.get(algo::Huffman<>::KEY_BITS));
//        util::Logger::Write(std::string_format("%X", word), false);
//    }
    util::Logger::WriteLn(std::string_format(" (%d bytes)", length/8), false);

    /*encoded*/
    // Encode
    reader.set_position(0);
    while(reader.get_position() != length) {
        const T word = T(reader.get(algo::Huffman<>::KEY_BITS));
        writer->put(this->dict[word].len, this->dict[word].word); //TODO
    }

    /*encoded stream*/
    size_t len = writer->get_position() / 8;
//    for (size_t i = 0; i < len; i++) {
//        util::Logger::Write(std::string_format("%X", writer->get_buffer()[i]), false);
//    }
    util::Logger::WriteLn(std::string_format(" (%d bytes)", len), false);

    /*decoded*/
    util::BitStreamReader enc(writer->get_buffer(), (writer->get_position() / 8) + 1);

    // readDictFromStream(enc);
    uint32_t dseq_len = 0u, dbit_len = 0u;
    this->dict.clear();
//    this->deleteTree(this->tree_root);

    while(this->read_huffman_dict_header(enc, dseq_len, dbit_len)) { // While header is followed by sequence
        while (dseq_len--) { // For each element, read {key, val}
            this->dict[T(enc.get(algo::Huffman<>::KEY_BITS))] = Codeword { enc.get(dbit_len), dbit_len };
            // TODO Add element to tree
        }
    }

//    util::BitStreamWriter out(length/8);

//    while (enc.get_position() <=  enc.get_size() * 8u) {
//        this->decode(this->tree_root, enc, out);
//    } util::Logger::WriteLn("", false);

//    out.set_position(0);
//    for (size_t i = 0; i < out.get_size(); i++) {
//        util::Logger::Write(std::string_format("%X", out.get_buffer()[i]), false);
//    } util::Logger::WriteLn("", false);

//    util::Logger::WriteLn("", false);
//    this->printTree();
//    util::Logger::WriteLn("", false);

    util::Logger::WriteLn("", false);

    this->printDict();

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
util::BitStreamWriter* algo::Huffman<T>::decode(util::BitStreamReader& reader) {
    const size_t table_size = reader.get(algo::Huffman<>::KEY_BITS);    ///< Get table size
    const size_t data_bits  = reader.get_size() * 8u;                   ///< Amount of data bits

    
    // TODO if first bit is zero => no Huffman table => do nothing, just pass the stream back
    // use internal flag to enable Huffman, if disabled, write 1 zero to stream before data,
    // and later just call huffman.decode() (see TODO this TODO)
    
//    for (size_t i = 0; i < table_size; i++) {
//        this->dict[T(reader.get(algo::Huffman<>::KEY_BITS))] = Codeword { reader.get(entry_bits), 0u };
//    }


    // TODO Create tree from dict
//    this->deleteTree(this->tree_root);
//    this->tree_root = util::allocVar<Node<>>(-1, 0);


//    for (const auto& pair : this->dict) {
//        // Sink leaf
//        Node<> *leaf = util::allocVar<Node<>>(pair.first);
//    }




    util::BitStreamWriter *writer = util::allocVar<util::BitStreamWriter>(data_bits);

    // Consume all other data, bit by bit and traverse Huffman tree to find word in dict
    while (reader.get_position() <= data_bits) {
        this->decode(this->tree_root, reader, *writer);
    }

    return writer;
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
static void printNode(const algo::Node<T> * const node, std::string s) {
    if (node == nullptr) {
        return;
    }

    if (node->left == nullptr && node->right == nullptr) {
        util::Logger::WriteLn(s + std::string_format(" => %X", node->data), false);
        return;
    }

    printNode(node->left, s + "0");
    printNode(node->right, s + "1");
}

template<class T>
void algo::Huffman<T>::printTree(void) {
    util::Logger::WriteLn("[Huffman] Tree:");

    printNode(this->tree_root, "");
}

template class algo::Node<uint8_t>;
template class algo::Huffman<uint8_t>;

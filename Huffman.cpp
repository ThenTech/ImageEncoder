#include "Huffman.hpp"
#include <numeric>
#include <queue>

#include "utils.hpp"
#include "Logger.hpp"


template<class T>
algo::Huffman<T>::Huffman(void) {

}

template<class T>
algo::Huffman<T>::~Huffman(void) {
    this->deleteTree(this->tree_root);
}

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
size_t algo::Huffman<T>::buildDict(const algo::Node<> * const node, std::vector<bool> stream) {
    if (node == nullptr) {
        return 0u;
    }

    // Check if leaf
    if (node->left == nullptr && node->right == nullptr) {
        const uint32_t size = uint32_t(stream.size());

        this->dict[node->data] = Codeword {
            std::accumulate(stream.begin(), stream.end(), uint32_t(0u),
                            [=](uint32_t x, uint32_t y) { return (x << 1u) | y; }),
            size
        };

        return size;
    }

    std::vector<bool> lstream(stream);
    lstream.push_back(false);
    stream.push_back(true);

    return std::max(this->buildDict(node->left , lstream),
                    this->buildDict(node->right, stream));
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
    }

    while (pq.size() > 1) {
        // Empty out queue and build leaves, starting with lowest freq
        // Result is a single Node with references to other Nodes in tree structure.
        algo::Node<> *left  = pq.top(); pq.pop();
        algo::Node<> *right = pq.top(); pq.pop();

        pq.push(util::allocVar<algo::Node<>>(-1, left->freq + right->freq, left, right));
    }

    this->tree_root = pq.top();

    const size_t h_table_bits        = this->buildDict(this->tree_root, std::vector<bool>());
    const size_t h_dict_total_length = (algo::Huffman<>::KEY_BITS + h_table_bits)
                                            * this->dict.size()     // Every {key: val} pair
                                     + algo::Huffman<>::KEY_BITS    // Length of table itself
                                     + algo::Huffman<>::SIZE_BITS;  // Bits per value

    util::Logger::WriteLn(std::string_format("[Huffman] {key:%d, val:%d} for %d entries + %d hdr bits (%.1f total bytes).",
                                             algo::Huffman<>::KEY_BITS, h_table_bits, this->dict.size(),
                                             (algo::Huffman<>::KEY_BITS + algo::Huffman<>::SIZE_BITS),
                                             float(h_dict_total_length) / 8.0f));

    util::BitStreamWriter *writer = util::allocVar<util::BitStreamWriter>((h_dict_total_length + length) / 8 + 1);

    writer->put(algo::Huffman<>::KEY_BITS , uint32_t(this->dict.size()));   ///< Put table size
    writer->put(algo::Huffman<>::SIZE_BITS, uint32_t(h_table_bits));        ///< Put bit length of a table value

    for (const auto& pair : this->dict) {
        writer->put(algo::Huffman<>::KEY_BITS, pair.first);  // Put Key
        writer->put(h_table_bits, pair.second.word);         // Put Val
    }


    /*******************************************************************************/

    /*ori*/
    reader.set_position(0);
    while(reader.get_position() != length) {
        const T word = T(reader.get(algo::Huffman<>::KEY_BITS));
        util::Logger::Write(std::string_format("%X", word), false);
    } util::Logger::WriteLn(std::string_format(" (%d bytes)", length/8), false);

    /*encoded*/
    reader.set_position(0);
    while(reader.get_position() != length) {
        const T word = T(reader.get(algo::Huffman<>::KEY_BITS));
        util::Logger::Write(std::string_format("%X", this->dict[word]), false);

        writer->put(this->dict[word].len, this->dict[word].word); //TODO
    } util::Logger::WriteLn("", false);

    /*decoded*/
    util::BitStreamReader enc(writer->get_buffer(), (writer->get_position() / 8) + 1);
    size_t table_size = enc.get(algo::Huffman<>::KEY_BITS);
    size_t entry_bits = enc.get(algo::Huffman<>::SIZE_BITS);
    enc.set_position(enc.get_position() + (algo::Huffman<>::KEY_BITS + entry_bits) * table_size);

    util::BitStreamWriter out(length/8);

    while (enc.get_position() <=  enc.get_size() * 8u) {
        this->decode(this->tree_root, enc, out);
    } util::Logger::WriteLn("", false);

    out.set_position(0);
    for (size_t i = 0; i < out.get_size(); i++) {
        util::Logger::Write(std::string_format("%X", out.get_buffer()[i]), false);
    } util::Logger::WriteLn("", false);

    util::Logger::WriteLn("", false);
    this->printTree();
    util::Logger::WriteLn("", false);

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
    const size_t entry_bits = reader.get(algo::Huffman<>::SIZE_BITS);   ///< Get entry bit length
    const size_t data_bits  = reader.get_size() * 8u;                   ///< Amount of data bits

    for (size_t i = 0; i < table_size; i++) {
        this->dict[T(reader.get(algo::Huffman<>::KEY_BITS))] = Codeword { reader.get(entry_bits), 0u };
    }


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

#ifndef HUFFMAN_HPP
#define HUFFMAN_HPP

#include "BitStream.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace algo {

    /**
     *  @brief  Huffman Node class
     */
    template<class T=uint8_t>
    class Node {
        public:
            const T  data;
            size_t   freq;
            Node    *left;
            Node    *right;

            /**
             *  @brief  Default ctor
             *
             *  @param  data
             *      Initial data in this node.
             *  @param  freq
             *      Initial frequency of this data element.
             *  @param  left
             *      A reference to the left child.
             *  @param  right
             *      A reference to the right child.
             */
            Node(const T& data, size_t freq=1u, Node *left=nullptr, Node *right=nullptr)
                : data(data), freq(freq), left(left), right(right)
            {
                // Empty
            }

            /**
             *  @brief  Comparison operator.
             *          Nodes with lower frequency has a higher priority.
             *
             *  @param  first
             *      The first Node to compare with.
             *  @param  second
             *      The second Node to compare with.
             *  @return Returns true if current Node has higher frequency.
             */
            struct comparator {
                bool operator()(const Node * const first, const Node * const second) {
                    return first->freq > second->freq;
                }
            };
    };

    /**
     *  Data struct for Huffman dictionary entries.
     */
    struct Codeword {
        uint32_t word;
        uint32_t len;
    };

    /**
     *  @brief Huffman class
     */
    template<class T=uint8_t>
    class Huffman {
        private:
            algo::Node<> *tree_root;

            std::unordered_map<T, Codeword> dict;

            void add_huffman_dict_header(uint32_t, uint32_t, util::BitStreamWriter&);
            bool read_huffman_dict_header(util::BitStreamReader&, uint32_t&, uint32_t&);

            void buildDict(const algo::Node<> * const, std::vector<bool>);
            void decode(const algo::Node<> * const, util::BitStreamReader&, util::BitStreamWriter&);

            void deleteTree(algo::Node<>*);
        public:
            Huffman(void);
            ~Huffman(void);

            void readDictFromStream(util::BitStreamReader&);

            util::BitStreamWriter* encode(util::BitStreamReader&);
            util::BitStreamWriter* decode(util::BitStreamReader&);

            void printDict(void);
            void printTree(void);

            static constexpr size_t KEY_BITS  = util::size_of<T>(); ///< Bit length for keys in Huffman dict

            static constexpr size_t DICT_HDR_HAS_ITEMS_BITS  = 1u;  ///< Whether there are dictionary items following (bit length)
            static constexpr size_t DICT_HDR_SEQ_LENGTH_BITS = 7u;  ///< Amunt of bits to represent the length of following items
            static constexpr size_t DICT_HDR_ITEM_BITS       = 4u;  ///< Amunt of bits to represent the length of following items
    };

    extern template class Node<uint8_t>;
    extern template class Huffman<uint8_t>;
}

#endif // HUFFMAN_HPP

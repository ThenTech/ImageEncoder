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
    typedef struct {
        uint32_t word;
        uint32_t len;
    } Codeword;

    /**
     *  @brief Huffman class
     */
    template<class T=uint8_t>
    class Huffman {
        private:
            algo::Node<> *tree_root;

            std::unordered_map<T, Codeword> dict;

            size_t buildDict(const algo::Node<> * const, std::vector<bool>);
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

            static constexpr size_t KEY_BITS  = util::size_of<T>();
            static constexpr size_t SIZE_BITS = util::size_of<T>();
    };

    extern template class Node<uint8_t>;
    extern template class Huffman<uint8_t>;
}

#endif // HUFFMAN_HPP

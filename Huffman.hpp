#ifndef HUFFMAN_HPP
#define HUFFMAN_HPP

#include "BitStream.hpp"
#include "Logger.hpp"

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
            const T       data;     ///< The actual data in this node.
            const size_t  freq;     ///< The frequency at which this data occured.
            Node         *left;     ///< A reference to child on the left ('0')
            Node         *right;    ///< A reference to child on the right ('1')

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
             *  @brief  Default dtor
             *          Deleting children will implicitly delete entire tree
             *          by recursive dtor calls on every Node.
             */
            ~Node(void) {
                util::deallocVar(left);
                util::deallocVar(right);
            }

            /**
             *  @brief  Returns true if this Node has no children,
             *          and therefor is a leaf.
             */
            inline bool isLeaf(void) const {
                return this->left == nullptr && this->right == nullptr;
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

            /**
             *  @brief  Print a tree structure starting from the given Node.
             *  @param  node
             *      The Node to start from.
             *  @param  s
             *      A string representation of the tree path ('0' for left and '1' for right)
             */
            static void printTree(const algo::Node<> * const node, std::string s = "") {
                if (node == nullptr) {
                    return;
                }

                if (node->left == nullptr && node->right == nullptr) {
                    util::Logger::WriteLn(s + std::string_format(" => %X", node->data), false);
                    return;
                }

                printTree(node->left , s + "0");
                printTree(node->right, s + "1");
            }
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

            static void add_huffman_dict_header(uint32_t, uint32_t, util::BitStreamWriter&);
            static bool read_huffman_dict_header(util::BitStreamReader&, uint32_t&, uint32_t&);

            void buildDict(const algo::Node<> * const, std::vector<bool>);
            void buildTree(util::BitStreamReader&);
            void treeAddLeaf(const std::pair<T, Codeword>&);

            void decode(util::BitStreamReader&, util::BitStreamWriter&);

            void deleteTree(algo::Node<>*);

        public:
            Huffman(void);
            ~Huffman(void);

            util::BitStreamWriter* encode(util::BitStreamReader&);
            util::BitStreamReader* decode(util::BitStreamReader&);

            void printDict(void);
            void printTree(void);

            static constexpr size_t KEY_BITS = util::size_of<T>();  ///< Bit length for keys in Huffman dict

            static constexpr size_t DICT_HDR_HAS_ITEMS_BITS  = 1u;  ///< Whether there are dictionary items following (bit length)
            static constexpr size_t DICT_HDR_SEQ_LENGTH_BITS = 7u;  ///< Amunt of bits to represent the length of following items
            static constexpr size_t DICT_HDR_ITEM_BITS       = 4u;  ///< Amunt of bits to represent the length of following items
    };

    extern template class algo::Node<uint8_t>;
    extern template class algo::Huffman<uint8_t>;
}

#endif // HUFFMAN_HPP

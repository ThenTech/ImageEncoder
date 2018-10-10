#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <functional>
#include <vector>
#include <fstream>
#include <cctype>
#include <typeinfo>
#include <type_traits>
#include <sstream>
#include <memory>

#ifndef _MSC_VER
    #include <cxxabi.h>
#endif

#if __cplusplus < 201703L
#error A C++17 compiler is required!
#endif

#include "Exceptions.hpp"

namespace std {
    /**	\brief	Trim whitespace from the start of the given string (in-place).
     *
     *	\param	s
     *		A reference to the string to perform the operation.
     */
    [[maybe_unused]] static inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                [](int c) {return !std::isspace(c);}));
    }

    /**	\brief	Trim whitespace from the end of the given string (in-place).
     *
     *	\param	s
     *		A reference to the string to perform the operation.
     */
    [[maybe_unused]] static inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(),
                [](int c) {return !std::isspace(c);}).base(), s.end());
    }

    /**	\brief	Trim whitespace from both start and end of the given string (in-place).
     *
     *	\param	s
     *		A reference to the string to perform the operation.
     */
    [[maybe_unused]] static inline void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    /**	\brief	Transform the string contents to uppercase (within the current locale) (in-place).
     *
     *	\param	str
     *		A reference to the string to perform the operation.
     */
    [[maybe_unused]] static inline void strToUpper(string &str) {
        std::transform(str.begin(), str.end(), str.begin(),
            [](std::string::value_type ch) {
                return std::use_facet<std::ctype<std::string::value_type>>(std::locale()).toupper(ch);
            }
        );
    }

    /**	\brief	Transform the string contents to uppercase (within the current locale) (copying).
     *
     *	\param	str
     *		A copy of the string to perform the operation.
     */
    [[maybe_unused]] static inline std::string strToUppercase(string str) {
        std::strToUpper(str);
        return str;
    }

    /**	\brief	Replace all consecutive occurrences of the given char within the given string (in-place).
     *
     *	\param	str
     *		A reference to the string to replace the character.
     *	\param	ch
     *		The characters to replace.
     */
    [[maybe_unused]] static inline void strReplaceConsecutive(string &str, const char ch) {
        str.erase(std::unique(str.begin(), str.end(),
                                [&](const char lhs, const char rhs) {
                                    return (lhs == ch) && (lhs == rhs);
                                }
                             ), str.end());
    }

    /**	\brief	Replace all occurrences of from with to in the given std::string str.
     *
     *	\param	str
     *		A reference to the string to replace a substring.
     *	\param	from
     *		A reference to a string to replace.
     *	\param	to
     *		A reference to a string to replace with.
     */
    [[maybe_unused]] static inline void strReplaceAll(string &str, const string& from, const string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    /**	\brief	Returns the internal actual class name of the given object o.
     *
     *	**Uses __abi::__cxa_demangle__ which is part of <cxxabi.h> included in all GCC compilers.**
     *
     *	If GCC is not used, type2name will revert to typeid(o).name() instead.
     *
     *	\tparam	T
     *		The type of object to get the name demangled from.
     *	\param	o
     *		The object to demangle the name from.
     *	\return
     *		Returns the class name of o.
     */
    template <class T>
    [[maybe_unused]] static const string type2name(T const& o) {
        #ifdef _CXXABI_H
            char *demang = abi::__cxa_demangle(typeid(o).name(), nullptr, nullptr, nullptr);
            string s(demang);
            std::free(demang);
        #else
            string s(typeid(o).name());
        #endif

        std::strReplaceAll(s, "std::", "");  // Remove std:: from output
        std::strReplaceAll(s, "dc::" , "");  // Remove dc:: from output
        return s;
    }

    template<typename ... Type>
    std::string string_format(const std::string& format, Type ...args) {
        const size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
        unique_ptr<char[]> buf(new char[size]);

        std::snprintf(buf.get(), size, format.c_str(), args...);

        return std::string(buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
    }
}

namespace util {
    /**
     * \brief  Find First Set
     *         This function identifies the least significant index or position of the
     *         bits set to one in the word.
     *
     * \param  value
     *      Value to find least significant index
     * \retval bitIndex
     *      Index of least significat bit at one
     */
    [[maybe_unused]] static inline uint8_t ffs(uint32_t value ) {;
        return uint8_t(32 - __builtin_clz(value));
    }

    /**
     * @brief   Cast enum type to underlining data type.
     * @param   e
     *      The enum value to cast.
     */
    template <typename E>
    constexpr inline auto to_underlying(E e) noexcept {
        return static_cast<std::underlying_type_t<E>>(e);
    }

    /**	\brief
     *		Convert the given char* to a variable of type T.
     *		Use this method instead of the raw C functions: atoi, atof, atol, atoll.
     *
     *	\tparam	T
     *		The type of object to cast to.
     *	\param	buffer
     *		The character buffer to convert.
     *	\return
     *		Returns a variable of type T with the value as given in buffer.
     */
    template <class T>
    [[maybe_unused]] static inline T lexical_cast(const char* buffer) {
        T out;
        std::stringstream cast;

        if (std::strToUppercase(std::string(buffer)).substr(0, 2) == "0X")
                cast << std::hex << buffer;
        else	cast << buffer;

        if (!(cast >> out))
            throw Exceptions::CastingException(buffer, std::type2name(out));

        return out;
    }

    /**	\brief	Read the given file and return a pointer to a string containing its contents.
     *
     *	\param	filename
     *		The (path and) name of the file to read.
     *
     *	\return	Returns a string pointer.
     *
     *	\exception	FileReadException
     *		Throws FileReadException if the file could not be read properly.
     */
    [[maybe_unused]] static inline const std::string* readStringFromFile(const std::string &filename) {
        std::string *str = new std::string();
        std::fstream file(filename);

        try {
            file.seekg(0, std::ios::end);
            str->reserve(size_t(file.tellg()));
            file.seekg(0, std::ios::beg);

            str->assign((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
        } catch (...) {
            delete str;
            file.close();
            throw Exceptions::FileReadException(filename);
        }

        file.close();
        return str;
    }


    /**
     *	\brief	Read the given file and return a pointer to a list containing its contents in binary data (raw chars).
     *
     *	\param	filename
     *		The (path and) name of the file to read.
     *
     *	\return	std::vector<char>*
     *			A vector with buffer->size() bytes containing a program in raw binary.
     *			Print with cast to (unsigned char) for proper viewing.
     *
     *	\exception	FileReadException
     *		Throws FileReadException if the file could not be read properly.
     */
    [[maybe_unused]] static inline std::vector<uint8_t>* readBinaryFile(const std::string &filename) {
        std::ifstream file(filename, std::ifstream::binary | std::ifstream::ate);

        if (!file.good()) {
            file.close();
            throw Exceptions::FileReadException(filename);
        }

        std::vector<uint8_t> *v_buff = new std::vector<uint8_t>();

        try {
//            file.seekg(0, std::ios::end);
            v_buff->reserve(size_t(file.tellg()));
            file.seekg(0, std::ios::beg);
            v_buff->assign((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        } catch (...) {
            delete v_buff;
            file.close();
            throw Exceptions::FileReadException(filename);
        }

        file.close();
        return v_buff;
    }

    /**
     *	\brief	Write the given char buffer to the given file.
     *
     *	\param	filename
     *		The (path and) name of the file to write to (will be created if it does not exist).
     *	\param	buffer
     *		The char buffer to write to a file.
     *	\param	length
     *		The length of the given char buffer.
     *
     *	\exception	FileWriteException
     *		Throws FileWriteException if the file could not be written properly.
     */
    [[maybe_unused]] static inline void writeBinaryFile(const std::string &filename, const uint8_t* buffer, size_t length) {
        std::ofstream file(filename, std::ofstream::binary);

        try {
            file.write(reinterpret_cast<const char*>(buffer), std::streamsize(length));
        } catch (...) {
            file.close();
            throw Exceptions::FileWriteException(filename);
        }

        file.close();
    }

    /*
     *	Overloaded methods to allocate an array of T of size x, y, z.
     */
    /**	\brief	Allocate an object of type T on the heap using `new T()`.
     *          Any argument will be passed down to the ctor of T.
     *
     *	\tparam	T
     *		The type of object to allocate.
     *	\return
     *		A pointer to the newly allocated object.
     */
    template <class T, class ... Type>
    [[maybe_unused]] static inline T* allocVar(Type ... args) {
        return new T(args...);
    }

    /**	\brief	Deallocate an object of type T that was allocated using SysUtils::allocVar<T>().
     *
     *	\tparam	T
     *		The type of object to deallocate.
     *	\param	*v
     *		A pointer to the object to deallocate.
     */
    template <class T>
    [[maybe_unused]] static inline void deallocVar(T* v) {
        delete v;
    }

    /**	\brief	Allocate an array of objects of type T and length x on the heap using `new T[x]()`.
     *
     *	\tparam	T
     *		The type of object to allocate.
     *	\param	x
     *		The length of the array in the first dimension.
     *	\return
     *		A pointer to the newly allocated object.
     */
    template <class T>
    [[maybe_unused]] static inline T* allocArray(size_t x) {
        return new T[x]();
    }

    /**	\brief	Deallocate an array of type T that was allocated using SysUtils::allocArray<T>(size_t).
     *
     *	\tparam	T
     *		The type of object to deallocate.
     *	\param	*a
     *		A pointer to the object to deallocate.
     */
    template <class T>
    [[maybe_unused]] static inline void deallocArray(T* a) {
        delete[] a;
    }

    /**	\brief	Allocate y arrays of objects of type T and length x on the heap.
     *
     *	\tparam	T
     *		The type of object to allocate.
     *	\param	x
     *		The length of the array in the first dimension.
     *	\param	y
     *		The length of the array in the second dimension.
     *	\return
     *		A pointer to the newly allocated object.
     */
    template <class T>
    [[maybe_unused]] static inline T** allocArray(size_t x, size_t y) {
        T **arr = new T*[x];
        for(size_t i = 0; i < x; i++) arr[i] = util::allocArray<T>(y);
        return arr;
    }

    /**	\brief	Deallocate an array of type T that was allocated using SysUtils::allocArray<T>(size_t, size_t).
     *
     *	\tparam	T
     *		The type of object to deallocate.
     *	\param	**a
     *		A pointer to the object to deallocate.
     *	\param	y
     *		The length of the array in the second dimension.
     */
    template <class T>
    [[maybe_unused]] static inline void deallocArray(T** a, size_t y) {
        for(size_t i = 0; i < y; i++) util::deallocArray(a[i]);
        util::deallocArray(a);
    }

    /**	\brief	Allocate z arrays of y arrays of objects of type T and length x on the heap.
     *
     *	\tparam	T
     *		The type of object to allocate.
     *	\param	x
     *		The length of the array in the first dimension.
     *	\param	y
     *		The length of the array in the second dimension.
     *	\param	z
     *		The length of the array in the third dimension.
     *	\return
     *		A pointer to the newly allocated object.
     */
    template <class T>
    [[maybe_unused]] static inline T*** allocArray(size_t x, size_t y, size_t z) {
        T ***arr = new T**[x];
        for(size_t i = 0; i < x; i++) arr[i] = util::allocArray<T>(y, z);
        return arr;
    }

    /**	\brief	Deallocate an array of type T that was allocated using SysUtils::allocArray<T>(size_t, size_t, size_t).
     *
     *	\tparam	T
     *		The type of object to deallocate.
     *	\param	***a
     *		A pointer to the object to deallocate.
     *	\param	y
     *		The length of the array in the second dimension.
     *	\param	z
     *		The length of the array in the third dimension.
     */
    template <class T>
    [[maybe_unused]] static inline void deallocArray(T*** a, size_t y, size_t z) {
        for(size_t i = 0; i < z; i++) util::deallocArray(a[i], y);
        util::deallocArray(a);
    }

    /**	\brief	Deallocate a vector containing pointers to type T,
     *			that was allocated using SysUtils::allocVar<T>() and then filled by push_back(SysUtils::allocVar<T>()).
     *
     *	\tparam	T
     *		The type of pointer to an object inside the std::vector to deallocate.
     *	\param	*v
     *		A pointer to the object to deallocate.
     */
    template <class T>
    [[maybe_unused]] static inline void deallocVector(std::vector<T*> *v) {
        for (T *i : *v) util::deallocVar(i);
        util::deallocVar(v);
    }
}

#endif // UTILS_HPP
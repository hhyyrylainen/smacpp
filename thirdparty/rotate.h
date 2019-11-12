// Bit rotate from: https://gist.github.com/pabigot/7550454
// Slightly modified version
/* See http://blog.regehr.org/archives/1063
 */
#include <limits>
#include <type_traits>
#include <typeinfo>

template<typename T>
T rotateLeft(T value, unsigned int count)
{
    static_assert(std::is_integral<T>::value, "rotate of non-integral type");
    static_assert(!std::is_signed<T>::value, "rotate of signed type");
    constexpr unsigned int num_bits{std::numeric_limits<T>::digits};
    static_assert(
        0 == (num_bits & (num_bits - 1)), "rotate value bit length not power of two");
    constexpr unsigned int count_mask{num_bits - 1};
    const unsigned int mb{count & count_mask};
    using promoted_type = typename std::common_type<int, T>::type;
    using unsigned_promoted_type = typename std::make_unsigned<promoted_type>::type;
    return ((unsigned_promoted_type{value} << mb) |
            (unsigned_promoted_type{value} >> (-mb & count_mask)));
}

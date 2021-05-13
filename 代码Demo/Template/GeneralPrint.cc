#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <forward_list>
#include <tuple>
#include <list>
#include <unordered_set>
#include <unordered_map>

template<typename T1, typename T2>
std::ostream& operator<<(std::ostream &os, const std::pair<T1, T2> &p)
{
    return os << '<' << p.first << ", " << p.second << '>';
}


template<typename T>
void MagicPrintExpendTuple(std::ostream& os, T t)
{
    os << t << ">";
}

template<typename T, typename...Args>
void MagicPrintExpendTuple(std::ostream& os, T t, Args...args)
{
    os << t << ", ";
    MagicPrintExpendTuple(os, args...);
}

template<typename...Types, size_t...I>
void MagicPrintTupleImpl(std::ostream &os, const std::tuple<Types...> &tp, std::index_sequence<I...>)
{
    MagicPrintExpendTuple(os, std::get<I>(tp)...);
}

template<typename...Types>
std::ostream& operator<<(std::ostream &os, const std::tuple<Types...> &tp)
{
    os << '<';
    MagicPrintTupleImpl(os, tp, std::make_index_sequence<std::tuple_size_v<std::tuple<Types...>>>{});
    return os;
}


template<typename Container,
         typename = typename Container::iterator, 
         typename = typename std::enable_if_t<!std::is_constructible_v<Container, const char*>>, 
         typename = typename std::enable_if_t<!std::is_constructible_v<Container, const wchar_t*>>, 
         typename = typename std::enable_if_t<!std::is_constructible_v<Container, const char32_t*>>>
std::ostream& operator<<(std::ostream &os, const Container& container)
{
    if(container.empty())
    {
        os << "[]";
        return os;
    }

    os << '[';
    for(auto it = container.begin(); it != container.end(); ++it)
    {
        if(it != container.begin() && it != container.end())
        {
            os << ", ";
        }
        os << *it;
    }
    os << "]";
    return os;
}





int main(int argc, char const *argv[])
{
    // 序列容器
    std::vector<int> vec_int = {1, 2, 3, 4, 5, 6};
    std::vector<const char *> vec_str = {"Hello", "Wrold"};
    std::list<double> list_double = {1.2, 2.54, 3.8, 4.2, 5.9, 6.7};
    std::forward_list<int> forwardlist_int = {1, 2, 3, 4, 5, 6};

    // 关联容器
    std::set<size_t> sets_sizet = {12, 19};
    std::map<int, std::tuple<std::string, int, double>> mp = {{12, {"zhang", 12, 12.4}}, {13, {"heng", 12, 12.4}}};
    std::unordered_set<char> unorderset_char = {'a', 'b', 'd'}; 
    std::unordered_map<char, std::string> unordermap_char_str = {{'a', "apple"}, {'b', "balance"}, {'d', "dog"}}; 

    // tuple
    auto tp = std::make_tuple(12, 213.23, "zhang");

    // pair
    auto pr = std::make_pair("hello", 12);

    std::cout << "vec_int:              " << vec_int << std::endl;
    std::cout << "vec_str               " << vec_str << std::endl;
    std::cout << "list_double           " << list_double << std::endl;
    std::cout << "forwardlist_int       " << forwardlist_int << std::endl;

    std::cout << "sets_sizet            " << sets_sizet << std::endl;
    std::cout << "mp                    " << mp << std::endl;
    std::cout << "unorderset_char       " << unorderset_char << std::endl;
    std::cout << "unordermap_char_str   " << unordermap_char_str << std::endl;

    std::cout << "tuple                 " << tp << std::endl;
    std::cout << "pr                    " << pr << std::endl;
    return 0;
}
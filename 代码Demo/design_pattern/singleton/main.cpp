#include <iostream>
#include <thread>
#include <vector>
#include <condition_variable>
#include <map>
#include <queue>
#include <stack>

using namespace std;


// class singleton {

// private:
//     singleton();
//     singleton(singleton&) = delete;
//     singleton(singleton&&) = delete;
//     singleton& operator=(singleton&) = delete;
//     singleton& operator=(singleton&&) = delete;

// public:
//     singleton* get_instance() {
//         if(instance == nullptr) {
//             lock_guard<std::mutex> lock(singleton_mutex);
//             if(instance == nullptr) {
//                 instance = new singleton();
//             }
//         }
//         return instance;
//     }
//     static singleton* instance;
//     static std::mutex singleton_mutex;
// };

// singleton* singleton::instance = nullptr;
// std::mutex singleton::singleton_mutex;

// class singleton {

// private:
//     singleton();
//     singleton(singleton&) = delete;
//     singleton(singleton&&) = delete;
//     singleton& operator=(singleton&) = delete;
//     singleton& operator=(singleton&&) = delete;

// public:
//     singleton* get_instance() {
//         static singleton* instance = new singleton();
//         return instance;
//     };
// };


// class singleton {

// private:
//     singleton();
//     singleton(singleton&) = delete;
//     singleton(singleton&&) = delete;
//     singleton& operator=(singleton&) = delete;
//     singleton& operator=(singleton&&) = delete;

// public:
//     singleton* get_instance() {
//         static singleton instance;
//         return &instance;
//     };
// };

// template<typename T>
// class singleton_template {

// public:
//     static T* get_instance() {
//         static T instance;
//         return &instance;
//     }

// private:
//     singleton_template(singleton_template&) = delete;
//     singleton_template(singleton_template&&) = delete;
//     singleton_template& operator=(singleton_template&) = delete;
//     singleton_template& operator=(singleton_template&&) = delete;

// protected:
//     singleton_template(){};
// };

// class singleton : public singleton_template<singleton> {
//     friend class singleton_template<singleton>;
    
// private:
//     singleton(){};
// };


// template<typename T>
// class singleton_template {

// public:
//     static T* get_instance() {
//         static T instance{token()};
//         return &instance;
//     }

// private:
//     singleton_template(singleton_template&) = delete;
//     singleton_template(singleton_template&&) = delete;
//     singleton_template& operator=(singleton_template&) = delete;
//     singleton_template& operator=(singleton_template&&) = delete;

// protected:
//     singleton_template() = default;
//     struct token {};
// };

// class singleton : public singleton_template<singleton> {

// public:
//     singleton(token){};
// };

// class hack : public singleton {
// public:
//     static singleton* hackInstance() {
//         return new singleton(token());
//     };
// };


template<typename T>
class singleton_template : public T {

private:
    singleton_template() = default;
    singleton_template(singleton_template&) = delete;
    singleton_template& operator=(singleton_template&) = delete;
    singleton_template& operator=(singleton_template&&) = delete;

public:
    static singleton_template* get_instance() {
        static singleton_template instance;
        return &instance;
    }
};

class singleclass {

protected:
    singleclass() = default;
};

using singleclass_singleton = singleton_template<singleclass>;

int main() {
    singleclass_singleton* p1 = singleclass_singleton::get_instance();
    singleclass_singleton* p2 = singleclass_singleton::get_instance();
    if(p1 == p2)
        cout << "haha" << endl;
    else
        cout << "lala" << endl;
}
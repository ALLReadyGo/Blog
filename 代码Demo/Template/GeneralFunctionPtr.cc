#include <vector>
#include <iostream>
#include <list>
#include <set>
#include <unordered_set>
#include <deque>
#include <queue>
#include <algorithm>
#include <stack>
#include <cmath>
#include <functional>

using namespace std;

template<typename R, typename...Args>
class FunctorBridge
{
  public:
    virtual FunctorBridge* clone() const = 0;
    virtual ~FunctorBridge() = default;
    virtual R invoke(Args...) const = 0;
};

template<typename Fun, typename R, typename...Args>
class FunctorBridgeImpl : public FunctorBridge<R, Args...>
{
  public:
    template<typename FunctorFwd>
    FunctorBridgeImpl(FunctorFwd&& f)
      : f_(std::forward<FunctorFwd>(f))
    {
    }

    FunctorBridge<R, Args...>* clone() const override 
    {
        return new FunctorBridgeImpl(f_);
    }

    virtual R invoke(Args...args) const
    {
        f_(std::forward<Args>(args)...);
    }

    ~FunctorBridgeImpl() override = default;

  private:
    Fun f_;
};

template<typename Signature>
class FunctionPtr;

template<typename R, typename...Args>
class FunctionPtr<R(Args...)>
{
  private:
    FunctorBridge<R, Args...> *bridge_;

  public:
    FunctionPtr()
      : bridge_(nullptr)
    {
    }

    FunctionPtr(const FunctionPtr &other)
      : bridge_(nullptr)
    {
        if(other.bridge_ != nullptr)
          bridge_ = other.bridge_->clone();
    }
    
    FunctionPtr(FunctionPtr &other)
      : FunctionPtr(static_cast<const FunctionPtr&>(other))
    {
    }

    FunctionPtr(FunctionPtr &&other)
      : bridge_(other.bridge_)
    {
        bridge_ = nullptr;
    }

    template<typename F>
    FunctionPtr(F&& f)
      : bridge_(nullptr)
    {
        bridge_ = new FunctorBridgeImpl<std::decay_t<F>, R, Args...>(std::forward<F>(f));
    }

    ~FunctionPtr()
    {
        delete bridge_;
    }

    FunctionPtr& operator=(const FunctionPtr& other)
    {
        FunctionPtr tmp(other);
        swap(*this, tmp);
        return *this;
    }

    FunctionPtr& operator=(FunctionPtr&& other)
    {
        delete bridge_;
        bridge_ = other.bridge_;
        other.bridge_ = nullptr;
        return *this;
    }

    friend void swap(FunctionPtr& fp1, FunctionPtr& fp2)
    {
        swap(fp1.bridge_, fp2.bridge_);
    }

    explicit operator bool() const
    {
        return bridge_ == nullptr;
    }

    R operator()(Args...args) const
    {
        return bridge_->invoke(std::forward<Args>(args)...);
    }
};

int main(int argc, char const *argv[])
{
    FunctionPtr<void()> ptr([](){
        std::cout << "Hello World" << std::endl;
    });
    ptr();
    return 0;
}


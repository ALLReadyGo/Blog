#ifndef GC_PTR__H__
#define GC_PTR__H__

#include <utility>
#include <cstdlib>
#include <new>
#include <set>
#include <iostream>
#include <cassert>

struct GCRecord;
class GCBase;

template<typename T>
class gc_ptr;



extern void gcRegister(GCRecord* record);
extern void gcBind(void *ptr, void *address);
extern void gcRebind(void *ptr, void *oldAddress, void *newAddress);
extern void gcDisBind(void *ptr, void *Address);
extern void gcInit(size_t gcCeiling, size_t gcCollectCycle);
extern void gcDestory();

struct GCRecord {
    void* start;
    long lenth;
    GCBase *gcbase;
};

class GCBase {

    template<typename T>
    friend class gc_ptr;

    template<typename U, typename ...TArgs>
    friend gc_ptr<U> make_gc(TArgs&& ...args);

private:
    GCRecord record;

public:
    GCBase(){};
    virtual ~GCBase(){};
};



template<typename T>
class gc_ptr {

    template<typename U>
    friend class gc_ptr;

    template<typename U, typename ...TArgs>
    friend gc_ptr<U> make_gc(TArgs&& ...args);

    template<typename U1, typename U2>
    friend gc_ptr<U1> dynamic_gc_cast(gc_ptr<U2>& gptr);

    template<typename U1, typename U2>
    friend gc_ptr<U1> static_gc_cast(gc_ptr<U2>& gptr);

private:
    T* ptr = nullptr;
    void* address = nullptr;

    void *addressOf(T *tp) {
        if(tp == nullptr)
            return nullptr;
        
        GCBase *base = dynamic_cast<GCBase*>(tp);
        void *address = base->record.start;
        assert(address != nullptr);
        return address;
    }

    gc_ptr(T *tp) :
        ptr(tp) {
        address = addressOf(ptr);
        gcBind(this, address);
    };

    
public:
    gc_ptr() {};

    gc_ptr(const gc_ptr<T> &gptr) :
        ptr(gptr.ptr), address(gptr.address) {
        
        gcBind(this, address);
    };

    template<typename U>
    gc_ptr(const gc_ptr<U> &gptr) : 
        ptr(dynamic_cast<T*>(gptr.ptr)), address(gptr.address){
        
        gcBind(this, address);
    }

    gc_ptr(gc_ptr<T> &&gptr) :
        ptr(gptr.ptr), address(gptr.address) {
        
        gcDisBind(&gptr, gptr.address);
        gptr.ptr = nullptr;
        gptr.address = nullptr;

        gcBind(this, address);
    };

    gc_ptr<T>& operator=(const gc_ptr<T>& gptr) {

        gcRebind(this, address, gptr.address);
        ptr = gptr.ptr;
        address = gptr.address;
        return *this;
    }

    T* operator->() {
        return ptr;
    }

    operator bool()const
    {
        return ptr != nullptr;
    }

    ~gc_ptr() {

        gcDisBind(this, address);
    }
};


template<typename T, typename ...TArgs>
gc_ptr<T> make_gc(TArgs&& ...args) {

    void *memory = malloc(sizeof(T));
    T* ptrT = new(memory)T(std::forward<TArgs>(args)...);
    GCBase *ptrBase = dynamic_cast<GCBase*>(ptrT);
    assert(ptrBase != nullptr);

    ptrBase->record = {memory, sizeof(T), ptrBase};
    
    gcRegister(&(ptrBase->record));
    auto gptr = gc_ptr<T>(ptrT);
    return gptr;
};

template<typename T, typename U>
gc_ptr<T> dynamic_gc_cast(gc_ptr<U>& gptr) {
    T *ptr = dynamic_cast<T*>(gptr.ptr);
    return gc_ptr<T>(ptr);
};


template<typename T, typename U>
gc_ptr<T> static_gc_cast(gc_ptr<U>& gptr) {
    T *ptr = static_cast<T*>(gptr.ptr);
    return gc_ptr<T>(ptr);
};


#define ENABLE_GC			public virtual GCBase

#endif
#include <set>
#include <climits>
#include <vector>
#include <stdlib.h>
#include <mutex>
#include "gc_ptr.h"

struct GCHandle {
    GCRecord record;
    int count = 0;
    std::set<GCHandle*> reference;
    bool mark = false;
    static const int FINDNOTE = INT_MIN;
};

extern void gcRegister(GCRecord* record);
extern void gcBind(void *ptr, void *address);
extern void gcRebind(void *ptr, void *oldAddress, void *newAddress);
extern void gcDisBind(void *ptr, void *address);
extern void gcInit(size_t gcCeiling, size_t gcCollectCycle);
extern void gcDestory();


struct GCHandle_Compare_Helper{
    
    bool operator()(GCHandle *lhs, GCHandle *rhs) {
        if(lhs->count == GCHandle::FINDNOTE) {
            return lhs->record.start < rhs->record.start;
        } else if(rhs->count == GCHandle::FINDNOTE) {
            return (intptr_t)(lhs->record.start) + lhs->record.lenth <= (intptr_t)rhs->record.start;
        } else {
            return lhs->record.start < rhs->record.start;
        }
    };
};

std::set<GCHandle*, GCHandle_Compare_Helper>         GC_Container;
std::mutex                  GC_Mutex;
size_t                      GC_Ceiling;
size_t                      GC_Collect_Cycle;
size_t                      GC_Current_Size;
size_t                      GC_Last_Collect_Size;


static void handleAlloc(GCRecord* record) {

    GCHandle *handle = new GCHandle;
    handle->record = *record;
    GC_Container.insert(handle);
};

static GCHandle* findHandle(void *address) {

    GCHandle key;
    key.record.start = address;
    key.record.lenth = 0;
    auto fined = GC_Container.find(&key);
    return fined != GC_Container.end() ? *fined : nullptr;
};

static GCHandle* findParent(void *ptr) {

    GCHandle key;
    key.record.start = ptr;
    key.count = GCHandle::FINDNOTE;
    auto fined = GC_Container.find(&key);
    return fined != GC_Container.end() ? *fined : nullptr;
};

static void garbageCollect(std::vector<GCHandle*> &garbage){

    std::vector<GCHandle*> marking;
    
    for(auto it : GC_Container) {
        it->mark = false;
        if(it->count > 0) {
            it->mark = true;
            marking.push_back(it);
        }
    }

    for(size_t i = 0; i < marking.size(); ++i) {
        for(auto it : marking[i]->reference) {
            if(!it->mark) {
                it->mark = true;
                marking.push_back(it);
            }
        }
    }

    for(auto it = GC_Container.begin(); it != GC_Container.end();) {

        if(!(*it)->mark) {
            garbage.push_back(*it);
            GC_Container.erase(it++);
        } else {
            it++;
        }
    }
}


static void garbageDestory(std::vector<GCHandle*> &garbage){

    for(auto it : garbage) {
        GCBase *gb = it->record.gcbase;
        gb->~GCBase();
        free(it->record.start);
        
        GC_Current_Size -= it->record.lenth;
        delete it;
    }
    GC_Last_Collect_Size = GC_Current_Size;
}

static void forceGC() {

    // std::cout << "Before GC Size" << GC_Current_Size << std::endl;

    std::vector<GCHandle*> garbage;
    garbageCollect(garbage);
    garbageDestory(garbage);
    // std::cout << "After GC Size" << GC_Current_Size << std::endl;
};

static void connect(void *ptr, void *address) {

    if(address == nullptr)
        return;

    auto parent = findParent(ptr);
    if(parent == nullptr) {             // 栈指针指向
        auto handle = findHandle(address);
        if(handle != nullptr) {
            handle->count++;
        }
    } else {                            // 堆指针指向
        auto handle = findHandle(address);
        if(handle != nullptr) {
            parent->reference.insert(handle);
        }
    } 
}

static void disConnect(void *ptr, void *address) {

    if(address == nullptr)
        return;

    auto parent = findParent(ptr);
    if(parent == nullptr) {             // 栈指针指向
        auto handle = findHandle(address);
        if(handle != nullptr) {
            handle->count--;
        }
    } else {                            // 堆指针指向
        auto handle = findHandle(address);
        if(handle != nullptr) {
            parent->reference.erase(handle);
        }
    }
}

void gcRegister(GCRecord* record) {

    if(GC_Current_Size > GC_Ceiling) {
        forceGC();
    } else if(GC_Current_Size - GC_Last_Collect_Size> GC_Collect_Cycle) {
        forceGC();
    }
    std::lock_guard<std::mutex> lock(GC_Mutex);
    handleAlloc(record);
    GC_Current_Size += record->lenth;
}

void gcBind(void *ptr, void *address) {

    std::lock_guard<std::mutex> lock(GC_Mutex);
    connect(ptr, address);
}

void gcRebind(void *ptr, void *oldAddress, void *newAddress) {

    std::lock_guard<std::mutex> lock(GC_Mutex);
    disConnect(ptr, oldAddress);
    connect(ptr, newAddress);
}

void gcDisBind(void *ptr, void *address) {

    std::lock_guard<std::mutex> lock(GC_Mutex);
    disConnect(ptr, address);
}

void gcInit(size_t gcCeiling, size_t gcCollectCycle) {
    
    GC_Ceiling = gcCeiling;
    GC_Collect_Cycle = gcCollectCycle;
    GC_Current_Size = 0;
    GC_Last_Collect_Size = 0;
}

void gcDestory() {
    std::vector<GCHandle*> garbage;
    for(auto it : GC_Container) {
        garbage.push_back(it);
    }
    garbageDestory(garbage);
    GC_Container = {};
};
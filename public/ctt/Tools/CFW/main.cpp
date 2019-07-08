//
//  main.cpp
//  TestCppProject
//
//  Created by dika on 2019/7/8.
//  Copyright © 2019 dika. All rights reserved.
//

#include <iostream>

template <typename Type>
struct DefaultDeleter
{
    void operator()(Type* Object)
    {
        delete Object;
    }
};

class RCBase {
public:
    RCBase()
    :rc(1)
    ,wrc(1)
    {}
    
    int rc;
    int wrc;
    
    virtual void DestroyObject() = 0;
    
    virtual ~RCBase(){}
};

template <typename T>
struct TRemoveRef { typedef T Type; };

template <typename T>
typename TRemoveRef<T>::Type&& MoveTemp(T&& Obj)
{
    typedef typename TRemoveRef<T>::Type CastType;
    return (CastType&&)Obj;
}

template <typename ObjectType, typename DeleterType>
class RCWithDeleter : private DeleterType, public RCBase
{
public:
    RCWithDeleter(ObjectType* InObject, DeleterType&& Deleter)
    : DeleterType(MoveTemp(Deleter))
    , Object(InObject)
    {}
    
    virtual void DestroyObject() override
    {
        (*static_cast<DeleterType*>(this))(Object);
    }
    
    virtual ~RCWithDeleter()
    {
        DestroyObject();
    }
    
private:
    ObjectType* Object;
};

template <typename ObjectType>
RCBase* NewDefaultRCController(ObjectType* Object)
{
    return new RCWithDeleter<ObjectType, DefaultDeleter<ObjectType>>(Object, DefaultDeleter<ObjectType>());
}

template <bool Predicate, typename Result = void>
class TEnableIf;

template <typename Result>
class TEnableIf<true, Result>
{
public:
    typedef Result Type;
};

template <typename Result>
class TEnableIf<false, Result>
{
};

template <typename From, typename To>
struct TPointerIsConvertibleFromTo
{
public:
    enum { Value = 0 };
};

template <class ObjectType>
class TShareRef
{
public:
    template <typename OtherType, typename = typename TEnableIf<TPointerIsConvertibleFromTo<OtherType, ObjectType>::Value>::Type>
    TShareRef(OtherType* InObject)
    : Object(InObject)
    , SharedRC(NewDefaultRCController(InObject))
    {}
    
    ~TShareRef()
    {
        delete SharedRC;
    }
    
private:
    ObjectType* Object;
    RCBase* SharedRC;
};

///////////////////////////////////////////

class People
{
    
};

class Son : public People
{
    
};

template<> struct TPointerIsConvertibleFromTo<Son, People>
{
    enum{ Value = 1 };
};

template<> struct TPointerIsConvertibleFromTo<People, Son>
{
    enum{ Value = 0 };
};


int main(int argc, const char * argv[])
{
    // insert code here...
    std::cout << "Hello, World!\n";
    
    TShareRef<People> people = TShareRef<People>(new Son());
    //TShareRef<Son> son = TShareRef<Son>(new People());
    
    //printf("%p, %p\n", &people, &son);
    printf("%p\n", &people);
    
    return 0;
}

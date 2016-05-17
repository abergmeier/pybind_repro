
#include <cstddef>
#include <atomic>
#include <limits>
#include <type_traits>
#include <utility>

#include <boost/operators.hpp>

#include <pybind11/pybind11.h>

template<class T>
class IntrusivePtr
: boost::less_than_comparable<IntrusivePtr<T>>
, boost::equality_comparable<IntrusivePtr<T>>
, boost::less_than_comparable<IntrusivePtr<T>, void const*>
, boost::equality_comparable<IntrusivePtr<T>, void const*>
{
public:
    typedef T Element;

    IntrusivePtr() noexcept;
    IntrusivePtr(Element* ptr, bool addRef = true);

    IntrusivePtr(IntrusivePtr const&);
    IntrusivePtr(IntrusivePtr&&) noexcept;

    ~IntrusivePtr();

    IntrusivePtr& operator=(IntrusivePtr const&);
    IntrusivePtr& operator=(IntrusivePtr&&);

    IntrusivePtr& operator=(Element* ptr);

    void reset();
    void reset(Element* ptr, bool addRef = true);

    Element& operator*() const noexcept;
    Element* operator->() const noexcept;
    Element* get() const noexcept;
    Element* detach() noexcept;

    explicit operator bool() const noexcept;

    // *Looks at MSVC disapprovingly*
#if !defined(_MSC_VER) || _MSC_VER >= 1900
    template<class U, typename std::enable_if<std::is_convertible<T*, U*>::value>::type* = nullptr>
    operator IntrusivePtr<U>() const&;
    template<class U, typename std::enable_if<std::is_convertible<T*, U*>::value>::type* = nullptr>
    operator IntrusivePtr<U>() &&;
#else // #if !defined(_MSC_VER) || _MSC_VER >= 1900
    template<class U, typename std::enable_if<std::is_convertible<T*, U*>::value>::type* = nullptr>
    operator IntrusivePtr<U>() const;
#endif // #if !defined(_MSC_VER) || _MSC_VER >= 1900

    void swap(IntrusivePtr&) noexcept;

    bool operator==(IntrusivePtr const&) const;
    bool operator<(IntrusivePtr const&) const;
    bool operator==(void const*) const;
    bool operator<(void const*) const;
    bool operator>(void const*) const;
private:
    Element* ptr_;
};

template<class U, class T>
IntrusivePtr<U> staticCast(IntrusivePtr<T> const&);
template<class U, class T>
IntrusivePtr<U> staticCast(IntrusivePtr<T>&&);

template<class U, class T>
IntrusivePtr<U> constCast(IntrusivePtr<T> const&);
template<class U, class T>
IntrusivePtr<U> constCast(IntrusivePtr<T>&&);

template<class U, class T>
IntrusivePtr<U> reinterpretCast(IntrusivePtr<T> const&);
template<class U, class T>
IntrusivePtr<U> reinterpretCast(IntrusivePtr<T>&&);

template<class Derived>
class RefCounted
{
public:
    std::size_t getRefCount() const;
protected:
    RefCounted();
    RefCounted(RefCounted const&);
    RefCounted(RefCounted&&);

    RefCounted& operator=(RefCounted const&);
    RefCounted& operator=(RefCounted&&);
private:
    mutable std::atomic<std::size_t> refCount_;

    friend void intrusivePtrRetain(Derived const* const ptr)
    {
        if (nullptr != ptr)
        {
            std::size_t const oldRefCount = ptr->RefCounted<Derived>::refCount_.fetch_add(1);
            (void)oldRefCount; // Suppress warning when asserts are disabled.
        }
    }

    friend void intrusivePtrRelease(Derived const* const ptr)
    {
        if (nullptr != ptr)
        {
            auto const oldRefCount = ptr->RefCounted<Derived>::refCount_.fetch_sub(1);
            assert(0 < oldRefCount);
            if (1 == oldRefCount)
            {
                delete ptr;
            }
        }
    }
};

/// IMPLEMENTATION
template<class T>
inline IntrusivePtr<T>::IntrusivePtr() noexcept
: ptr_(nullptr)
{}

template<class T>
inline IntrusivePtr<T>::IntrusivePtr(Element* const ptr, bool const addRef)
: ptr_(ptr)
{
    if(addRef)
    {
        intrusivePtrRetain(this->ptr_);
    }
}

template<class T>
inline IntrusivePtr<T>::IntrusivePtr(IntrusivePtr const& other)
: ptr_(other.ptr_)
{
    intrusivePtrRetain(this->ptr_);
}

template<class T>
inline IntrusivePtr<T>::IntrusivePtr(IntrusivePtr&& other) noexcept
: ptr_(other.detach())
{}

template<class T>
inline IntrusivePtr<T>::~IntrusivePtr()
{
    intrusivePtrRelease(this->ptr_);
}

template<class T>
inline auto IntrusivePtr<T>::operator=(IntrusivePtr const& rhs) -> IntrusivePtr&
{
    this->reset(rhs.ptr_);
    return *this;
}

template<class T>
inline auto IntrusivePtr<T>::operator=(IntrusivePtr&& rhs) -> IntrusivePtr&
{
    this->reset(rhs.detach(), false);
    return *this;
}

template<class T>
inline auto IntrusivePtr<T>::operator=(Element* const rhs) -> IntrusivePtr&
{
    this->reset(rhs);
    return *this;
}

template<class T>
inline void IntrusivePtr<T>::reset()
{
    intrusivePtrRelease(this->ptr_);
    this->ptr_ = nullptr;
}

template<class T>
inline void IntrusivePtr<T>::reset(Element* const ptr, bool const addRef)
{
    if (addRef)
    {
        intrusivePtrRetain(ptr);
    }
    intrusivePtrRelease(this->ptr_);
    this->ptr_ = ptr;
}

template<class T>
inline auto IntrusivePtr<T>::operator*() const noexcept -> Element&
{
    assert(nullptr != this->ptr_);
    return *this->ptr_;
}

template<class T>
inline auto IntrusivePtr<T>::operator->() const noexcept -> Element*
{
    return this->ptr_;
}

template<class T>
inline auto IntrusivePtr<T>::get() const noexcept -> Element*
{
    return this->ptr_;
}

template<class T>
inline auto IntrusivePtr<T>::detach() noexcept -> Element*
{
    auto const result = this->ptr_;
    this->ptr_ = nullptr;
    return result;
}

template<class T>
inline IntrusivePtr<T>::operator bool() const noexcept
{
    return nullptr != this->ptr_;
}

template<class T>
template<class U, typename std::enable_if<std::is_convertible<T*, U*>::value>::type*>
inline IntrusivePtr<T>::operator IntrusivePtr<U>() const&
{
    return IntrusivePtr<U>(this->ptr_);
}

template<class T>
template<class U, typename std::enable_if<std::is_convertible<T*, U*>::value>::type*>
inline IntrusivePtr<T>::operator IntrusivePtr<U>() &&
{
    return IntrusivePtr<U>(this->detach(), false);
}

template<class T>
inline void IntrusivePtr<T>::swap(IntrusivePtr& other) noexcept
{
    std::swap(this->ptr_, other.ptr_);
}

template<class T>
inline bool IntrusivePtr<T>::operator==(IntrusivePtr const& rhs) const
{
    return this->ptr_ == rhs.ptr_;
}

template<class T>
inline bool IntrusivePtr<T>::operator<(IntrusivePtr const& rhs) const
{
    return this->ptr_ < rhs.ptr_;
}

template<class T>
inline bool IntrusivePtr<T>::operator==(void const* const rhs) const
{
    return this->ptr_ == rhs;
}

template<class T>
inline bool IntrusivePtr<T>::operator<(void const* const rhs) const
{
    return this->ptr_ < rhs;
}

template<class T>
inline bool IntrusivePtr<T>::operator>(void const* const rhs) const
{
    return this->ptr_ > rhs;
}

template<class U, class T>
inline IntrusivePtr<U> staticCast(IntrusivePtr<T> const& iptr)
{
    return IntrusivePtr<U>(static_cast<U*>(iptr.get()));
}

template<class U, class T>
inline IntrusivePtr<U> staticCast(IntrusivePtr<T>&& iptr)
{
    return IntrusivePtr<U>(static_cast<U*>(iptr.detach()), false);
}

template<class U, class T>
inline IntrusivePtr<U> constCast(IntrusivePtr<T> const& iptr)
{
    return IntrusivePtr<U>(const_cast<U*>(iptr.get()));
}

template<class U, class T>
inline IntrusivePtr<U> constCast(IntrusivePtr<T>&& iptr)
{
    return IntrusivePtr<U>(const_cast<U*>(iptr->detach()), false);
}

template<class U, class T>
inline IntrusivePtr<U> reinterpretCast(IntrusivePtr<T> const& iptr)
{
    return IntrusivePtr<U>(reinterpret_cast<U*>(iptr.get()));
}

template<class U, class T>
inline IntrusivePtr<U> reinterpretCast(IntrusivePtr<T>&& iptr)
{
    return IntrusivePtr<U>(reinterpret_cast<U*>(iptr.detach()), false);
}

template<class Derived>
inline std::size_t RefCounted<Derived>::getRefCount() const
{
    return this->refCount_.load();
}

template<class Derived>
inline RefCounted<Derived>::RefCounted()
: refCount_(0)
{}

template<class Derived>
inline RefCounted<Derived>::RefCounted(RefCounted const&)
: refCount_(0)
{}

template<class Derived>
inline RefCounted<Derived>::RefCounted(RefCounted&&)
: refCount_(0)
{}

template<class Derived>
inline auto RefCounted<Derived>::operator=(RefCounted const&) -> RefCounted&
{
    return *this;
}

template<class Derived>
inline auto RefCounted<Derived>::operator=(RefCounted&&) -> RefCounted&
{
    return *this;
}

PYBIND11_DECLARE_HOLDER_TYPE(T, IntrusivePtr<T>);

struct Foo : public RefCounted<Foo> {
    int id() const { return 0; };
    void setId(int) { };
};

struct Bar : public Foo {
	static IntrusivePtr<Bar> construct() {
		return IntrusivePtr<Bar>(new Bar());
	}
};

namespace py = pybind11;

PYBIND11_PLUGIN(testme) {
    py::module m("testme");

    py::class_<Foo, IntrusivePtr<Foo>>(m, "Foo")
        .def_property("id", &Foo::id, &Foo::setId);

    py::class_<Bar, IntrusivePtr<Bar>>(m, "Bar", py::base<Foo>())
        .def(py::init<>())
        .def_static("construct", &Bar::construct)
    ;
    return m.ptr();
}

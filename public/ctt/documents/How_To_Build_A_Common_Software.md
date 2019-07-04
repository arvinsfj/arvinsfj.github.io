
# 如何编写更加通用的软件

> *作者：Arvin 日期：2019年07月01日*

---------------------------------

>BEGIN

今年伊始，一直在做C++三维渲染方面的工作。最近偶尔遇到了irrlitch（鬼火）引擎。这款3D渲染引擎比较精简适合个人研究，但不太适合用于产品环境，而且比较老旧没有适配最新的3d图形API（Vulkan、metal和DX12）。但是它的代码写的是真的好，个人很有兴趣去学习和研究，或者修改。本篇主要着眼于如何编写比较通用的跨平台的软件（框架）的方法。

### 一、思考
---------------------------------

软件适配平台主要存在2个方面，一个是平台相关的数据类型，其次是平台相关的api。具体来讲，主要是编译器和SDK的不同。编程语言的差异相对来讲比较小。

跨平台性是程序通用性的重要方面，其次是编程语言方面，最后是架构可扩展、模块可配置和算法的通用性。通用性好的程序应该在这些方面达到一个较好的水平。

程序通用性的大概思路是：程序中从平台的数据类型和API两个方面屏蔽平台的差异性。在C/C++中（后面使用C++进行分析），可以使用宏定义来配置和屏蔽平台的特性。

后面我们的使用C++和Irrlitch进行分析和说明。

### 二、技巧
---------------------------------

为什么使用C++呢？C++有几个非常重要的语言特性：支持类、有模版、可重载函数运算符和命名空间。类可以说是对操作api的抽象，模版是对数据类型的抽象，可重载函数提供了一组类似函数的抽象，命名空间提供了程序功能模块化的基础。并且，C++的这些抽象在使用正确的情况下并大幅降低程序的性能。因为C++提供了这些抽象性，使得程序的通用性比较强。

在C++11之前，C++是没有内存管理机制的，我们使用C++进行系统编程需要考虑通用的内存管理机制。C++使用include进行头文件包含，需要考虑重复包含的尴尬避免编译出错。尽可能不使用平台相关的第三方库。采用二级命名空间进行软件功能模块化。通用算法采用泛型编程，避免数据类型太具体导致的算法不通用（模板机制的核心思想：容器、迭代器和算法）。进行OOP编程，面向接口编程（软件设计采用抽象类```[接口]```），尽量不使用全局变量，全局的枚举类型，共用类型等，尽可能分配到合适的类或者结构体中（职责明确）。采用重载运算符方式简化方法调用（但是提高了代码阅读的难度，尽量让运算符跟所要表达的运算内涵一致）。异常处理需要考虑。

我们的生活是具体的，但是文学作品却是抽象的，所以它可以让更多的人产生共鸣。具体类型是模板类的特例，模板类型通用性更强，能适配的数据类型更多。模板类或者模板函数让我们更加关注功能算法本身，而非运算中的数据类型。比如：求两个数的较大值，这个数可能是浮点型也可能是整型，而这个求较大值的算法却是一样的，模板函数提供了这种通用性。

C++虽然很古老，但是提供了大量面向抽象性编程的特征。从最基础的条件宏编译，变量，指针，函数重载（运算符重载），到类封装和抽象类，再到泛型编程（支持类型模板），stl库，类型推断，lambda表达式，线程（这个可能不算），智能指针（内置引用计数内存管理机制）等。如果能提高功能的抽象性，它也就更加通用。模块化更多的是跟可配置可扩展相关。


```
//! WIN32 for Windows32
//! WIN64 for Windows64
// The windows platform and API support SDL and WINDOW device
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#define _IRR_WINDOWS_
#define _IRR_WINDOWS_API_
#define _IRR_COMPILE_WITH_WINDOWS_DEVICE_
#endif

//! WINCE is a very restricted environment for mobile devices
#if defined(_WIN32_WCE)
#define _IRR_WINDOWS_
#define _IRR_WINDOWS_API_
#define _IRR_WINDOWS_CE_PLATFORM_
#define _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#  error "Only Microsoft Visual Studio 7.0 and later are supported."
#endif

// XBox only suppots the native Window stuff
#if defined(_XBOX)
	#undef _IRR_WINDOWS_
	#define _IRR_XBOX_PLATFORM_
	#define _IRR_WINDOWS_API_
	//#define _IRR_COMPILE_WITH_WINDOWS_DEVICE_
	#undef _IRR_COMPILE_WITH_WINDOWS_DEVICE_
	//#define _IRR_COMPILE_WITH_SDL_DEVICE_

	#include <xtl.h>
#endif

#if defined(__APPLE__) || defined(MACOSX)
#if !defined(MACOSX)
#define MACOSX // legacy support
#endif
#define _IRR_OSX_PLATFORM_
#define _IRR_COMPILE_WITH_OSX_DEVICE_
#endif

```

上面的这段宏定义屏蔽了平台的差异性（在功能模块不直接使用原始的平台宏而是使用Irr的自定义宏来表示相同的内涵），同时提供了平台特征的配置性（根据宏定义来确定需要编译的代码，提供了跨平台编译的可能）。

```
#ifndef __IRR_TYPES_H_INCLUDED__
#define __IRR_TYPES_H_INCLUDED__

// code ...

#endif

```

上面的条件宏格式，避免了重复包含头文件的尴尬。

```
//! 8 bit unsigned variable.
/** This is a typedef for unsigned char, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef unsigned __int8		u8;
#else
typedef unsigned char		u8;
#endif

//! 8 bit signed variable.
/** This is a typedef for signed char, it ensures portability of the engine. */
#if defined(_MSC_VER) || ((__BORLANDC__ >= 0x530) && !defined(__STRICT_ANSI__))
typedef __int8			s8;
#else
typedef signed char		s8;
#endif

```

根据编译器的不同，采用不同的类型定义，后面功能模块使用的是相同自定义类型（具有统一内涵的类型），它屏蔽了不同编译器类型定义的尴尬。

```
#include <wchar.h>
#ifdef _IRR_WINDOWS_API_
//! Defines for s{w,n}printf because these methods do not match the ISO C
//! standard on Windows platforms, but it does on all others.
//! These should be int snprintf(char *str, size_t size, const char *format, ...);
//! and int swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...);
#if defined(_MSC_VER) && _MSC_VER > 1310 && !defined (_WIN32_WCE)
#define swprintf swprintf_s
#define snprintf sprintf_s
#elif !defined(__CYGWIN__)
#define swprintf _snwprintf
#define snprintf _snprintf
#endif

```

根据之前的宏定义配置，如果是win平台的api则将不同的函数名称修改成一致的函数名称（跟linux函数名称一致）。屏蔽了平台api名称的差异。

```
#ifndef __IRR_POSITION_H_INCLUDED__
#define __IRR_POSITION_H_INCLUDED__

#include "vector2d.h"

namespace irr
{
namespace core
{

// Use typedefs where possible as they are more explicit...

//! \deprecated position2d is now a synonym for vector2d, but vector2d should be used directly.
typedef vector2d<f32> position2df;

//! \deprecated position2d is now a synonym for vector2d, but vector2d should be used directly.
typedef vector2d<s32> position2di;
} // namespace core
} // namespace irr

// ...and use a #define to catch the rest, for (e.g.) position2d<f64>
#define position2d vector2d

#endif // __IRR_POSITION_H_INCLUDED__

```

采用了二级命名空间的模块化设计。irr可以理解成整个软件系统，core可以理解成核心模块。正常的软件系统应该都可以采用这种模块划分的方式。同时注意这种编程方式。


```
//! some standard function ( to remove dependencies )
#undef isdigit
#undef isspace
#undef isupper
inline s32 isdigit(s32 c) { return c >= '0' && c <= '9'; }
inline s32 isspace(s32 c) { return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v'; }
inline s32 isupper(s32 c) { return c >= 'A' && c <= 'Z'; }

```

inline的使用可以避免函数调用的消耗（直接将实现代码内嵌到该函数调用的地方，类似于代码宏）。还有就是参数和返回值数据类型的使用，全部采用之前自定义的数据类型（因为自定义的数据类型通用性更好，或者说抽象性更高）。最后注意一下，字符常量在编译器内部是数值类型的，是可以跟s32类型比较大小的。

```
class ITimer : public virtual IReferenceCounted
{
    // code ...
}

```

首先，看类的名字，使用I开头表示接口（或者说是抽象类，它包含了纯虚函数定义）。可以说该类定义是设计阶段编写的，用于表达软件设计的思路（需要那些api）。其次需要看一下使用了virtual继承方式，IReferenceCounted（对象引用计数机制的提供类）是大部分Irr类（构件对象？）需要继承的，为了避免C++多重继承导致的基类API的歧义，使用了virtual关键字，所有子类只使用或者保留基类的API实现。


```
void grab() const { ++ReferenceCounter; }

bool drop() const
{
	// someone is doing bad reference counting.
	_IRR_DEBUG_BREAK_IF(ReferenceCounter <= 0)

	--ReferenceCounter;
	if (!ReferenceCounter)
	{
		delete this;
		return true;
	}

	return false;
}

s32 getReferenceCount() const
{
	return ReferenceCounter;
}

```

上面三个函数是IReferenceCounted类公开的引用计数机制的API。任何继承了IReferenceCounted类的子类都具有引用计数内存管理功能。引用计数机制简单来讲就是通过一个变量的计数值来确定是否释放该对象。如果计数值为0（表示当前类实体没有被任何其他实体引用，也就是无用的类实体）则释放该对象。当然创建该实体的时候，计数值为1。C++11在语言级别提供该种内存管理机制，语法上更好。引用计数机制有一个坑：循环引用导致相互引用的对象都不能释放，解决方式是引入weak引用。

```

#ifndef __IRR_DIMENSION2D_H_INCLUDED__
#define __IRR_DIMENSION2D_H_INCLUDED__

#include "irrTypes.h"
#include "irrMath.h" // for irr::core::equals()

namespace irr
{
namespace core
{
	template <class T>
	class vector2d;

    //! Specifies a 2 dimensional size.
	template <class T>
	class dimension2d
	{
		public:
			//! Default constructor for empty dimension
			dimension2d() : Width(0), Height(0) {}
			//! Constructor with width and height
			dimension2d(const T& width, const T& height)
				: Width(width), Height(height) {}

			dimension2d(const vector2d<T>& other); // Defined in vector2d.h

			//! Use this constructor only where you are sure that the conversion is valid.
			template <class U>
			explicit dimension2d(const dimension2d<U>& other) :
				Width((T)other.Width), Height((T)other.Height) { }

			template <class U>
			dimension2d<T>& operator=(const dimension2d<U>& other)
			{
				Width = (T) other.Width;
				Height = (T) other.Height;
				return *this;
			}

			//! Equality operator
			bool operator==(const dimension2d<T>& other) const
			{
				return core::equals(Width, other.Width) &&
						core::equals(Height, other.Height);
			}

            // code ...

            //! Width of the dimension.
			T Width;
			//! Height of the dimension.
			T Height;
	};

    //! Typedef for an f32 dimension.
	typedef dimension2d<f32> dimension2df;
	//! Typedef for an unsigned integer dimension.
	typedef dimension2d<u32> dimension2du;

	//! Typedef for an integer dimension.
	/** There are few cases where negative dimensions make sense. Please consider using
		dimension2du instead. */
	typedef dimension2d<s32> dimension2di;

} // end namespace core
} // end namespace irr

```

上面是模板类的定义，其中抽象了Width和Height的成员变量的类型。并且在最后使用typedef关键字定义了特定几个类型的具体类。这个重复体现了模板类的类型上的抽象性，它具有更强的通用性。还有就是运算符重载的使用，大量简化了功能调用上简洁性（增大了阅读难度，因为同一种运算符可能内涵和实现不太一样）。


```
//! Sorts an array with size 'size' using heapsort.
template<class T>
inline void heapsort(T* array_, s32 size)
{
	// for heapsink we pretent this is not c++, where
	// arrays start with index 0. So we decrease the array pointer,
	// the maximum always +2 and the element always +1

	T* virtualArray = array_ - 1;
	s32 virtualSize = size + 2;
	s32 i;

	// build heap

	for (i=((size-1)/2); i>=0; --i)
		heapsink(virtualArray, i+1, virtualSize-1);

	// sort array, leave out the last element (0)
	for (i=size-1; i>0; --i)
	{
		T t = array_[0];
		array_[0] = array_[i];
		array_[i] = t;
		heapsink(virtualArray, 1, i + 1);
	}
}

```

模板函数的使用，更加关注算法实现而不是被算法操作数据的数据类型。上面的排序算法可以对任何类型（元素类型）的数组进行排序，而不是特定的数据类型。关于这点可以去看STL库的实现代码，里面应该充分说明了这种思想。模板（泛型）可能是C++最重要的特性之一，它抽象了数据类型但不会损耗执行速度（但会增加编译时间）。 关于模板我们还可以思考一个问题：模板是可以抽象（代表）任意数据类型的，函数指针是一种数据类型，那么模板能不能抽象函数指针呢？当然能，那么有什么作用？我们应该可以使用模板来实现函数指针作为参数传递给某个函数。或许还可以实现匿名函数。C++11中内置匿名函数。匿名函数可以作为数组过滤器，简化从批量数据中获取满足特定条件的数据。比如下面的代码（UE4的TArray实现）：

```
/**
	 * Searches an initial subrange of the array for the last occurrence of an element which matches the specified predicate.
	 *
	 * @param Pred Predicate taking array element and returns true if element matches search criteria, false otherwise.
	 * @param Count The number of elements from the front of the array through which to search.
	 * @returns Index of the found element. INDEX_NONE otherwise.
	 */
	template <typename Predicate>
	int32 FindLastByPredicate(Predicate Pred, int32 Count) const
	{
		check(Count >= 0 && Count <= this->Num());
		for (const ElementType* RESTRICT Start = GetData(), *RESTRICT Data = Start + Count; Data != Start; )
		{
			--Data;
			if (Pred(*Data))
			{
				return static_cast<int32>(Data - Start);
			}
		}
		return INDEX_NONE;
	}

```


```
//! Fast allocator, only to be used in containers inside the same memory heap.
/** Containers using it are NOT able to be used it across dll boundaries. Use this
when using in an internal class or function or when compiled into a static lib */
template<typename T>
class irrAllocatorFast
{
public:

	//! Allocate memory for an array of objects
	T* allocate(size_t cnt)
	{
		return (T*)operator new(cnt* sizeof(T));
	}

	//! Deallocate memory for an array of objects
	void deallocate(T* ptr)
	{
		operator delete(ptr);
	}

	//! Construct an element
	void construct(T* ptr, const T&e)
	{
		new ((void*)ptr) T(e);
	}

	//! Destruct an element
	void destruct(T* ptr)
	{
		ptr->~T();
	}
};

```

这是一个Irr的内存分配器实现。使用模板屏蔽（抽象）了该内存分配器具体管理的内存数据类型。因为作为一个通用的内存分配器，并不知道具体需要分配的内存是保存什么类型的数据的，其实也不应该去关心数据类型。其次需要注意operator new和operator delete是两个函数而非运算符（自己可以去查找资料），上面有几个非常难懂的C++写法，这里不多说，网上有资料说明。


```
#ifndef __IRR_ARRAY_H_INCLUDED__
#define __IRR_ARRAY_H_INCLUDED__

#include "irrTypes.h"
#include "heapsort.h"
#include "irrAllocator.h"
#include "irrMath.h"

namespace irr
{
namespace core
{
//! Self reallocating template array (like stl vector) with additional features.
/** Some features are: Heap sorting, binary search methods, easier debugging.
*/
template <class T, typename TAlloc = irrAllocator<T> >
class array
{
public:

// code...

private:
	T* data;
	u32 allocated;
	u32 used;
	TAlloc allocator;
	eAllocStrategy strategy:4;
	bool free_when_destroyed:1;
	bool is_sorted:1;
};


} // end namespace core
} // end namespace irr

```

这里是一个模板类定义（Irr自己实现的数组，包含标准STL不具备的一些特性），TAlloc是什么？可以认为它是一个自定义的irrAllocator<T>类型。这个地方使用了C++模板类型绑定机制。这个地方约束了irrAllocator模板类使用跟array模板类相同的数据类型T，但是这个T可以是任意数据类型。有点绕口，仔细思考一下。这个自定义array可以保存任意数据类型的批量数据。其他数据结构（链表、字典、字符串等）也可以使用这种方式实现，以保证通用性。


```
//! List iterator.
	class Iterator
	{
	public:
		Iterator() : Current(0) {}

		Iterator& operator ++()    { Current = Current->Next; return *this; }
		Iterator& operator --()    { Current = Current->Prev; return *this; }
		Iterator  operator ++(s32) { Iterator tmp = *this; Current = Current->Next; return tmp; }
		Iterator  operator --(s32) { Iterator tmp = *this; Current = Current->Prev; return tmp; }

		Iterator& operator +=(s32 num)
		{
			if(num > 0)
			{
				while (num-- && this->Current != 0) ++(*this);
			}
			else
			{
				while(num++ && this->Current != 0) --(*this);
			}
			return *this;
		}

		Iterator  operator + (s32 num) const { Iterator tmp = *this; return tmp += num; }
		Iterator& operator -=(s32 num) { return (*this)+=(-num); }
		Iterator  operator - (s32 num) const { return (*this)+ (-num); }

		bool operator ==(const Iterator&      other) const { return Current == other.Current; }
		bool operator !=(const Iterator&      other) const { return Current != other.Current; }
		bool operator ==(const ConstIterator& other) const { return Current == other.Current; }
		bool operator !=(const ConstIterator& other) const { return Current != other.Current; }

		#if defined (_MSC_VER) && (_MSC_VER < 1300)
			#pragma warning(disable:4284) // infix notation problem when using iterator operator ->
		#endif

		T & operator * () { return Current->Element; }
		T * operator ->() { return &Current->Element; }

	private:
		explicit Iterator(SKListNode* begin) : Current(begin) {}

		SKListNode* Current;

		friend class list<T>;
		friend class ConstIterator;
	};

```

Irr通用双链表的迭代器实现。其实并不太关心被迭代的容器数据，只关心迭代操作算法，这些操作全部使用运算符重载实现。最后给人的感觉是这个迭代器跟基本类型没什么区别，虽然它是一个类类型对象。从这个角度，某个具体的运算符可以操作的对象是不确定的或者说是抽象的。跟迭代器一样，你还可以使用运算符重载实现指针的对象封装进而实现智能指针扩展C++的特性。具体可以参考UE4自己实现的智能指针（非C++11自带的智能指针），UE4的智能指针采用的组合而非继承的方式实现的（实现的引用计数更好）。


```
#ifndef _I_IMAGE_WRITER_H_INCLUDED__
#define _I_IMAGE_WRITER_H_INCLUDED__

#include "IReferenceCounted.h"
#include "irrString.h"
#include "coreutil.h"

namespace irr
{
namespace io
{
	class IWriteFile;
} // end namespace io

namespace video
{
	class IImage;


//! Interface for writing software image data.
class IImageWriter : public IReferenceCounted
{
public:
	//! Check if this writer can write a file with the given extension
	/** \param filename Name of the file to check.
	\return True if file extension specifies a writable type. */
	virtual bool isAWriteableFileExtension(const io::path& filename) const = 0;

	//! Write image to file
	/** \param file File handle to write to.
	\param image Image to write into file.
	\param param Writer specific parameter, influencing e.g. quality.
	\return True if image was successfully written. */
	virtual bool writeImage(io::IWriteFile *file, IImage *image, u32 param = 0) const = 0;
};

} // namespace video
} // namespace irr

#endif // _I_IMAGE_WRITER_H_INCLUDED__

```

面向接口编程（或者说是面向设计编程）。在设计阶段，我们并不关心具体的API实现，我们只要定义某个API的作用（职责）。具体的实现类只要实现该接口即可。这可能是面向对象编程的核心思想。多态也是从这里产生的。抽象类一般只定义API不定义数据。或者说，面向对象编程提供一套抽象API（操作）的机制。相对的模板提供了抽象数据类型的机制。而结构体提供了数据的抽象。枚举类型提供了类别的抽象。某种条件下，数据和操作是可以互相转换的。面向接口编程，它的实现类是接口类的具体化，或者说是实现类根据具体条件（情况）去适配接口。接口类可以看作设计阶段某个对象的抽象模型，这个对象在外界看来只提供某种行为操作并不暴露它的本质属性（数据变量），换句话设计阶段主要关注对象的行为。实现阶段会根据实际情况提取对象的本质属性，或者说具体的对象才会有本质上的区别，抽象上来看它们是一样的没有区别。从某种意义上，继承并不是为了继承基类的属性（变量）而是为了行为上的多态。（每个人的理解不一样）

在模块划分上，Irr使用了设备、资源、场景和渲染几大模块。为什么这样划分，主要靠的是多年在3D引擎领域积累的经验。主要跟某个领域的需求相关。

这里说的设备、资源、场景、GUI和渲染都是相对抽象的概念，并不具体指那个设备，那种资源，那个场景和那个渲染平台。这也是从整体宏观角度对系统进行抽象。比如：设备可能是mac、win32、sdl、linux、帧缓存等。资源可能是xml、普通文件、压缩文件、图片等。场景主要是3D可视和不可视物体的总称，比如模型加载器、动画、碰撞、粒子系统和场景节点等。渲染可能是基于软渲染器、DX9、OpenGL等。GUI代指用户图形界面，有面板和具体UI构件，比如：菜单和菜单项，按钮，文本标签和文本输入框等。

这里不多讲具体领域的知识。但是作为软件系统设计人员必须对某个领域业务需求非常熟悉才行。主要是为了模块划分，使模块内的趋向高内聚，模块间减小耦合性。最终提高软件系统的可扩展性和可配置性。


### 三、随便说点
---------------------------------

本篇着重从通用性角度分析了Irr引擎在C++软件设计方面的技巧和原因。C++在抽象性方面对C作了重要的提升，比如：类封装和抽象类，模板泛型编程，函数重载和重载运算符等。其中穿插了软件系统模块化的一些知识。这些知识，在用C++对软件系统进行优秀设计和实现具有重要作用。

>END
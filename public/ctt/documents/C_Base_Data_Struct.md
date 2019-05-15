
# C数据结构 - Vector、Map、String

> *作者：Arvin 日期：2019年05月15日*

---------------------------------

>BEGIN

用C实现向量（可变数组）、字典和可变字符串等。这几种数据结构（容器）在日常开发种用的是比较多的，作用也很显而易见。

### 一、自动增长的特点
---------------------------------

众所周知，C的数组是固定的，一旦声明完成就不能进行扩容。这满足不了很多数据量不固定的场合，需要数据容器自动进行容量的扩充。从广义上来讲，数组是一段连续的内存，该内存可能位于栈区或者堆区。
数据类型决定了其占用内存的字节数量。而指针可以对数据类型进行统一抽象，（空）指针可以代表任何类型。而指针的大小是固定的4字节（32位机器）。
换句话，我们使用realloc函数来重新分配连续内存可以实现扩容要求，使用指针来封装数据类型，也就是使用指针（变量数据的地址）来代表某个数据，不管其数据类型。

以上，就是用C实现可变数组（向量）的原理。它可能跟C++中用模版来实现向量的方法并不相同，但也算一种思路吧。

有了可以保存任何类型数据的可变数组，实现字典和可变字符串就很简单了。字典，使用两个向量实现，一个保存key，另外一个保存对应key的value。字符串，使用一个向量保存char即可。

    
### 二、Vector实现
---------------------------------

数据定义：

```
typedef struct {
  void** data;//指向连续内存区域的指针data，这块内存保存void*类型的数据，即任何其他类型的（地址）指针，32位机占用固定的4字节。
  int capacity;//data指向的内存区域容量大小，以字节位单位
  int len;//实际的数据个数，以4字节位单位（void*类型）。
} Vector;

```

在我的概念里不想用```指针的指针```这样的词语来描述变量类型。

***指针变量***自身是一个变量，它的类型永远是（地址）指针，永远占用4字节。它本质跟其他的变量没什么区别，比如：int变量、char变量等。

***指针指向***这种说法也比较模糊不清，因为我们不会明白在编程中```指向```有什么意义。我更愿意用指针变量的值表示什么来说明问题。

***指针变量的值*** 表示什么呢？它是一个内存（栈区或者堆区）地址，地址在32机器中的表示必须是4字节（32位）。那这个地址所在的内存是什么？

***地址所在的内存***是什么？可能是一个char，也可能是一个int，还可能是一个struct。这里面缺了一个信息，就是这个地址表示的是一字节的数据，还是多字节的数据。

这就是指针的抽象性所在，它能封装其他类型的数据就是这个原因。在定义指针的时候，不仅要指定变量自身的类型（当然是地址指针啦），还要指定指针变量的值（地址）所在内存区域的数据类型。

当然你也可以在定义的时候指定```地址所在区域```的数据类型为不确定，这个类型就是```void```类型。当然也可以指定```地址所在区域```的数据类型为指针（地址）类型。

国内教育所养成的坏习惯，我们在下面还会使用```指向```这一个概念。上面已经解释的很清楚了。

这里有一个问题：```char*```指针类型变量是指向一个字符还是指向一串连续的字符（字符串）呢？在C里是都可以的，需要具体使用环境来确定。这是C的一个缺点，指针不确定性太强了，增加了程序复杂性（模糊性+依赖性）。在给函数传递字符串的时候，需要函数参数带上字符串的长度也是这个原因。

换句话，我们可以根据代码中指针是否跟着一个长度（大小、容量、个数等）来确定该指针指向的内存是单个变量或者是多个连续的变量（变量数组）。不带长度（size等）表示该指针指向单个变量。

---------------------------------

回到正题，上面的Vector结构体具体代表什么？

```void* *data;```代表data是指针类型的变量，它的值（地址）所在区域保存着```void*```类型的数据变量。```void*```类型是指针类型，占用4字节。我只想说到这，因为继续细分暂时没什么必要。

data指向的内存区域是单个变量还是多个连续的变量呢？我们需要看下面的数据定义代码：```int capacity;```capacity汉语是容量的意思，这点暗示了data指针指向的内存包含多个变量（即变量数组）。具体情况需要在后面的使用代码中进行确认。```int len;```根据经验len大多数情况就是length的缩写，也就是表示长度。

根据后面的使用，Vector中的data指针指向capacity字节的一块连续内存，其中len表示真实使用过的内存大小，或者说是Vector中的void*类型变量个数。

```
Vector *new_vec(void);//初始化函数
void vec_push(Vector *v, void *elem);//变量入栈函数
void vec_pushi(Vector *v, int val);//数值变量入栈函数，这个地方虽然有点巧妙但是不推荐大家大量使用。
void *vec_pop(Vector *v);//变量出栈函数
void *vec_last(Vector *v);//获取Vector中的最后一个变量
bool vec_contains(Vector *v, void *elem);//判断变量是否在Vector中

```

我们提供以上6个函数作为Vector的基本操作函数。其实在程序设计阶段，上面的定义说明已经表示设计已经完成了。在软件工程中，实现并不是最重要的。

注意一下，上面除初始化函数的每个函数都带有```Vector* v```参数，即函数操作的Vector对象v。这也是C一个不方便的地方，如果是C++、Go等编程语言，是不需要参数中带操作对象的。

```
Vector *new_vec() {
  Vector *v = malloc(sizeof(Vector));
  v->data = malloc(sizeof(void *) * 16);
  v->capacity = 16;
  v->len = 0;
  return v;
}

void vec_push(Vector *v, void *elem) {
  if (v->len == v->capacity) {
    v->capacity *= 2;//容量呈当前容量的2倍进行增长
    v->data = realloc(v->data, sizeof(void *) * v->capacity);//自动增长特性的关键
  }
  v->data[v->len++] = elem;
}

void vec_pushi(Vector *v, int val) {
  vec_push(v, (void *)(intptr_t)val);//虽然巧妙但是不推荐使用，将int数值当成指针常量使用
}

void *vec_pop(Vector *v) {
  assert(v->len);
  return v->data[--v->len];
}

void *vec_last(Vector *v) {
  assert(v->len);
  return v->data[v->len - 1];//此地只取出最后的变量，并没有进行出栈操作，不改变Vector的任何状态
}

bool vec_contains(Vector *v, void *elem) {
  for (int i = 0; i < v->len; i++)
    if (v->data[i] == elem)
      return true;
  return false;
}

```

大家随意的看看实现就好，并不难！


### 三、Map实现
---------------------------------

数据定义：

```
typedef struct {
  Vector *keys;
  Vector *vals;
} Map;

```

两个向量（可变数组）实现，它们之间通过index下标进行一一对应。这里体现了数据封装的重要性，因为重要的数据底层操作不能直接暴露给其他开发者，否则很容易出错。比如这里的，隐含的下标一一映射关系。

```
Map *new_map(void);
void map_put(Map *map, char *key, void *val);
void map_puti(Map *map, char *key, int val);
void *map_get(Map *map, char *key);
int map_geti(Map *map, char *key, int default_);

```

没什么好说的。

```
Map *new_map(void) {
  Map *map = malloc(sizeof(Map));
  map->keys = new_vec();//存放key的向量
  map->vals = new_vec();//存放value的向量
  return map;
}

void map_put(Map *map, char *key, void *val) {
  vec_push(map->keys, key);//入栈key
  vec_push(map->vals, val);//入栈value
}

void map_puti(Map *map, char *key, int val) {
  map_put(map, key, (void *)(intptr_t)val);//int整数作为指针进行保存
}

void *map_get(Map *map, char *key) {
  for (int i = map->keys->len - 1; i >= 0; i--)
    if (!strcmp(map->keys->data[i], key))
      return map->vals->data[i];
  return NULL;
}

int map_geti(Map *map, char *key, int default_) {
  for (int i = map->keys->len - 1; i >= 0; i--)
    if (!strcmp(map->keys->data[i], key))
      return (int)(intptr_t)map->vals->data[i];//指针作为int整数进行返回
  return default_;
}

```

### 四、String实现
---------------------------------

数据定义：

```
typedef struct {
  char* data;//data指针变量保存字符串的头地址
  int capacity;//容量
  int len;//字符串长度
} StringBuilder;

```

准确应该叫作字符串构造对象。

```
StringBuilder *new_sb(void);
void sb_add(StringBuilder *sb, char c);//添加单个字符
void sb_append(StringBuilder *sb, char *s);//追加字符串
void sb_append_n(StringBuilder *sb, char *s, int len);//追加n个字符
char *sb_get(StringBuilder *sb);//返回字符串

```

```
StringBuilder *new_sb(void) {
  StringBuilder *sb = malloc(sizeof(StringBuilder));
  sb->data = malloc(8);
  sb->capacity = 8;
  sb->len = 0;
  return sb;
}

static void sb_grow(StringBuilder *sb, int len) {
  if (sb->len + len <= sb->capacity)
    return;

  while (sb->len + len > sb->capacity)
    sb->capacity *= 2;
  sb->data = realloc(sb->data, sb->capacity);//容量自动增长
}

void sb_add(StringBuilder *sb, char c) {
  sb_grow(sb, 1);
  sb->data[sb->len++] = c;
}

void sb_append_n(StringBuilder *sb, char *s, int len) {
  sb_grow(sb, len);
  memcpy(sb->data + sb->len, s, len);
  sb->len += len;
}

void sb_append(StringBuilder *sb, char *s) {
  sb_append_n(sb, s, (int)strlen(s));
}

char *sb_get(StringBuilder *sb) {
  sb_add(sb, '\0');//最后添加```'\0'```构造出字符串
  return sb->data;
}

```

原理跟Vector差不多。字符串构造主要是存储的是确定的char类型。


---------------------------------

附加一个有趣的函数：

```
int roundup(int x, int align) {
  return (x + align - 1) & ~(align - 1);
}

```

作用是x根据align向上取整。可用在计算结构体在满足字节对齐的前提下所占用的总字节数。

首先要注意的是```align-1```代表什么。

比如align为4，```align-1```等于3，二进制表示为```0011```，```~(align-1)```为```1100```。

```(x + align - 1)```表示x的二进制表示只要低2位包含1就会进位到第3位，这就是```向上```进位的意思。

向上进位之后呢？是要```取整```。取整就是将低2位置0即可。也就是进行```& ~(align - 1)```的运算。


### 五、随便说点
---------------------------------

1. 注意自动增长的C实现方式，可能也是任何高级语言实现可变数组的方式。
2. 这三种数据结构还是比较好用的，实用也很简单。
3. 指针还是C的精髓所在，提供了基本的抽象功能，不过也引进了模糊复杂性。
4. 向下取整怎么做呢？大概如右：```x & ~(align - 1)```

>END




# xv6操作系统收集多核信息（mpinit）

> *作者：Arvin 日期：2018年7月24日*

---------------------------------

>BEGIN

x86多核cpu架构采用了多个lapic和ioapic芯片，来处理处理器之间的互相中断（lapic）和外设的i/o中断。lapic一般是跟处理器核心绑定的，i/o一般在CPU之外（主板上）独立芯片支持。该篇收集的信息大部分是关于这两种中断芯片信息的。

### 一、前言
----------------------------------

该篇涉及到大量MP架构的硬件知识，最好先看一下Intel的文档，[多核架构说明](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_1_4.pdf)（MPS）。里面涉及4种结构体：floating pointer、configuration table header、processor table entry、I/O APIC table entry。

### 二、数据结构定义
----------------------------------

4个结构体定义如下：

```
struct mp {             // floating pointer
  uchar signature[4];           // "_MP_"
  void *physaddr;               // phys addr of MP config table
  uchar length;                 // 1
  uchar specrev;                // [14]
  uchar checksum;               // all bytes must add up to 0
  uchar type;                   // MP system config type
  uchar imcrp;
  uchar reserved[3];
};

struct mpconf {         // configuration table header
  uchar signature[4];           // "PCMP"
  ushort length;                // total table length
  uchar version;                // [14]
  uchar checksum;               // all bytes must add up to 0
  uchar product[20];            // product id
  uint *oemtable;               // OEM table pointer
  ushort oemlength;             // OEM table length
  ushort entry;                 // entry count
  uint *lapicaddr;              // address of local APIC
  ushort xlength;               // extended table length
  uchar xchecksum;              // extended table checksum
  uchar reserved;
};

struct mpproc {         // processor table entry
  uchar type;                   // entry type (0)
  uchar apicid;                 // local APIC id
  uchar version;                // local APIC verison
  uchar flags;                  // CPU flags
    #define MPBOOT 0x02           // This proc is the bootstrap processor.
  uchar signature[4];           // CPU signature
  uint feature;                 // feature flags from CPUID instruction
  uchar reserved[8];
};

struct mpioapic {       // I/O APIC table entry
  uchar type;                   // entry type (2)
  uchar apicno;                 // I/O APIC id
  uchar version;                // I/O APIC version
  uchar flags;                  // I/O APIC flags
  uint *addr;                  // I/O APIC address
};

// Table entry types
#define MPPROC    0x00  // One per processor
#define MPBUS     0x01  // One per bus
#define MPIOAPIC  0x02  // One per I/O APIC
#define MPIOINTR  0x03  // One per bus interrupt source
#define MPLINTR   0x04  // One per system interrupt source

```

前两个结构体mp和mpconf，分别是floating pointer结构体和configuration table header结构体。后面两个结构体是configuration table（配置表）的实体，配置表的实体有多种类型，这些类型定义在最下面的5个宏中。配置表的基地址，是mp结构体的physaddr字段。而mp的基地址需要我们自己查找，根据文档（MPS），有下面3种情况：

```
a. In the first kilobyte of Extended BIOS Data Area (EBDA), or

b. Within the last kilobyte of system base memory (e.g., 639K-640K for systems with 640
KB of base memory or 511K-512K for systems with 512 KB of base memory) if the
EBDA segment is undefined, or

c. In the BIOS ROM address space between 0F0000h and 0FFFFFh.

翻译下来：

a. 扩展BIOS数据区域（EBDA）的前1K内存中

b. 如果EBDA段没有定义，则在系统基础内存的后1K内存中

c. 在BIOS ROM的 0xF0000h 和 0xFFFFFh 之间的地址空间中

```

这里还提供一些[BDA信息](http://arvinsfj.github.io/public/ctt/documents/osxv6/BIOS_Data_Area.htm)。

MP配置数据结构图，如下所示：

![MP Configuration Data Structures](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_conf.png)

最后附一张完整的apic的连接图：

![MP Configuration Data Structures](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_apic.png)


### 三、收集MP信息
----------------------------------

先看mpinit函数：

```
void
mpinit(void)
{
  uchar *p, *e;
  struct mp *mp;
  struct mpconf *conf;
  struct mpproc *proc;
  struct mpioapic *ioapic;

  bcpu = &cpus[0];
  if((conf = mpconfig(&mp)) == 0)
    return;
  ismp = 1;
  lapic = (uint*)conf->lapicaddr;
  for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
    switch(*p){
    case MPPROC:
      proc = (struct mpproc*)p;
      if(ncpu != proc->apicid){
        cprintf("mpinit: ncpu=%d apicid=%d\n", ncpu, proc->apicid);
        ismp = 0;
      }
      if(proc->flags & MPBOOT)
        bcpu = &cpus[ncpu];
      cpus[ncpu].id = ncpu;
      ncpu++;
      p += sizeof(struct mpproc);
      continue;
    case MPIOAPIC:
      ioapic = (struct mpioapic*)p;
      ioapicid = ioapic->apicno;
      p += sizeof(struct mpioapic);
      continue;
    case MPBUS:
    case MPIOINTR:
    case MPLINTR:
      p += 8;
      continue;
    default:
      cprintf("mpinit: unknown config type %x\n", *p);
      ismp = 0;
    }
  }
  if(!ismp){
    // Didn't like what we found; fall back to no MP.
    ncpu = 1;
    lapic = 0;
    ioapicid = 0;
    return;
  }

  if(mp->imcrp){
    // Bochs doesn't support IMCR, so this doesn't run on Bochs.
    // But it would on real hardware.
    outb(0x22, 0x70);   // Select IMCR
    outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
  }
}

```

收集的信息包括：是否是多处理器架构（ismp）、lapic入口地址（lapic）、cpu核心数量（ncpu）、boot处理器（bcpu）、ioapic编号（ioapicid）、lapic编号（```cpus[ncpu].id = ncpu;```）。这些信息，在后面的apic中断设置需要用到。

mpinit首先调用mpconfig函数拿到mp结构体实体指针和mpconf结构体实体指针。然后通过mpconf（配置表头部）找到多核配置表实体，并且遍历表，获取我们需要收集的信息。遍历的时候主要关注MPPROC实体和MPIOAPIC实体。遍历完成后，主要设置IMCR寄存器，屏蔽外部中断。

关于IMCR寄存器的知识，参考[文档MPS](http://arvinsfj.github.io/public/ctt/documents/osxv6/mp_1_4.pdf)

注意上面的for循环中conf+1，实际是跳过多核配置表头，直接遍历配置表实体。


mpconfig函数如下：

```
static struct mpconf*
mpconfig(struct mp **pmp)
{
  struct mpconf *conf;
  struct mp *mp;

  if((mp = mpsearch()) == 0 || mp->physaddr == 0)
    return 0;
  conf = (struct mpconf*) p2v((uint) mp->physaddr);
  if(memcmp(conf, "PCMP", 4) != 0)
    return 0;
  if(conf->version != 1 && conf->version != 4)
    return 0;
  if(sum((uchar*)conf, conf->length) != 0)
    return 0;
  *pmp = mp;
  return conf;
}

```

mpconfig函数实际返回2个数据，一个是通过return返回mpconf指针，一个是通过参数pmp返回mp指针。

mpconfig函数主要是通过mpsearch函数找到mp实体，通过mp可以找到mpconf实体，然后做一些校验，最后返回两个数据。


mpsearch函数如下：

```
// Search for the MP Floating Pointer Structure, which according to the
// spec is in one of the following three locations:
// 1) in the first KB of the EBDA;
// 2) in the last KB of system base memory;
// 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
static struct mp*
mpsearch(void)
{
  uchar *bda;
  uint p;
  struct mp *mp;

  bda = (uchar *) P2V(0x400);
  if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
    if((mp = mpsearch1(p, 1024)))
      return mp;
  } else {
    p = ((bda[0x14]<<8)|bda[0x13])*1024;
    if((mp = mpsearch1(p-1024, 1024)))
      return mp;
  }
  return mpsearch1(0xF0000, 0x10000);
}

// Look for an MP structure in the len bytes at addr.
static struct mp*
mpsearch1(uint a, int len)
{
  uchar *e, *p, *addr;

  addr = p2v(a);
  e = addr+len;
  for(p = addr; p < e; p += sizeof(struct mp))
    if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
      return (struct mp*)p;
  return 0;
}

static uchar
sum(uchar *addr, int len)
{
  int i, sum;
  
  sum = 0;
  for(i=0; i<len; i++)
    sum += addr[i];
  return sum;
}

```

mpsearch函数按照之前描述的三种情况在内存区域寻找mp实体。```bda = (uchar *) P2V(0x400);```因为cpu开启了分页机制，这里需要将bda的物理地址0x400转换成虚拟地址P2V(0x400)。后面的按位或(|)操作都是通过两个字节组建一个地址。具体为什么要向左移动4位，为什么要乘上1024，自己参考[BDA文档](http://arvinsfj.github.io/public/ctt/documents/osxv6/BIOS_Data_Area.htm)。

mpsearch1函数，注意```addr = p2v(a);```语句，因为BDA里面存储的还是物理地址，需要转换成虚拟地址。之后，按照步长```sizeof(struct mp)```遍历[addr, addr+len)区域的内存，直到找到mp实体或者没有找到返回0。mp实体的判断条件是```memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0```，前4个字节是```"_MP_"```并且mp实体所有字节的和等于0。

sum函数是求内存区域内所有字节值的和。

补充说明一下mpbcpu函数：

```
int
mpbcpu(void)
{
  return bcpu-cpus;
}

struct cpu cpus[NCPU];
static struct cpu *bcpu;

```

功能是返回boot处理器在cpus数组中的（位置）索引。注意：cpus数组中的结构体cpu实体，在mpinit中设置了id，也就是lapic的编号。

----------------------------------

> END


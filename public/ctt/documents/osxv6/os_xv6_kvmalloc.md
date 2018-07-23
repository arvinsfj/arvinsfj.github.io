
# xv6操作系统内核页目录和页表的重构（kvmalloc）

> *作者：Arvin 日期：2018年7月23日*

---------------------------------

>BEGIN

在main函数中，调用了kvmalloc函数，它是重新给内核分配页目录、页表和页面的函数。运行main函数之前，内核临时设置了一个页目录entrypgdir表和使用了PSE扩展（只能使用[0, 4M）的物理内存，初步满足了main函数运行的需要。kvmalloc函数调用之后，内核将采用正常模式的分页机制（二级页表方式），而非PSE模式。

### 一、前言
----------------------------------

内核为什么需要重新分配页表呢？为什么不一开始就初始化好内核的页表而使用临时的entrypgdir页目录？原因就是kinit1函数。在kinit1函数调用之前，内存是没有页面结构（页面单链表）的，全部是单独的物理地址，不能按照页面进行分配和释放。这个时候是不能获取页面的，而页目录和页表都是页面形式存在的。

### 二、重构内核的页目录和页表
----------------------------------

kvmalloc函数定义如下：

```
// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void
kvmalloc(void)
{
  kpgdir = setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void
switchkvm(void)
{
  lcr3(v2p(kpgdir));   // switch to the kernel page table
}

static inline uint v2p(void *a) { return ((uint) (a))  - KERNBASE; }

static inline void
lcr3(uint val) 
{
  asm volatile("movl %0,%%cr3" : : "r" (val));
}

```

kpgdir是全局变量```pde_t *kpgdir;```，它是内核的页目录基地址（虚拟地址形式），通过v2p函数得到它的物理地址，通过lcr3函数加载该物理地址。页目录的切换会导致地址空间的切换（后面会遇到内核地址空间跟用户地址空间的切换，就是这种方式实现的）。

最关键的setupkvm函数，它是该功能的核心函数。

```
pde_t*
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
  if (p2v(PHYSTOP) > (void*)DEVSPACE)
    panic("PHYSTOP too high");
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start, 
                (uint)k->phys_start, k->perm) < 0)
      return 0;
  return pgdir;
}

static struct kmap {
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};

```

kalloc分配一个页面给pgdir（也就是内核的页目录），并用0填充该页面（kalloc返回的是填充1的页面）。然后，遍历kmap结构体数组（该数组包含了内核页表虚拟内存到物理内存的映射关系），并使用mappages函数建立真正的虚拟内存到物理内存的映射（建立页目录项目、页表和页表项目）。

NELEM宏定义如下：

```
// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

```

获取数组的大小size。

上面的遍历是采用指针的遍历，不是正常的索引遍历。

kmap数组，前三条比较关键，分别是I/O部分映射（虚拟地址KERNBASE映射到物理地址0，大小是EXTMEM，权限是PTE_W），内核代码段和只读数据段映射（内核开始的虚拟地址KERNLINK映射到物理地址V2P(KERNLINK)，大小是V2P(data)-V2P(KERNLINK)，权限0，可运行和可读），内核数据段和其它内存映射（数据段data开始的虚拟地址映射到物理地址V2P(data)，大小是PHYSTOP-V2P(data)，权限是PTE_W）。最后一条不知道是什么意思。

mappages函数定义如下：

```
// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;
  
  a = (char*)PGROUNDDOWN((uint)va);
  last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
  for(;;){
    if((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if(*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

```

函数的第一个参数是页目录基地址（pgdir）， 第二个参数是虚拟地址（va），第三个参数是需要映射的内存大小（size），第四个参数是该段内存的权限（perm）。

``` a = (char*)PGROUNDDOWN((uint)va);```虚拟地址va向低地址取页面地址a。为什么不是向高地址呢？不清楚。

```last = (char*)PGROUNDDOWN(((uint)va) + size - 1);```取va+size-1的页面地址。

上面2句话，就是确定[va, va+size)内存段的开始页面地址和结束页面地址（区域按照页面对齐）。这里的地址都是虚拟地址。

walkpgdir函数返回地址va在页表中的页项目地址。之后使用```*pte = pa | perm | PTE_P;``` 填写该页表项目。pa是虚拟地址va对应的物理地址，perm是页面的权限，PTE_P表示该页面已经在于内存中了。由于size可能需要分配多个页面，所以使用了for循环。

walkpgdir函数定义如下：

```
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)p2v(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table 
    // entries, if necessary.
    *pde = v2p(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

```

虚拟地址共32位，由三部分组成：高10位是页目录的索引、中间10位是页表的索引、最后12位是物理地址的偏移量。

```
// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/ 

// page directory index
#define PDX(va)         (((uint)(va) >> PDXSHIFT) & 0x3FF)

// page table index
#define PTX(va)         (((uint)(va) >> PTXSHIFT) & 0x3FF)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((uint)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

#define PGSHIFT         12      // log2(PGSIZE)
#define PTXSHIFT        12      // offset of PTX in a linear address
#define PDXSHIFT        22      // offset of PDX in a linear address

```

PDX宏获取虚拟地址va页目录的索引，PTX宏获取虚拟地址va页表的索引。PGADDR宏获取页目录或者页表中的高20位，即页面的基地址。

walkpgdir函数首先获取虚拟地址va对应的页目录表项基地址pde，如果表项（```*pde```）的PTE_P位为1（即页目录表项对应的页表存在于内存中），则获取该页表的基地址；如果表项不存在则分配一个页面作为页表并用0填充该页表，然后将该页表（页面）的基地址写入页目录表项（```*pde```）中。语句```*pde = v2p(pgtab) | PTE_P | PTE_W | PTE_U;```就是该写入操作。v2p(pgtab)是页表的物理基地址，其他三项是页面（页表）的访问属性。最后，返回虚拟地址va对应的在页表（基地址为pgtab）中的表项的地址（```&pgtab[PTX(va)]```）。

到此为止，重建内核的页目录和页表已经完成。页目录中存在1条内核页表的基地址和访问属性表项（此时页目录表只存在1条表项），内核页表中包含4条表项，分别映射kmap数组中4个元素所定义的4段内存。

xv6的代码写的比较通用，像函数setupkvm、switchkvm、mappages和walkpgdir都不仅仅在内核初始化的时候用到，其他地方也会用到。分析上面代码的时候注意各个页面的初始化数值，以及各种宏、函数、变量的命名规则。

### 三、关于内存管理的额外补充
----------------------------------

在main函数里面，还会调用seginit函数，对段进行重新分配。函数如下：

```
// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpunum()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);

  // Map cpu, and curproc
  c->gdt[SEG_KCPU] = SEG(STA_W, &c->cpu, 8, 0);

  lgdt(c->gdt, sizeof(c->gdt));
  loadgs(SEG_KCPU << 3);
  
  // Initialize cpu-local storage.
  cpu = c;
  proc = 0;
}

```

每个cpu对内存分为7段，分别是：空段0、内核代码段、内核数据段、内核CPU数据段、用户代码段、用户数据段、进程任务状态段。

用户段的DPL（描述符特权级别）数值为DPL_USER，也就是0x3。其他段该字段都为0。内核CPU数据段主要是暂存cpu的信息，大小为8字节，描述符（段表索引）保存在gs段寄存器。除了空段和内核CPU数据段，其他段都是单位映射（从0开始，大小4GB），逻辑地址即是虚拟地址（线性地址）。数据段都是可读可写（不可执行）权限，代码段都是可读可执行（不可写）权限。

函数的最后使用新的段描述符表基地址加载gdtr寄存器。最后，记录当前的cpu指针。

--------------------------------

内存管理的其他部分，比如：用户空间的页表切换、初始化、分配、释放等，后面进程管理部分再考虑。

> END


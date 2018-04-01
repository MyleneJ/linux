/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_ARM_CPUTYPE_H
#define __ASM_ARM_CPUTYPE_H

#include <linux/stringify.h>
#include <linux/kernel.h>
#include <asm/cputype_def.h>

extern unsigned int processor_id;

#ifdef CONFIG_CPU_CP15
#define read_cpuid(reg)							\
	({								\
		unsigned int __val;					\
		asm("mrc	p15, 0, %0, c0, c0, " __stringify(reg)	\
		    : "=r" (__val)					\
		    :							\
		    : "cc");						\
		__val;							\
	})

/*
 * The memory clobber prevents gcc 4.5 from reordering the mrc before
 * any is_smp() tests, which can cause undefined instruction aborts on
 * ARM1136 r0 due to the missing extended CP15 registers.
 */
#define read_cpuid_ext(ext_reg)						\
	({								\
		unsigned int __val;					\
		asm("mrc	p15, 0, %0, c0, " ext_reg		\
		    : "=r" (__val)					\
		    :							\
		    : "memory");					\
		__val;							\
	})

#elif defined(CONFIG_CPU_V7M)

#include <asm/io.h>
#include <asm/v7m.h>

#define read_cpuid(reg)							\
	({								\
		WARN_ON_ONCE(1);					\
		0;							\
	})

static inline unsigned int __attribute_const__ read_cpuid_ext(unsigned offset)
{
	return readl(BASEADDR_V7M_SCB + offset);
}

#else /* ifdef CONFIG_CPU_CP15 / elif defined (CONFIG_CPU_V7M) */

/*
 * read_cpuid and read_cpuid_ext should only ever be called on machines that
 * have cp15 so warn on other usages.
 */
#define read_cpuid(reg)							\
	({								\
		WARN_ON_ONCE(1);					\
		0;							\
	})

#define read_cpuid_ext(reg) read_cpuid(reg)

#endif /* ifdef CONFIG_CPU_CP15 / else */

#ifdef CONFIG_CPU_CP15
/*
 * The CPU ID never changes at run time, so we might as well tell the
 * compiler that it's constant.  Use this function to read the CPU ID
 * rather than directly reading processor_id or read_cpuid() directly.
 */
static inline unsigned int __attribute_const__ read_cpuid_id(void)
{
	return read_cpuid(CPUID_ID);
}

static inline unsigned int __attribute_const__ read_cpuid_cachetype(void)
{
	return read_cpuid(CPUID_CACHETYPE);
}

static inline unsigned int __attribute_const__ read_cpuid_mputype(void)
{
	return read_cpuid(CPUID_MPUIR);
}

#elif defined(CONFIG_CPU_V7M)

static inline unsigned int __attribute_const__ read_cpuid_id(void)
{
	return readl(BASEADDR_V7M_SCB + V7M_SCB_CPUID);
}

static inline unsigned int __attribute_const__ read_cpuid_cachetype(void)
{
	return readl(BASEADDR_V7M_SCB + V7M_SCB_CTR);
}

static inline unsigned int __attribute_const__ read_cpuid_mputype(void)
{
	return readl(BASEADDR_V7M_SCB + MPU_TYPE);
}

#else /* ifdef CONFIG_CPU_CP15 / elif defined(CONFIG_CPU_V7M) */

static inline unsigned int __attribute_const__ read_cpuid_id(void)
{
	return processor_id;
}

#endif /* ifdef CONFIG_CPU_CP15 / else */

static inline unsigned int __attribute_const__ read_cpuid_implementor(void)
{
	return (read_cpuid_id() & 0xFF000000) >> 24;
}

static inline unsigned int __attribute_const__ read_cpuid_revision(void)
{
	return read_cpuid_id() & 0x0000000f;
}

/*
 * The CPU part number is meaningless without referring to the CPU
 * implementer: implementers are free to define their own part numbers
 * which are permitted to clash with other implementer part numbers.
 */
static inline unsigned int __attribute_const__ read_cpuid_part(void)
{
	return read_cpuid_id() & ARM_CPU_PART_MASK;
}

static inline unsigned int __attribute_const__ __deprecated read_cpuid_part_number(void)
{
	return read_cpuid_id() & 0xFFF0;
}

static inline unsigned int __attribute_const__ xscale_cpu_arch_version(void)
{
	return read_cpuid_id() & ARM_CPU_XSCALE_ARCH_MASK;
}

static inline unsigned int __attribute_const__ read_cpuid_tcmstatus(void)
{
	return read_cpuid(CPUID_TCM);
}

static inline unsigned int __attribute_const__ read_cpuid_mpidr(void)
{
	return read_cpuid(CPUID_MPIDR);
}

/* StrongARM-11x0 CPUs */
#define cpu_is_sa1100() (read_cpuid_part() == ARM_CPU_PART_SA1100)
#define cpu_is_sa1110() (read_cpuid_part() == ARM_CPU_PART_SA1110)

/*
 * Intel's XScale3 core supports some v6 features (supersections, L2)
 * but advertises itself as v5 as it does not support the v6 ISA.  For
 * this reason, we need a way to explicitly test for this type of CPU.
 */
#ifndef CONFIG_CPU_XSC3
#define cpu_is_xsc3()	0
#else
static inline int cpu_is_xsc3(void)
{
	unsigned int id;
	id = read_cpuid_id() & 0xffffe000;
	/* It covers both Intel ID and Marvell ID */
	if ((id == 0x69056000) || (id == 0x56056000))
		return 1;

	return 0;
}
#endif

#if !defined(CONFIG_CPU_XSCALE) && !defined(CONFIG_CPU_XSC3) && \
    !defined(CONFIG_CPU_MOHAWK)
#define	cpu_is_xscale_family() 0
#else
static inline int cpu_is_xscale_family(void)
{
	unsigned int id;
	id = read_cpuid_id() & 0xffffe000;

	switch (id) {
	case 0x69052000: /* Intel XScale 1 */
	case 0x69054000: /* Intel XScale 2 */
	case 0x69056000: /* Intel XScale 3 */
	case 0x56056000: /* Marvell XScale 3 */
	case 0x56158000: /* Marvell Mohawk */
		return 1;
	}

	return 0;
}
#endif

/*
 * Marvell's PJ4 and PJ4B cores are based on V7 version,
 * but require a specical sequence for enabling coprocessors.
 * For this reason, we need a way to distinguish them.
 */
#if defined(CONFIG_CPU_PJ4) || defined(CONFIG_CPU_PJ4B)
static inline int cpu_is_pj4(void)
{
	unsigned int id;

	id = read_cpuid_id();
	if ((id & 0xff0fff00) == 0x560f5800)
		return 1;

	return 0;
}
#else
#define cpu_is_pj4()	0
#endif

static inline int __attribute_const__ cpuid_feature_extract_field(u32 features,
								  int field)
{
	int feature = (features >> field) & 15;

	/* feature registers are signed values */
	if (feature > 7)
		feature -= 16;

	return feature;
}

#define cpuid_feature_extract(reg, field) \
	cpuid_feature_extract_field(read_cpuid_ext(reg), field)

#endif

/*
 * File: apic.c
 * Document created by Fred Nora - (2023)
 * Credits:
 * Some parts was ported from Sirius-x86-64, created by Nelson Cole.
 *
 *    APIC - Advanced Programmable Interrupt Controller.
 *    Controlador APIC.
 *    APIC ("Advanced Programmable Interrupt Controller") 
 *    is the updated Intel standard for the older PIC.
 *   The APIC is used for 
 *   + sophisticated interrupt redirection, and for 
 *   + sending interrupts between processors.
 *   Stuff:
 *       +Discrete APIC.
 *       +Integrated local APICs.
 *       +APIC timer.
 *       +I/O APICs.
 * Credits:
 *     https://wiki.osdev.org/Symmetric_Multiprocessing
 *     Nelson Cole. (Sirius-x86-64).
 * History:
 *     2015 - Created by Fred Nora.
 *     2023 - Implementing/Porting more basic routines.
 */
// See:
// https://wiki.osdev.org/APIC
// https://wiki.osdev.org/Symmetric_Multiprocessing
// https://www.cheesecake.org/sac/smp.html


// credits: MP Specification.
// APIC has three interrupt modes.
// 1. PIC Mode — 
// effectively bypasses all APIC components and 
// forces the system to operate in single-processor mode.
// 2. Virtual Wire Mode — 
// uses an APIC as a virtual wire, 
// but otherwise operates the same as PIC Mode.
// 3. Symmetric I/O Mode — 
// enables the system to operate with more than one processor.
// Interrupt mode configuration register (IMCR). 
// This register controls whether the interrupt signals 
// that reach the BSP come from the master PIC or 
// from the local APIC.
// In this mode, I/O interrupts are generated by the I/O APIC. 
// All 8259 interrupt lines are either masked or 
// work together with the I/O APIC in a mixed mode.
// Before entering Symmetric I/O Mode, 
// either the BIOS or the operating system 
// must switch out of PIC Mode by changing the IMCR.
// In general, the operating system must use 
// the STARTUP IPI to wake up application processors in
// systems with integrated APICs, 
// but must use INIT IPI in systems with the 82489DX APIC.
// When the system is operated in Symmetric I/O Mode, 
// the operating system may enable the
// LINTIN0 and LINTIN1 of any or all local APICs as necessary.

#include <kernel.h>

// Initialization control.
struct lapic_info_d  LAPIC;

#define APIC_CONFIG_DATA_LVT(TimerMode,Mask,TriggerMode,Remote,InterruptInput,DeliveryMode,Vector)(\
(((unsigned int)(TimerMode) &0x3 ) << 17) |\
(((unsigned int)(Mask) &0x1 ) << 16) |\
(((unsigned int)(TriggerMode) &0x1 ) << 15) |\
(((unsigned int)(Remote) &0x1 ) << 14) |\
(((unsigned int)(InterruptInput) &0x1 ) << 13) |\
(((unsigned int)(DeliveryMode) &0x7 ) << 8) |\
((unsigned int)(Vector) &0xff )\
)

unsigned int localId=0;

#define APIC_NULL  0

//
// == private functions: prototypes =====================
//

static unsigned int local_apic_read_command(unsigned short addr);
static void local_apic_write_command(unsigned short addr,unsigned int val);

static unsigned int local_apic_get_id(void);
static unsigned int local_apic_get_version(void);
static void local_apic_eoi(void);

// apic stuffs for x86.
static inline void imcr_pic_to_apic (void);
static inline void imcr_apic_to_pic (void);

static void __apic_disable_legacy_pic(void);

// =====================

/* Flush caches */
/*
void flush_cashes(void)
{
	__asm__("wbinvd");
}
*/


// #todo: 
// Definir porta 70h usada nesse arquivo. ??
static unsigned int local_apic_read_command(unsigned short addr)
{
    if( (void *) LAPIC.lapic_va == NULL ){
        panic("local_apic_read_command: LAPIC.lapic_va\n");
    }

    return *( (volatile unsigned int *)(LAPIC.lapic_va + addr));
}

static void local_apic_write_command(unsigned short addr,unsigned int val)
{
    if( (void *) LAPIC.lapic_va == NULL ){
        panic("local_apic_write_command: LAPIC.lapic_va\n");
    }

    *( (volatile unsigned int *)(LAPIC.lapic_va + addr)) = val;
}

// Get local apic id.
// see: https://wiki.osdev.org/APIC#Local_APIC_registers
// bits 24~31 for pentium 4 and later.
static unsigned int local_apic_get_id(void)
{
    return (unsigned int) (local_apic_read_command(LAPIC_APIC_ID) >> 24) & 0xFF;
}

// Get local apic version.
// see: https://wiki.osdev.org/APIC#Local_APIC_registers
// bits: 0~7 for Integrated APIC.
// 10H~15H
static unsigned int local_apic_get_version(void)
{
    return (unsigned int) (local_apic_read_command(LAPIC_APIC_VERSION) & 0xFF);
}


// EOI Register:
// Write to the register with offset 0xB0 using the value 0 
// to signal an end of interrupt. 
// A non-zero value may cause a general protection fault.
static void local_apic_eoi(void)
{
    local_apic_write_command( (unsigned short) LAPIC_EOI, (unsigned int) 0 );
}

// Spurious Interrupt Vector Register
// Set the Spurious Interrupt Vector Register bit 8 to 
// start receiving interrupts.
// The offset is 0xF0. 
// The low byte contains the number of the spurious interrupt. 
// As noted above, you should probably set this to 0xFF. 
// To enable the APIC, set bit 8 (or 0x100) of this register. 
// If bit 12 is set then EOI messages will not be broadcast. 
// All the other bits are currently reserved.

/*
static void local_apic_set_spurious_interrupt(void);
static void local_apic_set_spurious_interrupt(void)
{
    // #todo:
    // write_reg(0xF0, ReadRegister(0xF0) | 0x100);
}
*/

// =========


/*
 * imcr_pic_to_apic:
 * Handle interrupt mode configuration register (IMCR).
 * This register controls whether the interrupt signals that reach 
 * the BSP come from the master PIC or from the local APIC. 
 * Before entering Symmetric I/O Mode, either the BIOS or 
 * the operating system must switch out of PIC Mode by changing the IMCR.
 */

/*
The IMCR is supported by two read/writable or write-only I/O ports, 22h and 23h, which receive
address and data respectively. To access the IMCR, write a value of 70h to I/O port 22h, which
selects the IMCR. Then write the data to I/O port 23h. The power-on default value is zero, which
connects the NMI and 8259 INTR lines directly to the BSP. Writing a value of 01h forces the
NMI and 8259 INTR signals to pass through the APIC.
The IMCRP bit of the MP feature
information bytes enables the operating system to detect whether the IMCR is
implemented.
*/

static inline void imcr_pic_to_apic (void)
{
// Select IMCR register.
// First, write the value `70h` to port `22h`. 
// This action selects the IMCR for configuration.
    out8 ((unsigned short)0x22, (unsigned char) 0x70);

// NMI and 8259 INTR go through APIC.
// Next, write the desired data value to port `23h`. 
// This data defines the configuration.
// 0x01 - Route to APIC.
    out8 ((unsigned short)0x23, (unsigned char) 0x01);
}

static inline void imcr_apic_to_pic (void)
{
// Select IMCR register.
// First, write the value `70h` to port `22h`. 
// This action selects the IMCR for configuration.
    out8 ((unsigned short)0x22, (unsigned char) 0x70);

// NMI and 8259 INTR go directly to BSP.
// Next, write the desired data value to port `23h`. 
// This data defines the configuration.
// 0x01 - Do NOT route to APIC.
    out8 ((unsigned short)0x23, (unsigned char) 0x00);
}

/*
 creadits: MP Specification
Cache flushing—The processor can generate special flush and 
write-back bus cycles that must be used by external caches 
in a manner that maintains cache coherency. 
The actual responses are implementation-specific and 
may vary from design to design. 
A program can initiate hardware cache flushing by executing a 
WBINVD instruction. This instruction is only guaranteed 
to flush the caches of the local processor.
*/

/*
inline void invalidate_cache_flush(void);
inline void invalidate_cache_flush(void)
{
    asm ("wbinvd");
}
*/

// =================

// #bugbug
// This routine used a lot of hardcoded addresses.
// PAGETABLE_RES5, LAPIC_VA, PD_ENTRY_LAPIC, KERNEL_PD_PA.
void lapic_initializing(unsigned long lapic_pa)
{
// >> Called by I_init in x64init.c
// >> Called by x64_probe_smp_via_acpi() in x64.c
// Setup BSP's local APIC.

    printk("lapic_initializing: \n");

    LAPIC.initialized = FALSE;

// Invalid address.
// 0xFEE00000
// see: x64gpa.h
    if (lapic_pa != __LAPIC_PA)
    {
        LAPIC.initialized = FALSE;
        //panic("lapic_initializing: lapic_pa\n");
        printk("lapic_initializing: lapic_pa\n");
        return;
    }

// #todo
// Do we have apic support in this processor?
    //if (has_apic() != TRUE)
        //panic("lapic_initializing: APIC not supported\n");

// ===================
// Mapping

// page table
// Isso serve pra pegarmos um endereço físico
// que servira de base para criarmos uma pagetable.
// Mas endereço físico e virtual são iguais nessa região.
// Identidade 1:1.
    unsigned long *pt_lapic = (unsigned long *) get_table_pointer_va();

// -------------
// pa
    LAPIC.lapic_pa = (unsigned long) (lapic_pa & 0xFFFFFFFF);
// -------------
// va
// see: x64gva.h
    LAPIC.lapic_va = (unsigned long) LAPIC_VA;

// -------------------------------------
// Mapping area for registers.

    int map_status = -1;
    map_status = (int) mm_map_2mb_region( LAPIC.lapic_pa, LAPIC.lapic_va );
    if (map_status != 0)
        panic("lapic_initializing: on mm_map_2mb_region()\n");

//==========================================

// flush tlb
// #bugbug
// Maybe we need to call a method for that.

    asm ("movq %cr3, %rax");
    asm ("movq %rax, %cr3");

//=====================================

// Let's get some information about the BSP LAPIC.

// Check values into the registers:
// see: 
// https://wiki.osdev.org/APIC#Local_APIC_registers

// ---------------
// ID
    int localid = (int) local_apic_get_id();
    LAPIC.local_id = (int) (localid & 0xFF);
    //printk("localid: %d\n",LAPIC.local_id);

// ---------------
// Version
// 8bits
// 10H~15H
    int localversion = (int) local_apic_get_version();
    LAPIC.local_version = (int) (localversion & 0xFF);
    //printk("localversion: %xH\n", LAPIC.local_version);

//=====================================
// Destination Format Register (DFR)
// Value after reset, flat mode
// depois de invalidar o pic?
    //*(volatile unsigned int*)(LAPIC.lapic_va + ?) = 0xFFFFFFFF; 
// Logical Destination Register (LDR)
// All cpus use logical id 1
    //*(volatile unsigned int*)(LAPIC.lapic_va + ?) = 0x01000000; 
    //*(volatile unsigned int*)(LAPIC.lapic_va + 0x20) = 8;

// Print:
    printk("ID: %d | VERSION: %xH\n",
        LAPIC.local_id,
        LAPIC.local_version );

    LAPIC.initialized = TRUE;
}

//
// ======================================================
//

// #todo
// Testando um código encontrado em https://wiki.osdev.org/APIC.
// setting up the APIC:
 
/* 
 * has_apic:
 *     returns a 'true' value if the CPU supports APIC
 *  and if the local APIC hasn't been disabled in MSRs
 *  note that this requires CPUID to be supported.
 */
// #todo: Change function name.
// See: cpuid.h
int has_apic (void)
{
    int hasAPIC = FALSE;
    unsigned int eax=0;
    unsigned int ebx=0;
    unsigned int ecx=0;
    unsigned int edx=0;

// #bugbug
// Do we have cpuid support?
    cpuid( 1, eax, ebx, ecx, edx );
    hasAPIC = (int) (edx & CPUID_FEAT_EDX_APIC);

    return (int) hasAPIC;
}

// Set the physical address for local APIC registers.
// Is it possible to change the base?
// But we have a default adddress, it is 0xFEE00000.
// Enable APIC by setting up the bit 11.
void cpu_set_apic_base(unsigned long apic) 
{
   unsigned int edx=0;

// Enable APIC by setting up the bit 11.
   unsigned int eax = 
       (unsigned int) (apic & 0xfffff000) | IA32_APIC_BASE_MSR_ENABLE;

//#ifdef __PHYSICAL_MEMORY_EXTENSION__
//   edx = (apic >> 32) & 0x0f;
//#endif
 
    cpuSetMSR (
        (unsigned int) IA32_APIC_BASE_MSR, 
        (unsigned int) eax, 
        (unsigned int) edx );
}
 
/*
 * Get the physical address of the APIC registers page
 * make sure you map it to virtual memory ;)
 */
unsigned long cpu_get_apic_base(void) 
{
    unsigned long APIC_base_address = 0;
    unsigned int eax=0;
    unsigned int edx=0;

    cpuGetMSR(
        (unsigned int) IA32_APIC_BASE_MSR, 
        (unsigned int) &eax, 
        (unsigned int) &edx );

/* 
#ifdef __PHYSICAL_MEMORY_EXTENSION__
   return (eax & 0xfffff000) | ((edx & 0x0f) << 32);
#else
   return (eax & 0xfffff000);
#endif
*/

    APIC_base_address =  (unsigned long) (eax & 0xfffff000);

    return (unsigned long) APIC_base_address;
}

// #todo: 
// #danger !!!
/* Section 11.4.1 of 3rd volume of Intel SDM recommends 
  mapping the base address page as strong uncacheable 
  for correct APIC operation. */
void enable_apic(void)
{
// Called by I_kmain() in main.c.

// #todo
// We need to setup a lot of registers 
// before enabling the apic.

    printk("enable_apic:\n");

// #todo
// Do we have apic support in this processor?
    //if (has_apic() != TRUE)
        //panic("enable_apic: APIC not supported\n");

    if (LAPIC.initialized != TRUE)
        panic("enable_apic: LAPIC not initialized\n");

// Hardware enable the Local APIC if it wasn't enabled.
// Went to the end of this routine.
    //cpu_set_apic_base( cpu_get_apic_base() );

//
// Disable legacy PIC
//

    __apic_disable_legacy_pic();


// Destination Format Register (DFR)
// LAPIC_DFR
// Logical Destination Mode
    // Flat mode
    local_apic_write_command(
        (unsigned short) 0x00e0, 
        (unsigned int) 0xffffffff);


// Logical Destination Register (LDR)
// LAPIC_LDR
// All cpus use logical id 1. ?
    local_apic_write_command(
        (unsigned short) 0x00d0, 
        (unsigned int) 0x01000000);


    localId = local_apic_get_id();

// -- Interrupts ----------------------------------------------------

// Task Priority Register (TPR), to inhibit softint delivery?
// Clear task priority to enable all interrupts.
    local_apic_write_command(
        (unsigned short) 0x0080, 
        (unsigned int) 0 );

//---------------------
//#test
//#todo

// #warning: We already did that above.
//Task Priority Register (TPR), to inhibit softint delivery
    *(volatile unsigned int*)(LAPIC.lapic_va + LAPIC_TASK_PRIORITY) = 
        0; //0x20;

// Timer interrupt vector, 
// to disable timer interrupts
    *(volatile unsigned int*)(LAPIC.lapic_va + LAPIC_LVT_TIMER) = 
        APIC_CONFIG_DATA_LVT(0,1,0,0,0,APIC_NULL,0x20);

// Performance counter interrupt, 
// to disable performance counter interrupts
    *(volatile unsigned int*)(LAPIC.lapic_va + LAPIC_LVT_PERF) = 
        APIC_CONFIG_DATA_LVT(APIC_NULL,1,0,0,0,0,0x21);

// Local interrupt 0, 
// to enable normal external interrupts, Trigger Mode = Level
    *(volatile unsigned int*)(LAPIC.lapic_va + LAPIC_LVT_LINT0) = 
        APIC_CONFIG_DATA_LVT(APIC_NULL,1,APIC_NULL,APIC_NULL,1,7,0x22);

// Local interrupt 1, 
// to enable normal NMI processing
    *(volatile unsigned int*)(LAPIC.lapic_va + LAPIC_LVT_LINT1) = 
        APIC_CONFIG_DATA_LVT(APIC_NULL,1,APIC_NULL,APIC_NULL,0,4,0x23);

// Error interrupt, 
// to disable error interrupts
    *(volatile unsigned int*)(LAPIC.lapic_va + LAPIC_LVT_ERR) = 
        APIC_CONFIG_DATA_LVT(APIC_NULL,1,APIC_NULL,APIC_NULL,APIC_NULL,0,0x24);

/*
Spurious-Interrupt Vector Register:
The Spurious-Interrupt Vector Register contains 
 + the bit to enable and disable the local APIC. It also has 
 + a field to specify the interrupt vector number to be delivered 
   to the processor in the event of a spurious interrupt. 
This register is 32 bits and has the following format:

... | 9  | 8  | (  7~4  | 3 | 2 | 1 | 0) 
... | FC | EN | (VECTOR | 1 | 1 | 1 | 1)

EN bit (8):
This allows software to enable or disable the APIC module at any time. 
Writing a value of 1 to this bit enables the APIC module, and 
writing a value of 0 disables it.

VECTOR(7~4):
Bits 4-7 of this field are programmable by software.
This field of the Spurious-Interrupt Vector Register 
specifies which interrupt vector is delivered to the processor 
in the event of a spurious interrupt. 
The processor then transfers control to the 
interrupt handler in the IDT, 
at the vector number delivered to it by the APIC. 
Basically, the VECTOR field specifies 
which interrupt handler to transfer control 
to in the event of a spurious interrupt.
VECTOR(0~3)
Bits 0-3 of this vector field are hard-wired to 1111b, or 15. 

So:
   0x1FF:
   1 ((1111) 1111).
   We are setting the bits 4~7 of the vector
   Interrupt 256 i guess.
*/

// LAPIC_SVR = 0x00F0
// Value = 0x1FF.
// Interrupt 256 i guess.

    local_apic_write_command (
        (unsigned short) 0x00F0,  
        (unsigned int) (  0x100 | 0xFF ) );

/*
//
// Global enable
    enableapic(
        cpu_get_apic_base() 
    );
*/

// Hardware enable the Local APIC if it wasn't enabled.
// Enable APIC by setting up the bit 11.
    cpu_set_apic_base( cpu_get_apic_base() );

/*
 // #debug
    printk("APIC Global enable 0x%x \n",
        getapicbase() );
*/

/*
    printk("Setup APIC Timer 0 ...\\\\\n");
    apic_timer(lapicbase);
*/
// see: apictim.c

// #todo #bugbug
// e o timer precisa mudar o vetor 
// pois 32 ja esta sendo usado pela redirection table.

    apic_timer();
}


/*
 # SIPI Sequence 
 
void APIC::wakeupSequence(U32 apicId, U8 pvect);
void APIC::wakeupSequence(U32 apicId, U8 pvect)
{
                ICRHigh hreg = {
				.destField = apicId
		};
 
		ICRLow lreg(DeliveryMode::INIT, Level::Deassert, TriggerMode::Edge);
 
		xAPICDriver::write(APIC_REGISTER_ICR_HIGH, hreg.value);
		xAPICDriver::write(APIC_REGISTER_ICR_LOW, lreg.value);
 
		lreg.vectorNo = pvect;
		lreg.delvMode = DeliveryMode::StartUp;
 
		Dbg("APBoot: Wakeup sequence following...");
 
		xAPICDriver::write(APIC_REGISTER_ICR_HIGH, hreg.value);
		xAPICDriver::write(APIC_REGISTER_ICR_LOW, lreg.value);
}
*/ 

// NOTE: ICRLow and ICRHigh are types in the Silcos kernel. If your code uses direct bit
// manipulations you must replace some code with bit operations.



void local_apic_send_init(unsigned int apic_id)
{
// Send one time.
// Respect the order: 
// First 0x310 then 0x300.

// High (0x310)
    local_apic_write_command(
        LAPIC_ICR_HIGH, 
        (apic_id << 24) );

// Low (0x300)
// (No vector? vector 0?)
    local_apic_write_command(
        LAPIC_ICR_LOW,
        ICR_INIT | 
        ICR_PHYSICAL | 
        ICR_ASSERT | 
        ICR_EDGE | 
        ICR_NO_SHORTHAND );

// Loop
    while ( local_apic_read_command(LAPIC_ICR_LOW) & ICR_SEND_PENDING )
    {
        asm volatile ("pause;");
    };
}



void 
local_apic_send_startup(
    unsigned int apic_id, 
    unsigned int vector )
{
// Send one time.
// Respect the order: 
// First 0x310 then 0x300.

    //#todo
    //vector = (vector & 0xFF);

// High (0x310)
    local_apic_write_command(
        LAPIC_ICR_HIGH, 
        (apic_id << 24) );

// Low (0x300)
    local_apic_write_command(
        LAPIC_ICR_LOW,
        vector | 
        ICR_STARTUP | 
        ICR_PHYSICAL | 
        ICR_ASSERT | 
        ICR_EDGE | 
        ICR_NO_SHORTHAND );

// Loop
    while ( local_apic_read_command(LAPIC_ICR_LOW) & ICR_SEND_PENDING )
    {
        asm volatile ("pause;");
    }
}

// Send INIT IPI
void Send_INIT_IPI_Once(unsigned int apic_id)
{
// One single time.
    local_apic_write_command(0x280, 0);  // Clear APIC errors.
    local_apic_send_init(apic_id);
    mdelay(100); // wait 10 msec
}

// Send STARTUP IPI (twice)
void Send_STARTUP_IPI_Twice(unsigned int apic_id)
{
// Twice

    unsigned int vector_number = 0x8;
    long i=0;
    for (i=0; i<2; i++)
    {
        local_apic_write_command(0x280, 0);  // Clear APIC errors.
        local_apic_send_startup(apic_id, vector_number);
        mdelay(200); // wait 200 msec
    };
}

static void __apic_disable_legacy_pic(void)
{
// Legacy PIC mask all off.

    printk("apic_disable_legacy_pic:\n");

// Envia ICW1 reset
    out8(0x20,0x11);	// reset PIC 1
    out8(0xA0,0x11);	// reset PIC 2
// Envia ICW2 start novo PIC 1 e 2
    out8(0x21,0x20);	// PIC 1 localiza no IDT 39-32 
    out8(0xA1,0x28);	// PIC 2 localiza no IDT 47-40
// Envia ICW3
    out8(0x21,0x04);	// IRQ2 conexao em cascata com o PIC 2
    out8(0xA1,0x02);
// Envia ICW4
    out8(0x21,0x01);
    out8(0xA1,0x01);
// OCW1
// Desabilita todas as interrupcoes
    out8(0x21,0xFF);
    out8(0xA1,0xFF);
}

//
// End
//


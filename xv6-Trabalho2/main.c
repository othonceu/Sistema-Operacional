#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void startothers(void);
static void mpmain(void)  __attribute__((noreturn));
extern pde_t *kpgdir;
extern char end[]; // first address after kernel loaded from ELF file / primeiro endereço após o kernel carregado do arquivo ELF

// Bootstrap processor starts running C code here.		/ O processador do Bootstrap começa a executar o código C aqui.
// Allocate a real stack and switch to it, first 		/Alocar uma pilha real e mudar para ela, primeiro.
// doing some setup required for memory allocator to work. 	/fazendo algumas configurações necessárias para o alocador de memória funcionar.
int
main(void)
{
  kinit1(end, P2V(4*1024*1024)); // phys page allocator      /alocador de páginas phys
  kvmalloc();      // kernel page table                      /tabela de páginas do kernel
  mpinit();        // detect other processors                /detectar outros processadores
  lapicinit();     // interrupt controller                   /controlador de interrupção
  seginit();       // segment descriptors 		     /descritores de segmento
  picinit();       // disable pic                            /desativar pic
  ioapicinit();    // another interrupt controller           /outro controlador de interrupção
  consoleinit();   // console hardware                       /console de hardware
  uartinit();      // serial port                            /porta serial
  pinit();         // process table                          /tabela de processo
  tvinit();        // trap vectors                           /vetores de trap
  binit();         // buffer cache 			     /buffer cache
  fileinit();      // file table                             /tabela de arquivos
  ideinit();       // disk 				     /disco
  startothers();   // start other processors 	             /inicie outros processadores
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()  /deve vir depois de startothers
  userinit();      // first user process                                    /primeiro processo do usuário
  mpmain();        // finish this processor's setup                         /terminar a configuração deste processador
}

// Other CPUs jump here from entryother.S. 				    /Outras CPUs saltam aqui de entryother.S.
static void
mpenter(void)
{
  switchkvm();
  seginit();
  lapicinit();
  mpmain();
}

// Common CPU setup code.  					           /Código de configuração de CPU comum.
static void mpmain(void)
{
  cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
  idtinit();       // load idt register					  /carregar o registro do idt
  xchg(&(mycpu()->started), 1); // tell startothers() we're up            /dizer startothers () estamos acordados
  scheduler();     // start running processes			          /começar a executar processos
}

pde_t entrypgdir[];  // For entry.S 					  /Para entrada.S

// Start the non-boot (AP) processors.					/Inicia os processadores de não inicialização (AP).
static void
startothers(void)
{
  extern uchar _binary_entryother_start[], _binary_entryother_size[];
  uchar *code;
  struct cpu *c;
  char *stack;

  // Write entry code to unused memory at 0x7000.		/Escreva o código de entrada na memória não utilizada em 0x7000.
  // The linker has placed the image of entryother.S in		/O vinculador colocou a imagem de entryother.S em
  // _binary_entryother_start.					/ _binary_entryother_start.
  code = P2V(0x7000);
  memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);

  for(c = cpus; c < cpus+ncpu; c++){
    if(c == mycpu())  // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what    		/Diga ao entryother.S que pilha usar, onde entrar e o que
    // pgdir to use. We cannot use kpgdir yet, because the AP processor			/pgdir para usar. Ainda não podemos usar o kpgdir, porque o processador AP
    // is running in low  memory, so we use entrypgdir for the APs too.			/está rodando com pouca memória, então usamos o entrypgdir para os APs também
    stack = kalloc();
    *(void**)(code-4) = stack + KSTACKSIZE;
    *(void(**)(void))(code-8) = mpenter;
    *(int**)(code-12) = (void *) V2P(entrypgdir);

    lapicstartap(c->apicid, V2P(code));

    // wait for cpu to finish mpmain()				/espere que a CPU termine mpmain ()
    while(c->started == 0)
      ;
  }
}

// The boot page table used in entry.S and entryother.S.				/A tabela da página de inicialização usada em entry.S e entryother.S.
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4Mbyte pages.

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.


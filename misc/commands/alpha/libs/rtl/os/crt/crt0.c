// crt0.c
// Standard crt0 for Gramado OS ring 3 applications.


#include <types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <rtl/gramado.h>

#define LSH_TOK_DELIM  " \t\r\n\a" 
#define SPACE  " "
#define TOKENLIST_MAX_DEFAULT  80

//static const unsigned int something=1234;

const char *argv[] = { 
    "-flag1", 
    "-flag2", 
    "-flag3", 
    "-flag4", 
    NULL 
};


/*
	$USER:      Gives current user's name.
	$PATH:      Gives search path for commands.
	$PWD:       Gives the path of present working directory.
	$HOME:      Gives path of home directory.
	$HOSTNAME:  Gives name of the host.
	$LANG:      Gives the default system language.
	$EDITOR:    Gives default file editor.
	$UID:       Gives user ID of current user.
	$SHELL:     Gives location of current user's shell program.
*/

const char *my_environ[] = { 

    "DISPLAY=kgws-or-gwssrv",  //#todo
    "EDITOR=gramcode",
    "HOME=/HOME",
    "HOSTNAME=gramado",
    "LANG=en-us",
    "LANG2=pt-br",
    "LANGX=en-br",
    "OSTYPE=gramado",   //#todo
    "PATH=/:/PROGRAMS:/GRAMADO",
    "PS1=~$",
    "PS2=:>",
    "PWD=/",             //#bugbug: This is the working directory.
    "SHELL=gdeshell",
    "TERM=noraterm",     //#bugbug
    "TMP=/TMP",
    "UID=no-id",         //#todo
    "USER=anon",  
    NULL 
};


// ==============================
// Import from the ring3 application.
extern int main(int argc, char *argv[]);

// ==============================

// # importante
// Esses aplicativos rodam no terminal.
// Esses aplicativos escrevem em stdout.
// O terminal precisa conhecer esse stdout para ler.

// #todo
// Explain this argument better.

void crt0(unsigned long rdi)
{
// This function never returns.

    // #todo
    // We can get the command line from 'stdin'.

// #warning
// Only the last 8 bits are used
// as a return value to the father.
    int main_ret=0;

// Token support.
    char *tokenList[TOKENLIST_MAX_DEFAULT];
    char *token;
    int token_count=0;
    int index=0;

// #todo
// Explain it better.
// Is it possible to access this address?

    unsigned long *surface_config = (unsigned long *) rdi;

// #importante
// Linha de comandos passada pelo shell.
// See:
// globals/gva.h
// #define FRONTBUFFER_VA    0xC0400000
// #define BACKBUFFER_VA     0xC0800000
// So. We are using the top of the front buffer
// as a shared memory. No problem for now.
// #todo
// Change this address. We need a valid one.

    // #todo
    //char *shared_info = (char *) (0xC0800000 -0x100);
    //char *shared_info = "nothing nothing";
    //char *shared_info;
    //char buffer[4096];

// Environment.
// The library will have this default environment.
// #todo: Change the name to '__libc_default_environ'
    environ = my_environ;

/*
// #debug
#ifdef TEDITOR_VERBOSE
	printf ("\n");
	printf ("crt0: Initializing ...\n");
	//printf("\n");
	//printf(".\n");
	printf ("..\n");
	printf ("# MESSAGE={%s} #\n", shared_info );
	printf ("..\n");
	//printf(".\n");
	//printf("\n");
	//#debug
	//while(1){ asm ("pause"); }
#endif
*/

/*
 #debug
#ifdef TEDITOR_VERBOSE
	// #debug 	
	// Mostra a quantidade de argumentos. 

	printf("\n");
	printf("token_count={%d}\n", token_count );

    //Mostra os primeiros argumentos.
    for ( index=0; index < token_count; index++ )
    {
        token = (char *) tokenList[index];
        if ( token == NULL )
        {
            printf ("crt0: for fail!\n")
            goto hang;
        }
        printf ("# argv{%d}={%s} #\n", index, tokenList[index] );
    };
#endif
*/

//
// Initialize the library.
//

    // Stage 1
    // #debug: put char
    //sc80(65,'1',0,0); 

// ---------------------------------------------------------
// #debug
// Probably this is the first message in the serial debug 
// when a new process is initializing. Let's make it 
// friendly visable.

    debug_print ("+\n");
    debug_print ("++ crt0.c: Initializing (commands/) process\n");
    debug_print ("+\n");


// ------------------------------------
// Initialize heap support.
// See: stdlib.c
    int rt_status = FALSE;
    rt_status = (int) libcInitRT();
    // #debug: put char
    if (rt_status != TRUE)
    {
        sc80(65,'e',0,0);
        //exit(1);
    }

    // Stage 2
    // #debug: put char
    //sc80(65,'2',0,0);

// ------------------------------------
// Initialize the stadard stream.
// OUT: void
// See: stdio.c
    stdioInitialize();

// #todo
// Chamar esse ao invés de chamar os dois acima.
//See: sysdeps/x86/x86start.c

    //x86start ( ( token_count, tokenList, default_env );

/*
#ifdef TEDITOR_VERBOSE
    //Inicializando o editor propriamente dito.
	printf("Calling main ... \n"); 
#endif
*/

//
// #bugbug: Commands do NOT have focus.
//

// Setup this thread as foreground thread,
// this way the app can receive input via stdin.
    // rtl_focus_on_this_thread();

// ===================================================
// #test
// Reading command line from a file.

    char buffer[4096];
    memset(buffer, 0, 512);

// Copy
// NO focus, we can't read from stdin, let's try stderr.
    int n=0;
    n = read(
            fileno(stderr),  //fileno(stdin),
            buffer,
            512 );
// finzalize
   buffer[511] = 0;

    //if(n<=0){
        //#bugbug: We can't do this
        //stdout = stderr;
        //printf("crt0.c: n<=0");
        //fflush(stdout);
    //}


// Depois de lido o stdin e colocada a cmdline no buffer local,
// então é hora de apagarmos os arquivo, para que outro
// programa consiga usar o arquivo.
// Tambem atualizaremos a estrutura em ring3.
// GRAMADO_SEEK_CLEAR = 1000.
// see in kernel: kunistd.c e kstdio.c.
    lseek( fileno(stdin), 0, 1000);
    rewind(stdin);

/*
    // from shared buffer
    // extra heap3.
    char *shared_buffer = (char *) 0x30E00000;  //extra heap 3.
    memcpy(buffer,shared_buffer,512);
    shared_buffer[511]=0;
*/

// ===================================================
// Tokenizing.
// Criando o ambiente.
// Transferindo os ponteiros do vetor para o ambiente.

    //tokenList[0] = strtok ( &shared_info[0], LSH_TOK_DELIM );
    tokenList[0] = strtok ( buffer, LSH_TOK_DELIM );
    
// Salva a primeira palavra digitada.
    token = (char *) tokenList[0];
    index=0; 

    while (token != NULL)
    {
        // Coloca na lista.
        // Salva a primeira palavra digitada.
        tokenList[index] = token;

        //#debug
        //printf("shellCompare: %s \n", tokenList[i] );

        token = strtok( NULL, LSH_TOK_DELIM );

        // Incrementa o índice da lista
        index++;

        // Salvando a contagem.
        token_count = index;
    };

// Finalizando a lista.
    tokenList[index] = NULL;
// ===================================================

// Changing  the interrupt flag and the iopl.
// The taskswithing will not work without this.

/*
funcionou ... mas a proteção só é ativada 
ao fim da inicialização da libc ... 
pouco antes de saltar para a função main()
isso significa que eu posso decidir se a libc 
ativa a proteção ou não ... dependendo se 
é um driver ou aplicativo normal.
por exemplo: o crt0 do aplicativo ativa a proteçao 
e o crt0 do driver, não ativa.
*/

// The IOPL defines the minimum CPL required to 
// directly access I/O ports and to 
// execute I/O Sensitive Instructions (IN, INS, OUT, OUTS, CLI, STI). 

    asm volatile ("int $199");

//
// Call main().
//

    // IN: argc, argv.
    main_ret = (int) main( token_count, tokenList );

// Check against stdlin.h
    switch (main_ret){
    case EXIT_SUCCESS:
        exit(EXIT_SUCCESS);
        break;
    case EXIT_FAILURE:
        exit(EXIT_FAILURE);
        break;
    default:
        exit(-1);
        break; 
    };

//
// No return!
//

    //printf ("libc03-crts-crt0: *fail\n");
    exit(-1);

    //printf ("libc03-crts-crt0: *hang\n");
    while (1) { 
        asm ("pause"); 
    };
}


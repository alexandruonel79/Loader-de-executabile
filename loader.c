/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include "exec_parser.h"
#include <unistd.h>
#include <fcntl.h>

static so_exec_t *exec;
static int fd;
static int* vPagini;    //vectorul pentru pagini
static int vContor=0;   //contorul vectorului pentru pagini

static int cautaPagina(int nrpagina,int vaddr) {

    for (int i=0; i<vContor; i++) {
        if (vPagini[i]==vaddr+nrpagina*getpagesize())
            return 1;
    }
    return 0;
}

static void segv_handler(int signum, siginfo_t *info, void *context) {
    //primul pas gasesc segmentul si pagina

    int gasit=0;//daca nu gasesc niciun segment

    for (int i=0; i<exec->segments_no; i++) {//parcurg segmentele
        //verific daca ma incadrez in segmentul curent
        if ((uintptr_t)info->si_addr>=(uintptr_t)exec->segments[i].vaddr &&
            (uintptr_t)info->si_addr<=(uintptr_t)exec->segments[i].vaddr+exec->segments[i].mem_size) {

            //ma aflu pe segmentul bun
            gasit=1;

            uintptr_t nrpagina=-1;  //pagina problematica

            for (uintptr_t j=exec->segments[i].vaddr; j<exec->segments[i].vaddr+exec->segments[i].mem_size; j+=getpagesize()) {
                nrpagina++;

                //conditia pentru a fi pagina cautata
                if ((j<=(uintptr_t)info->si_addr) && ((uintptr_t)info->si_addr<(j+getpagesize()))) {
                    break;
                }
            }

            if (cautaPagina(nrpagina,exec->segments[i].vaddr) == 0) {   //daca pagina nu a mai fost mapata,o mapez
                //trebuie mapata
                void *adresa = mmap((void*)(exec->segments[i].vaddr + nrpagina * getpagesize()), getpagesize(),
                                PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED,
                                fd, 0);     //mapez pagina
                //mapare nereusita ->segm fault
                if (adresa==MAP_FAILED)
                    SIG_DFL(signum);

                //citesc datele din executabil 
                //verific cazul in care ma incadrez 
                if (exec->segments[i].file_size>getpagesize()*nrpagina) {
                    //segm fault daca pozitionarea esueaza
                   if (lseek(fd,getpagesize()*nrpagina+exec->segments[i].offset,SEEK_SET) == -1) {
                        SIG_DFL(signum);
                   }
                    //citim datele si verificam daca depaseste
                    if (exec->segments[i].file_size<(nrpagina+1)*getpagesize()) {    
                        //segm fault daca citirea esueaza
                        if (read(fd, adresa, exec->segments[i].file_size-getpagesize()*nrpagina) == -1) {
                            SIG_DFL(signum);
                        }   
                    }
                    else {//segm fault daca citirea esueaza
                        if (read(fd, adresa, getpagesize()) == -1){
                            SIG_DFL(signum);
                        }    
                    }
                }

                //pun permisiunile
                if (mprotect(adresa,getpagesize(),exec->segments[i].perm) == -1){
                    SIG_DFL(signum);
                }  

                //marchez ca mapata punand adresa paginii in vector
                vPagini[vContor]=exec->segments[i].vaddr+nrpagina*getpagesize();    

                vContor++;  //cresc numarul de pagini mapate
                
            }
            else {
                SIG_DFL(signum);    //daca pagina a fost deja mappata folosim dau segm fault
            }

        }
    }
    //nu face parte din segmente deci segm fault
    if (gasit == 0) {  
        SIG_DFL(signum);
    }

}

int so_init_loader(void)
{
    int rc;
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = segv_handler;
    sa.sa_flags = SA_SIGINFO;
    rc = sigaction(SIGSEGV, &sa, NULL);
    if (rc < 0)
    {
        perror("sigaction");
        return -1;
    }
    return 0;
}

static int init_vectorfrecv()
{
    int contor=0;

    for(int i=0; i<exec->segments_no; i++) {
        //calculez numarul de pagini
        contor+=exec->segments[i].mem_size/getpagesize()*2;
    }

    vPagini=calloc(contor,sizeof(int));

    if(vPagini==NULL) {
        return -1;
    }

    return 0;
}
int so_execute(char *path, char *argv[])
{

    exec = so_parse_exec(path);

    fd = open(path, O_RDONLY);

    if (init_vectorfrecv() == -1) {
        printf("alocare nereusita\n");
        return -1;
    }

    if (!exec)
        return -1;

    so_start_exec(exec, argv);

    return -1;
}

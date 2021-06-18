#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#define MESSAGESIZE 4096

int main (int argc, char *argv[]) {

    int server;
    int cliente; 
    if((cliente = open("tmp/myfifocliente",O_WRONLY))<0){
            perror("Erro ao abrir o pipe do cliente"); 
            return -1;
        }

    if(!strcmp(argv[1],"status")){
        write(cliente, argv[1],strlen(argv[1]));
        close(cliente);
    }
    else if(!strcmp(argv[1],"transform")){ 
        char mensagem[5000];
        char res[5000];
        res[0] = 0; 

        
        for(int cont = 1; cont<argc;cont++){
            sprintf(mensagem,argv[cont]);
            strcat(res,mensagem);
            strcat(res," ");
        }
        write(cliente, res,strlen(res));
        close(cliente);

    }

    if ((server = open("tmp/myfifoserver",O_RDONLY))<0){
        perror("Erro ao abrir o pipe do server");
        return -1; 
    }

    int leitura = 0; 
    char buffer[5000];
    while(1){
        if((leitura = read(server,buffer,5000))>0){
            write(1,buffer,leitura);
        }
    }
}
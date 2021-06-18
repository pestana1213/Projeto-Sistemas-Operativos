#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h> 
#include <errno.h>
#include <poll.h>

int server,cliente;
int numprocessos = 0; 
int altoc, baixoc, ecoc, rapidoc, lentoc;
int alto, baixo, eco, rapido, lento;
char *tasks[1024]; 
int podecomecar; 
char *altoo, *baixoo, *ecoo, *rapidoo, *lentoo; 

typedef struct polllfd {
    int   fd;         /* file descriptor */
    short events;     /* requested events */
    short revents;    /* returned events */
}polllfd;

typedef struct quequetarefas{
    char **processoemespera;
    int tamanho; 
    int posicao;  
}quequetarefas; 

int executarqueque (quequetarefas *q) {
    if (q->tamanho >= 0 && q->posicao <= q->tamanho) {
        if (deixacomecar(q->processoemespera[q->posicao])) return 1;
    }
    return 0;
}

int deixacomecar (char *command) {
    char *comando = strdup(command);
    char *found;
    for (int i = 0; i < 3; i++) {
        found = strsep(&comando, " ");
    }
    found = strsep(&comando, " ");
    do {
        if (!strcmp(found, "alto")) if (altoc >= alto) return 0;
        if (!strcmp(found, "baixo")) if (baixoc >= baixo) return 0;
        if (!strcmp(found, "eco")) if (ecoc >= eco) return 0;
        if (!strcmp(found, "rapido")) if (rapidoc >= rapido) return 0;
        if (!strcmp(found, "lento")) if (lentoc >= lento) return 0;
    } while ((found = strsep(&comando, " ")) != NULL);
    free(comando);
    return 1;
}

void terminar_handler(int signum){
    for (int i = 0; i<=numprocessos;i++) wait(NULL); //Espera que todos os filhos acabem de ser executados
    write(1,"\nServer terminado com sucesso\n",strlen("\nServer terminado com sucesso\n"));
    _exit(0);
}

void filhoterminou_handler(int signum) {
    char *tok = strdup(tasks[--numprocessos]); //Guarda o comando (Alterações necessárias aqui)
    strsep(&tok, " ");
    strsep(&tok, " ");
    strsep(&tok, " ");
    char *resto = strsep(&tok, "\n");
    libertaespaco(resto); //Coloca os filtros como novamente disponíveis.
    free(tok);
}

void libertaespaco(char *arg) {
    char *dup = strdup(arg);
    char *tok;
    while((tok = strsep(&dup, " "))) {
        if (!strcmp(tok, "alto")) altoc--;
        if (!strcmp(tok, "baixo")) baixoc--;
        if (!strcmp(tok, "eco")) ecoc--; 
        if (!strcmp(tok, "rapido")) rapidoc--;
        if (!strcmp(tok, "lento")) lentoc--;
    }
    free(dup);
}

int transform(char *acao){ //usado para executar o transform em pedidos que estao na queque 
    char *guardaacao[5000]; 
    guardaacao[0] = 0;

    strcat(guardaacao,acao);
    char mensagem[5000];
    char res[5000];
    res[0] = 0; 
    char *found;   
    char *output,*input,*comando[100];  
    int contador = 1; 
    char *token = strtok(acao, " ");
    char *comand;
    podecomecar = 1;
    int nroacoes = 0; 
    do{
        char *aux = strdup(token);
        char *found = strsep(&aux, " ");
        
        if (contador == 2){
            input = found; 
        }
        if (contador == 3){
            output = found; 
        }
        if (contador >= 4){
            comando[contador-4] = found;
            nroacoes ++;
        }
        contador ++; 
    }while((token = strtok(NULL," ")));


    for (int i=0; i<nroacoes; i++){ 
        if(!strcmp(comando[i],"alto")){
            if(altoc<alto){
                altoc ++; 
            }
            else{
                podecomecar = 0; 
            }
            comand = ("bin/aurrasd-filters/aurrasd-gain-double");
        }
        else if(!strcmp(comando[i],"eco")){
            if(ecoc<eco){
                ecoc ++; 
            }
            else{
                podecomecar = 0; 
            }
            comand = ("bin/aurrasd-filters/aurrasd-echo");
        }
        else if(!strcmp(comando[i],"rapido")){
            if(rapidoc<rapido){
                rapidoc ++; 
            }
            else{
                podecomecar = 0; 
            }
        comand = ("bin/aurrasd-filters/aurrasd-tempo-double");
        }
        else if(!strcmp(comando[i],"lento")){
            if(lentoc<lento){
                lentoc ++; 
            }
            else{
                podecomecar = 0; 
            }
        comand = ("bin/aurrasd-filters/aurrasd-tempo-half");
        }
        else if(!strcmp(comando[i],"baixo")){
            if(baixoc<baixo){
                baixoc ++; 
            }
            else{
                podecomecar = 0; 
            }
        comand = ("bin/aurrasd-filters/aurrasd-gain-half");
        }
    }

    if(podecomecar == 1){    
        sprintf(mensagem,"A processar...\n");
        strcat(res,mensagem);
        write(server,res,strlen(res));
        tasks[numprocessos] = strdup(guardaacao);
        numprocessos ++;

        if(!fork()){  
            int input_f;
            if ((input_f = open(input, O_RDONLY)) < 0) {
                perror("Error opening input file");
                return -1;
            }

            dup2(input_f, 0);
            close(input_f);
            
            int output_f;
            if ((output_f = open(output, O_CREAT | O_TRUNC | O_WRONLY)) < 0)
            {
                perror("Error creating output file");
                return -1;
            }

            dup2(output_f, 1);
            close(output_f);

            execvp(*comand, comand);
            _exit(0);
        }
    }
}

int main (int argc, char *argv[]) {
    int lidos = 0;
    alto = baixo = eco = rapido = lento = 0;
    altoc = baixoc = ecoc = rapidoc = lentoc = 0;  
    int leitura = 0; 
    int count = 0;
    char acao[1024];
    polllfd cpusave; 
    quequetarefas *q;
    struct stat teste;
    q = calloc(1, sizeof(struct quequetarefas));
    q->processoemespera = (char **) calloc(100, sizeof(char *));
    q->posicao = 0;
    q->tamanho = -1;

    if(stat("tmp/myfifocliente",&teste)<0){
        if(errno!=ENOENT){
            perror("Erro no stat");
            return -1;
        }
    }
    else{
        if(unlink("tmp/myfifocliente")<0){
            perror("Erro no unlink");
            return -1;
        }
    }
    
    if(stat("tmp/myfifoserver",&teste)<0){
        if(errno!=ENOENT){
            perror("Erro no stat");
            return -1;
        }
    }
    else{
        if(unlink("tmp/myfifoserver")<0){
            perror("Erro no unlink");
            return -1;
        }
    }

    if (mkfifo("tmp/myfifocliente", 0600) == -1) {
        perror("Erro a abrir o pipe");
        return 1;
    }
    
    if (mkfifo("tmp/myfifoserver", 0600) == -1) {
        perror("Erro a abrir o pipe");
        return 1;
    }
    signal(SIGCHLD, filhoterminou_handler);
    signal(SIGINT,terminar_handler);

    //inicializaçao 
    if (argc == 3) {
        char buffer[1024];
        int fd_config;
        if ((fd_config = open(argv[1], O_RDONLY)) == -1) {
            perror("Erro ao abrir o ficheiro de configuração");
            return 1;
        }
        while ((lidos = read(fd_config, buffer, 1024) > 0)) {
            char *token = strtok(buffer, "\n"); 
            do {
                char *aux = strdup(token);
                char *found = strsep(&aux, " ");

                if (!strcmp(found, "alto")) {
                    altoo = strdup(strsep(&aux, " "));
                    alto = atoi(strsep(&aux, "\n"));
                }
                else if (!strcmp(found, "baixo")) {
                    baixoo = strdup(strsep(&aux, " "));
                    baixo = atoi(strsep(&aux, "\n"));
                }
                else if (!strcmp(found, "eco")) {
                    ecoo = strdup(strsep(&aux, " "));
                    eco = atoi(strsep(&aux, "\n"));
                }
                else if (!strcmp(found, "rapido")) {
                    rapidoo = strdup(strsep(&aux, " "));
                    rapido = atoi(strsep(&aux, "\n"));
                }
                else {
                    lentoo = strdup(strsep(&aux, " "));
                    lento = atoi(strsep(&aux, "\n"));
                }
            } while ((token = strtok(NULL, "\n")));
        }
        close(fd_config); 
    }
    else{
        perror(" Impossivel abrir servidor");
        _exit(1);
    }

   if((cliente = open("tmp/myfifocliente",O_RDONLY))<0){
        perror("Erro ao abrir o pipe do cliente");
        return -1;
    }
    if((server = open("tmp/myfifoserver",O_WRONLY))<0){
        perror("Erro ao abrir o pipe do servidor");
        return -1;
    }

    cpusave.fd = cliente; 
    cpusave.events = POLLIN;
    cpusave.revents = POLLOUT;

    while(1){
        if (executarqueque(q) == 1) { //Verifica se pode executar algum pedido que esta na queque.
            char *comandoQ = strdup(q->processoemespera[q->posicao]);
            q->posicao++;
                if(deixacomecar(strdup(comandoQ))) {
                    write(server, "Processing...\n", strlen("Processing...\n"));
                    transform(comandoQ);
            }
        }

        poll(cpusave.fd,1,-1);
        leitura = read(cliente, acao,1024);
        acao[leitura] = 0; //em cada iteraçao "limpa" o que esta na consola

        if(leitura<0){
            perror("Erro no a ler o ficheiro");
            exit(1);
        }
             
        if(!strcmp(acao,"status")){
            char mensagem[5000];
            char res[5000];
            res[0] = 0; 
            for(int i=0;i<numprocessos;i++){    
                sprintf(mensagem,"\nTask #%d: %s",i+1,tasks[i]); 
                strcat(res,mensagem);
            }

            sprintf(mensagem, "\nStatus: \n");
            strcat(res,mensagem);
            sprintf(mensagem, "Filtro alto: %d/%d (Current/Max)\n", altoc, alto);
            strcat(res,mensagem);
            sprintf(mensagem, "Filtro baixo: %d/%d (Current/Max)\n", baixoc, baixo);
            strcat(res,mensagem);
            sprintf(mensagem, "Filtro eco: %d/%d (Current/Max)\n", ecoc, eco);
            strcat(res,mensagem);
            sprintf(mensagem, "Filtro rapido: %d/%d (Current/Max)\n", rapidoc, rapido);
            strcat(res,mensagem);
            sprintf(mensagem, "Filtro lento: %d/%d(Current/Max)\n", lentoc, lento);
            strcat(res,mensagem);
            sprintf(mensagem, "Pid: %d\n",getpid());
            strcat(res,mensagem);
            write(server,res, strlen(res));
        }
        
        else if(!strncmp(acao,"transform",9)){
            char *guardaacao[5000]; 
            guardaacao[0] = 0;

            strcat(guardaacao,acao);
            char mensagem[5000];
            char res[5000];
            res[0] = 0; 
            char *found;   
            char *output,*input,*comando[100];  
            int contador = 1; 
            char *token = strtok(acao, " ");
            char *comand;
            podecomecar = 1;
            int nroacoes = 0; 
            do{
                char *aux = strdup(token);
                char *found = strsep(&aux, " ");
                
                if (contador == 2){
                    input = found; 
                }
                if (contador == 3){
                    output = found; 
                }
                if (contador >= 4){
                    comando[contador-4] = found;
                    nroacoes ++;
                }
                contador ++; 
            }while((token = strtok(NULL," ")));


            for (int i=0; i<nroacoes; i++){ 
                if(!strcmp(comando[i],"alto")){
                    if(altoc<alto){
                        altoc ++; 
                    }
                    else{
                        podecomecar = 0; 
                    }
                    comand = ("bin/aurrasd-filters/aurrasd-gain-double");
                }
                else if(!strcmp(comando[i],"eco")){
                    if(ecoc<eco){
                        ecoc ++; 
                    }
                    else{
                        podecomecar = 0; 
                    }
                    comand = ("bin/aurrasd-filters/aurrasd-echo");
                }
                else if(!strcmp(comando[i],"rapido")){
                    if(rapidoc<rapido){
                        rapidoc ++; 
                    }
                    else{
                        podecomecar = 0; 
                    }
                comand = ("bin/aurrasd-filters/aurrasd-tempo-double");
                }
                else if(!strcmp(comando[i],"lento")){
                    if(lentoc<lento){
                        lentoc ++; 
                    }
                    else{
                        podecomecar = 0; 
                    }
                comand = ("bin/aurrasd-filters/aurrasd-tempo-half");
                }
                else if(!strcmp(comando[i],"baixo")){
                    if(baixoc<baixo){
                        baixoc ++; 
                    }
                    else{
                        podecomecar = 0; 
                    }
                comand = ("bin/aurrasd-filters/aurrasd-gain-half");
                }
            }
            //char *comand = executavel(input,output,comando[0]);
            if(podecomecar == 1){    
                sprintf(mensagem,"A processar...\n");
                strcat(res,mensagem);
                write(server,res,strlen(res));
                tasks[numprocessos] = strdup(guardaacao);
                numprocessos ++;

                if(!fork()){  
                    int input_f;
                    if ((input_f = open(input, O_RDONLY)) < 0) {
                        perror("Error opening input file");
                        return -1;
                    }

                    dup2(input_f, 0);
                    close(input_f);
                    
                    int output_f;
                    if ((output_f = open(output, O_CREAT | O_TRUNC | O_WRONLY)) < 0)
                    {
                        perror("Error creating output file");
                        return -1;
                    }

                    dup2(output_f, 1);
                    close(output_f);

                    execvp(*comand, comand);
                    _exit(0);
                }
            }
            else
            {
                write(server,"Pending...", strlen("Pending..."));
                q->processoemespera[++q->tamanho] = strdup(comando);
            }
        }
    }
}

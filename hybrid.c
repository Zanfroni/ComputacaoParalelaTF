#include <stdio.h>
#include <time.h>
#include "mpi.h"
#include "omp.h"

#define TAREFAS 500000 // Numero de tarefas no saco de trabalho, processo 0 é o mestre
#define OMP_THREADS 8 // Numero de Threads que o Open MP vai separar
double best_value = 0; // Variavel global representando o melhor valor encontrado (precisa ser protegida)

// Mensagens de comunicacao
int KILL = -1;
int REQUEST = -2;
int RESULT = -3;

// Funcao que realiza a soma de indices do vetor
// Ela basicamente pega o indice x e soma com TODOS
// os valores do vetor. Faz isso para todos os indices do vetor
// resultando em complexidade O(n^2)
double generateCombinations(float v[], int index) {
                
    // Para o OpenMP, o mesmo pode ser trabalhado dentro do codigo
    // Foi definido que os indices serao repartidos pelo numero de threads
    // e foi necessario proteger a variavel best_value, por ser critica para
    // os resultados adquiridos. Foi necessario declarar variavel de cada como
    // aux_value e zona critica para best_value (decisao perigosa)
    double aux_value = 0;
    int k;

        for(int k = 0; k < TAREFAS; k++){
            //printf("threads passou %d\n", omp_get_thread_num());
            aux_value = v[k] + v[index];

            // Definir como zona critica compromteu violentamente
            // o desempenho, tornando seu tempo pior que o do
            // sequencial. A remocao tornou extremamente mais
            // rapido, mas os resultados saem em grande parte erraticos
            #pragma omp critical
                if(aux_value > best_value){
                    best_value = aux_value;
                }
            //printf("Valor %.1f\n", aux_value);
        }
}


// Funcao principal, completamente remodelada para funcionar com
// MPI com iniciativa dos escravos.
int main(int argc, char** argv) {
    
    int my_rank; // Identificador deste processo
    int proc_n; // Numero de processos disparados pelo usuário na linha de comando (np)
    int message; // Buffer para as mensagens
    MPI_Status status; // Estrutura que guarda o estado de retorno de mensagens MPI

    MPI_Init(&argc , &argv); // funcao que inicializa o MPI, todo o código paralelo esta abaixo

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); // pega pega o numero do processo atual (rank)
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n); // pega informação do numero de processos (quantidade total)


    /*Inicio da secao das variaveis a serem testadas no algoritmo*/
    int slaves = proc_n - 1;
    int work = TAREFAS;
    int seed;
    float v[TAREFAS];

    for(int w = 0; w < TAREFAS; w++){
        v[w] = w;
    }
    
    int mpi_seed;
    double mpi_value;
    /*Fim da secao*/
    
    
    
    
    // LOGICA DO MESTRE
    if(my_rank == 0){
        
        // Inicia contagem de tempo. Nota-se que o proprio mestre
        // finaliza a contagem quando todos os escravos estiverem mortos
        clock_t t;
        t = clock();
        
        while(slaves > 0){
            // Mestre inicialmente recebe uma mensagem dos escravos
            // Esta mensagem precisa ser algo do escravo que represente um REQUEST ou RESULT,
            // caso contrario o mestre VAI ignorar
            MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
            
            // Se for REQUEST, ele tem que mandar um seed para o escravo
            // trabalhar e fazer as somas
            if(message == REQUEST){
                // CASO nao tenha mais trabalho, ele pede para o escravo
                // se matar
                if(work == 0){
                    MPI_Send(&KILL, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
                    slaves--;
                    continue;
                }
                
                // Agora ele tem que mandar para o escravo o seed para geracao das permutacoes
                seed = work;
                MPI_Send(&seed, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
                work -= OMP_THREADS;
                
            // Se for RESULT, ele tem que pegar os dados do escravo,
            // que neste caso sao o melhor valor encontrado com as permutacoes
            // e o indice usado para a soma
            } else if(message == RESULT) {
                
                MPI_Recv(&mpi_seed, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
                MPI_Recv(&mpi_value, 1, MPI_DOUBLE, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
                //printf("Trabalho no indice %d finalizado. Melhor valor foi %f\n",mpi_seed,mpi_value);
            }
        }
        
        t = clock() - t;
        double time_taken = ((double)t)/CLOCKS_PER_SEC;
        printf("Tempo de execucao: %f segundos\n", time_taken);
        
        
        
        
        
    // LOGICA DO ESCRAVO
    } else {
        while(1){
            
            // Peco trabalho para o mestre
            MPI_Send(&REQUEST, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            
            //Recebe o indice para trabalhar
            MPI_Recv(&message, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
            
            //Se mata se a mensagem recebida for KILL
            if(message == -1){
                MPI_Finalize();
                return 0;
            }

            int t = message;
            
            // Realiza as operacoes no algoritmo escolhido.
            #pragma omp parallel for num_threads(OMP_THREADS)
                for(int r = t; r > r-OMP_THREADS; r--){
                    // PRINT PARA DEBUG E CONFIRMACAO QUE ESTA FUNCIONANDO
                    //printf("ESTA THREAD: %d, DA MAQUINA %d, FICOU COM INDICE: %d\n",omp_get_thread_num(), my_rank, r);
                    generateCombinations(v, r);
                }
            
            // Avisa para o mestre que vai enviar os resultados
            MPI_Send(&RESULT, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            
            // Manda o indice e o valor final para o mestre
            MPI_Send(&t, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(&best_value, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
        }
    }
    // Mestre se mata e o codigo se encerra
    MPI_Finalize();
    return 0;
}

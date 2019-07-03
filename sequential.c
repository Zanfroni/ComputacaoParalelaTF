#include <stdio.h>
#include <time.h>

// Variaveis auxiliares
#define VETOR_SIZE 500000
double best_value = 0;

// Funcao que realiza a soma de indices do vetor
// Ela basicamente pega o indice x e soma com TODOS
// os valores do vetor. Faz isso para todos os indices do vetor
// resultando em complexidade O(n^2)
double generateCombinations(float v[], int index) {
                
    double aux_value = 0;
    for(int k = 0; k < VETOR_SIZE; k++){
        aux_value = v[k] + v[index];
        if(aux_value > best_value){
            best_value = aux_value;
        }
        //printf("Valor %.1f\n", aux_value);
    }
}


// Funcao sequencial
// Simplesmente realiza uma iteracao do tamanho do vetor e chama a cada iteracao a funcao que faz o calculo,
// mandando o seu indice. Isto sera feito (VETOR_SIZE x VETOR_SIZE) vezes, sendo de complexidade O(n^2)
int main() {
    float v[VETOR_SIZE];
    for(int w = 0; w < VETOR_SIZE; w++){
        v[w] = w;
    }
    
    clock_t t;
    t = clock();
    
    for(int r = 0; r < VETOR_SIZE; r++){
        generateCombinations(v, r);
        //printf("\n");
    }
    
    printf("\n\n\nBEST VALUE FINAL FOI %.2f\n\n\n", best_value);
    
    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Tempo de execucao: %f segundos\n", time_taken);
}

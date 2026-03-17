#include <stdio.h>
#include <stdlib.h>
#include "b_tree.h"

void main()
{    
    FILE *dados = fopen("fonte.txt", "a+");
    FILE *indice = fopen("indice.txt", "a+");
    
    char texto_linha[100];

    arvB btree = criarArv();

    fclose(dados);
    fclose(indice);

    return 0;
}


NO *criarNo() {

    NO *novo_no = (NO*) malloc(sizeof(NO));

    if(novo_no == NULL)
        return NULL;

    novo_no->eh_folha = 1;

    for(int i = 0; i < M; i++)
        novo_no->filhos[i] = NULL;

    return novo_no;
}

arvB criarArv(FILE *dados, FILE *arq_indice) {
    arvB btree = criarNo(); 

    char texto_linha[100];
    int posicao = 0;

    // O fgets lê a linha inteira de uma vez. Quando o laço repetir, 
    // ele já vai ler a próxima linha automaticamente.
    while(fgets(texto_linha, sizeof(texto_linha), dados) != NULL) {
        
        // 1. Salvar o tamanho da linha ANTES do strtok estragar a string original!
        int tamanho_linha = strlen(texto_linha);

        // 2. Extrair o PRIMEIRO valor (o índice)
        char *token = strtok(texto_linha, ",");
        if (token == NULL) continue; // Prevenção contra linhas vazias
        int id_indice = atoi(token);

        // 3. Extrair o SEGUNDO valor (a matrícula)
        token = strtok(NULL, ",");
        if (token == NULL) continue; // Prevenção contra linhas mal formatadas
        int matricula = atoi(token);

        // 4. Montar a struct da chave
        chave nova_chave;
        nova_chave.indice = id_indice;
        nova_chave.matricula = matricula;

        // 5. Cadastrar na árvore (e possivelmente gravar no arquivo de índice)
        cadastrar(arq_indice, nova_chave, btree);

        // 6. Atualizar a posição (deslocamento em bytes no arquivo)
        posicao += tamanho_linha;
    }

    return btree;
}

int cadastrar(FILE *arvore, chave nova_chave, NO *no) {
    if (no->eh_folha) {
        // Verifica se ainda há espaço no nó folha (M-1 é o máximo de chaves)
        if (no->num_chaves < M - 1) {
            
            // Começa da última chave inserida
            int i = no->num_chaves - 1;

            // Move as chaves maiores que a nova_chave para a direita para abrir espaço
            while (i >= 0 && nova_chave.matricula < no->chaves[i].matricula) {
                no->chaves[i + 1] = no->chaves[i];
                i--;
            }

            // Insere a chave no espaço vazio que encontramos (ou no final)
            no->chaves[i + 1] = nova_chave;
            no->num_chaves++;

            return 1; // Indicador de sucesso

        } else {
            // Nó folha está cheio. O Split (divisão do nó) seria tratado aqui.
            return 0; // Indicador de que precisa tratar o nó cheio
        }
    } else {
        // Se não for folha, encontra o filho correto para descer na árvore
        int i = no->num_chaves - 1;

        // Procura a posição do ponteiro do filho descendo até encontrar
        // uma chave menor que a nova chave
        while (i >= 0 && nova_chave.matricula < no->chaves[i].matricula) {
            i--;
        }
        
        // O índice correto do filho é i + 1
        i++; 

        // Faz a chamada recursiva descendo para o filho correspondente
        return cadastrar(arvore, nova_chave, no->filhos[i]);
    }
}

int pesquisar(FILE *arvore, int matricula);
int gravar(FILE *arvore);
int sair(FILE *arvore);
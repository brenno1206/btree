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


NO *criarNo(int id_pagina) {
    NO *novo_no = (NO*) malloc(sizeof(NO));

    if(novo_no == NULL)
        return NULL;

    novo_no->id_pagina = id_pagina;
    novo_no->eh_folha = 1;
    novo_no->num_chaves = 0;

    // Em disco, não existe 'NULL'. Usamos -1 para indicar que o filho não existe.
    for(int i = 0; i < M; i++) {
        novo_no->filhos[i] = -1;
    }

    return novo_no;
}


// Agora a função retorna o ID da página raiz (geralmente 0), em vez de um ponteiro na RAM
int criarArv(FILE *dados, FILE *arq_indice) {
    
    // 1. Cria a página raiz na RAM com ID 0
    NO *raiz = criarNo(0);
    if (raiz == NULL) return -1;

    // 2. Grava a raiz vazia no disco LOGO DE CARA em formato TXT com tamanho fixo.
    fseek(arq_indice, 0, SEEK_SET);
    
    // Formato exigido: nó, qtd, [chave, pos_na_fonte]..., [filho]...
    // Usamos %03d e %04d para garantir que o número de caracteres nunca mude.
    // Exemplo para M=3 (2 chaves, 3 filhos): 
    // 000, 0, [-001, -001], [-001, -001], [-001, -001, -001]
    fprintf(arq_indice, "%03d, %d, [%04d, %04d], [%04d, %04d], [%03d, %03d, %03d]\n", 
            raiz->id_pagina, 
            raiz->num_chaves,
            -1, -1, // Placeholder para Chave 1 (matricula, indice)
            -1, -1, // Placeholder para Chave 2 (matricula, indice)
            raiz->filhos[0], raiz->filhos[1], raiz->filhos[2]);

    // 3. Libera da RAM! 
    free(raiz);

    char texto_linha[100];
    int posicao_bytes_fonte = 0; // Isso será o nosso 'pos_na_fonte'

    while(fgets(texto_linha, sizeof(texto_linha), dados) != NULL) {
        int tamanho_linha = strlen(texto_linha);

        char *token = strtok(texto_linha, ",");
        if (token == NULL) continue;
        int id_indice = atoi(token); 

        token = strtok(NULL, ",");
        if (token == NULL) continue;
        int matricula = atoi(token);

        chave nova_chave;
        nova_chave.matricula = matricula;
        nova_chave.indice = posicao_bytes_fonte; 

        // 4. Inicia o cadastro sempre a partir da raiz (Página 0)
        cadastrar(arq_indice, nova_chave, 0);

        // 5. Atualiza o offset para a próxima linha do fonte.txt
        posicao_bytes_fonte += tamanho_linha;
    }

    return 0; // Retorna o ID da página raiz
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
#define B_TREE_H

#include <stdio.h>

#define M 3

typedef Pagina *ArvoreB;

typedef struct _dado {
    int indice;
    int matricula;
    char nome[50];
    int telefone;
} dado;

typedef struct _chave {
    int matricula; 
    int indice; // posição do registro no arquivo de dados
} chave;

typedef struct _NO {
    int id_pagina; // Identificador único para cada página (pode ser um número sequencial ou um offset no arquivo)
    chave chaves[M - 1]; // Cada nó pode conter no máximo M-1
    int filhos[M];
    int num_chaves;
    int eh_folha;
} NO;

typedef struct NO *arvB;

No *criarNo();

int cadastrar(FILE *arvore, dado *elementos, NO *no);
int pesquisar(FILE *arvore, int matricula);
int gravar(FILE *arvore);
int sair(FILE *arvore);

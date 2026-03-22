#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>
#include <stdlib.h>

#define M 3

typedef struct dado {
    int posicao;
    int matricula;
    char nome[50];
    char telefone[15];
} dado;

typedef struct chave {
    int matricula;
    int posicao;
} chave;

typedef struct NO {
    int qtd;
    chave chaves[M-1];
    struct NO *filhos[M];
    int eh_folha;
} NO;

typedef struct NO *arvB;

// Protótipos
arvB *criarArv(FILE *dados);
NO *criarNO();

int cadastrar(arvB *raiz, chave *chave_nova); 
int pesquisar(arvB *raiz, FILE *dados, int matricula);
int gravar(arvB *raiz, FILE *indice);
int destruirArvore(arvB *raiz);

#endif
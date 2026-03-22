#include <stdio.h>
#include <stdlib.h>
#include "btree.h"

// ============================================================================
// PROTÓTIPOS DE FUNÇÕES INTERNAS (AUXILIARES)
// Declarar aqui garante que o compilador as conheça, evitando erros de ordem.
// ============================================================================
int inserirRecursivo(NO *atual, chave *nova, chave *promovida, NO **novo_filho_dir);
void realizarSplit(NO *atual, chave *nova, NO *filho_da_nova, chave *promovida, NO **novo_filho_dir);
NO* buscarNaArvore(NO *atual, int matricula, int *pos_encontrada);
void mapearArvore(NO *atual, NO **mapa, int *contador_id);
int obterIdNo(NO **mapa, int total_nos, NO *alvo);
void liberarNo(NO *atual);

// ============================================================================
// FUNÇÃO PRINCIPAL
// ============================================================================
int main()
{
    // 'a+' para dados (adiciona no fim) e 'w+' para índice (sobrescreve o estado)
    FILE *dados = fopen("fonte.txt", "a+");
    FILE *indice = fopen("indice.txt", "w+");

    if (dados == NULL){
        printf("Erro: O arquivo 'fonte.txt' não foi aberto.\n");
        return 1;
    }

    if (indice == NULL){
        printf("Erro: O arquivo 'indice.txt' não foi aberto.\n");
        fclose(dados); // Boa prática: fechar o que já foi aberto em caso de erro
        return 1;
    }

    arvB* btree = criarArv(dados);

    if (btree == NULL) {
        printf("Falha ao criar arvore B.\n");
    } else {
        // Exemplo de uso: aqui você chamaria gravar(btree, indice), pesquisar(), etc.
        // gravar(btree, indice);
        destruirArvore(btree); // Limpa a memória antes de sair
    }

    fclose(dados);
    fclose(indice);

    return 0;
}

// ============================================================================
// 1. CRIAÇÃO E INICIALIZAÇÃO
// ============================================================================

NO *criarNO() {
    NO *no = (NO*) malloc(sizeof(NO));

    if(no == NULL){
        printf("Erro: Falha ao alocar memória para o nó.\n");
        return NULL;
    }

    no->qtd = 0;
    no->eh_folha = 1; // Por padrão, todo nó nasce como folha

    // Inicializa ponteiros com NULL para evitar lixo de memória
    for(int i = 0; i < M; i++){
        no->filhos[i] = NULL;
    }

    // Inicializa chaves com -1 (indicando espaço vazio)
    for(int i = 0; i < M-1; i++){
        no->chaves[i].matricula = -1;
        no->chaves[i].posicao = -1;
    }

    printf("[DEBUG] Nó criado com sucesso (folha=%d, qtd=%d)\n", no->eh_folha, no->qtd);
    return no;
}

arvB *criarArv(FILE *dados) {
    if (dados == NULL) {
        printf("Erro: Arquivo de dados é NULL em criarArv.\n");
        return NULL;
    }

    // Aloca o ponteiro que vai segurar a raiz da árvore
    arvB* raiz = (arvB*) malloc(sizeof(arvB));
    if(raiz == NULL){
        printf("Erro: Falha ao alocar memória para raiz.\n");
        return NULL;
    }

    *raiz = criarNO();
    if (*raiz == NULL) {
        printf("Erro: Falha ao criar o nó raiz.\n");
        free(raiz);
        return NULL;
    }
    
    rewind(dados);
    popularArv(raiz, dados); 

    return raiz;
}

int popularArv(arvB *raiz, FILE *dados) {
    char texto_linha[200];

    // Lê linha por linha do arquivo de dados original
    while(fgets(texto_linha, sizeof texto_linha, dados) != NULL) {
        int posicao = 0;
        int matricula = 0;

        // Tenta extrair a posição e a matrícula. Ignora o resto da string.
        if (sscanf(texto_linha, " %d , %d", &posicao, &matricula) == 2) {
            chave chave_lida = { matricula, posicao };
            cadastrar(raiz, &chave_lida); 
        } else {
            printf("Aviso: Linha com formato inesperado ignorada: %s", texto_linha);
        }
    }
    return 1;
}

// ============================================================================
// 2. INSERÇÃO E BALANCEAMENTO (SPLIT)
// ============================================================================

int cadastrar(arvB *raiz, chave *chave_nova) {
    if (raiz == NULL || *raiz == NULL || chave_nova == NULL) return -1;

    chave promovida;
    NO *novo_filho_dir = NULL;

    // Dispara a recursão para tentar inserir a chave
    int splitou = inserirRecursivo(*raiz, chave_nova, &promovida, &novo_filho_dir);

    // Se o retorno foi 1, significa que a raiz original transbordou e rachou ao meio
    if (splitou) {
        NO *nova_raiz = criarNO();
        nova_raiz->eh_folha = 0; // Nova raiz nunca é folha (terá filhos)
        nova_raiz->qtd = 1;
        nova_raiz->chaves[0] = promovida; // Recebe a chave que subiu
        
        // Conecta a antiga raiz à esquerda e o novo nó gerado à direita
        nova_raiz->filhos[0] = *raiz;
        nova_raiz->filhos[1] = novo_filho_dir;
        
        // Atualiza a árvore para apontar para o novo "topo"
        *raiz = nova_raiz; 
    }
    return 0;
}

int inserirRecursivo(NO *atual, chave *nova, chave *promovida, NO **novo_filho_dir) {
    int i = atual->qtd - 1;

    // CASO 1: Chegamos numa folha (onde a inserção real acontece)
    if (atual->eh_folha) {
        if (atual->qtd < M - 1) {
            // Acomoda a nova chave empurrando as maiores para a direita
            while (i >= 0 && atual->chaves[i].matricula > nova->matricula) {
                atual->chaves[i + 1] = atual->chaves[i];
                i--;
            }
            atual->chaves[i + 1] = *nova;
            atual->qtd++;
            return 0; // Inseriu sem problemas
        } else {
            // Nó está cheio: precisamos rachar a folha
            realizarSplit(atual, nova, NULL, promovida, novo_filho_dir);
            return 1; // Avisa o pai que houve split
        }
    } 
    // CASO 2: Nó interno (apenas navegamos descendo para o filho correto)
    else {
        // Encontra o ponteiro de descida correto
        while (i >= 0 && atual->chaves[i].matricula > nova->matricula) {
            i--;
        }
        i++; // Índice do filho escolhido

        chave ch_promovida_filho;
        NO *novo_filho = NULL;

        // Desce para o filho
        int splitou = inserirRecursivo(atual->filhos[i], nova, &ch_promovida_filho, &novo_filho);

        // Se o filho transbordou, precisamos acomodar a chave que subiu
        if (splitou) {
            if (atual->qtd < M - 1) {
                // Há espaço neste nó interno para acomodar a chave que subiu
                int j = atual->qtd - 1;
                while (j >= i) {
                    atual->chaves[j + 1] = atual->chaves[j];
                    atual->filhos[j + 2] = atual->filhos[j + 1];
                    j--;
                }
                atual->chaves[i] = ch_promovida_filho;
                atual->filhos[i + 1] = novo_filho;
                atual->qtd++;
                return 0; // Problema resolvido, não propaga mais o split
            } else {
                // Nó interno também está cheio (Split em cascata)
                realizarSplit(atual, &ch_promovida_filho, novo_filho, promovida, novo_filho_dir);
                return 1; // Propaga para o avô
            }
        }
        return 0; 
    }
}

// Função unificada de Split (serve tanto para folhas quanto para nós internos)
void realizarSplit(NO *atual, chave *nova, NO *filho_da_nova, chave *promovida, NO **novo_filho_dir) {
    chave temp_chaves[M];
    NO *temp_filhos[M + 1];

    // 1. Copia dados para arrays temporários maiores (M) para caber o estouro
    for (int j = 0; j < M - 1; j++) {
        temp_chaves[j] = atual->chaves[j];
        temp_filhos[j] = atual->filhos[j];
    }
    temp_filhos[M - 1] = atual->filhos[M - 1];

    // 2. Insere a nova chave ordenadamente no array temporário
    int i = M - 2;
    while (i >= 0 && temp_chaves[i].matricula > nova->matricula) {
        temp_chaves[i + 1] = temp_chaves[i];
        temp_filhos[i + 2] = temp_filhos[i + 1];
        i--;
    }
    temp_chaves[i + 1] = *nova;
    temp_filhos[i + 2] = filho_da_nova; // Se for folha, recebe NULL automaticamente

    // 3. Prepara o novo nó direito
    NO *novoNo = criarNO();
    novoNo->eh_folha = atual->eh_folha; // Herda a característica (se era folha, continua folha)
    int meio = M / 2;

    // 4. Mantém a metade inferior no nó atual
    atual->qtd = meio;
    for (int j = 0; j < meio; j++) {
        atual->chaves[j] = temp_chaves[j];
        atual->filhos[j] = temp_filhos[j];
    }
    atual->filhos[meio] = temp_filhos[meio]; 

    // 5. Separa a chave central para ser promovida ao pai
    *promovida = temp_chaves[meio];

    // 6. Move a metade superior para o novo nó direito
    novoNo->qtd = M - 1 - meio;
    for (int j = 0; j < novoNo->qtd; j++) {
        novoNo->chaves[j] = temp_chaves[meio + 1 + j];
        novoNo->filhos[j] = temp_filhos[meio + 1 + j];
    }
    novoNo->filhos[novoNo->qtd] = temp_filhos[M];

    // 7. Retorna o novo nó criado por referência
    *novo_filho_dir = novoNo;
}

// ============================================================================
// 3. BUSCA E RECUPERAÇÃO DE DADOS
// ============================================================================

NO* buscarNaArvore(NO *atual, int matricula, int *pos_encontrada) {
    if (atual == NULL) return NULL;
    
    int i = 0;
    while (i < atual->qtd && matricula > atual->chaves[i].matricula) {
        i++;
    }
    
    // Encontrou no nó atual
    if (i < atual->qtd && matricula == atual->chaves[i].matricula) {
        *pos_encontrada = atual->chaves[i].posicao;
        return atual; 
    }
    
    // Não encontrou e não tem para onde descer
    if (atual->eh_folha) return NULL;
    
    // Desce para o filho apropriado
    return buscarNaArvore(atual->filhos[i], matricula, pos_encontrada);
}

int pesquisar(arvB *raiz, FILE *dados, int matricula) {
    if (raiz == NULL || *raiz == NULL) return 0;

    int posicao_dado = -1;
    NO *encontrado = buscarNaArvore(*raiz, matricula, &posicao_dado);

    if (encontrado == NULL) {
        printf("Aviso: Matrícula %d não encontrada na Árvore B.\n", matricula);
        return 0;
    }

    // Como achamos na árvore, vamos cruzar com o arquivo texto
    rewind(dados); 
    char linha[256];
    int achou_no_arquivo = 0;
    dado info;

    while (fgets(linha, sizeof(linha), dados) != NULL) {
        int pos_lida, mat_lida;
        
        // Verifica primeiro apenas as chaves para ter performance
        if (sscanf(linha, "%d,%d", &pos_lida, &mat_lida) == 2) {
            if (pos_lida == posicao_dado && mat_lida == matricula) {
                // Lê o resto da linha se for a correta
                // Nota: info.telefone é char[], não usa '&'
                sscanf(linha, "%d,%d,%[^,],%s", &info.posicao, &info.matricula, info.nome, info.telefone);
                achou_no_arquivo = 1;
                break;
            }
        }
    }

    if (achou_no_arquivo) {
        printf("\n=== Registro Encontrado ===\n");
        printf("Posição..: %d\n", info.posicao);
        printf("Matrícula: %d\n", info.matricula);
        printf("Nome.....: %s\n", info.nome);
        printf("Telefone.: %s\n", info.telefone);
        printf("===========================\n");
        return 1;
    } else {
        printf("Erro Crítico: Chave na árvore não corresponde a um dado no arquivo.\n");
        return 0;
    }
}

// ============================================================================
// 4. PERSISTÊNCIA (GRAVAÇÃO DO ÍNDICE)
// ============================================================================

void mapearArvore(NO *atual, NO **mapa, int *contador_id) {
    if (atual == NULL) return;
    
    // Atribui um ID sequencial para o nó guardando no array
    mapa[*contador_id] = atual;
    (*contador_id)++;
    
    // Pré-ordem: Grava o pai e depois os filhos
    if (!atual->eh_folha) {
        for (int i = 0; i <= atual->qtd; i++) {
            mapearArvore(atual->filhos[i], mapa, contador_id);
        }
    }
}

int obterIdNo(NO **mapa, int total_nos, NO *alvo) {
    if (alvo == NULL) return -1;
    for (int i = 0; i < total_nos; i++) {
        if (mapa[i] == alvo) return i; // A posição no mapa vira o ID
    }
    return -1;
}

int gravar(arvB *raiz, FILE *indice) {
    if (raiz == NULL || *raiz == NULL) {
        printf("Aviso: Árvore vazia. Nada a gravar.\n");
        return 0;
    }
    if (indice == NULL) return 0;

    rewind(indice); // Garante que vamos sobrescrever desde o começo

    NO *mapa[1000]; 
    int total_nos = 0;
    
    // 1. Gera IDs numéricos para todos os ponteiros
    mapearArvore(*raiz, mapa, &total_nos);

    // 2. Grava no formato engessado para facilitar o load futuro
    for (int i = 0; i < total_nos; i++) {
        NO *no = mapa[i];

        // ID e ORDEM
        fprintf(indice, "%d,%d,", i, M); 

        // Chaves
        for (int j = 0; j < M - 1; j++) {
            if (j < no->qtd) fprintf(indice, "%d,", no->chaves[j].matricula);
            else fprintf(indice, "-1,"); 
        }

        // Posições
        for (int j = 0; j < M - 1; j++) {
            if (j < no->qtd) fprintf(indice, "%d,", no->chaves[j].posicao);
            else fprintf(indice, "-1,"); 
        }

        // Filhos (Substitui ponteiro pelo ID no array)
        for (int j = 0; j < M; j++) {
            if (!no->eh_folha && j <= no->qtd) {
                fprintf(indice, "%d", obterIdNo(mapa, total_nos, no->filhos[j]));
            } else {
                fprintf(indice, "-1"); 
            }
            if (j < M - 1) fprintf(indice, ",");
        }
        fprintf(indice, "\n"); // Quebra a linha do nó
    }

    printf("Sucesso: Árvore indexada (%d nós gravados no total).\n", total_nos);
    return 1;
}

// ============================================================================
// 5. GERENCIAMENTO DE MEMÓRIA (DESTRUIÇÃO)
// ============================================================================

void liberarNo(NO *atual) {
    if (atual == NULL) return;

    // Pós-ordem: Apaga os filhos ANTES de apagar o pai para não perder referências
    if (!atual->eh_folha) {
        for (int i = 0; i <= atual->qtd; i++) {
            liberarNo(atual->filhos[i]);
        }
    }
    free(atual); // Destrói de baixo para cima
}

int destruirArvore(arvB *raiz) {
    if (raiz == NULL) return 0; 

    if (*raiz != NULL) {
        liberarNo(*raiz); 
        *raiz = NULL; 
    }

    free(raiz); // Libera a casca principal da árvore
    printf("Sistema: Árvore destruída. Memória RAM liberada com sucesso!\n");
    return 1;
}
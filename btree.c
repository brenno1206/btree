#include <stdio.h>
#include <stdlib.h>
#include <string.h> // CORREÇÃO: Necessário para usar strcspn() no main
#include "btree.h"

// ============================================================================
// PROTÓTIPOS DE FUNÇÕES INTERNAS (AUXILIARES)
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
    FILE *dados = fopen("fonte.txt", "a+");
    FILE *indice = fopen("indice.txt", "w+");

    if (dados == NULL){
        printf("Erro: O arquivo 'fonte.txt' não foi aberto.\n");
        return 1;
    }

    if (indice == NULL){
        printf("Erro: O arquivo 'indice.txt' não foi aberto.\n");
        fclose(dados); 
        return 1;
    }

    arvB* btree = criarArv(dados);

    if (btree == NULL) {
        printf("Falha ao criar arvore B.\n");
        fclose(dados);
        fclose(indice);
        return 1;
    }

    // Conta as linhas do arquivo texto para descobrir a próxima "posição" disponível
    int proxima_posicao = 1;
    rewind(dados);
    char c;
    char ultimo_char = '\n';
    
    while((c = fgetc(dados)) != EOF) {
        if(c == '\n') proxima_posicao++;
        ultimo_char = c;
    }

    // === TRAVA DE SEGURANÇA ADICIONADA AQUI ===
    // Se o arquivo não está vazio e a última linha não terminou com uma quebra de linha
    if (ftell(dados) > 0 && ultimo_char != '\n') {
        proxima_posicao++; // Compensa a linha do arquivo que ficou sem o '\n'
        
        // Força a quebra de linha no arquivo para os próximos cadastros não grudarem
        fseek(dados, 0, SEEK_END);
        fprintf(dados, "\n");
        fflush(dados);
    }
    // ==========================================

    int opcao = 0;
    while (opcao != 4) {
        printf("\n========== MENU ARVORE B ==========\n");
        printf("1. Cadastrar Aluno\n");
        printf("2. Buscar Aluno\n");
        printf("3. Gravar Arvore\n");
        printf("4. Sair\n");
        printf("===================================\n");
        printf("Escolha uma opcao: ");
        
        if (scanf("%d", &opcao) != 1) {
            while(getchar() != '\n');
            printf("Entrada invalida!\n");
            continue;
        }

        switch(opcao) {
            case 1: {
                dado novo_aluno;
                printf("\n--- Cadastrar Novo Aluno ---\n");
                printf("Matricula: ");
                scanf("%d", &novo_aluno.matricula);
                getchar();

                printf("Nome: ");
                fgets(novo_aluno.nome, sizeof(novo_aluno.nome), stdin);
                novo_aluno.nome[strcspn(novo_aluno.nome, "\n")] = '\0';

                printf("Telefone: ");
                fgets(novo_aluno.telefone, sizeof(novo_aluno.telefone), stdin);
                novo_aluno.telefone[strcspn(novo_aluno.telefone, "\n")] = '\0';

                novo_aluno.posicao = proxima_posicao++;

                fseek(dados, 0, SEEK_END);
                fprintf(dados, "%d,%d,%s,%s\n", novo_aluno.posicao, novo_aluno.matricula, novo_aluno.nome, novo_aluno.telefone);
                fflush(dados);

                chave chave_nova = {novo_aluno.matricula, novo_aluno.posicao};
                cadastrar(btree, &chave_nova);
                
                printf("Sucesso: Aluno cadastrado! (Posicao interna: %d)\n", novo_aluno.posicao);
                break;
            }
            case 2: {
                int mat_busca;
                printf("\n--- Buscar Aluno ---\n");
                printf("Digite a Matricula: ");
                scanf("%d", &mat_busca);
                
                pesquisar(btree, dados, mat_busca);
                break;
            }
            case 3: {
                printf("\n--- Gravar Indice ---\n");
                gravar(btree, indice);
                fflush(indice);
                break;
            }
            case 4: {
                printf("\nEncerrando o sistema e limpando memoria...\n");
                break;
            }
            default:
                printf("\nOpcao invalida. Tente novamente.\n");
        }
    }

    destruirArvore(btree);
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
    no->eh_folha = 1;

    for(int i = 0; i < M; i++){
        no->filhos[i] = NULL;
    }

    for(int i = 0; i < M-1; i++){
        no->chaves[i].matricula = -1;
        no->chaves[i].posicao = -1;
    }

    return no;
}

arvB *criarArv(FILE *dados) {
    if (dados == NULL) return NULL;

    arvB* raiz = (arvB*) malloc(sizeof(arvB));
    if(raiz == NULL) return NULL;

    *raiz = criarNO();
    if (*raiz == NULL) {
        free(raiz);
        return NULL;
    }
    
    rewind(dados);
    popularArv(raiz, dados); 

    return raiz;
}

int popularArv(arvB *raiz, FILE *dados) {
    char texto_linha[200];

    while(fgets(texto_linha, sizeof texto_linha, dados) != NULL) {
        int posicao = 0;
        int matricula = 0;

        // CORREÇÃO: Formato ajustado para garantir extração correta baseada no arquivo
        if (sscanf(texto_linha, "%d,%d", &posicao, &matricula) == 2) {
            chave chave_lida = { matricula, posicao };
            cadastrar(raiz, &chave_lida); 
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

    int splitou = inserirRecursivo(*raiz, chave_nova, &promovida, &novo_filho_dir);

    if (splitou) {
        NO *nova_raiz = criarNO();
        nova_raiz->eh_folha = 0;
        nova_raiz->qtd = 1;
        nova_raiz->chaves[0] = promovida;
        
        nova_raiz->filhos[0] = *raiz;
        nova_raiz->filhos[1] = novo_filho_dir;
        
        *raiz = nova_raiz; 
    }
    return 0;
}

int inserirRecursivo(NO *atual, chave *nova, chave *promovida, NO **novo_filho_dir) {
    int i = atual->qtd - 1;

    if (atual->eh_folha) {
        if (atual->qtd < M - 1) {
            while (i >= 0 && atual->chaves[i].matricula > nova->matricula) {
                atual->chaves[i + 1] = atual->chaves[i];
                i--;
            }
            atual->chaves[i + 1] = *nova;
            atual->qtd++;
            return 0;
        } else {
            realizarSplit(atual, nova, NULL, promovida, novo_filho_dir);
            return 1;
        }
    } 
    else {
        while (i >= 0 && atual->chaves[i].matricula > nova->matricula) {
            i--;
        }
        i++;

        chave ch_promovida_filho;
        NO *novo_filho = NULL;

        int splitou = inserirRecursivo(atual->filhos[i], nova, &ch_promovida_filho, &novo_filho);

        if (splitou) {
            if (atual->qtd < M - 1) {
                int j = atual->qtd - 1;
                while (j >= i) {
                    atual->chaves[j + 1] = atual->chaves[j];
                    atual->filhos[j + 2] = atual->filhos[j + 1];
                    j--;
                }
                atual->chaves[i] = ch_promovida_filho;
                atual->filhos[i + 1] = novo_filho;
                atual->qtd++;
                return 0;
            } else {
                realizarSplit(atual, &ch_promovida_filho, novo_filho, promovida, novo_filho_dir);
                return 1;
            }
        }
        return 0; 
    }
}

void realizarSplit(NO *atual, chave *nova, NO *filho_da_nova, chave *promovida, NO **novo_filho_dir) {
    chave temp_chaves[M];
    NO *temp_filhos[M + 1];

    for (int j = 0; j < M - 1; j++) {
        temp_chaves[j] = atual->chaves[j];
        temp_filhos[j] = atual->filhos[j];
    }
    temp_filhos[M - 1] = atual->filhos[M - 1];

    int i = M - 2;
    while (i >= 0 && temp_chaves[i].matricula > nova->matricula) {
        temp_chaves[i + 1] = temp_chaves[i];
        temp_filhos[i + 2] = temp_filhos[i + 1];
        i--;
    }
    temp_chaves[i + 1] = *nova;
    temp_filhos[i + 2] = filho_da_nova;

    NO *novoNo = criarNO();
    novoNo->eh_folha = atual->eh_folha;
    int meio = M / 2;

    atual->qtd = meio;
    for (int j = 0; j < meio; j++) {
        atual->chaves[j] = temp_chaves[j];
        atual->filhos[j] = temp_filhos[j];
    }
    atual->filhos[meio] = temp_filhos[meio]; 

    *promovida = temp_chaves[meio];

    novoNo->qtd = M - 1 - meio;
    for (int j = 0; j < novoNo->qtd; j++) {
        novoNo->chaves[j] = temp_chaves[meio + 1 + j];
        novoNo->filhos[j] = temp_filhos[meio + 1 + j];
    }
    novoNo->filhos[novoNo->qtd] = temp_filhos[M];

    *novo_filho_dir = novoNo;
}

// ============================================================================
// 3. BUSCA E RECUPERAÇÃO DE DADOS
// ============================================================================

// ============================================================================
// 3. BUSCA E RECUPERAÇÃO DE DADOS
// ============================================================================

NO* buscarNaArvore(NO *atual, int matricula, int *pos_encontrada) {
    if (atual == NULL) return NULL;
    
    int i = 0;
    while (i < atual->qtd && matricula > atual->chaves[i].matricula) {
        i++;
    }
    
    if (i < atual->qtd && matricula == atual->chaves[i].matricula) {
        *pos_encontrada = atual->chaves[i].posicao;
        return atual; 
    }
    
    if (atual->eh_folha) return NULL;
    
    return buscarNaArvore(atual->filhos[i], matricula, pos_encontrada);
}

int pesquisar(arvB *raiz, FILE *dados, int matricula) {
    if (raiz == NULL || *raiz == NULL) return 0;

    int posicao_dado = -1;
    NO *encontrado = buscarNaArvore(*raiz, matricula, &posicao_dado);

    if (encontrado == NULL) {
        printf("Aviso: Matricula %d nao encontrada na Arvore B.\n", matricula);
        return 0;
    }

    rewind(dados); 
    char linha[256];
    int achou_no_arquivo = 0;
    dado info;

    // 1. TENTATIVA RÁPIDA: Pula direto para a linha calculada
    for (int i = 1; i < posicao_dado; i++) {
        if (fgets(linha, sizeof(linha), dados) == NULL) break; 
    }

    // Lê a linha alvo e usa um sscanf mais flexível (%s) que ignora quebras de linha sujas
    if (fgets(linha, sizeof(linha), dados) != NULL) {
        if (sscanf(linha, "%d,%d,%[^,],%s", &info.posicao, &info.matricula, info.nome, info.telefone) == 4) {
            if (info.matricula == matricula) {
                achou_no_arquivo = 1;
            }
        }
    }

    // 2. MODO DE SEGURANÇA (FALLBACK): Se o pulo falhou (ex: arquivo tem linhas em branco), varre do início
    if (!achou_no_arquivo) {
        rewind(dados);
        while (fgets(linha, sizeof(linha), dados) != NULL) {
            if (sscanf(linha, "%d,%d,%[^,],%s", &info.posicao, &info.matricula, info.nome, info.telefone) == 4) {
                if (info.posicao == posicao_dado && info.matricula == matricula) {
                    achou_no_arquivo = 1;
                    break;
                }
            }
        }
    }

    if (achou_no_arquivo) {
        printf("\n=== Registro Encontrado ===\n");
        printf("Posicao..: %d\n", info.posicao);
        printf("Matricula: %d\n", info.matricula);
        printf("Nome.....: %s\n", info.nome);
        printf("Telefone.: %s\n", info.telefone);
        printf("===========================\n");
        return 1;
    } else {
        printf("Erro Critico: Chave na arvore nao corresponde ao dado na posicao %d do arquivo.\n", posicao_dado);
        return 0;
    }
}
// ============================================================================
// 4. PERSISTÊNCIA (GRAVAÇÃO DO ÍNDICE)
// ============================================================================

void mapearArvore(NO *atual, NO **mapa, int *contador_id) {
    if (atual == NULL) return;
    
    mapa[*contador_id] = atual;
    (*contador_id)++;
    
    if (!atual->eh_folha) {
        for (int i = 0; i <= atual->qtd; i++) {
            mapearArvore(atual->filhos[i], mapa, contador_id);
        }
    }
}

int obterIdNo(NO **mapa, int total_nos, NO *alvo) {
    if (alvo == NULL) return -1;
    for (int i = 0; i < total_nos; i++) {
        if (mapa[i] == alvo) return i; 
    }
    return -1;
}

int gravar(arvB *raiz, FILE *indice) {
    if (raiz == NULL || *raiz == NULL) {
        printf("Aviso: Arvore vazia. Nada a gravar.\n");
        return 0;
    }
    if (indice == NULL) return 0;

    rewind(indice);

    NO *mapa[1000]; // Nota: Se sua árvore passar de 1000 nós, aumente este valor.
    int total_nos = 0;
    
    mapearArvore(*raiz, mapa, &total_nos);

    for (int i = 0; i < total_nos; i++) {
        NO *no = mapa[i];

        fprintf(indice, "%d,%d,", i, M); 

        for (int j = 0; j < M - 1; j++) {
            if (j < no->qtd) fprintf(indice, "%d,", no->chaves[j].matricula);
            else fprintf(indice, "-1,"); 
        }

        for (int j = 0; j < M - 1; j++) {
            if (j < no->qtd) fprintf(indice, "%d,", no->chaves[j].posicao);
            else fprintf(indice, "-1,"); 
        }

        for (int j = 0; j < M; j++) {
            if (!no->eh_folha && j <= no->qtd) {
                fprintf(indice, "%d", obterIdNo(mapa, total_nos, no->filhos[j]));
            } else {
                fprintf(indice, "-1"); 
            }
            if (j < M - 1) fprintf(indice, ",");
        }
        fprintf(indice, "\n");
    }

    printf("Sucesso: Arvore indexada (%d nos gravados no total).\n", total_nos);
    return 1;
}

// ============================================================================
// 5. GERENCIAMENTO DE MEMÓRIA (DESTRUIÇÃO)
// ============================================================================

void liberarNo(NO *atual) {
    if (atual == NULL) return;

    if (!atual->eh_folha) {
        for (int i = 0; i <= atual->qtd; i++) {
            liberarNo(atual->filhos[i]);
        }
    }
    free(atual); 
}

int destruirArvore(arvB *raiz) {
    if (raiz == NULL) return 0; 

    if (*raiz != NULL) {
        liberarNo(*raiz); 
        *raiz = NULL; 
    }

    free(raiz); 
    printf("Sistema: Arvore destruida. Memoria RAM liberada com sucesso!\n");
    return 1;
}
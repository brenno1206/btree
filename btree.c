#include <stdio.h>
#include <stdlib.h>
#include "btree.h"

int inserirRecursivo(NO *atual, chave *nova, chave *promovida, NO **novo_filho_dir);

int main()
{
    FILE *dados = fopen("fonte.txt", "a+");
    FILE *indice = fopen("indice.txt", "w+");

    if (dados == NULL){
        printf("O arquivo 'fonte.txt' não foi aberto.\n");
        return 1;
    }

    if (indice == NULL){
        printf("O arquivo 'indice.txt' não foi aberto.\n");
        return 1;
    }

    arvB* btree = criarArv(dados);

    if (btree == NULL) {
        printf("Falha ao criar arvore B.\n");
    }

    fclose(dados);
    fclose(indice);

    return 0;
}

NO *criarNO() {

    NO *no = (NO*) malloc(sizeof(NO));

    if(no == NULL){
        printf("Falha ao alocar memória para nó\n");
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

    printf("[DEBUG] Nó criado com sucesso (folha=%d, qtd=%d)\n", no->eh_folha, no->qtd);

    return no;
}

arvB *criarArv(FILE *dados) {
    if (dados == NULL) {
        printf("arquivo de dados eh NULL em criarArv\n");
        return NULL;
    }

    arvB* raiz = (arvB*) malloc(sizeof(arvB));

    if(raiz == NULL){
        printf("Falha ao alocar memória para raiz\n");
        return NULL;
    }

    *raiz = criarNO();

    if (*raiz == NULL) {
        printf("Falha ao criar no raiz\n");
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

        if (sscanf(texto_linha, " %d , %d", &posicao, &matricula) == 2) {
            chave chave_lida = { matricula, posicao };
            cadastrar(raiz, &chave_lida); 
        } else {
            printf("Linha com formato inesperado: %s", texto_linha);
        }
    }
    return 1;
}

int cadastrar(arvB *raiz, chave *chave_nova) {
    if (raiz == NULL || *raiz == NULL || chave_nova == NULL) return -1;

    chave promovida;
    NO *novo_filho_dir = NULL;

    // Tenta inserir a partir da raiz original
    int splitou = inserirRecursivo(*raiz, chave_nova, &promovida, &novo_filho_dir);

    if (splitou) {
        // Se a raiz dividiu, a árvore "cresce" criando uma nova raiz acima dela
        NO *nova_raiz = criarNO();
        nova_raiz->eh_folha = 0;
        nova_raiz->qtd = 1;
        nova_raiz->chaves[0] = promovida;
        
        // Os filhos da nova raiz são a raiz antiga e o novo nó gerado no split
        nova_raiz->filhos[0] = *raiz;
        nova_raiz->filhos[1] = novo_filho_dir;
        
        // Atualiza o ponteiro da árvore para apontar para a nova raiz
        *raiz = nova_raiz; 
    }

    return 0;
}


int inserirRecursivo(NO *atual, chave *nova, chave *promovida, NO **novo_filho_dir) {
    int i = atual->qtd - 1;

    if (atual->eh_folha) {
        if (atual->qtd < M - 1) {
            // Tem espaço na folha: apenas insere
            while (i >= 0 && atual->chaves[i].matricula > nova->matricula) {
                atual->chaves[i + 1] = atual->chaves[i];
                i--;
            }
            atual->chaves[i + 1] = *nova;
            atual->qtd++;
            return 0; // Não houve split
        } 
        else {
            // NÓ FOLHA CHEIO: Chama a função de split passando NULL como filho
            realizarSplit(atual, nova, NULL, promovida, novo_filho_dir);
            return 1; // Avisa o pai que houve split
        }
    } else {
        // NÃO É FOLHA: Descobre por qual filho descer
        while (i >= 0 && atual->chaves[i].matricula > nova->matricula) {
            i--;
        }
        i++; // Índice do filho correto

        chave ch_promovida_filho;
        NO *novo_filho = NULL;

        // Desce recursivamente
        int splitou = inserirRecursivo(atual->filhos[i], nova, &ch_promovida_filho, &novo_filho);

        if (splitou) {
            if (atual->qtd < M - 1) {
                // Tem espaço no nó interno: apenas acomoda a chave promovida
                int j = atual->qtd - 1;
                while (j >= i) {
                    atual->chaves[j + 1] = atual->chaves[j];
                    atual->filhos[j + 2] = atual->filhos[j + 1];
                    j--;
                }
                atual->chaves[i] = ch_promovida_filho;
                atual->filhos[i + 1] = novo_filho;
                atual->qtd++;
                return 0; // Split resolvido
            } 
            else {
                // NÓ INTERNO CHEIO: Chama a função de split passando o novo_filho
                realizarSplit(atual, &ch_promovida_filho, novo_filho, promovida, novo_filho_dir);
                return 1; // Propaga o split para cima
            }
        }
        return 0; 
    }
}

void realizarSplit(NO *atual, chave *nova, NO *filho_da_nova, chave *promovida, NO **novo_filho_dir) {
    chave temp_chaves[M];
    NO *temp_filhos[M + 1];

    // 1. Copia os dados atuais para os arrays temporários
    for (int j = 0; j < M - 1; j++) {
        temp_chaves[j] = atual->chaves[j];
        temp_filhos[j] = atual->filhos[j];
    }
    temp_filhos[M - 1] = atual->filhos[M - 1];

    // 2. Encontra a posição e insere a nova chave e seu filho no array temporário
    int i = M - 2;
    while (i >= 0 && temp_chaves[i].matricula > nova->matricula) {
        temp_chaves[i + 1] = temp_chaves[i];
        temp_filhos[i + 2] = temp_filhos[i + 1];
        i--;
    }
    temp_chaves[i + 1] = *nova;
    temp_filhos[i + 2] = filho_da_nova; // Se for folha, isso será NULL, o que é perfeito!

    // 3. Prepara a divisão
    NO *novoNo = criarNO();
    novoNo->eh_folha = atual->eh_folha; // O novo nó herda a "natureza" do nó original
    int meio = M / 2;

    // 4. Metade inferior volta para o nó atual
    atual->qtd = meio;
    for (int j = 0; j < meio; j++) {
        atual->chaves[j] = temp_chaves[j];
        atual->filhos[j] = temp_filhos[j];
    }
    atual->filhos[meio] = temp_filhos[meio]; // Último filho do lado esquerdo

    // 5. A chave do meio sobe
    *promovida = temp_chaves[meio];

    // 6. Metade superior vai para o novo nó direito
    novoNo->qtd = M - 1 - meio;
    for (int j = 0; j < novoNo->qtd; j++) {
        novoNo->chaves[j] = temp_chaves[meio + 1 + j];
        novoNo->filhos[j] = temp_filhos[meio + 1 + j];
    }
    novoNo->filhos[novoNo->qtd] = temp_filhos[M];

    // 7. Retorna o novo nó criado
    *novo_filho_dir = novoNo;
}


// Função auxiliar recursiva para descer na Árvore B e encontrar a posição
NO* buscarNaArvore(NO *atual, int matricula, int *pos_encontrada) {
    if (atual == NULL) return NULL;
    
    int i = 0;
    // Percorre as chaves do nó até encontrar o local certo
    while (i < atual->qtd && matricula > atual->chaves[i].matricula) {
        i++;
    }
    
    // Verificamos se encontramos a chave exata neste nó
    if (i < atual->qtd && matricula == atual->chaves[i].matricula) {
        *pos_encontrada = atual->chaves[i].posicao;
        return atual; 
    }
    
    // Se chegamos numa folha e a chave não está aqui, ela não existe
    if (atual->eh_folha) {
        return NULL;
    }
    
    // Se não for folha, desce recursivamente para o filho adequado
    return buscarNaArvore(atual->filhos[i], matricula, pos_encontrada);
}

// Função principal solicitada
int pesquisar(arvB *raiz, FILE *dados, int matricula) {
    if (raiz == NULL || *raiz == NULL) return 0;

    int posicao_dado = -1;
    NO *encontrado = buscarNaArvore(*raiz, matricula, &posicao_dado);

    if (encontrado == NULL) {
        printf("Matrícula %d não encontrada na Árvore B.\n", matricula);
        return 0;
    }

    // Achou na árvore! Agora vamos buscar o restante das infos no fonte.txt
    rewind(dados); // Volta para o começo do arquivo
    char linha[256];
    int achou_no_arquivo = 0;
    dado info;

    while (fgets(linha, sizeof(linha), dados) != NULL) {
        int pos_lida, mat_lida;
        
        // Lê rapidamente só os primeiros dados da linha para confirmar
        if (sscanf(linha, "%d,%d", &pos_lida, &mat_lida) == 2) {
            if (pos_lida == posicao_dado && mat_lida == matricula) {
                
                // Puxa as informações formatadas. O %[^,] serve para ler o nome 
                // lidando com os espaços, parando apenas quando bater na vírgula.
                // Caso a linha tenha dados extras concatenados no final, o sscanf
                // vai apenas focar nos campos que pedimos e ignorar o resto com segurança.
                sscanf(linha, "%d,%d,%[^,],%s", &info.posicao, &info.matricula, info.nome, &info.telefone);
                
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
        printf("Inconsistência: Chave encontrada na árvore, mas arquivo fonte não contém o dado.\n");
        return 0;
    }
}



// ==========================================
// FUNÇÕES AUXILIARES PARA GRAVAÇÃO
// ==========================================

// Função que percorre a árvore e guarda todos os nós em um array para dar um "ID" a eles
void mapearArvore(NO *atual, NO **mapa, int *contador_id) {
    if (atual == NULL) return;
    
    mapa[*contador_id] = atual;
    (*contador_id)++;
    
    // Se não for folha, mapeia os filhos também
    if (!atual->eh_folha) {
        for (int i = 0; i <= atual->qtd; i++) {
            mapearArvore(atual->filhos[i], mapa, contador_id);
        }
    }
}

// Função que busca qual é o "ID" numérico de um ponteiro específico
int obterIdNo(NO **mapa, int total_nos, NO *alvo) {
    if (alvo == NULL) return -1;
    for (int i = 0; i < total_nos; i++) {
        if (mapa[i] == alvo) return i; // Retorna a posição (que funciona como ID)
    }
    return -1;
}

// ==========================================
// FUNÇÃO PRINCIPAL DE GRAVAÇÃO
// ==========================================

int gravar(arvB *raiz, FILE *indice) {
    if (raiz == NULL || *raiz == NULL) {
        printf("Árvore vazia. Nada a gravar.\n");
        return 0;
    }
    if (indice == NULL) return 0;

    // Volta o cursor do arquivo para o início para reescrever o índice
    rewind(indice); 

    // Cria um mapa para até 1000 nós (aumente se sua árvore for ficar gigante)
    NO *mapa[1000]; 
    int total_nos = 0;
    
    // 1. Mapeia todos os nós para gerar os IDs
    mapearArvore(*raiz, mapa, &total_nos);

    // 2. Grava nó por nó no formato fixo
    for (int i = 0; i < total_nos; i++) {
        NO *no = mapa[i];

        // Grava: numnó, ORDEM
        fprintf(indice, "%d,%d,", i, M); 

        // Grava: chaves (matriculas)
        for (int j = 0; j < M - 1; j++) {
            if (j < no->qtd) {
                fprintf(indice, "%d,", no->chaves[j].matricula);
            } else {
                fprintf(indice, "-1,"); // Preenche o vazio com -1
            }
        }

        // Grava: posicoes na fonte
        for (int j = 0; j < M - 1; j++) {
            if (j < no->qtd) {
                fprintf(indice, "%d,", no->chaves[j].posicao);
            } else {
                fprintf(indice, "-1,"); // Preenche o vazio com -1
            }
        }

        // Grava: filhos (convertendo os ponteiros para seus IDs numéricos)
        for (int j = 0; j < M; j++) {
            if (!no->eh_folha && j <= no->qtd) {
                int id_filho = obterIdNo(mapa, total_nos, no->filhos[j]);
                fprintf(indice, "%d", id_filho);
            } else {
                fprintf(indice, "-1"); // Nó folha ou filho inexistente
            }
            
            // Adiciona a vírgula, exceto no último elemento da linha
            if (j < M - 1) fprintf(indice, ",");
        }
        
        // Pula para a próxima linha (próximo nó)
        fprintf(indice, "\n");
    }

    printf("Árvore salva com sucesso no índice! (%d nós gravados)\n", total_nos);
    return 1;
}

// ==========================================
// FUNÇÕES PARA DESTRUIR A ÁRVORE
// ==========================================

// Função auxiliar recursiva que apaga de baixo para cima (Pós-ordem)
void liberarNo(NO *atual) {
    if (atual == NULL) return;

    // Se não for folha, primeiro chamamos a liberação para todos os filhos
    if (!atual->eh_folha) {
        // Um nó com 'qtd' chaves tem 'qtd + 1' filhos
        for (int i = 0; i <= atual->qtd; i++) {
            liberarNo(atual->filhos[i]);
        }
    }

    // Depois que todos os filhos foram liberados (ou se já era uma folha),
    // podemos liberar a memória do próprio nó em segurança.
    free(atual);
}

// Função principal solicitada
int destruirArvore(arvB *raiz) {
    // Verifica se o ponteiro da árvore existe
    if (raiz == NULL) return 0; 

    // Se a árvore não estiver vazia, libera todos os nós
    if (*raiz != NULL) {
        liberarNo(*raiz); 
        *raiz = NULL; // Boa prática: anular o ponteiro após liberar a memória
    }

    // Lembra do `malloc(sizeof(arvB))` que você fez na função `criarArv`?
    // Precisamos liberar ele também!
    free(raiz); 

    printf("Árvore destruída com sucesso e memória liberada!\n");
    return 1;
}
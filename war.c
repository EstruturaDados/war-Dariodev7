/*
  war_estruturado.c
  Versão humana e modular do desafio "WAR Estruturado".
  Implementa os níveis:
   - Novato: vetor estático (5 territórios), cadastro e exibição.
   - Aventureiro: alocação dinâmica (calloc), fases de ataque com rand().
   - Mestre: modularização total, missões aleatórias, menu interativo.

 

  Autor: (Dario pereira)
  Data: (203/11/2025)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_NOME 30
#define MAX_COR 15
#define QTD_PADRAO 5
#define LINHA_BUFFER 200

/* ----- Estrutura de Território ----- */
typedef struct {
    char nome[MAX_NOME];
    char cor[MAX_COR];
    int tropas;
    int conquistado_por; /* -1 = neutro/unowned (não usado em níveis simples), ou id do jogador/owner */
} Territorio;

/* ----- Helpers de entrada e limpeza ----- */

/* remove newline final se presente */
void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') s[len - 1] = '\0';
}

/* lê texto com validação de não-vazio */
void ler_texto(const char *prompt, char *dest, size_t maxlen) {
    char buffer[LINHA_BUFFER];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            clearerr(stdin);
            printf("Erro de leitura. Tente novamente.\n");
            continue;
        }
        trim_newline(buffer);
        if (strlen(buffer) == 0) {
            printf("Entrada não pode ser vazia. Tente novamente.\n");
            continue;
        }
        strncpy(dest, buffer, maxlen - 1);
        dest[maxlen - 1] = '\0';
        return;
    }
}

/* lê inteiro não-negativo com validação */
int ler_inteiro(const char *prompt) {
    char buffer[LINHA_BUFFER];
    int valor;
    char extra;
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            clearerr(stdin);
            printf("Erro de leitura. Tente novamente.\n");
            continue;
        }
        if (sscanf(buffer, " %d %c", &valor, &extra) == 1) {
            if (valor < 0) {
                printf("Valor não pode ser negativo. Informe 0 ou mais.\n");
                continue;
            }
            return valor;
        }
        printf("Entrada inválida. Digite um número inteiro (ex: 5).\n");
    }
}

/* pausa curta (apenas aguardando ENTER) para melhor usabilidade */
void pause_enter(void) {
    printf("\nPressione ENTER para continuar...");
    fflush(stdout);
    char buf[LINHA_BUFFER];
    fgets(buf, sizeof(buf), stdin);
}

/* imprime uma linha separadora */
void linha(void) {
    printf("-----------------------------------------------\n");
}

/* ----- Funções comuns: exibir mapa ----- */
void exibir_territorios(const Territorio *lista, int quantidade) {
    linha();
    printf("Mapa atual (%d territórios):\n", quantidade);
    linha();
    for (int i = 0; i < quantidade; ++i) {
        printf("[%d] Nome: %-20s | Cor: %-8s | Tropas: %3d\n",
               i + 1, lista[i].nome, lista[i].cor, lista[i].tropas);
    }
    linha();
}

/* ----- Nível Novato: vetor estático ----- */
void nivel_novato(void) {
    printf("\n=== Nível Novato: Cadastro Inicial dos Territórios ===\n");
    Territorio territorios[QTD_PADRAO];

    for (int i = 0; i < QTD_PADRAO; ++i) {
        printf("\nTerritório %d de %d\n", i + 1, QTD_PADRAO);
        ler_texto("  Nome do território: ", territorios[i].nome, MAX_NOME);
        ler_texto("  Cor do exército: ", territorios[i].cor, MAX_COR);
        territorios[i].tropas = ler_inteiro("  Número de tropas: ");
    }

    printf("\nCadastro concluído. Exibindo o mapa:\n");
    exibir_territorios(territorios, QTD_PADRAO);

    pause_enter();
}

/* ----- Nível Aventureiro: dinâmica + batalhas ----- */

/* cria e retorna um vetor alocado com calloc para n territórios (inicializa a zero) */
Territorio *criar_territorios_dinamicos(int n) {
    Territorio *vet = calloc((size_t)n, sizeof(Territorio));
    if (!vet) {
        fprintf(stderr, "Erro: falha ao alocar memória.\n");
        exit(EXIT_FAILURE);
    }
    return vet;
}

/* simula um turno de batalha entre dois territórios
   regras:
    - atacante rola 1 dado (1..6)
    - defensor rola 1 dado (1..6)
    - se atacante >= defensor -> defensor perde 1 tropa
    - se defensor perde todas -> conquiste (cena simples: zera e copia nome cor? Mantemos nome e cor do defensor, mas setamos tropas a 0; no novo dono, o atacante pode movimentar tropas se quisermos)
   retorno: 1 se território foi conquistado, 0 caso contrário
*/
int simular_batalha(Territorio *atacante, Territorio *defensor) {
    int d_atk = (rand() % 6) + 1;
    int d_def = (rand() % 6) + 1;

    printf("\nDados: Atacante rolou %d | Defensor rolou %d\n", d_atk, d_def);

    if (d_atk >= d_def) {
        /* atacante vence empate favorece atacante */
        defensor->tropas -= 1;
        if (defensor->tropas < 0) defensor->tropas = 0;
        printf("Resultado: Atacante vence o confronto. Defensor perde 1 tropa.\n");
        if (defensor->tropas == 0) {
            /* conquista: defensor agora tem 0 tropas -> será conquistado
               Implementamos conquista simples: transferir 1 tropa do atacante para o conquistado (se possível).
            */
            printf("O território '%s' foi esvaziado e será conquistado!\n", defensor->nome);
            if (atacante->tropas > 1) {
                atacante->tropas -= 1;
                defensor->tropas = 1;
                printf("Uma tropa foi transferida do atacante para o território conquistado.\n");
            } else {
                /* atacante não tem tropas extras para ocupar; mantém 0 tropas no recém-conquistado */
                defensor->tropas = 0;
                printf("Atacante não tinha tropas sobrando para ocupar; território fica com 0 tropas.\n");
            }
            return 1;
        }
    } else {
        /* defensor vence */
        atacante->tropas -= 1;
        if (atacante->tropas < 0) atacante->tropas = 0;
        printf("Resultado: Defensor vence. Atacante perde 1 tropa.\n");
    }
    return 0;
}

void nivel_aventureiro(void) {
    printf("\n=== Nível Aventureiro: Batalhas Estratégicas ===\n");
    int n = QTD_PADRAO;
    Territorio *territorios = criar_territorios_dinamicos(n);

    /* cadastro inicial */
    printf("Cadastro dos %d territórios (dinâmico):\n", n);
    for (int i = 0; i < n; ++i) {
        printf("\nTerritório %d de %d\n", i + 1, n);
        ler_texto("  Nome do território: ", territorios[i].nome, MAX_NOME);
        ler_texto("  Cor do exército: ", territorios[i].cor, MAX_COR);
        territorios[i].tropas = ler_inteiro("  Número de tropas: ");
    }

    /* loop de batalhas */
    while (1) {
        exibir_territorios(territorios, n);
        printf("Opções:\n");
        printf("  0 - Sair do nível Aventureiro\n");
        printf("  1 - Atacar\n");
        int opc = ler_inteiro("Escolha uma opção: ");
        if (opc == 0) break;
        if (opc != 1) {
            printf("Opção inválida.\n");
            continue;
        }

        int idx_atk = ler_inteiro("Escolha o índice do território atacante (1 a 5): ") - 1;
        int idx_def = ler_inteiro("Escolha o índice do território defensor (1 a 5): ") - 1;

        if (idx_atk < 0 || idx_atk >= n || idx_def < 0 || idx_def >= n) {
            printf("Índices inválidos. Deve ser entre 1 e %d.\n", n);
            continue;
        }
        if (idx_atk == idx_def) {
            printf("Atacante e defensor não podem ser o mesmo território.\n");
            continue;
        }
        if (territorios[idx_atk].tropas <= 0) {
            printf("Território atacante '%s' não tem tropas suficientes para atacar.\n", territorios[idx_atk].nome);
            continue;
        }
        if (territorios[idx_def].tropas < 0) territorios[idx_def].tropas = 0;

        printf("\nIniciando batalha: %s (tropas %d) -> %s (tropas %d)\n",
               territorios[idx_atk].nome, territorios[idx_atk].tropas,
               territorios[idx_def].nome, territorios[idx_def].tropas);

        /* simula uma rodada; se conquistado, já ajustamos */
        int conquistado = simular_batalha(&territorios[idx_atk], &territorios[idx_def]);

        if (conquistado) {
            printf("Território conquistado! Atualizando mapa...\n");
        } else {
            printf("Batalha terminou. Atualizando mapa...\n");
        }

        pause_enter();
    }

    free(territorios);
    printf("Saindo do Nível Aventureiro.\n");
    pause_enter();
}

/* ----- Nível Mestre: modularização total e missões ----- */

/* enum de missões possíveis */
typedef enum {
    MISS_NENHUMA = 0,
    MISS_DESTRUIR_VERDE,
    MISS_CONQUISTAR_3
} MissaoTipo;

/* Gera e retorna uma missão aleatória (entre as disponíveis) */
MissaoTipo gerar_missao_aleatoria(void) {
    int r = rand() % 2; /* 0 ou 1 */
    if (r == 0) return MISS_DESTRUIR_VERDE;
    return MISS_CONQUISTAR_3;
}

/* inicializa automaticamente 5 territórios com exemplos (nomes padronizados) */
void inicializar_automatico(Territorio *lista, int n) {
    const char *nomes[QTD_PADRAO] = {"Aldea", "Montanha", "Planície", "Fortaleza", "Vale"};
    const char *cores[QTD_PADRAO] = {"Verde", "Vermelho", "Azul", "Amarelo", "Verde"};
    int tropas_iniciais[QTD_PADRAO] = {3, 4, 2, 5, 1};

    for (int i = 0; i < n; ++i) {
        strncpy(lista[i].nome, nomes[i % QTD_PADRAO], MAX_NOME - 1);
        lista[i].nome[MAX_NOME - 1] = '\0';
        strncpy(lista[i].cor, cores[i % QTD_PADRAO], MAX_COR - 1);
        lista[i].cor[MAX_COR - 1] = '\0';
        lista[i].tropas = tropas_iniciais[i % QTD_PADRAO];
        lista[i].conquistado_por = -1;
    }
}

/* Verifica missão: destruir exército verde = todos territórios de cor "Verde" com tropas == 0 */
int verificar_destruir_verde(const Territorio *lista, int n) {
    for (int i = 0; i < n; ++i) {
        /* compara cor case-insensitive com "Verde" */
        char tmp[MAX_COR];
        strncpy(tmp, lista[i].cor, MAX_COR - 1);
        tmp[MAX_COR - 1] = '\0';
        for (char *p = tmp; *p; ++p) *p = (char)tolower(*p);
        if (strstr(tmp, "verde") != NULL) {
            if (lista[i].tropas > 0) return 0; /* ainda existe tropa verde */
        }
    }
    return 1; /* todos verdes destruídos (ou não há verdes) */
}

/* conta quantos territórios têm tropas > 0 e qual é a cor vencedora (opcional) */
int contar_territorios_conquistados(const Territorio *lista, int n) {
    int cnt = 0;
    for (int i = 0; i < n; ++i) if (lista[i].tropas > 0) cnt++;
    return cnt;
}

/* conta quantos territórios foram 'conquistados' por um jogador (simples: tropas > 0 consideramos controlado) */
int contar_conquistados_por(const Territorio *lista, int n, const char *cor) {
    int cnt = 0;
    for (int i = 0; i < n; ++i) {
        if (strstr(lista[i].cor, cor) != NULL && lista[i].tropas > 0) cnt++;
    }
    return cnt;
}

/* função que verifica missão atual; retorna 1 se cumprida, 0 caso contrário */
int verificar_missao(const Territorio *lista, int n, MissaoTipo missao) {
    if (missao == MISS_DESTRUIR_VERDE) {
        return verificar_destruir_verde(lista, n);
    } else if (missao == MISS_CONQUISTAR_3) {
        /* missão: conquistar 3 territórios (tropas > 0) pelo jogador — aqui simplificamos: jogador é cor "Vermelho" */
        int cont = 0;
        for (int i = 0; i < n; ++i) if (lista[i].tropas > 0) cont++;
        return (cont >= 3); /* se qualquer cor tem 3 territórios com tropas > 0, consideramos missão cumprida para demonstração */
    }
    return 0;
}

/* Implementa menu e loop do Nível Mestre */
void nivel_mestre(void) {
    printf("\n=== Nível Mestre: Missões e Modularização ===\n");
    const int n = QTD_PADRAO;
    Territorio lista[n];

    /* inicialização automática para jogo mais rápido */
    inicializar_automatico(lista, n);

    /* gera missão aleatória */
    MissaoTipo missao_atual = gerar_missao_aleatoria();
    printf("Missão atribuída: ");
    if (missao_atual == MISS_DESTRUIR_VERDE) {
        printf("DESTRUIR o exército Verde (todas tropa de cor 'Verde' devem ser zeradas).\n");
    } else if (missao_atual == MISS_CONQUISTAR_3) {
        printf("CONQUISTAR 3 territórios (ter 3 territórios com tropas > 0).\n");
    } else {
        printf("Nenhuma missão.\n");
    }

    /* Loop principal */
    while (1) {
        exibir_territorios(lista, n);
        printf("Menu Mestre:\n");
        printf("  1 - Atacar\n");
        printf("  2 - Verificar Missão\n");
        printf("  0 - Sair do Nível Mestre\n");
        int opc = ler_inteiro("Escolha uma opção: ");
        if (opc == 0) break;
        if (opc == 1) {
            int idx_atk = ler_inteiro("Escolha o território atacante (1 a 5): ") - 1;
            int idx_def = ler_inteiro("Escolha o território defensor (1 a 5): ") - 1;
            if (idx_atk < 0 || idx_atk >= n || idx_def < 0 || idx_def >= n) {
                printf("Índices inválidos.\n");
                continue;
            }
            if (idx_atk == idx_def) {
                printf("Atacante e defensor não podem ser o mesmo.\n");
                continue;
            }
            if (lista[idx_atk].tropas <= 0) {
                printf("Território atacante não tem tropas.\n");
                continue;
            }

            int conquistado = simular_batalha(&lista[idx_atk], &lista[idx_def]);
            if (conquistado) {
                /* representação simples: copiamos a cor do atacante para o território conquistado */
                strncpy(lista[idx_def].cor, lista[idx_atk].cor, MAX_COR - 1);
                lista[idx_def].cor[MAX_COR - 1] = '\0';
                printf("Território %s agora tem cor '%s'.\n", lista[idx_def].nome, lista[idx_def].cor);
            }

            pause_enter();
        } else if (opc == 2) {
            printf("\nVerificando missão...\n");
            int ok = verificar_missao(lista, n, missao_atual);
            if (ok) {
                printf("Parabéns — missão cumprida!\n");
                /* quando vencer, podemos gerar nova missão ou encerrar */
                printf("Deseja gerar uma nova missão? (1 = sim / 0 = não): ");
                int r = ler_inteiro("");
                if (r == 1) {
                    missao_atual = gerar_missao_aleatoria();
                    printf("Nova missão gerada.\n");
                } else {
                    printf("Mantendo missão corrente.\n");
                }
            } else {
                printf("Missão ainda não cumprida. Continue jogando!\n");
            }
            pause_enter();
        } else {
            printf("Opção inválida.\n");
        }
    }

    printf("Saindo do Nível Mestre.\n");
    pause_enter();
}

/* ----- Main: menu de seleção dos níveis ----- */
int main(void) {
    /* seed para aleatoriedade */
    srand((unsigned int)time(NULL));

    printf("===========================================\n");
    printf("  Bem-vindo ao Desafio WAR Estruturado\n");
    printf("  Escolha o nível que deseja jogar:\n");
    printf("    1 - Novato (vetor estático)\n");
    printf("    2 - Aventureiro (alocação dinâmica + batalhas)\n");
    printf("    3 - Mestre (missões e modularização)\n");
    printf("    0 - Sair\n");
    printf("===========================================\n");

    while (1) {
        int escolha = ler_inteiro("Digite a opção (0-3): ");
        if (escolha == 0) {
            printf("Encerrando. Obrigado por jogar!\n");
            break;
        } else if (escolha == 1) {
            nivel_novato();
        } else if (escolha == 2) {
            nivel_aventureiro();
        } else if (escolha == 3) {
            nivel_mestre();
        } else {
            printf("Opção inválida. Tente novamente.\n");
        }
    }

    return 0;
}

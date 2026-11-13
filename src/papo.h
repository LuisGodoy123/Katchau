/*
 * papo.h
 * ------------------------------------------------------------------
 * Uma conversa do jogador com alguém da cidade (um morador ou o Mack):
 * o jogador aperta o botão de conversar (o quadrado, o X, ou a tecla E)
 * na frente dele, e os dois trocam as falas de um dos papos daquela
 * pessoa, uma de cada vez. Cada fala fica o tempo de ser lida e passa
 * sozinha; apertando de novo, ela passa logo. As falas do jogador começam
 * com ">" na tabela (o ">" não aparece).
 *
 * Não usa a raylib (regra 2 do roadmap).
 * ------------------------------------------------------------------
 */

#ifndef PAPO_H
#define PAPO_H

#define MAIS_FALAS_NO_PAPO  10

typedef struct {
    const char *falas[MAIS_FALAS_NO_PAPO];   /* NULL no fim; as do jogador começam com ">" */
} RoteiroDoPapo;

typedef struct {
    const RoteiroDoPapo *roteiro;   /* NULL: sem conversa */
    int atual;                      /* a fala de agora */
    float restante;                 /* segundos até ela passar sozinha */
    float duracao;                  /* segundos que ela dura inteira */
} Papo;

void comecarPapo(Papo *papo, const RoteiroDoPapo *roteiro);
void passarPapo(Papo *papo, float tempo);      /* o tempo passa: a fala acaba sozinha */
void avancarPapo(Papo *papo);                  /* o jogador apertou: a próxima fala */
int papoAtivo(const Papo *papo);
/* A fala de agora (sem o ">"); doJogador diz se é dele. NULL: sem conversa */
const char *falaDoPapo(const Papo *papo, int *doJogador);
float duracaoDaFalaDoPapo(const Papo *papo);

#endif

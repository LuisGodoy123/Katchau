/*
 * materiais_do_modelo.h
 * ------------------------------------------------------------------
 * O que é cada material dos modelos de pista que vieram de outros jogos
 * (interlagos_materiais.h, montreal_materiais.h): o chão que o carro sente em
 * cima dele, que a ferramenta ferramentas/medir_pista.c procura, e o jeito de
 * desenhar (os shaders de PIXELS_DOS_MODELOS, em mundo.c).
 * ------------------------------------------------------------------
 */

#ifndef MATERIAIS_DO_MODELO_H
#define MATERIAIS_DO_MODELO_H

typedef enum {
    CHAO_NENHUM,    /* prédios, árvores, postes: não é chão nem barreira */
    CHAO_PISTA,     /* o asfalto da pista, com as linhas pintadas */
    CHAO_ZEBRA,
    CHAO_ESCAPE,    /* área de escape asfaltada ou pintada */
    CHAO_GRAMA,
    CHAO_PIT,       /* o piso do pit lane e dos boxes */
    CHAO_MURO       /* muros, guard-rail, pneus e alambrados: param o carro */
} ChaoDoMaterial;

typedef enum {
    DESENHO_OPACO,      /* a imagem como ela é, sem o alfa */
    DESENHO_RECORTE,    /* furado onde o alfa é baixo (folhas, alambrados) */
    DESENHO_ASFALTO,    /* o asfalto da pista, na mesma escala em toda a volta */
    DESENHO_DECALQUE,   /* tinta sobre o asfalto: onde o alfa é baixo, aparece o asfalto */
    DESENHO_SOMBRA,     /* escurece o que está embaixo, na medida do alfa (a borracha do traçado) */
    DESENHO_TELA,       /* alambrados e grades: transparentes na medida do alfa (de longe, um véu) */
    DESENHO_GRAMA,      /* a grama do jogo */
    DESENHO_TERRENO,    /* o chão em volta (uma foto de satélite): perto do autódromo vira a grama do jogo */
    DESENHO_ESCONDIDO
} DesenhoDoMaterial;

typedef struct {
    ChaoDoMaterial chao;
    DesenhoDoMaterial desenho;
} MaterialDoModelo;

#endif

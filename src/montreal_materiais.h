/*
 * montreal_materiais.h
 * ------------------------------------------------------------------
 * O que é cada material de recursos/modelos/montreal.glb (o Circuito
 * Gilles-Villeneuve), na ordem em que a raylib os carrega (o 0 é o material
 * padrão que ela põe na frente; o material i do modelo é o i + 1 daqui).
 *
 * O modelo veio de outro jogo. O asfalto da pista já traz as linhas brancas da
 * borda pintadas na imagem; a borracha do traçado é uma camada por cima, que
 * escurece na medida do alfa; os alambrados são planos com a imagem de uma tela
 * (transparente na medida do alfa), e as folhas e os tufos de capim são recortes.
 * Os prédios, as arquibancadas e as placas dividem uma imagem só (environment):
 * os degraus das arquibancadas são a região dela entre MONTREAL_ASSENTO_U0..U1 e
 * V0..V1, nas faces de cima.
 * ------------------------------------------------------------------
 */

#ifndef MONTREAL_MATERIAIS_H
#define MONTREAL_MATERIAIS_H

#include "materiais_do_modelo.h"

#define TOTAL_MATERIAIS_MONTREAL 26

#define MATERIAL_DA_BORRACHA_DE_MONTREAL 1    /* groove_01 */
#define MATERIAL_DOS_PREDIOS_DE_MONTREAL 21   /* EnvironmentMat */
#define MATERIAL_DA_AGUA_DE_MONTREAL     3    /* Water */

/* Os degraus das arquibancadas, na imagem dos prédios (u e v já dentro de 0..1, com o v da raylib) */
#define MONTREAL_ASSENTO_U0  0.500f
#define MONTREAL_ASSENTO_U1  0.625f
#define MONTREAL_ASSENTO_V0  0.125f
#define MONTREAL_ASSENTO_V1  0.250f

/* O piso do pit lane e da frente das garagens também está na imagem dos prédios (as faces de cima, nesta faixa) */
#define MONTREAL_PISO_V0     0.014f   /* as passarelas sobre a pista ficam logo fora dela (0,011 e 0,048) */
#define MONTREAL_PISO_V1     0.046f

static const MaterialDoModelo MATERIAIS_MONTREAL[TOTAL_MATERIAIS_MONTREAL] = {
    /*  0 (padrão da raylib)  */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  1 groove_01           */ { CHAO_NENHUM, DESENHO_SOMBRA },    /* a borracha do traçado */
    /*  2 Glass_Black         */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  3 Water               */ { CHAO_GRAMA, DESENHO_OPACO },      /* a bacia de remo e o rio */
    /*  4 GitterMat           */ { CHAO_MURO, DESENHO_TELA },        /* os alambrados */
    /*  5 KerbMat             */ { CHAO_ZEBRA, DESENHO_OPACO },
    /*  6 GuardrailMat        */ { CHAO_MURO, DESENHO_OPACO },
    /*  7 GravelMat           */ { CHAO_GRAMA, DESENHO_OPACO },      /* a caixa de brita: fora da pista, como a grama */
    /*  8 RunoffgreenMat      */ { CHAO_ESCAPE, DESENHO_OPACO },
    /*  9 RunoffredMat        */ { CHAO_ESCAPE, DESENHO_OPACO },
    /* 10 WallMat             */ { CHAO_MURO, DESENHO_OPACO },
    /* 11 EmptyMat            */ { CHAO_NENHUM, DESENHO_OPACO },     /* dois prédios grandes, de cor lisa */
    /* 12 TiresMat            */ { CHAO_MURO, DESENHO_OPACO },
    /* 13 GrassMat            */ { CHAO_GRAMA, DESENHO_OPACO },
    /* 14 GrassbetonredMat    */ { CHAO_ESCAPE, DESENHO_OPACO },     /* bloquete de concreto com grama */
    /* 15 GrassbetonMat       */ { CHAO_ESCAPE, DESENHO_OPACO },
    /* 16 Asphalt02Mat        */ { CHAO_ESCAPE, DESENHO_OPACO },     /* sem as linhas: escape, paddock e ruas de serviço */
    /* 17 GradientMat         */ { CHAO_GRAMA, DESENHO_OPACO },      /* a beira: do asfalto para a grama */
    /* 18 Asphalt03Mat        */ { CHAO_PISTA, DESENHO_OPACO },      /* o asfalto novo de algumas curvas e das rotas de fuga */
    /* 19 AsphaltMat          */ { CHAO_PISTA, DESENHO_OPACO },      /* a pista e o pit lane */
    /* 20 ZinkMat             */ { CHAO_NENHUM, DESENHO_OPACO },     /* estruturas de metal */
    /* 21 EnvironmentMat      */ { CHAO_NENHUM, DESENHO_OPACO },     /* prédios, arquibancadas, placas */
    /* 22 trees_02            */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 23 trees_01            */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 24 MarksMat            */ { CHAO_PISTA, DESENHO_TELA },       /* a linha de chegada e o grid, pintados */
    /* 25 grass_straws_01     */ { CHAO_NENHUM, DESENHO_RECORTE },   /* tufos de capim */
};

#endif

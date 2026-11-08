/*
 * interlagos_materiais.h
 * ------------------------------------------------------------------
 * O que é cada material de recursos/modelos/interlagos.glb, na ordem em que a
 * raylib os carrega (o 0 é o material padrão que ela põe na frente).
 *
 * O modelo veio de outro jogo, onde o canal alfa das imagens tem vários usos:
 * na maioria é o brilho (aqui é ignorado), nas folhas e alambrados é o recorte,
 * nas linhas pintadas é a tinta sobre o asfalto e na borracha do traçado e nas
 * sombras pintadas é o quanto escurece. "chao" diz o que o carro sente em cima
 * dele, e é o que a ferramenta ferramentas/medir_interlagos.c procura.
 * ------------------------------------------------------------------
 */

#ifndef INTERLAGOS_MATERIAIS_H
#define INTERLAGOS_MATERIAIS_H

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
} MaterialDeInterlagos;

#define TOTAL_MATERIAIS_INTERLAGOS 178

/* O muro de concreto (cmwalla) também tem as paredes invisíveis do jogo original: triângulos
 * de dezenas de metros de altura perto da pista. Eles não são desenhados nem param o carro */
#define MATERIAL_COM_PAREDES_INVISIVEIS 105
#define ALTURA_DA_PAREDE_INVISIVEL      6.0f   /* metros: um triângulo mais alto que isso é dela */

/* As imagens gastas do modelo (concreto manchado e enferrujado, guard-rail enferrujado) dão ao
 * autódromo um ar de abandonado: o jogo troca por um concreto limpo e pelo guard-rail novo */
#define MATERIAL_DO_CONCRETO_MANCHADO   105    /* cmwalla */
#define MATERIAL_DO_CONCRETO_ENFERRUJADO 22    /* cmwallc */
#define MATERIAL_DO_CONCRETO_COM_MUSGO  91     /* wallc: o muro do canal da Descida do Lago */
#define MATERIAL_DO_GUARD_RAIL_ENFERRUJADO 103 /* armcob */
#define MATERIAL_DO_GUARD_RAIL_NOVO     126    /* armcoa */

/* O lago da Descida do Lago vem com uma água azul-clara, quase da cor do céu: de longe parece
 * um buraco no mapa. O jogo escurece para um verde-azulado de água */
#define MATERIAL_DO_LAGO                7      /* lakes */

static const MaterialDeInterlagos MATERIAIS_INTERLAGOS[TOTAL_MATERIAIS_INTERLAGOS] = {
    /*   0 (padrão da raylib)              */ { CHAO_NENHUM, DESENHO_OPACO },
    /*   1 redbuild_windows               */ { CHAO_NENHUM, DESENHO_OPACO },
    /*   2 portaopreto                    */ { CHAO_NENHUM, DESENHO_OPACO },
    /*   3 Material_#5166                 */ { CHAO_NENHUM, DESENHO_OPACO },
    /*   4 racecontrol_white              */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*   5 box_vidro                      */ { CHAO_NENHUM, DESENHO_OPACO },
    /*   6 vidro_predio                   */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*   7 lakes                          */ { CHAO_GRAMA, DESENHO_OPACO },
    /*   8 Material_#5384                 */ { CHAO_NENHUM, DESENHO_OPACO },
    /*   9 portaobrilho                   */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  10 TAPETE                         */ { CHAO_ESCAPE, DESENHO_OPACO },
    /*  11 roadpits                       */ { CHAO_PIT, DESENHO_OPACO },
    /*  12 structures                     */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  13 gstands_3_fence_chroma         */ { CHAO_NENHUM, DESENHO_TELA },
    /*  14 gstands_3_fence_1              */ { CHAO_NENHUM, DESENHO_TELA },
    /*  15 box_alpha                      */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  16 pit_garage_alpha               */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  17 brz_pit_garages_b              */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  18 material_#14                   */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  19 groove                         */ { CHAO_NENHUM, DESENHO_SOMBRA },
    /*  20 roadseam                       */ { CHAO_PISTA, DESENHO_DECALQUE },
    /*  21 fencea                         */ { CHAO_MURO, DESENHO_TELA },
    /*  22 cmwallc                        */ { CHAO_MURO, DESENHO_OPACO },
    /*  23 startlight_gantry_wires        */ { CHAO_NENHUM, DESENHO_TELA },
    /*  24 fenceb                         */ { CHAO_MURO, DESENHO_TELA },
    /*  25 gstands_supports_chroma        */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  26 gstands_supports               */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  27 gstands_fence                  */ { CHAO_NENHUM, DESENHO_TELA },
    /*  28 gstands_fence_chroma           */ { CHAO_NENHUM, DESENHO_TELA },
    /*  29 gstands_seats                  */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  30 fencec                         */ { CHAO_MURO, DESENHO_TELA },
    /*  31 TyreRed                        */ { CHAO_NENHUM, DESENHO_ESCONDIDO },   /* sobra do modelo, solta na pista */
    /*  32 t56_build_white_cememt         */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  33 t56_build_roof                 */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  34 t56_build_walls                */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  35 radiotowerfence2               */ { CHAO_NENHUM, DESENHO_TELA },
    /*  36 radiotower_fence               */ { CHAO_MURO, DESENHO_TELA },
    /*  37 radiotower_metal               */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  38 houses                         */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  39 flats_new_1                    */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  40 flats_3                        */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  41 flats_2                        */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  42 floodlight_lamp                */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  43 paddock_light_metal            */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  44 outerbuilds                    */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  45 Material_#5258                 */ { CHAO_GRAMA, DESENHO_TERRENO },
    /*  46 redbuild_supports              */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  47 redbuild_bricks                */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  48 bgbuildingsa                   */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  49 flats_4                        */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  50 flats_5                        */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  51 flats_1                        */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  52 gstands1                       */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  53 watertower                     */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  54 concrete_structures_2_building */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  55 concrete_stuctures_2_cement    */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  56 racecontrol_bars               */ { CHAO_NENHUM, DESENHO_TELA },
    /*  57 MultiMat_134_Slot_#1           */ { CHAO_NENHUM, DESENHO_ESCONDIDO },
    /*  58 pit_roof                       */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  59 gstand_3_cement_2              */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  60 pitbuild_tower                 */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  61 gseseatsa                      */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  62 tvscreen                       */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  63 radiotower                     */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  64 solid                          */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  65 pit_teto                       */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  66 predioparede                   */ { CHAO_NENHUM, DESENHO_RECORTE },
    /*  67 cement_no_ao                   */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  68 misc_unique                    */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  69 whitepanel                     */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  70 conc_gfloor                    */ { CHAO_PIT, DESENHO_OPACO },
    /*  71 garage_walls_sides             */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  72 garage_interior_backwall       */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  73 cement_trim_horizontal         */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  74 cement_trim_post               */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  75 material_#1!0                  */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  76 standard_1                     */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  77 advert_side_a                  */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  78 mtr_prefab_podium_a            */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  79 brz_radio_building_a_tx        */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  80 material_#15                   */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  81 grasscreteverge                */ { CHAO_GRAMA, DESENHO_OPACO },
    /*  82 roadverge                      */ { CHAO_GRAMA, DESENHO_OPACO },
    /*  83 rmblc                          */ { CHAO_ZEBRA, DESENHO_OPACO },
    /*  84 rmbga                          */ { CHAO_ZEBRA, DESENHO_OPACO },
    /*  85 rmbla                          */ { CHAO_ZEBRA, DESENHO_OPACO },
    /*  86 concdrainc                     */ { CHAO_ESCAPE, DESENHO_OPACO },
    /*  87 concdrain1                     */ { CHAO_ESCAPE, DESENHO_OPACO },
    /*  88 concdrainb                     */ { CHAO_ESCAPE, DESENHO_OPACO },
    /*  89 rdilr                          */ { CHAO_ESCAPE, DESENHO_OPACO },
    /*  90 walla                          */ { CHAO_MURO, DESENHO_OPACO },
    /*  91 wallc                          */ { CHAO_MURO, DESENHO_OPACO },
    /*  92 distmarker                     */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  93 rdils                          */ { CHAO_ESCAPE, DESENHO_OPACO },
    /*  94 material_#447                  */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  95 silverbars                     */ { CHAO_NENHUM, DESENHO_TELA },
    /*  96 pitwlfloor                     */ { CHAO_NENHUM, DESENHO_OPACO },
    /*  97 marshut_fence                  */ { CHAO_MURO, DESENHO_TELA },
    /*  98 marshut                        */ { CHAO_MURO, DESENHO_OPACO },
    /*  99 pr_bluepanel                   */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 100 pr_brickwall                   */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 101 prhousefront                   */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 102 rdilolda                       */ { CHAO_ESCAPE, DESENHO_OPACO },
    /* 103 armcob                         */ { CHAO_MURO, DESENHO_OPACO },
    /* 104 xsec                           */ { CHAO_PISTA, DESENHO_DECALQUE },
    /* 105 cmwalla                        */ { CHAO_MURO, DESENHO_OPACO },
    /* 106 wallda                         */ { CHAO_MURO, DESENHO_OPACO },
    /* 107 pr_support                     */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 108 genshadow                      */ { CHAO_NENHUM, DESENHO_SOMBRA },
    /* 109 gstands_petro                  */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 110 advertisements                 */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 111 startlight_poles               */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 112 advertisements2                */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 113 twalla                         */ { CHAO_MURO, DESENHO_OPACO },
    /* 114 gstands_boxes                  */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 115 gstands_boxes_ao               */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 116 gstands_roof                   */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 117 gstands_cement                 */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 118 gsdseatsa                      */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 119 wallb                          */ { CHAO_MURO, DESENHO_OPACO },
    /* 120 startlight_gantry              */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 121 gstands_t6_metal               */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 122 gstands_t6_steps               */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 123 gstands_t6_cement              */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 124 cmwallb                        */ { CHAO_MURO, DESENHO_OPACO },
    /* 125 lightmats                      */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 126 armcoa                         */ { CHAO_MURO, DESENHO_OPACO },
    /* 127 tents                          */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 128 gstands_3_fence                */ { CHAO_NENHUM, DESENHO_TELA },
    /* 129 concrete_structures_building   */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 130 concrete_stuctures_cement      */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 131 concrete_structure_building    */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 132 pwallmetal                     */ { CHAO_MURO, DESENHO_OPACO },
    /* 133 armcopost                      */ { CHAO_MURO, DESENHO_OPACO },
    /* 134 support_beams                  */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 135 stall_numbers_1                */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 136 stall_numbers_2                */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 137 footbridge_steps_chroma        */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 138 footbrige_steps                */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 139 footbridge_pillars             */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 140 footbridge_cement_panels       */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 141 fencepole                      */ { CHAO_MURO, DESENHO_TELA },
    /* 142 gstands_stairs                 */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 143 gstands_steps_2                */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 144 material_#436                  */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 145 material_#487                  */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 146 gstands_wcement                */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 147 transparent                    */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 148 illum                          */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 149 main                           */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 150 Material_#5387                 */ { CHAO_GRAMA, DESENHO_GRAMA },
    /* 151 Material_#5386                 */ { CHAO_GRAMA, DESENHO_GRAMA },
    /* 152 Material_#5165                 */ { CHAO_GRAMA, DESENHO_GRAMA },
    /* 153 rdlna                          */ { CHAO_PISTA, DESENHO_DECALQUE },
    /* 154 concw                          */ { CHAO_ESCAPE, DESENHO_OPACO },
    /* 155 creteg                         */ { CHAO_ZEBRA, DESENHO_OPACO },
    /* 156 roadd                          */ { CHAO_PISTA, DESENHO_DECALQUE },
    /* 157 rdily                          */ { CHAO_ESCAPE, DESENHO_OPACO },
    /* 158 rdilws                         */ { CHAO_ESCAPE, DESENHO_DECALQUE },
    /* 159 rdmarblesa                     */ { CHAO_PISTA, DESENHO_ASFALTO },
    /* 160 rdmarblessf                    */ { CHAO_PISTA, DESENHO_DECALQUE },
    /* 161 roadsf                         */ { CHAO_PISTA, DESENHO_DECALQUE },
    /* 162 roadanl                        */ { CHAO_PISTA, DESENHO_ASFALTO },
    /* 163 rdilblue                       */ { CHAO_ESCAPE, DESENHO_OPACO },
    /* 164 rdilgreen                      */ { CHAO_ESCAPE, DESENHO_OPACO },
    /* 165 inerfield                      */ { CHAO_GRAMA, DESENHO_GRAMA },
    /* 166 roada                          */ { CHAO_PISTA, DESENHO_DECALQUE },
    /* 167 rdilblack                      */ { CHAO_ESCAPE, DESENHO_OPACO },
    /* 168 grassv2a                       */ { CHAO_GRAMA, DESENHO_GRAMA },
    /* 169 Material_#5163                 */ { CHAO_GRAMA, DESENHO_TERRENO },
    /* 170 Material_#5385                 */ { CHAO_NENHUM, DESENHO_OPACO },
    /* 171 pit_toldo                      */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 172 tree_2                         */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 173 trees                          */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 174 pit-glass                      */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 175 gstands_seats_chroma           */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 176 bushes                         */ { CHAO_NENHUM, DESENHO_RECORTE },
    /* 177 roada                          */ { CHAO_NENHUM, DESENHO_ESCONDIDO },   /* sobra do modelo, solta na pista */
};

#endif

/*
 * moradores_mapa.h
 * ------------------------------------------------------------------
 * O mapa de Radiator Springs para os moradores (incluído só por
 * moradores.c): os lugares de cada um, o grafo das ruas, os percursos,
 * os programas e as falas de cada morador.
 *
 * Tudo nos eixos do jogo, em metros (rumo 0 olhando para -z, crescendo
 * para a direita). Os nós foram medidos no mundo aberto montado por
 * ferramentas/montar_mundo_aberto.py (renders de cima e da rua, e raios
 * conferindo cada vaga e cada trecho):
 *   L* / O*  a rua principal, faixa de quem vai para leste (z 138,8) e
 *            para oeste (z 131,5): mão pela direita;
 *   RL / RO  a rodovia, depois da entrada leste;
 *   ret_*    os retornos (oeste, leste e na rodovia);
 *   TS / TN  o caminho pela areia até a pista de treino (indo / voltando) e
 *            OV_* o oval;
 *   entrada_* e vaga_*  de cada lugar (a vaga é a do dono);
 *   porta_* / dentro_*  a Casa Della Tires: o Luigi e o Guido estacionam
 *            dentro da loja, lado a lado, olhando a porta de garagem
 *            (recursos/modelos/loja_do_luigi.json); o caminho entra pela
 *            porta de garagem e vai até a vaga de cada um. No Cozy Cone, o McQueen fica
 *            dentro do cone 1 e a Sally, dentro da recepção, atrás do balcão
 *            (cozy_cone_quartos.json e cozy_cone_recepcao.json), os dois
 *            olhando a porta: entram de ré. Para a ré começar com o carro
 *            de lado (como numa vaga da rua) e não de costas, o caminho
 *            chega de lado: o do cone sobe pela praça (praca_*) até a frente
 *            da porta, e o da recepção vira dentro dela (volta_* e manobra_*).
 *            Quem vem buscar o McQueen para na praça (visitas_*).
 * As ruas têm mão: cada nó lista para onde se vai a partir dele.
 * ------------------------------------------------------------------
 */

/* Os lugares: a vaga do dono (um nó do grafo) e para onde o carro aponta, estacionado. Quem
 * estaciona de ré (olhando para a rua) entra de ré e sai de frente; quem estaciona de frente
 * (a Flo, olhando o balcão) entra de frente e sai de ré */
enum {
    LUGAR_LUIGI, LUGAR_GUIDO, LUGAR_RAMONE, LUGAR_FLO, LUGAR_CURIOS, LUGAR_DOC, LUGAR_MCQUEEN,
    LUGAR_SALLY, LUGAR_SARGENTO, LUGAR_FILLMORE, LUGAR_MATE, LUGAR_XERIFE, LUGAR_RUIVO
};

static const Lugar LUGARES[] = {
    [LUGAR_LUIGI]    = { "Casa Della Tires (Luigi)", "vaga_luigi",    0.525f, NULL, 0.0f, 0 },
    [LUGAR_GUIDO]    = { "Casa Della Tires (Guido)", "vaga_guido",    0.525f, NULL, 0.0f, 0 },
    [LUGAR_RAMONE]   = { "Ramone's House of Body Art", "vaga_ramone", 3.142f, NULL, 0.0f, 0 },
    [LUGAR_FLO]      = { "Flo's V8 Café",            "vaga_flo",      0.000f, NULL, 0.0f, 0 },
    [LUGAR_CURIOS]   = { "Radiator Springs Curios",  "vaga_lizzie",   0.000f, NULL, 0.0f, 0 },
    [LUGAR_DOC]      = { "Clínica do Doc Hudson",    "vaga_doc",      3.142f, NULL, 0.0f, 0 },
    [LUGAR_MCQUEEN]  = { "Cozy Cone (o cone do McQueen)", "vaga_mcqueen", 1.5623f,
                         "visitas_do_cone_do_mcqueen", -1.5623f, 0 },
    [LUGAR_SALLY]    = { "Cozy Cone (a recepção da Sally)", "vaga_sally", 0.000f,
                         NULL, 0.0f, 0x2 },   /* a visita só cabe a oeste dela: a leste, o balcão e um vaso */
    [LUGAR_SARGENTO] = { "Sarge's Surplus Hut",      "vaga_sarge",    3.142f, NULL, 0.0f, 0 },
    [LUGAR_FILLMORE] = { "Fillmore's Taste-In",      "vaga_fillmore", 3.142f, NULL, 0.0f, 0 },
    [LUGAR_MATE]     = { "Tow Mater Towing & Salvage", "vaga_mater",  6.137f, NULL, 0.0f, 0 },
    [LUGAR_XERIFE]   = { "atrás do outdoor da entrada leste", "vaga_sheriff", 3.142f, NULL, 0.0f, 0 },
    [LUGAR_RUIVO]    = { "o quartel, junto da estátua do Stanley", "vaga_red", 1.571f, NULL, 0.0f, 0 },
};

/* O grafo das ruas */
static const NoDaRua RUAS[] = {
    { "L306", 306.0f, 138.8f, { "L330" } },
    { "O306", 306.0f, 131.5f, { "ret_oeste_1", "entrada_quartel" } },
    { "L330", 330.0f, 138.8f, { "L360" } },
    { "O330", 330.0f, 131.5f, { "O306" } },
    { "L360", 360.0f, 138.8f, { "L390" } },
    { "O360", 360.0f, 131.5f, { "O330" } },
    { "L390", 390.0f, 138.8f, { "L420" } },
    { "O390", 390.0f, 131.5f, { "O360" } },
    { "L420", 420.0f, 138.8f, { "L450" } },
    { "O420", 420.0f, 131.5f, { "O390" } },
    { "L450", 450.0f, 138.8f, { "L480", "entrada_casa_della_tires", "entrada_ramone" } },
    { "O450", 450.0f, 131.5f, { "O420" } },
    { "L480", 480.0f, 138.8f, { "L505" } },
    { "O480", 480.0f, 131.5f, { "O450" } },
    { "L505", 505.0f, 138.8f, { "L530" } },
    { "O505", 505.0f, 131.5f, { "O480", "entrada_casa_della_tires", "entrada_ramone" } },
    { "L530", 530.0f, 138.8f, { "L555", "entrada_flo", "entrada_curios" } },
    { "O530", 530.0f, 131.5f, { "O505" } },
    { "L555", 555.0f, 138.8f, { "L580" } },
    { "O555", 555.0f, 131.5f, { "O530" } },
    { "L580", 580.0f, 138.8f, { "L605" } },
    { "O580", 580.0f, 131.5f, { "O555", "entrada_flo", "entrada_curios" } },
    { "L605", 605.0f, 138.8f, { "L630", "entrada_doc" } },
    { "O605", 605.0f, 131.5f, { "O580" } },
    { "L630", 630.0f, 138.8f, { "L655", "entrada_cozy", "praca_do_cone_do_mcqueen" } },
    { "O630", 630.0f, 131.5f, { "O605" } },
    { "L655", 655.0f, 138.8f, { "L680" } },
    { "O655", 655.0f, 131.5f, { "O630", "entrada_doc" } },
    { "L680", 680.0f, 138.8f, { "L700", "entrada_sarge" } },
    { "O680", 680.0f, 131.5f, { "O655", "entrada_cozy" } },
    { "L700", 700.0f, 138.8f, { "L715" } },
    { "O700", 700.0f, 131.5f, { "O680" } },
    { "L715", 715.0f, 134.4f, { "L735" } },
    { "O715", 715.0f, 130.6f, { "O700", "entrada_sarge" } },
    { "L735", 735.0f, 134.4f, { "L755", "entrada_fillmore" } },
    { "O735", 735.0f, 130.6f, { "O715" } },
    { "L755", 755.0f, 134.4f, { "L770" } },
    { "O755", 755.0f, 130.6f, { "O735" } },
    { "L770", 770.0f, 138.8f, { "L795" } },
    { "O770", 770.0f, 131.5f, { "O755", "entrada_fillmore" } },
    { "L795", 795.0f, 138.8f, { "L820" } },
    { "O795", 795.0f, 131.5f, { "O770" } },
    { "L820", 820.0f, 138.8f, { "L855" } },
    { "O820", 820.0f, 131.5f, { "O795" } },
    { "L855", 855.0f, 138.8f, { "L880" } },
    { "O855", 855.0f, 131.5f, { "O820" } },
    { "L880", 880.0f, 138.8f, { "L910" } },
    { "O880", 880.0f, 131.5f, { "O855" } },
    { "L910", 910.0f, 138.8f, { "L940", "entrada_mater" } },
    { "O910", 910.0f, 131.5f, { "O880" } },
    { "L940", 940.0f, 138.8f, { "L970" } },
    { "O940", 940.0f, 131.5f, { "O910" } },
    { "L970", 970.0f, 138.8f, { "L1002", "TS150" } },
    { "O970", 970.0f, 131.5f, { "O940", "entrada_mater" } },
    { "L1002", 1002.0f, 138.8f, { "L1030" } },
    { "O1002", 1002.0f, 131.5f, { "O970" } },
    { "L1030", 1030.0f, 138.8f, { "L1060" } },
    { "O1030", 1030.0f, 131.5f, { "O1002", "TS150" } },
    { "L1060", 1060.0f, 138.8f, { "L1090" } },
    { "O1060", 1060.0f, 131.5f, { "O1030" } },
    { "L1090", 1090.0f, 138.8f, { "L1120" } },
    { "O1090", 1090.0f, 131.5f, { "O1060" } },
    { "L1120", 1120.0f, 138.8f, { "L1150" } },
    { "O1120", 1120.0f, 131.5f, { "O1090" } },
    { "L1150", 1150.0f, 138.8f, { "L1170", "entrada_sheriff" } },
    { "O1150", 1150.0f, 131.5f, { "O1120" } },
    { "L1170", 1170.0f, 138.8f, { "L1195" } },
    { "O1170", 1170.0f, 131.5f, { "O1150" } },
    { "L1195", 1195.0f, 138.8f, { "RL1230", "ret_leste_1" } },
    { "O1195", 1195.0f, 131.5f, { "O1170", "entrada_sheriff" } },
    { "ret_oeste_1", 297.0f, 130.0f, { "ret_oeste_2" } },
    { "ret_oeste_2", 292.0f, 136.5f, { "ret_oeste_3" } },
    { "ret_oeste_3", 297.0f, 141.5f, { "L306" } },
    { "RL1230", 1230.0f, 138.1f, { "RL1265" } },
    { "RO1230", 1230.0f, 132.3f, { "O1195" } },
    { "RL1265", 1265.0f, 138.1f, { "RL1300" } },
    { "RO1265", 1265.0f, 132.3f, { "RO1230" } },
    { "RL1300", 1300.0f, 138.1f, { "RL1335" } },
    { "RO1300", 1300.0f, 132.3f, { "RO1265" } },
    { "RL1335", 1335.0f, 138.0f, { "RL1370" } },
    { "RO1335", 1335.0f, 132.2f, { "RO1300" } },
    { "RL1370", 1370.0f, 137.9f, { "RL1405" } },
    { "RO1370", 1370.0f, 132.1f, { "RO1335" } },
    { "RL1405", 1405.0f, 137.8f, { "RL1440" } },
    { "RO1405", 1405.0f, 132.0f, { "RO1370" } },
    { "RL1440", 1440.0f, 137.7f, { "RL1475" } },
    { "RO1440", 1440.0f, 131.9f, { "RO1405" } },
    { "RL1475", 1475.0f, 137.6f, { "RL1510" } },
    { "RO1475", 1475.0f, 131.8f, { "RO1440" } },
    { "RL1510", 1510.0f, 137.5f, { "RL1545" } },
    { "RO1510", 1510.0f, 131.7f, { "RO1475" } },
    { "RL1545", 1545.0f, 137.4f, { "RL1580" } },
    { "RO1545", 1545.0f, 131.6f, { "RO1510" } },
    { "RL1580", 1580.0f, 137.3f, { "RL1615" } },
    { "RO1580", 1580.0f, 131.5f, { "RO1545" } },
    { "RL1615", 1615.0f, 137.2f, { "RL1650" } },
    { "RO1615", 1615.0f, 131.4f, { "RO1580" } },
    { "RL1650", 1650.0f, 137.1f, { "ret_rodovia_1" } },
    { "RO1650", 1650.0f, 131.3f, { "RO1615" } },
    { "ret_rodovia_1", 1664.0f, 137.8f, { "ret_rodovia_2" } },
    { "ret_rodovia_2", 1670.5f, 134.2f, { "ret_rodovia_3" } },
    { "ret_rodovia_3", 1664.0f, 130.6f, { "RO1650" } },
    { "ret_leste_1", 1205.0f, 139.8f, { "ret_leste_2" } },
    { "ret_leste_2", 1210.5f, 135.2f, { "ret_leste_3" } },
    { "ret_leste_3", 1205.0f, 130.6f, { "O1195" } },
    { "TS150", 998.5f, 150.0f, { "TS185" } },
    { "TS185", 998.5f, 185.0f, { "TS220" } },
    { "TS220", 998.5f, 220.0f, { "TS255" } },
    { "TS255", 998.5f, 255.0f, { "TS290" } },
    { "TS290", 998.5f, 290.0f, { "TS325" } },
    { "TS325", 998.5f, 325.0f, { "OV_T0" } },
    { "TN325", 1005.5f, 325.0f, { "TN290" } },
    { "TN290", 1005.5f, 290.0f, { "TN255" } },
    { "TN255", 1005.5f, 255.0f, { "TN220" } },
    { "TN220", 1005.5f, 220.0f, { "TN185" } },
    { "TN185", 1005.5f, 185.0f, { "TN150" } },
    { "TN150", 1005.5f, 150.0f, { "L1030", "O970" } },
    { "OV_T0", 1000.0f, 345.0f, { "OV_T1" } },
    { "OV_T1", 972.0f, 350.5f, { "OV_T2" } },
    { "OV_T2", 948.0f, 368.0f, { "OV_T3" } },
    { "OV_T3", 935.0f, 395.0f, { "OV_W1" } },
    { "OV_W1", 931.0f, 430.0f, { "OV_W2" } },
    { "OV_W2", 930.0f, 470.0f, { "OV_W3" } },
    { "OV_W3", 929.0f, 510.0f, { "OV_W4" } },
    { "OV_W4", 928.0f, 550.0f, { "OV_W5" } },
    { "OV_W5", 927.0f, 590.0f, { "OV_W6" } },
    { "OV_W6", 925.0f, 630.0f, { "OV_W7" } },
    { "OV_W7", 923.0f, 670.0f, { "OV_B0" } },
    { "OV_B0", 927.0f, 705.0f, { "OV_B1" } },
    { "OV_B1", 936.0f, 735.0f, { "OV_B2" } },
    { "OV_B2", 955.0f, 757.0f, { "OV_B3" } },
    { "OV_B3", 980.0f, 768.5f, { "OV_B4" } },
    { "OV_B4", 1005.0f, 768.5f, { "OV_B5" } },
    { "OV_B5", 1030.0f, 758.0f, { "OV_B6" } },
    { "OV_B6", 1050.0f, 738.0f, { "OV_B7" } },
    { "OV_B7", 1062.0f, 710.0f, { "OV_E1" } },
    { "OV_E1", 1066.0f, 680.0f, { "OV_E2" } },
    { "OV_E2", 1067.0f, 640.0f, { "OV_E3" } },
    { "OV_E3", 1068.0f, 600.0f, { "OV_E4" } },
    { "OV_E4", 1069.0f, 560.0f, { "OV_E5" } },
    { "OV_E5", 1069.0f, 520.0f, { "OV_E6" } },
    { "OV_E6", 1069.0f, 480.0f, { "OV_E7" } },
    { "OV_E7", 1069.0f, 440.0f, { "OV_E8" } },
    { "OV_E8", 1065.0f, 400.0f, { "OV_T7" } },
    { "OV_T7", 1052.0f, 368.0f, { "OV_T8" } },
    { "OV_T8", 1028.0f, 350.5f, { "OV_T0", "TN325" } },
    { "entrada_casa_della_tires", 480.5f, 145.8f, { "L505", "O450", "porta_casa_della_tires" } },
    { "porta_casa_della_tires", 479.2f, 158.7f, { "entrada_casa_della_tires", "dentro_casa_della_tires" } },
    { "dentro_casa_della_tires", 470.2f, 169.5f, { "porta_casa_della_tires", "vaga_luigi", "vaga_guido" } },
    { "vaga_luigi", 467.9f, 173.4f, { "dentro_casa_della_tires" } },
    { "vaga_guido", 464.8f, 171.6f, { "dentro_casa_della_tires" } },
    { "entrada_ramone", 483.0f, 125.0f, { "L505", "O450", "vaga_ramone" } },
    { "vaga_ramone", 483.0f, 118.5f, { "entrada_ramone" } },
    { "entrada_flo", 547.0f, 124.5f, { "L580", "O530", "flo_bombas" } },
    { "flo_bombas", 547.0f, 104.0f, { "entrada_flo", "vaga_flo" } },
    { "vaga_flo", 547.0f, 82.5f, { "flo_bombas" } },
    { "entrada_curios", 561.0f, 144.8f, { "L580", "O530", "vaga_lizzie" } },
    { "vaga_lizzie", 561.0f, 150.0f, { "entrada_curios" } },
    { "entrada_doc", 634.5f, 125.5f, { "L655", "O605", "vaga_doc" } },
    { "vaga_doc", 634.5f, 119.5f, { "entrada_doc" } },
    { "entrada_cozy", 660.0f, 146.5f,
      { "L680", "O630", "praca_do_cone_do_mcqueen", "cozy_patio", "visitas_do_cone_do_mcqueen" } },
    { "praca_do_cone_do_mcqueen", 649.0f, 145.5f, { "porta_do_cone_do_mcqueen" } },
    { "porta_do_cone_do_mcqueen", 649.0f, 155.5f, { "vaga_mcqueen", "entrada_cozy" } },
    { "vaga_mcqueen", 628.56f, 155.53f, { "porta_do_cone_do_mcqueen" } },
    { "visitas_do_cone_do_mcqueen", 646.0f, 167.0f, { "entrada_cozy" } },
    { "cozy_patio", 678.0f, 160.0f, { "entrada_cozy", "porta_da_recepcao" } },
    { "porta_da_recepcao", 698.2f, 172.5f, { "cozy_patio", "dentro_da_recepcao" } },
    { "dentro_da_recepcao", 698.2f, 183.0f, { "porta_da_recepcao", "volta_da_recepcao" } },
    { "volta_da_recepcao", 701.0f, 188.0f, { "manobra_da_recepcao" } },
    { "manobra_da_recepcao", 694.0f, 189.0f, { "vaga_sally" } },
    { "vaga_sally", 697.9f, 199.6f, { "dentro_da_recepcao" } },
    { "entrada_sarge", 701.0f, 126.8f, { "L715", "O680", "vaga_sarge" } },
    { "vaga_sarge", 701.0f, 119.5f, { "entrada_sarge" } },
    { "entrada_fillmore", 755.5f, 125.5f, { "L770", "O735", "vaga_fillmore" } },
    { "vaga_fillmore", 755.5f, 115.5f, { "entrada_fillmore" } },
    { "entrada_mater", 942.0f, 145.0f, { "L970", "O910", "mater_portao" } },
    { "mater_portao", 942.0f, 151.0f, { "entrada_mater", "vaga_mater" } },
    { "vaga_mater", 943.4f, 169.2f, { "mater_portao" } },
    { "entrada_sheriff", 1180.0f, 126.0f, { "L1195", "O1150", "vaga_sheriff" } },
    { "vaga_sheriff", 1180.0f, 115.0f, { "entrada_sheriff" } },
    { "entrada_quartel", 285.0f, 124.5f, { "ret_oeste_2", "vaga_red" } },
    { "vaga_red", 270.5f, 126.5f, { "entrada_quartel" } },
};

/* Os percursos: nós por onde passar, em ordem, antes de ir para o lugar do programa (entre um e outro, o
 * caminho mais curto). A ronda do xerife vai pela rua principal até o retorno do oeste e volta; o passeio
 * pela rodovia vai até o retorno dela e volta; o treino dá duas voltas no oval da pista de treino */
enum { PERCURSO_RONDA, PERCURSO_RODOVIA, PERCURSO_TREINO };
static const char *const PERCURSOS[][8] = {
    [PERCURSO_RONDA]   = { "ret_oeste_2", "L1150", NULL },
    [PERCURSO_RODOVIA] = { "ret_rodovia_2", NULL },
    [PERCURSO_TREINO]  = { "OV_B4", "OV_T8", "OV_B4", "OV_T8", NULL },
};

/* As pessoas (a ordem da tabela PESSOAS) */
enum { P_MCQUEEN, P_MATE, P_SALLY, P_DOC, P_LUIGI, P_GUIDO, P_RAMONE, P_FLO, P_XERIFE, P_SARGENTO,
       P_FILLMORE, P_LIZZIE, P_RUIVO };

/* Um programa: { lugar, percurso, convidado, peso, de, até }. O lugar é para onde vai (e estaciona);
 * o percurso, um passeio antes de chegar; o convidado, quem ele passa para buscar (se está em casa) e vai
 * junto; o peso, a chance perto dos outros programas; de e até, quantos segundos fica lá. Um programa
 * com a própria casa como lugar é um passeio que volta para casa (a ronda, o treino) */
#define SO (-1)   /* sem percurso, sem convidado */

/* Cada pessoa: o nome, o carro, a casa, quanto tempo fica em casa (de, até), quanto espera antes da
 * primeira saída, a pressa (a velocidade na cidade é VELOCIDADE_NA_CIDADE vezes isto), quem ela acompanha
 * sempre, os programas, o que diz ao jogador que para perto e o que diz conversando com qualquer um */
static const Pessoa PESSOAS[] = {
    [P_MCQUEEN] = { "McQueen", 0, LUGAR_MCQUEEN, 50, 90, 25, 1.15f, SO,
        { { LUGAR_FLO, SO, SO, 3, 40, 70 }, { LUGAR_LUIGI, SO, SO, 1, 30, 45 }, { LUGAR_RAMONE, SO, SO, 1, 30, 45 },
          { LUGAR_FLO, PERCURSO_RODOVIA, P_SALLY, 2, 50, 80 } },
        { "Katchau! Bora apostar uma corrida?", "E aí! Curtindo a cidade?" },
        { "Katchau!", "Essa cidade é melhor que qualquer pista.", "Daqui a pouco tem treino com o Doc." } },
    [P_MATE] = { "Mate", 8, LUGAR_MATE, 35, 70, 8, 1.05f, SO,
        { { LUGAR_FLO, SO, SO, 3, 35, 60 }, { LUGAR_FLO, SO, P_MCQUEEN, 3, 40, 70 }, { LUGAR_LUIGI, SO, SO, 1, 25, 40 },
          { LUGAR_SARGENTO, SO, SO, 1, 25, 40 }, { LUGAR_RAMONE, SO, SO, 1, 25, 40 }, { LUGAR_CURIOS, SO, SO, 1, 25, 40 } },
        { "E aí, parceiro! Tá precisando de um reboque?", "Ô, amigão! Já viu meu ferro-velho?" },
        { "Cê viu o xerife hoje? Tá bravo que só.", "Tô precisando de umas peças novas pro ferro-velho.",
          "Dizem que vai ter corrida grande no fim de semana!" } },
    [P_SALLY] = { "Sally", 4, LUGAR_SALLY, 70, 110, 40, 1.0f, SO,
        { { LUGAR_FLO, SO, SO, 3, 40, 60 }, { LUGAR_CURIOS, SO, SO, 1, 30, 45 }, { LUGAR_DOC, SO, SO, 1, 25, 40 },
          { LUGAR_FLO, PERCURSO_RODOVIA, P_MCQUEEN, 2, 50, 80 } },
        { "Bem-vindo ao Cozy Cone! Quer um cone pra descansar?", "Já deu uma volta pela rodovia? A vista é linda." },
        { "O Cozy Cone está quase lotado essa semana.", "A cidade anda bem mais movimentada, né?",
          "Daqui a pouco o sol bate bonito nas montanhas." } },
    [P_DOC] = { "Doc Hudson", 2, LUGAR_DOC, 90, 140, 60, 1.0f, SO,
        { { LUGAR_FLO, SO, SO, 3, 40, 60 }, { LUGAR_DOC, PERCURSO_TREINO, P_MCQUEEN, 2, 30, 50 },
          { LUGAR_DOC, PERCURSO_TREINO, SO, 1, 30, 50 }, { LUGAR_SALLY, SO, SO, 1, 25, 40 } },
        { "Hm. Cuidado com essas curvas, garoto.", "Precisa de um check-up? A clínica está aberta." },
        { "Hm.", "Tem muito carro correndo por aí sem pensar.", "Na terra, quem freia na hora certa ganha." } },
    [P_LUIGI] = { "Luigi", 6, LUGAR_LUIGI, 80, 120, 15, 0.95f, SO,
        { { LUGAR_FLO, SO, SO, 3, 40, 60 }, { LUGAR_RAMONE, SO, SO, 1, 30, 40 }, { LUGAR_SALLY, SO, SO, 1, 30, 40 } },
        { "Pneus novos? Os melhores da Rota 66!", "Bem-vindo à Casa Della Tires!" },
        { "Os melhores pneus da Rota 66, eu garanto!", "Um dia ainda vem uma Ferrari aqui na loja." } },
    [P_GUIDO] = { "Guido", 7, LUGAR_GUIDO, 80, 120, 15, 1.0f, P_LUIGI,
        { { LUGAR_FLO, SO, SO, 1, 40, 60 } },
        { "Pit stop!", "Pneus? Pneus!" },
        { "Pit stop!", "Si, si!" } },
    [P_RAMONE] = { "Ramone", 5, LUGAR_RAMONE, 60, 100, 30, 1.0f, SO,
        { { LUGAR_FLO, SO, SO, 5, 60, 100 }, { LUGAR_LUIGI, SO, SO, 1, 30, 40 }, { LUGAR_FILLMORE, SO, SO, 1, 30, 40 } },
        { "Que tal uma pintura nova, cara?", "Essa lataria tá pedindo um trato, hein." },
        { "Tô pensando em pintar de roxo amanhã.", "Tem gente precisando de um trato na lataria por aqui." } },
    [P_FLO] = { "Flo", MODELO_DA_FLO, LUGAR_FLO, 150, 240, 140, 0.95f, SO,
        { { LUGAR_RAMONE, SO, SO, 1, 40, 60 }, { LUGAR_SALLY, SO, SO, 1, 30, 40 } },
        { "Encosta aí, docinho! Óleo fresquinho saindo.", "Oi, querido! Vai querer o de sempre?" },
        { "O café tá cheio hoje, graças a Deus!", "Senta aí, que o óleo tá saindo." } },
    [P_XERIFE] = { "Xerife", MODELO_DO_XERIFE, LUGAR_XERIFE, 60, 100, 20, 1.15f, SO,
        { { LUGAR_XERIFE, PERCURSO_RONDA, SO, 3, 0, 0 }, { LUGAR_FLO, SO, SO, 2, 30, 50 }, { LUGAR_RUIVO, SO, SO, 1, 20, 30 } },
        { "Tudo em ordem por aqui. Ande devagar.", "Na cidade, o limite é de 30 por hora." },
        { "Tudo em ordem por aqui.", "Movimento calmo hoje na estrada." } },
    [P_SARGENTO] = { "Sargento", 9, LUGAR_SARGENTO, 70, 100, 35, 1.0f, SO,
        { { LUGAR_FILLMORE, SO, SO, 2, 30, 40 }, { LUGAR_FLO, SO, SO, 2, 40, 60 }, { LUGAR_FLO, SO, P_FILLMORE, 1, 40, 60 } },
        { "Atenção, recruta! Pneus calibrados?", "Postura! Para-choque erguido!" },
        { "Ordem e disciplina!", "Aqui se acorda às seis em ponto." } },
    [P_FILLMORE] = { "Fillmore", 10, LUGAR_FILLMORE, 80, 120, 70, 0.85f, SO,
        { { LUGAR_SARGENTO, SO, SO, 2, 30, 40 }, { LUGAR_FLO, SO, SO, 2, 40, 60 }, { LUGAR_RAMONE, SO, SO, 1, 30, 40 } },
        { "Paz, cara. Experimenta o meu combustível orgânico.", "Respira fundo, irmão. Sente a energia." },
        { "Tudo é uma questão de energia, cara.", "Paz e amor, irmão." } },
    [P_LIZZIE] = { "Lizzie", MODELO_DA_LIZZIE, LUGAR_CURIOS, 100, 150, 45, 0.65f, SO,
        { { LUGAR_FLO, SO, SO, 3, 50, 80 }, { LUGAR_DOC, SO, SO, 1, 30, 40 }, { LUGAR_SALLY, SO, SO, 1, 30, 40 } },
        { "Você é novo por aqui? Leva um adesivo de lembrança!", "Ah, você é aquele moço... como é mesmo o seu nome?" },
        { "No meu tempo, a estrada passava bem aqui na frente!", "Que dia é hoje mesmo?",
          "O Stanley ia adorar ver a cidade assim." } },
    [P_RUIVO] = { "Ruivo", MODELO_DO_RUIVO, LUGAR_RUIVO, 150, 240, 100, 0.9f, SO,
        { { LUGAR_FLO, SO, SO, 1, 30, 40 }, { LUGAR_XERIFE, SO, SO, 1, 20, 30 } },
        { NULL },   /* tímido: não fala com o jogador, se encolhe */
        { "(sorri tímido)", "(rega as flores com cuidado)" } },
};

/* As conversas de cada par: as falas alternadas, a primeira de "a" (as outras duplas usam as falas de
 * cada um, da tabela PESSOAS) */
static const Conversa CONVERSAS[] = {
    { P_LUIGI, P_GUIDO, { "Guido, separa os pneus de faixa branca!", "Pit stop!",
                          "Que dia lindo pra vender pneus, hein?", "Si, si!",
                          "Um dia ainda vem uma Ferrari aqui na loja.", "Ferrari!" } },
    { P_SARGENTO, P_FILLMORE, { "Abaixa esse som, hippie! Ainda é horário de expediente.", "Relaxa, cara. É o som da natureza.",
                                "Disciplina! É isso que falta nesta cidade.", "Já experimentou meu combustível orgânico?",
                                "Nem pensar. Meu tanque é do bom e velho diesel.", "Paz, irmão. Paz." } },
    { P_FLO, P_RAMONE, { "Pintura nova de novo, querido?", "Pra você, gata, uma cor nova todo dia.",
                         "Fica pro óleo da tarde?", "Só se for do seu lado." } },
    { P_MATE, P_MCQUEEN, { "Bora dar um rolê, amigão?", "Bora! Mas hoje sem tombar trator, hein.",
                           "Sabia que eu sou o melhor de ré do mundo?", "Já sei, Mate. Você me conta toda semana.",
                           "Melhores amigos pra sempre!", "Pra sempre, parceiro." } },
    { P_SALLY, P_MCQUEEN, { "Vamos dar uma volta pela estrada?", "Com você? Sempre.",
                            "Daqui a pouco o sol bate bonito nas montanhas.", "Melhor vista da Rota 66." } },
    { P_DOC, P_MCQUEEN, { "Na terra é diferente, garoto. Joga a traseira antes da curva.", "Pode deixar, Doc.",
                          "E não esquece: corrida não é só velocidade.", "Bora pra pista, então?" } },
    { P_XERIFE, P_RUIVO, { "Tudo tranquilo no quartel, Ruivo?", "(acena tímido)",
                           "As flores estão bonitas este ano.", "(sorri)" } },
    { P_XERIFE, P_FLO, { "O de sempre, Flo.", "Saindo um óleo quentinho, xerife.",
                         "Movimento calmo hoje na estrada.", "Melhor assim, benzinho." } },
    { P_MATE, P_SARGENTO, { "Bom dia, Sargento!", "Bom dia, recruta! Esse guincho está torto.",
                            "É de nascença, Sargento!", "Hmpf." } },
};

/* O que todo mundo diz, conversando com qualquer um */
static const char *const FALAS_DE_SEMPRE[] = {
    "Que calor, hein?", "Movimento fraco hoje na Rota 66, né?", "Viu o carro novo que passou na cidade?",
    "Dizem que vem corrida grande por aí.", "Bom dia! Tudo nos trinques?", "O pôr do sol hoje vai ser bonito.",
    "Precisando de alguma coisa, é só chamar.", "Essa estrada já viu cada coisa...",
};

/* O que dizem quando o jogador passa rápido rente a eles (o Ruivo só se assusta) */
static const char *const SUSTOS[] = { "Ei, olha a velocidade!", "Opa! Mais devagar aí!", "Quase me acerta!" };
#define SUSTO_DA_LIZZIE "Ai, que susto! No meu tempo ninguém corria assim."

/* O xerife atrás do jogador */
#define XERIFE_VAI      "Ei, você aí! Encosta!"
#define XERIFE_PEGOU    "Devagar, rapaz! Na cidade, o limite é de 30 por hora."
#define XERIFE_DESISTE  "Hmpf... da próxima vez eu te pego."
